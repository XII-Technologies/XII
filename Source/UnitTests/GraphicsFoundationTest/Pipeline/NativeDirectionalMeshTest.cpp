/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <Shaders/Pipeline/Passes/Geometry/DirectionalMeshConstants.h>
#include <Shaders/Pipeline/Passes/VirtualShadowMap/VirtualShadowMapConstants.h>
XII_DEFINE_AS_POD_TYPE(xiiDirectionalMeshConstants);
XII_DEFINE_AS_POD_TYPE(xiiVirtualShadowSamplingConstants);
namespace
{
  template <typename T> xiiRenderGraphBufferHandle UploadDirectionalInput(xiiRenderGraph& graph, xiiStringView name, xiiArrayPtr<T> source)
  {
    struct Data
    {
      xiiRenderGraphBufferHandle m_Buffer;
      xiiDynamicArray<xiiUInt8>  m_Bytes;
    };
    const auto pass = graph.AddPass<Data>(name, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription desc; desc.m_Mode=xiiGALBufferMode::Structured; desc.m_BindFlags=xiiGALBindFlags::ShaderResource; desc.m_uiSize=source.GetCount()*sizeof(T); desc.m_uiElementByteStride=sizeof(T);
      data.m_Buffer=builder.WriteBuffer(name,desc,xiiGALResourceStateFlags::CopyDestination); data.m_Bytes=xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(source.GetPtr()),static_cast<xiiUInt32>(desc.m_uiSize)); }, [](const Data& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Buffer), 0, data.m_Bytes); });
    return pass.first->m_Buffer;
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeDirectionalMesh);
XII_CREATE_SIMPLE_TEST(NativeDirectionalMesh, RepeatedAuthoredRasterAndRetainedConstants)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(true)))
    return;
  if (environment.GetDevice()->GetFeatures().m_MeshShaders != xiiGALDeviceFeatureState::Enabled)
  {
    xiiTestFramework::Output(xiiTestOutput::Message, "Directional mesh raster requires mesh shader support.");
    return;
  }
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiVirtualShadowMapSettings settings;
  settings.m_uiVirtualResolution = 64;
  settings.m_uiPageSize          = 16;
  settings.m_uiPhysicalPageCount = 2;
  if (!XII_TEST_RESULT(xiiVirtualShadowMapManager::Configure(settings)) || !XII_TEST_RESULT(xiiGeometryResidencyManager::EnsureInitialized()) || !XII_TEST_RESULT(xiiMaterialManager::EnsureInitialized()))
    return;
  xiiMaterialResourceDescriptor materialDescription;
  materialDescription.m_hShader   = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Geometry/DirectionalMesh.xiiShader");
  materialDescription.m_BaseColor = xiiColor(0.8f, 0.6f, 0.4f, 1);
  const auto material             = xiiResourceManager::CreateResource<xiiMaterialResource>("Directional mesh material", std::move(materialDescription));
  const auto materialHandle       = xiiMaterialManager::AcquireMaterialResource(material);
  if (!XII_TEST_BOOL(!materialHandle.IsInvalidated()))
    return;
  XII_SCOPE_EXIT(xiiMaterialManager::ReleaseMaterialResource(material));
  xiiGeometry box;
  box.AddBox(xiiVec3(0.4f), true);
  xiiMeshBufferResourceDescriptor mesh;
  mesh.AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, true);
  const auto             buffer = xiiResourceManager::CreateResource<xiiMeshBufferResource>("Directional cube", std::move(mesh));
  xiiGeometryDescription definition;
  definition.m_Lods.ExpandAndGetRef().m_hMeshBuffer = buffer;
  const auto geometryHandle                         = xiiGeometryResidencyManager::RegisterGeometry(definition);
  if (!XII_TEST_BOOL(!geometryHandle.IsInvalidated()))
    return;
  XII_SCOPE_EXIT(xiiGeometryResidencyManager::UnregisterGeometry(geometryHandle, 0));
  xiiGeometryResidencyManager::RequestResidency(geometryHandle, 0, 0);
  const auto                                 asset = xiiLoadPipelineLibraryResource("DirectionalMesh");
  xiiResourceLock<xiiRenderPipelineResource> source(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(source.IsValid() && source->GetAuthoring() && source->GetPipeline()))
    return;
  const auto authored = source->GetAuthoring();
  xiiUuid    constantsId, samplingId, geometryConstantsId;
  for (const auto& pass : authored->m_Descriptor.m_Passes)
  {
    if (pass.m_sType == "xiiDirectionalMeshConstantsPass")
      constantsId = pass.m_Id;
    if (pass.m_sType == "xiiVirtualShadowSamplingConstantsPass")
      samplingId = pass.m_Id;
    if (pass.m_sType == "xiiMatrixRasterConstantsPass")
      geometryConstantsId = pass.m_Id;
  }
  if (!XII_TEST_BOOL(constantsId.IsValid() && samplingId.IsValid() && geometryConstantsId.IsValid()))
    return;
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({authored->m_Descriptor.m_Id, asset.GetResourceID()});
  const auto           geometry       = root.AddPass("Device geometry", "xiiGeometryUploadPass");
  const auto           materialUpload = root.AddPass("Device material residency", "xiiMaterialUploadPass");
  const auto           tables         = root.AddPass("Device shadow tables", "xiiVirtualShadowTablesPass");
  xiiVariantDictionary commandProperties;
  commandProperties.Insert("MeshGroupCountX", 1U);
  commandProperties.Insert("MeshGroupCountY", 1U);
  const auto           commands = root.AddPass("Supplied work dimensions", "xiiMeshDrawCommandsPass", commandProperties);
  xiiVariantDictionary shadowProperties;
  shadowProperties.Insert("Width", 16U);
  shadowProperties.Insert("Height", 16U);
  const auto shadow = root.AddPass("Empty ordinary shadow", "xiiDepthTargetPass", shadowProperties);
  for (xiiStringView name : {"VisibleMeshlets", "VisibleMeshletCount"})
    root.m_Interface.PushBack({name, true, root.m_Passes[commands].m_Id, name});
  for (xiiUInt32 branch = 0; branch < 2; ++branch)
  {
    xiiVariantDictionary image;
    image.Insert("Width", branch ? 16U : 32U);
    image.Insert("Height", branch ? 32U : 16U);
    image.Insert("Format", xiiUInt32(xiiGALResourceFormat::RGBA32Float));
    image.Insert("Color", xiiColor(0, 0, 0.25f, 1));
    const auto color = root.AddPass(branch ? "Green target" : "Red target", "xiiClearTexturePass", image);
    image.Remove("Format");
    image.Remove("Color");
    const auto depth     = root.AddPass(branch ? "Green depth" : "Red depth", "xiiDepthTargetPass", image);
    const auto node      = root.AddSubgraph(branch ? "Green mesh" : "Red mesh", authored->m_Descriptor.m_Id);
    auto&      constants = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    constants.m_Path.PushBack(constantsId);
    constants.m_Properties.Insert("AmbientColor", branch ? xiiVec3(0, 0.5f, 0) : xiiVec3(0.5f, 0, 0));
    constants.m_Properties.Insert("SelectionPulseStrength", 0.0f);
    auto& geometryConstants = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    geometryConstants.m_Path.PushBack(geometryConstantsId);
    geometryConstants.m_Properties.Insert("Width", branch ? 16U : 32U);
    geometryConstants.m_Properties.Insert("Height", branch ? 32U : 16U);
    auto& sampling = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    sampling.m_Path.PushBack(samplingId);
    sampling.m_Properties.Insert("Enabled", false);
    sampling.m_Properties.Insert("LightId", branch ? 7U : 3U);
    for (xiiStringView name : {"SceneInstances", "SkinningMatrices", "VisibleMeshlets", "VisibleMeshletCount"})
      root.m_Interface.PushBack({name, true, root.m_Passes[node].m_Id, name});
    XII_TEST_RESULT(root.Connect(materialUpload, "SurfaceMaterials", node, "SurfaceMaterials"));
    for (xiiStringView name : {"GeometryMetadata", "MeshletMetadata"})
      XII_TEST_RESULT(root.Connect(geometry, name, node, name));
    for (xiiStringView name : {"DrawCommands", "DrawCommandCount"})
      XII_TEST_RESULT(root.Connect(commands, name, node, name));
    XII_TEST_RESULT(root.Connect(tables, "Residency", node, "Residency"));
    XII_TEST_RESULT(root.Connect(tables, "PhysicalAtlas", node, "VirtualShadowAtlas"));
    XII_TEST_RESULT(root.Connect(tables, "VirtualPageTable", node, "VirtualPageTable"));
    XII_TEST_RESULT(root.Connect(shadow, "DepthTarget", node, "ShadowMap"));
    XII_TEST_RESULT(root.Connect(color, "Texture", node, "ColorTarget"));
    XII_TEST_RESULT(root.Connect(depth, "DepthTarget", node, "DepthTarget"));
    for (xiiStringView name : {"Color", "Depth", "RasterConstants", "SamplingConstants"})
    {
      xiiStringBuilder external;
      external.SetFormat("{}{}", branch ? "Green" : "Red", name);
      root.m_Interface.PushBack({external, false, root.m_Passes[node].m_Id, name});
    }
  }
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Repeated authored directional mesh", std::move(root));
  xiiRenderPipelineInstance pipeline;
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(pipeline.Synchronize(resource)) || !XII_TEST_RESULT(context.Initialize("Directional mesh readback")))
    return;
  for (xiiUInt32 phase = 0; phase < 4; ++phase)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    xiiGeometryResidencyManager::ProcessStreaming(frame.GetFrameIndex(), frame.GetCompletedFrame(), xiiGeometryResidencyManager::GetConfiguration().m_uiUploadBudgetPerFrameBytes);
    xiiMaterialManager::BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    const auto* record = xiiGeometryResidencyManager::GetGpuRecord(geometryHandle);
    if (!XII_TEST_BOOL(record && record->m_uiResidentLodMask == 1))
      return;
    auto& graph = context.GetGraph();
    auto& cache = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    xiiGpuSceneInstance scene;
    scene.m_GlobalTransform          = xiiMat4::MakeTranslation(xiiVec3(0, 0, 0.5f));
    scene.m_PreviousGlobalTransform  = scene.m_GlobalTransform;
    scene.m_uiGeometryIndex          = geometryHandle.GetInternalID().m_InstanceIndex;
    scene.m_uiObjectId               = 1;
    scene.m_uiMaterialIndex          = materialHandle.GetInternalID().m_InstanceIndex;
    scene.m_uiFlags                  = xiiGpuInstanceFlags::Enabled;
    xiiMat4                     pose = xiiMat4::MakeIdentity();
    xiiDynamicArray<xiiVec2U32> pairs;
    for (xiiUInt32 i = 0; i < record->m_Lods[0].m_uiMeshletCount; ++i)
      pairs.PushBack(xiiVec2U32(0, record->m_Lods[0].m_uiMeshletMetadataOffset + i));
    xiiUInt32                                        count     = pairs.GetCount();
    const auto                                       instances = UploadDirectionalInput(graph, "Instances", xiiMakeArrayPtr(&scene, 1));
    const auto                                       skinning  = UploadDirectionalInput(graph, "Pose", xiiMakeArrayPtr(&pose, 1));
    const auto                                       visible   = UploadDirectionalInput(graph, "Visible", pairs.GetArrayPtr());
    const auto                                       counter   = UploadDirectionalInput(graph, "Count", xiiMakeArrayPtr(&count, 1));
    const auto*                                      compiled  = pipeline.GetPipeline();
    xiiHybridArray<xiiRenderPipelineInputBinding, 5> inputs;
    inputs.PushBack({graph, compiled->FindInterface("SceneInstances", true), instances});
    inputs.PushBack({graph, compiled->FindInterface("SkinningMatrices", true), skinning});
    inputs.PushBack({graph, compiled->FindInterface("VisibleMeshlets", true), visible});
    inputs.PushBack({graph, compiled->FindInterface("VisibleMeshletCount", true), counter});
    xiiVariantDictionary parameters;
    parameters.Insert("DirectionalSun", xiiVec4(0, 0, -1, 0));
    const auto built = pipeline.Build(nullptr, graph, context.GetBlackboard(), parameters, inputs);
    if (!XII_TEST_RESULT_MSG(built, "%s", pipeline.GetLastBuildError().GetStartPointer()))
      return;
    xiiRenderGraphTextureHandle colors[2], depths[2];
    xiiRenderGraphBufferHandle  uniforms[2], sampling[2];
    for (xiiUInt32 branch = 0; branch < 2; ++branch)
    {
      const auto output = [&](xiiStringView name) {
        xiiStringBuilder key;
        key.SetFormat("{}{}", branch ? "Green" : "Red", name);
        return pipeline.GetInterfaceBinding(compiled->FindInterface(key, false));
      };
      colors[branch]   = output("Color")->m_hTexture;
      depths[branch]   = output("Depth")->m_hTexture;
      uniforms[branch] = output("RasterConstants")->m_hBuffer;
      sampling[branch] = output("SamplingConstants")->m_hBuffer;
      struct Export
      {
        xiiRenderGraphTextureHandle m_Color, m_Depth;
        xiiRenderGraphBufferHandle  m_Constants, m_Sampling;
      };
      graph.AddPass<Export>(branch ? "Retain green" : "Retain red", xiiGALCommandQueueFlags::Graphics, [&](Export& data, xiiRenderGraphBuilder& builder) {
        data.m_Color=builder.ReadTexture(colors[branch],xiiGALResourceStateFlags::CopySource); builder.ExportTexture(data.m_Color,xiiGALResourceStateFlags::CopySource);
        data.m_Depth=builder.ReadTexture(depths[branch],xiiGALResourceStateFlags::CopySource); builder.ExportTexture(data.m_Depth,xiiGALResourceStateFlags::CopySource);
        data.m_Constants=builder.ReadBuffer(uniforms[branch],xiiGALResourceStateFlags::CopySource); builder.ExportBuffer(data.m_Constants,xiiGALResourceStateFlags::CopySource);
        data.m_Sampling=builder.ReadBuffer(sampling[branch],xiiGALResourceStateFlags::CopySource); builder.ExportBuffer(data.m_Sampling,xiiGALResourceStateFlags::CopySource); }, [](const Export&, xiiRenderGraphPassContext&) {}, true);
    }
    if (phase == 3)
      pipeline.Clear(); // Record only owned captures after every pass instance is destroyed.
    graph.EndSetup();
    xiiStringBuilder error;
    if (!XII_TEST_RESULT_MSG(graph.Compile({.m_bEnableAsyncQueues = false}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
      return;
    XII_TEST_INT(graph.GetStatistics().m_uiQueueSubmissionCount, 1);
    frame.End();
    if (phase != 3)
      continue;
    environment.GetDevice()->WaitIdle();
    XII_TEST_BOOL(graph.GetExportedTexture(colors[0]) != graph.GetExportedTexture(colors[1]));
    for (xiiUInt32 branch = 0; branch < 2; ++branch)
    {
      const auto color = graph.GetExportedTexture(colors[branch]);
      const auto depth = graph.GetExportedTexture(depths[branch]);
      const auto size  = color->GetDescription().m_Size;
      xiiVec4    pixel;
      if (xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), color.Borrow(), size.width / 2, size.height / 2, pixel))
      {
        XII_TEST_FLOAT(pixel.x, branch ? 0.0f : 0.4f, 0.001f);
        XII_TEST_FLOAT(pixel.y, branch ? 0.3f : 0.0f, 0.001f);
        XII_TEST_FLOAT(pixel.z, 0, 0.001f);
        XII_TEST_FLOAT(pixel.w, 1, 0.001f);
      }
      if (xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), color.Borrow(), 0, 0, pixel))
      {
        XII_TEST_FLOAT(pixel.z, 0.25f, 0.001f);
        XII_TEST_FLOAT(pixel.w, 1, 0.001f);
      }
      float z = 0;
      if (xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), depth.Borrow(), size.width / 2, size.height / 2, z))
        // Identity clip projection and back-face culling expose the cube's -Z face:
        // translation 0.5 minus half of its authored 0.4 extent.
        XII_TEST_FLOAT(z, 0.3f, 0.0001f);
      xiiDirectionalMeshConstants       constants;
      xiiVirtualShadowSamplingConstants shadowConstants;
      if (xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(uniforms[branch]).Borrow(), 0, constants))
      {
        XII_TEST_FLOAT(constants.AmbientColor.x, branch ? 0 : 0.5f, 0.0001f);
        XII_TEST_FLOAT(constants.AmbientColor.y, branch ? 0.5f : 0, 0.0001f);
        XII_TEST_FLOAT(constants.SunDirectionIntensity.w, 0, 0.0001f);
      }
      if (xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(sampling[branch]).Borrow(), 0, shadowConstants))
      {
        XII_TEST_INT(shadowConstants.VirtualShadowDirectionalLightId, branch ? 7 : 3);
        XII_TEST_INT(shadowConstants.VirtualShadowResolution, 64);
        XII_TEST_INT(shadowConstants.VirtualShadowPageSize, 16);
        XII_TEST_INT(shadowConstants.VirtualShadowEnabled, 0);
      }
    }
    xiiTestFramework::Output(xiiTestOutput::Details, "Directional mesh: %u compiled logical operations, %u submissions, %u barriers.", graph.GetStatistics().m_uiCompiledPassCount, graph.GetStatistics().m_uiQueueSubmissionCount, graph.GetStatistics().m_uiTotalBarrierCount);
  }
}
