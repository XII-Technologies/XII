/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/MeshDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowTablesPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>

namespace
{
  template <typename T>
  xiiRenderGraphBufferHandle UploadVirtualInput(xiiRenderGraph& graph, xiiStringView name, xiiArrayPtr<T> source)
  {
    struct Data
    {
      xiiRenderGraphBufferHandle m_Buffer;
      xiiDynamicArray<xiiUInt8>  m_Bytes;
    };
    const auto pass = graph.AddPass<Data>(name, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription buffer;
      buffer.m_Mode = xiiGALBufferMode::Structured; buffer.m_BindFlags = xiiGALBindFlags::ShaderResource;
      buffer.m_uiSize = source.GetCount() * sizeof(T); buffer.m_uiElementByteStride = sizeof(T);
      data.m_Buffer = builder.WriteBuffer(name, buffer, xiiGALResourceStateFlags::CopyDestination);
      data.m_Bytes.SetCountUninitialized(static_cast<xiiUInt32>(buffer.m_uiSize));
      xiiMemoryUtils::Copy(data.m_Bytes.GetData(), reinterpret_cast<const xiiUInt8*>(source.GetPtr()), data.m_Bytes.GetCount()); }, [](const Data& data, xiiRenderGraphPassContext& context) { context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_Buffer), 0, data.m_Bytes); });
    return pass.first->m_Buffer;
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(NativeVirtualShadowPages);
XII_CREATE_SIMPLE_TEST(NativeVirtualShadowPages, ResourceRasterAndRevisionSafeCompletion)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  if (environment.GetDevice()->GetFeatures().m_MeshShaders != xiiGALDeviceFeatureState::Enabled)
  {
    xiiTestFramework::Output(xiiTestOutput::Message, "Native virtual pages require the explicitly selected mesh raster resource.");
    return;
  }
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiVirtualShadowMapSettings settings;
  settings.m_uiVirtualResolution = settings.m_uiPageSize = 16;
  settings.m_uiPhysicalPageCount                         = 2;
  settings.m_uiMaxPageAllocations = settings.m_uiMaxPageRasterizations = 2;
  settings.m_uiMaxFeedbackRequests                                     = 8;
  if (!XII_TEST_RESULT(xiiVirtualShadowMapManager::Configure(settings)) || !XII_TEST_RESULT(xiiGeometryResidencyManager::EnsureInitialized()) || !XII_TEST_RESULT(xiiMaterialManager::EnsureInitialized()))
    return;
  xiiGeometry box;
  box.AddBox(xiiVec3(0.4f), true);
  xiiMeshBufferResourceDescriptor mesh;
  mesh.AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, true);
  const auto             buffer = xiiResourceManager::CreateResource<xiiMeshBufferResource>("Virtual page cube", std::move(mesh));
  xiiGeometryDescription geometryDefinition;
  geometryDefinition.m_Lods.ExpandAndGetRef().m_hMeshBuffer = buffer;
  const auto geometryHandle                                 = xiiGeometryResidencyManager::RegisterGeometry(geometryDefinition);
  XII_SCOPE_EXIT(xiiGeometryResidencyManager::UnregisterGeometry(geometryHandle, 0));
  xiiGeometryResidencyManager::RequestResidency(geometryHandle, 0, 0);
  const auto                resource = xiiLoadPipelineLibraryResource("ShadowMeshDepth");
  xiiRenderPipelineInstance pipelines[2];
  xiiUInt64                 revisions[2] = {};
  for (auto& pipeline : pipelines)
    if (!XII_TEST_RESULT(pipeline.Synchronize(resource)))
      return;
  XII_TEST_BOOL(pipelines[0].GetPipeline() == pipelines[1].GetPipeline());
  xiiRenderGraphContext context;
  if (!XII_TEST_RESULT(context.Initialize("Native virtual page graph")))
    return;
  xiiMeshDrawCommandsPass commands;
  commands.m_uiMeshGroupCountX = 1;
  xiiDynamicArray<xiiRenderPipelinePin> commandPins;
  commands.GetPins(commandPins);
  XII_TEST_RESULT(commands.InitializePinBindings(commandPins));
  xiiRenderPipelineInstance tables;
  if (!XII_TEST_RESULT(tables.Synchronize(xiiLoadPipelineLibraryResource("VirtualShadowTables"))))
    return;
  xiiRenderPipelineInstance workPipelines[2], publisher;
  for (auto& work : workPipelines)
    if (!XII_TEST_RESULT(work.Synchronize(xiiLoadPipelineLibraryResource("VirtualShadowRasterWork"))))
      return;
  if (!XII_TEST_RESULT(publisher.Synchronize(xiiLoadPipelineLibraryResource("VirtualShadowPagePublish"))))
    return;
  xiiVirtualShadowRasterPage staleRecycled;
  for (xiiUInt32 phase = 0; phase < 5; ++phase)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    if (phase == 0 || phase == 3)
    {
      xiiVirtualShadowPageRequest requests[2];
      requests[0].m_Page.m_uiLightId = 1;
      requests[1].m_Page.m_uiLightId = phase == 3 ? 3 : 2;
      requests[0].m_uiPriority       = 10;
      requests[1].m_uiPriority       = 20;
      xiiVirtualShadowMapManager::SubmitFeedback(phase == 0 ? xiiMakeArrayPtr(requests) : xiiMakeArrayPtr(&requests[1], 1));
    }
    if (phase == 3)
    {
      xiiVirtualShadowPageMapping replacement;
      xiiVirtualShadowPageId      page;
      page.m_uiLightId = 3;
      XII_TEST_BOOL(xiiVirtualShadowMapManager::TryGetMapping(page, replacement));
      XII_TEST_INT(replacement.m_uiPhysicalPage, staleRecycled.m_uiPhysicalPage);
      XII_TEST_BOOL(!xiiVirtualShadowMapManager::MarkPageRendered(staleRecycled));
    }
    if (phase == 4)
    {
      XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiDirtyPageCount, 0);
      XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiRasterizedPageCount, 2);
      break;
    }
    auto& graph = context.GetGraph();
    auto& cache = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    xiiGeometryResidencyManager::ProcessStreaming(frame.GetFrameIndex(), frame.GetCompletedFrame(), xiiMath::MaxValue<xiiUInt64>());
    const auto geometry  = xiiGeometryResidencyManager::AddUploadPass(graph, frame.GetFrameIndex());
    const auto materials = xiiMaterialManager::AddUploadPasses(graph);
    if (!XII_TEST_RESULT(tables.Build(nullptr, graph, context.GetBlackboard())))
      return;
    const auto residency = tables.GetInterfaceBinding(tables.GetPipeline()->FindInterface("Residency", false))->m_pFrameData.Downcast<const xiiVirtualShadowResidencySnapshot>();
    if (!XII_TEST_BOOL(residency != nullptr))
      return;
    auto                                        upload = residency->m_Bindings;
    xiiDynamicArray<xiiVirtualShadowRasterPage> pages;
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      const auto light      = i ? (phase == 3 ? 3U : 2U) : 1U;
      auto       projection = xiiMat4::MakeScaling(xiiVec3(light == 2 ? 2.0f : 1.0f, light == 2 ? 2.0f : 1.0f, -1));
      projection.SetTranslationVector(xiiVec3(0, 0, 1));
      xiiStaticArray<xiiShadowCascadeDescription, 4> cascades;
      cascades.ExpandAndGetRef().m_mViewProjection = projection;
      xiiRenderPipelineBinding projectionInput, residencyInput;
      projectionInput.m_pFrameData                   = XII_DEFAULT_NEW(xiiShadowCascadeSnapshot, graph.GetSetupSerial(), cascades, 16U, xiiRenderGraphBufferHandle());
      residencyInput.m_pFrameData                    = residency;
      const auto*                         definition = workPipelines[i].GetPipeline();
      const xiiRenderPipelineInputBinding inputs[]   = {{graph, definition->FindInterface("Cascades", true), projectionInput}, {graph, definition->FindInterface("Residency", true), residencyInput}};
      xiiVariantDictionary                parameters;
      parameters.Insert("ShadowRasterLightId", light);
      parameters.Insert("ShadowRasterMaxPages", 1U);
      if (!XII_TEST_RESULT(workPipelines[i].Build(nullptr, graph, context.GetBlackboard(), parameters, xiiMakeArrayPtr(inputs))))
        return;
      const auto batch = workPipelines[i].GetInterfaceBinding(definition->FindInterface("RasterPages", false))->m_pFrameData.Downcast<const xiiVirtualShadowRasterBatch>();
      if (!XII_TEST_BOOL(batch && batch->m_Pages.GetCount() <= 1))
        return;
      pages.PushBackRange(batch->m_Pages);
    }
    if (!XII_TEST_INT(pages.GetCount(), phase == 0 || phase == 3 ? 2 : 1))
      return;
    if (phase == 0)
      staleRecycled = pages[1];
    if (phase == 0)
    {
      struct Initial
      {
        xiiRenderGraphTextureHandle m_Image;
      };
      upload.m_hPhysicalAtlas = graph.AddPass<Initial>("Initialize virtual atlas fixture", xiiGALCommandQueueFlags::Graphics, [&](Initial& data, xiiRenderGraphBuilder& builder) {
        data.m_Image = builder.ReadWriteTexture(upload.m_hPhysicalAtlas, xiiGALResourceStateFlags::DepthWrite);
        builder.SetPassRenderPassManaged(); }, [](const Initial& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().ClearDepthStencilView(execute.GetTexture(data.m_Image)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0.9f, 0); }).first->m_Image;
    }
    const auto* record = xiiGeometryResidencyManager::GetGpuRecord(geometryHandle);
    if (!XII_TEST_BOOL(record && record->m_uiResidentLodMask == 1))
      return;
    xiiGpuSceneInstance scene;
    scene.m_uiGeometryIndex          = geometryHandle.GetInternalID().m_InstanceIndex;
    scene.m_uiObjectId               = 1;
    scene.m_GlobalTransform          = xiiMat4::MakeTranslation(xiiVec3(0, 0, 0.5f));
    scene.m_PreviousGlobalTransform  = scene.m_GlobalTransform;
    xiiMat4                     pose = xiiMat4::MakeIdentity();
    xiiDynamicArray<xiiVec2U32> pairs;
    for (xiiUInt32 i = 0; i < record->m_Lods[0].m_uiMeshletCount; ++i)
      pairs.PushBack(xiiVec2U32(0, record->m_Lods[0].m_uiMeshletMetadataOffset + i));
    xiiUInt32                                 count        = pairs.GetCount();
    const auto                                instances    = UploadVirtualInput(graph, "Virtual scene", xiiMakeArrayPtr(&scene, 1));
    const auto                                skinning     = UploadVirtualInput(graph, "Virtual pose", xiiMakeArrayPtr(&pose, 1));
    const auto                                visible      = UploadVirtualInput(graph, "Virtual pairs", pairs.GetArrayPtr());
    const auto                                visibleCount = UploadVirtualInput(graph, "Virtual count", xiiMakeArrayPtr(&count, 1));
    xiiDynamicArray<xiiRenderPipelineBinding> bindings;
    bindings.SetCount(commandPins.GetCount());
    bindings[commands.m_Visible.GetBindingIndex()].m_hBuffer = visible;
    bindings[commands.m_Count.GetBindingIndex()].m_hBuffer   = visibleCount;
    xiiVariantDictionary         parameters;
    xiiRenderPipelinePassContext prepare{nullptr, graph, context.GetBlackboard(), parameters, commandPins, bindings, "Virtual mesh commands"};
    if (!XII_TEST_RESULT(commands.Build(prepare)))
      return;
    for (const auto& page : pages)
    {
      auto& pipeline = pipelines[page.m_uiPhysicalPage];
      if (revisions[page.m_uiPhysicalPage] != page.m_uiContentRevision)
      {
        pipeline.InvalidateTemporalHistory();
        revisions[page.m_uiPhysicalPage] = page.m_uiContentRevision;
      }
      const auto*                                       compiled = pipeline.GetPipeline();
      xiiHybridArray<xiiRenderPipelineInputBinding, 10> inputs;
      const auto                                        bind = [&](xiiStringView name, xiiRenderGraphBufferHandle value) {
        inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface(name, true), value));
      };
      bind("SceneInstances", instances);
      bind("SkinningMatrices", skinning);
      bind("VisibleMeshlets", visible);
      bind("VisibleMeshletCount", visibleCount);
      bind("GeometryMetadata", geometry.m_hGeometryMetadata);
      bind("MeshletMetadata", geometry.m_hMeshletMetadata);
      bind("SurfaceMaterials", materials.m_hSurfaceData);
      bind("DrawCommands", bindings[commands.m_Commands.GetBindingIndex()].m_hBuffer);
      bind("DrawCommandCount", bindings[commands.m_CommandCount.GetBindingIndex()].m_hBuffer);
      inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("DepthTarget", true), upload.m_hPhysicalAtlas));
      parameters.Insert("RasterViewProjection", page.m_ViewProjection);
      parameters.Insert("RasterWidth", 16U);
      parameters.Insert("RasterHeight", 16U);
      parameters.Insert("RasterMeshGroupCountX", 1U);
      parameters.Insert("RasterMeshGroupCountY", 1U);
      parameters.Insert("RasterViewport", xiiVec4U32(page.m_Viewport.x, page.m_Viewport.y, 16, 16));
      if (!XII_TEST_RESULT(pipeline.Build(nullptr, graph, context.GetBlackboard(), parameters, inputs)))
        return;
      upload.m_hPhysicalAtlas = pipeline.GetInterfaceBinding(compiled->FindInterface("ShadowDepth", false))->m_hTexture;
    }
    auto                     published   = upload;
    const auto*              publication = publisher.GetPipeline();
    xiiRenderPipelineBinding batchInput;
    batchInput.m_pFrameData                                 = XII_DEFAULT_NEW(xiiVirtualShadowRasterBatch, graph.GetSetupSerial(), residency, xiiDynamicArray<xiiVirtualShadowRasterPage>(pages));
    const xiiRenderPipelineInputBinding publicationInputs[] = {
      {graph, publication->FindInterface("RasterPages", true), batchInput},
      {graph, publication->FindInterface("RasterAtlas", true), upload.m_hPhysicalAtlas},
      {graph, publication->FindInterface("PhysicalPageTable", true), upload.m_hPhysicalPageTable},
      {graph, publication->FindInterface("VirtualPageTable", true), upload.m_hVirtualPageTable}};
    if (phase == 0)
    {
      xiiHybridArray<xiiRenderPipelineInputBinding, 4> invalidInputs;
      invalidInputs.PushBackRange(xiiMakeArrayPtr(publicationInputs));
      invalidInputs[2].m_Binding.m_hBuffer = upload.m_hVirtualPageTable;
      xiiRenderPipelineInstance invalid;
      XII_TEST_RESULT(invalid.Synchronize(xiiLoadPipelineLibraryResource("VirtualShadowPagePublish")));
      XII_TEST_BOOL(invalid.Build(nullptr, graph, context.GetBlackboard(), {}, invalidInputs).Failed());
      XII_TEST_BOOL(invalid.GetLastBuildError().FindSubString("captured device allocations") != nullptr);
      invalid.Clear();
      invalidInputs[2] = publicationInputs[2];
      auto duplicate   = pages;
      duplicate.PushBack(pages[0]);
      invalidInputs[0].m_Binding.m_pFrameData = XII_DEFAULT_NEW(xiiVirtualShadowRasterBatch, graph.GetSetupSerial(), residency, std::move(duplicate));
      XII_TEST_RESULT(invalid.Synchronize(xiiLoadPipelineLibraryResource("VirtualShadowPagePublish")));
      XII_TEST_BOOL(invalid.Build(nullptr, graph, context.GetBlackboard(), {}, invalidInputs).Failed());
      XII_TEST_BOOL(invalid.GetLastBuildError().FindSubString("stale, duplicate") != nullptr);
      XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiDirtyPageCount, 2);
    }
    if (!XII_TEST_RESULT(publisher.Build(nullptr, graph, context.GetBlackboard(), {}, xiiMakeArrayPtr(publicationInputs))))
      return;
    published.m_hPhysicalAtlas     = publisher.GetInterfaceBinding(publication->FindInterface("ReadyAtlas", false))->m_hTexture;
    published.m_hPhysicalPageTable = publisher.GetInterfaceBinding(publication->FindInterface("ReadyPhysicalPageTable", false))->m_hBuffer;
    published.m_hVirtualPageTable  = publisher.GetInterfaceBinding(publication->FindInterface("ReadyVirtualPageTable", false))->m_hBuffer;
    if (phase == 0)
    {
      XII_TEST_BOOL(xiiVirtualShadowMapManager::InvalidatePage(pages[0].m_Page));
    }
    const auto atlas = published.m_hPhysicalAtlas;
    graph.AddPass<xiiUInt32>("Export native virtual atlas", xiiGALCommandQueueFlags::Graphics, [atlas](auto&, xiiRenderGraphBuilder& builder) { builder.ExportTexture(builder.ReadTexture(atlas, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    graph.EndSetup();
    if (phase == 1)
    {
      frame.End(); // Abandoned setup must not publish CPU residency or consume the next frame's budget.
      XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiDirtyPageCount, 1);
      continue;
    }
    xiiStringBuilder error;
    if (!XII_TEST_RESULT_MSG(graph.Compile({.m_bEnableAsyncQueues = phase == 0}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
      return;
    const auto& statistics = graph.GetStatistics();
    xiiTestFramework::Output(xiiTestOutput::Details, "Virtual pages (phase %u): %u logical operations, %u queue submissions, %u render-pass groups.", phase, statistics.m_uiCompiledPassCount, statistics.m_uiQueueSubmissionCount, statistics.m_uiMergeGroupCount);
    if (phase != 0)
    {
      XII_TEST_INT(statistics.m_uiQueueSubmissionCount, 1);
    }
    frame.End();
    environment.GetDevice()->WaitIdle();
    XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiDirtyPageCount, phase == 2 ? 1 : 2);
    const auto image = graph.GetExportedTexture(atlas);
    for (const auto& page : pages)
    {
      float value = 0;
      if (xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), image.Borrow(), page.m_Viewport.x + 8, 8, value))
      {
        XII_TEST_FLOAT(value, 0.7f, 0.001f);
      }
      if (xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), image.Borrow(), page.m_Viewport.x + 5, 8, value))
      {
        XII_TEST_FLOAT(value, page.m_Page.m_uiLightId == 2 ? 0.7f : 0.0f, 0.001f);
      }
    }
    const auto table         = graph.GetExportedBuffer(published.m_hVirtualPageTable);
    const auto physicalTable = graph.GetExportedBuffer(published.m_hPhysicalPageTable);
    xiiUInt32  matched       = 0;
    for (xiiUInt32 bucket = 0; bucket < published.m_uiVirtualTableCapacity; ++bucket)
    {
      xiiGpuVirtualShadowPage entry;
      if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), table.Borrow(), (published.m_uiVirtualTableBaseIndex + bucket) * sizeof(entry), entry))
        return;
      for (const auto& page : pages)
        if (entry.m_uiVirtualKeyLow == xiiUInt32(page.m_Page.GetPackedValue()) && entry.m_uiVirtualKeyHigh == xiiUInt32(page.m_Page.GetPackedValue() >> 32))
        {
          ++matched;
          XII_TEST_INT(entry.m_uiFlags & 2U, phase == 0 && page.m_Page.m_uiLightId == 1 ? 2 : 0);
          xiiGpuVirtualShadowPage physicalEntry;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), physicalTable.Borrow(), (published.m_uiFrameBaseIndex + page.m_uiPhysicalPage) * sizeof(physicalEntry), physicalEntry))
            return;
          XII_TEST_INT(physicalEntry.m_uiVirtualKeyLow, entry.m_uiVirtualKeyLow);
          XII_TEST_INT(physicalEntry.m_uiVirtualKeyHigh, entry.m_uiVirtualKeyHigh);
          XII_TEST_INT(physicalEntry.m_uiFlags, entry.m_uiFlags);
        }
    }
    XII_TEST_INT(matched, pages.GetCount());
    if (phase == 2)
    {
      XII_TEST_BOOL(xiiVirtualShadowMapManager::InvalidatePage(pages[0].m_Page));
    }
  }
}
