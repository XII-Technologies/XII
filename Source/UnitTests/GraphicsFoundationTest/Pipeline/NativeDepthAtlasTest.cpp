/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
namespace
{
  template <typename T> xiiRenderGraphBufferHandle UploadAtlasInput(xiiRenderGraph& graph, xiiStringView name, xiiArrayPtr<T> values)
  {
    struct Data
    {
      xiiRenderGraphBufferHandle m_Buffer;
      xiiDynamicArray<xiiUInt8>  m_Bytes;
    };
    const auto emitted = graph.AddPass<Data>(name, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription description;
      description.m_uiSize = values.GetCount() * sizeof(T); description.m_uiElementByteStride = sizeof(T);
      description.m_Mode = xiiGALBufferMode::Structured; description.m_BindFlags = xiiGALBindFlags::ShaderResource;
      data.m_Buffer = builder.WriteBuffer(name, description, xiiGALResourceStateFlags::CopyDestination);
      data.m_Bytes = xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(values.GetPtr()), values.GetCount() * sizeof(T)); }, [](const Data& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Buffer), 0, data.m_Bytes); });
    return emitted.first->m_Buffer;
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(NativeDepthAtlas);
static void TestReusableShadowRegions(bool meshShaders)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(meshShaders)))
    return;
  if (meshShaders && environment.GetDevice()->GetFeatures().m_MeshShaders != xiiGALDeviceFeatureState::Enabled)
  {
    xiiTestFramework::Output(xiiTestOutput::Message, "Mesh shadow regions require mesh shader support.");
    return;
  }
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  if (!XII_TEST_RESULT(xiiGeometryResidencyManager::EnsureInitialized()) || !XII_TEST_RESULT(xiiMaterialManager::EnsureInitialized()))
    return;
  xiiGeometry box;
  box.AddBox(xiiVec3(0.4f), true);
  xiiMeshBufferResourceDescriptor mesh;
  mesh.AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, true);
  const auto             buffer = xiiResourceManager::CreateResource<xiiMeshBufferResource>("Native shadow cube", std::move(mesh));
  xiiGeometryDescription description;
  description.m_Lods.ExpandAndGetRef().m_hMeshBuffer = buffer;
  const auto handle                                  = xiiGeometryResidencyManager::RegisterGeometry(description);
  if (!XII_TEST_BOOL(!handle.IsInvalidated()))
    return;
  xiiGeometryResidencyManager::RequestResidency(handle, 0, 0);
  XII_SCOPE_EXIT(xiiGeometryResidencyManager::UnregisterGeometry(handle, 0));
  const auto                                 asset = xiiLoadPipelineLibraryResource("ShadowDepth");
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  const auto authoring = loaded->GetAuthoring();
  xiiUuid    rasterId, clearId, constantsId;
  for (const auto& pass : authoring->m_Descriptor.m_Passes)
  {
    if (pass.m_sType == "xiiDepthRasterPass")
      rasterId = pass.m_Id;
    if (pass.m_sType == "xiiDepthRegionClearPass")
      clearId = pass.m_Id;
    if (pass.m_sType == "xiiMatrixRasterConstantsPass")
      constantsId = pass.m_Id;
  }
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({authoring->m_Descriptor.m_Id, asset.GetResourceID()});
  xiiVariantDictionary targetProperties;
  targetProperties.Insert("Width", 32U);
  targetProperties.Insert("Height", 16U);
  targetProperties.Insert("Depth", 0.9f);
  const auto           target    = root.AddPass("Atlas allocation", "xiiDepthTargetPass", targetProperties);
  const auto           geometry  = root.AddPass("Shared geometry", "xiiGeometryUploadPass");
  const auto           materials = root.AddPass("Shared materials", "xiiMaterialUploadPass");
  xiiVariantDictionary commandProperties;
  if (meshShaders)
  {
    commandProperties.Insert("MeshGroupCountX", 1U);
    commandProperties.Insert("MeshGroupCountY", 1U);
  }
  const auto commands = root.AddPass("Explicit draw commands", meshShaders ? "xiiMeshDrawCommandsPass" : "xiiVertexPullDrawCommandsPass", commandProperties);
  for (const xiiStringView name : {"VisibleMeshlets", "VisibleMeshletCount"})
    root.m_Interface.PushBack({name, true, root.m_Passes[commands].m_Id, name});
  if (!meshShaders)
  {
    for (const xiiStringView name : {"SceneInstances", "VisibilityView"})
      root.m_Interface.PushBack({name, true, root.m_Passes[commands].m_Id, name});
    XII_TEST_RESULT(root.Connect(geometry, "GeometryMetadata", commands, "GeometryMetadata"));
    XII_TEST_RESULT(root.Connect(geometry, "MeshletMetadata", commands, "MeshletMetadata"));
  }
  xiiUInt32     previous    = target;
  xiiStringView previousPin = "DepthTarget";
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto       node = root.AddSubgraph(i ? "Second shadow tile" : "First shadow tile", authoring->m_Descriptor.m_Id);
    const xiiVec4U32 viewport(i ? 18 : 2, 2, 12, 12);
    for (const auto id : {rasterId, clearId})
    {
      auto& override = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      override.m_Path.PushBack(id);
      override.m_Properties.Insert("Viewport", viewport);
      if (id == rasterId)
        override.m_Properties.Insert("UseMeshShaders", meshShaders);
      if (id == clearId)
        override.m_Properties.Insert("ClearDepth", i ? 0.1f : 0.0f);
    }
    auto& matrix = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    matrix.m_Path.PushBack(constantsId);
    auto projection = xiiMat4::MakeScaling(xiiVec3(i ? 2.0f : 1.0f, i ? 2.0f : 1.0f, -1));
    projection.SetTranslationVector(xiiVec3(0, 0, 1));
    matrix.m_Properties.Insert("ViewProjection", projection);
    matrix.m_Properties.Insert("Width", 12U);
    matrix.m_Properties.Insert("Height", 12U);
    for (const auto& mapping : authoring->m_Descriptor.m_Interface)
    {
      if (!mapping.m_bInput)
        continue;
      if (mapping.m_sName == "DepthTarget")
      {
        XII_TEST_RESULT(root.Connect(previous, previousPin, node, mapping.m_sName));
      }
      else if (mapping.m_sName == "GeometryMetadata" || mapping.m_sName == "MeshletMetadata")
      {
        XII_TEST_RESULT(root.Connect(geometry, mapping.m_sName, node, mapping.m_sName));
      }
      else if (mapping.m_sName == "SurfaceMaterials")
      {
        XII_TEST_RESULT(root.Connect(materials, mapping.m_sName, node, mapping.m_sName));
      }
      else if (mapping.m_sName == "DrawCommands" || mapping.m_sName == "DrawCommandCount")
      {
        XII_TEST_RESULT(root.Connect(commands, mapping.m_sName, node, mapping.m_sName));
      }
      else
        root.m_Interface.PushBack({mapping.m_sName, true, root.m_Passes[node].m_Id, mapping.m_sName});
    }
    previous    = node;
    previousPin = "ShadowDepth";
  }
  root.m_Interface.PushBack({"Atlas", false, root.m_Passes[previous].m_Id, "ShadowDepth"});
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(meshShaders ? "Native mesh shadow atlas regions" : "Native vertex-pull shadow atlas regions", std::move(root));
  xiiRenderPipelineInstance pipelines[2];
  xiiRenderGraphContext     contexts[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
    if (!XII_TEST_RESULT(pipelines[i].Synchronize(resource)) || !XII_TEST_RESULT(contexts[i].Initialize("Native atlas")))
      return;
  for (xiiUInt32 frameNumber = 0; frameNumber < 6; ++frameNumber)
  {
    const xiiUInt32     viewIndex = frameNumber % 2;
    auto&               pipeline  = pipelines[viewIndex];
    auto&               context   = contexts[viewIndex];
    auto&               graph     = context.GetGraph();
    auto&               cache     = context.GetResourceCache();
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    xiiGeometryResidencyManager::ProcessStreaming(frame.GetFrameIndex(), frame.GetCompletedFrame(), xiiGeometryResidencyManager::GetConfiguration().m_uiUploadBudgetPerFrameBytes);
    const auto* record = xiiGeometryResidencyManager::GetGpuRecord(handle);
    if (!XII_TEST_BOOL(record && record->m_uiResidentLodMask == 1))
      return;
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    xiiGpuSceneInstance scene;
    scene.m_GlobalTransform         = xiiMat4::MakeTranslation(xiiVec3(0, 0, 0.5f));
    scene.m_PreviousGlobalTransform = scene.m_GlobalTransform;
    scene.m_uiObjectId              = 1;
    scene.m_uiFlags                 = xiiGpuInstanceFlags::Enabled;
    scene.m_uiGeometryIndex         = handle.GetInternalID().m_InstanceIndex;
    xiiMat4              pose       = xiiMat4::MakeIdentity();
    xiiGpuVisibilityView view;
    view.m_ViewProjectionMatrix          = xiiMat4::MakeIdentity();
    view.m_OcclusionViewProjectionMatrix = view.m_ViewProjectionMatrix;
    view.m_uiGeometryBaseIndex           = xiiGeometryResidencyManager::GetMetadataBaseIndex(frame.GetFrameIndex());
    xiiDynamicArray<xiiVec2U32> pairs;
    for (xiiUInt32 i = 0; i < record->m_Lods[0].m_uiMeshletCount; ++i)
      pairs.PushBack(xiiVec2U32(0, record->m_Lods[0].m_uiMeshletMetadataOffset + i));
    xiiUInt32                                        count        = pairs.GetCount();
    const auto                                       instances    = UploadAtlasInput(graph, "Atlas scene", xiiMakeArrayPtr(&scene, 1));
    const auto                                       skinning     = UploadAtlasInput(graph, "Atlas pose", xiiMakeArrayPtr(&pose, 1));
    const auto                                       snapshot     = UploadAtlasInput(graph, "Atlas view", xiiMakeArrayPtr(&view, 1));
    const auto                                       visible      = UploadAtlasInput(graph, "Atlas pairs", pairs.GetArrayPtr());
    const auto                                       visibleCount = UploadAtlasInput(graph, "Atlas count", xiiMakeArrayPtr(&count, 1));
    const auto*                                      compiled     = pipeline.GetPipeline();
    xiiHybridArray<xiiRenderPipelineInputBinding, 5> inputs;
    inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("SceneInstances", true), instances));
    inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("SkinningMatrices", true), skinning));
    if (!meshShaders)
      inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("VisibilityView", true), snapshot));
    inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("VisibleMeshlets", true), visible));
    inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("VisibleMeshletCount", true), visibleCount));
    xiiVariantDictionary runtime;
    if (frameNumber >= 4)
    {
      auto projection = xiiMat4::MakeScaling(xiiVec3(2, 2, -1));
      projection.SetTranslationVector(xiiVec3(0, 0, 1));
      runtime.Insert("RasterViewProjection", projection);
      runtime.Insert("RasterPreviousViewProjection", projection);
      runtime.Insert("RasterWidth", 48U);
      runtime.Insert("RasterHeight", 16U);
      runtime.Insert("RasterViewport", xiiVec4U32(2, 2, 12, 12));
      runtime.Insert("RasterMeshGroupCountX", 1U);
      runtime.Insert("RasterMeshGroupCountY", 1U);
    }
    if (!XII_TEST_RESULT(pipeline.Build(nullptr, graph, context.GetBlackboard(), runtime, inputs)))
      return;
    const auto atlas = pipeline.GetInterfaceBinding(compiled->FindInterface("Atlas", false))->m_hTexture;
    graph.AddPass<xiiUInt32>("Export shadow atlas", xiiGALCommandQueueFlags::Graphics, [atlas](auto&, xiiRenderGraphBuilder& builder) { builder.ExportTexture(builder.ReadTexture(atlas, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    graph.EndSetup();
    xiiStringBuilder error;
    if (!XII_TEST_RESULT_MSG(graph.Compile({.m_bEnableAsyncQueues = frameNumber < 4}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
      return;
    if (frameNumber == 3 || frameNumber == 5)
    {
      const auto& statistics = graph.GetStatistics();
      xiiTestFramework::Output(xiiTestOutput::Details, "Shadow atlas (async=%u): %u compiled logical operations, %u queue submissions, %u barriers.", frameNumber < 4 ? 1U : 0U, statistics.m_uiCompiledPassCount, statistics.m_uiQueueSubmissionCount, statistics.m_uiTotalBarrierCount);
    }
    if (frameNumber >= 4)
    {
      XII_TEST_INT(graph.GetStatistics().m_uiQueueSubmissionCount, 1);
    }
    frame.End();
    const auto image = graph.GetExportedTexture(atlas);
    XII_TEST_INT(image->GetDescription().m_Size.width, frameNumber >= 4 ? 48 : 32);
    XII_TEST_BOOL(pipeline.GetPipeline() == compiled);
    for (xiiUInt32 i = 0; i < compiled->GetDescriptor().m_Passes.GetCount(); ++i)
      if (compiled->GetDescriptor().m_Passes[i].m_sType == "xiiMatrixRasterConstantsPass")
      {
        xiiVariantDictionary authored;
        XII_TEST_RESULT(pipeline.GetPass(i)->GetProperties(authored));
        XII_TEST_INT(authored["Width"].Get<xiiUInt32>(), 12);
      }

    for (const xiiVec2U32 pixel : {xiiVec2U32(0, 0), xiiVec2U32(16, 8), xiiVec2U32(31, 15)})
    {
      float value = 0;
      if (xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), image.Borrow(), pixel.x, pixel.y, value))
      {
        XII_TEST_FLOAT(value, 0.9f, 0.00001f);
      }
    }
    for (const xiiVec2U32 pixel : {xiiVec2U32(2, 2), xiiVec2U32(18, 2)})
    {
      float value = 1;
      if (xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), image.Borrow(), pixel.x, pixel.y, value))
      {
        XII_TEST_FLOAT(value, frameNumber >= 4 ? (pixel.x == 18 ? 0.9f : 0.1f) : (pixel.x == 18 ? 0.1f : 0.0f), 0.00001f);
      }
    }
    for (const xiiVec2U32 pixel : {xiiVec2U32(6, 8), xiiVec2U32(22, 8)})
    {
      float value = 0;
      if (xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), image.Borrow(), pixel.x, pixel.y, value))
      {
        XII_TEST_FLOAT(value, frameNumber >= 4 ? (pixel.x == 6 ? 0.7f : 0.9f) : (pixel.x == 6 ? 0.0f : 0.7f), 0.001f);
      }
    }
    for (const xiiVec2U32 pixel : {xiiVec2U32(8, 8), xiiVec2U32(24, 8)})
    {
      float value = 0;
      if (xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), image.Borrow(), pixel.x, pixel.y, value))
      {
        XII_TEST_FLOAT(value, frameNumber >= 4 && pixel.x == 24 ? 0.9f : 0.7f, 0.001f);
      }
    }
  }
}

XII_CREATE_SIMPLE_TEST(NativeDepthAtlas, ReusableShadowRegions)
{
  TestReusableShadowRegions(false);
}
XII_CREATE_SIMPLE_TEST(NativeDepthAtlas, ReusableMeshShadowRegions)
{
  TestReusableShadowRegions(true);
}
