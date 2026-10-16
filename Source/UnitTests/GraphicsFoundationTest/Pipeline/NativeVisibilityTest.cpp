/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>

class xiiNativeCullSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNativeCullSource, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_Scene{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Geometry{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_View{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Candidates{false, xiiGALBindFlags::ShaderResource};
  xiiUInt32                        m_uiBaseId   = 41;
  bool                             m_bSelectOne = false;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override
  {
    const auto*         parameter = context.m_Parameters.GetValue("Phase");
    const auto          phase     = parameter ? parameter->Get<xiiUInt32>() : 0U;
    const float         distance  = phase == 0 ? 4.5f : phase == 3 ? 4.1f :
                                                                     3.9f;
    const xiiUInt32     capacity  = phase >= 6 ? 129 : 67;
    xiiGpuSceneInstance records[129];
    for (xiiUInt32 i = 0; i < capacity; ++i)
    {
      records[i].m_uiGeometryIndex    = 0;
      records[i].m_uiObjectId         = m_uiBaseId + i + (phase >= 3 ? 100 : 0);
      records[i].m_uiFlags            = i == 65 ? 0 : xiiGpuInstanceFlags::Enabled;
      records[i].m_uiVisibilityMask   = i == 64 ? 0 : 1;
      records[i].m_BoundsCenterRadius = xiiVec4(i == 66 ? 102.0f : distance, 0, 0, 1);
    }
    xiiGpuGeometryRecord geometry;
    geometry.m_uiGeneration                     = phase >= 2 ? 8 : 7;
    geometry.m_uiLodCount                       = 2;
    geometry.m_uiResidentLodMask                = 3;
    geometry.m_Lods[0].m_fMinimumScreenCoverage = 16;
    geometry.m_Lods[1].m_fMinimumScreenCoverage = 0;
    xiiGpuVisibilityView view;
    view.m_ViewProjectionMatrix          = xiiMat4::MakeIdentity();
    view.m_OcclusionViewProjectionMatrix = xiiMat4::MakeIdentity();
    for (auto& plane : view.m_FrustumPlanes)
      plane = xiiVec4::MakeZero();
    view.m_FrustumPlanes[0] = xiiVec4(1, 0, 0, -100);
    view.m_CameraPosition   = xiiVec4::MakeZero();
    view.m_ViewportAndHiZ   = xiiVec4(64, 64, 0, 0);
    const auto token        = xiiUInt32(context.m_Graph.GetFrameIndex() % 65535U);
    view.m_LodParameters    = xiiVec4(0.1f, float(token), float((token + 65534U) % 65535U), 0);
    view.m_uiInstanceCount  = phase == 5 ? 0 : capacity;
    view.m_uiVisibilityMask = 1;
    view.m_uiUseCandidates  = m_bSelectOne ? 1 : 0;
    view.m_uiCandidateCount = phase == 5 ? 0 : m_bSelectOne ? 1 :
                                                              capacity;
    xiiUInt32 selection[129];
    for (xiiUInt32 i = 0; i < capacity; ++i)
      selection[i] = i;
    xiiRenderPassResourceScope scope(context);
    const auto                 upload = [&](xiiStringView name, xiiUInt32 stride, xiiArrayPtr<const xiiUInt8> bytes) {
      struct Data
      {
        xiiRenderGraphBufferHandle m_Output;
        xiiDynamicArray<xiiUInt8>  m_Bytes;
      };
      const auto emitted = context.m_Graph.AddPass<Data>(name, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
          xiiGALBufferCreationDescription description;
          description.m_uiSize = bytes.GetCount();
          description.m_uiElementByteStride = stride;
          description.m_Mode = xiiGALBufferMode::Structured;
          description.m_BindFlags = xiiGALBindFlags::ShaderResource;
          data.m_Output = builder.WriteBuffer(name, description, xiiGALResourceStateFlags::CopyDestination);
          data.m_Bytes = bytes; }, [](const Data& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Output), 0, data.m_Bytes); });
      return emitted.first->m_Output;
    };
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Scene, upload("Synthetic Instances", sizeof(xiiGpuSceneInstance), xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(records), capacity * sizeof(xiiGpuSceneInstance)))));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Geometry, upload("Synthetic Residency", sizeof(geometry), xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&geometry), sizeof(geometry)))));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_View, upload("Synthetic Culling View", sizeof(view), xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&view), sizeof(view)))));
    return context.SetOutput(m_Candidates, upload("Synthetic Candidates", sizeof(xiiUInt32), xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(selection), (m_bSelectOne ? 1 : capacity) * sizeof(xiiUInt32))));
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNativeCullSource, 1, xiiRTTIDefaultAllocator<xiiNativeCullSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("SceneInstances", m_Scene)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("GeometryMetadata", m_Geometry)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("VisibilityView", m_View)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Candidates", m_Candidates)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("BaseId", m_uiBaseId),
      XII_MEMBER_PROPERTY("SelectOne", m_bSelectOne),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiNativeCullExport : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNativeCullExport, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin m_Visible, m_Count, m_Lod;
  xiiRenderPipelineBufferInputPin m_Previous{true};
  xiiResult                       Build(xiiRenderPipelinePassContext& context) override
  {
    context.m_Graph.AddPass<xiiUInt32>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto&, xiiRenderGraphBuilder& builder) {
        for (auto handle : {context.GetInputBuffer(m_Visible), context.GetInputBuffer(m_Count), context.GetInputBuffer(m_Lod), context.GetInputBuffer(m_Previous)})
          if (handle.IsValid()) builder.ExportBuffer(builder.ReadBuffer(handle, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNativeCullExport, 1, xiiRTTIDefaultAllocator<xiiNativeCullExport>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Visible", m_Visible)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Count", m_Count)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Lod", m_Lod)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Previous", m_Previous)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST_GROUP(NativeVisibility);
XII_CREATE_SIMPLE_TEST(NativeVisibility, InstanceCullingAndExplicitHistory)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor leaf;
  leaf.AddPass("Input snapshots", "xiiNativeCullSource");
  leaf.AddPass("Invalidate current LOD", "xiiLodHistoryResetPass");
  leaf.AddPass("Reset visible count", "xiiCounterResetPass");
  leaf.AddPass("Cull instances", "xiiInstanceCullPass");
  leaf.AddPass("Retain LOD history", "xiiBufferHistoryPass");
  leaf.AddPass("Read back", "xiiNativeCullExport");
  for (auto pin : {"SceneInstances", "GeometryMetadata", "VisibilityView", "Candidates"})
    XII_TEST_RESULT(leaf.Connect(0, pin, 3, pin));
  XII_TEST_RESULT(leaf.Connect(0, "SceneInstances", 1, "SceneInstances"));
  XII_TEST_RESULT(leaf.Connect(1, "LodHistory", 3, "InitializedLodHistory"));
  XII_TEST_RESULT(leaf.Connect(2, "Counter", 3, "Counter"));
  XII_TEST_RESULT(leaf.Connect(3, "LodHistory", 4, "Buffer"));
  XII_TEST_RESULT(leaf.Connect(4, "PreviousBuffer", 3, "PreviousLodHistory"));
  XII_TEST_RESULT(leaf.Connect(3, "VisibleInstances", 5, "Visible"));
  XII_TEST_RESULT(leaf.Connect(3, "VisibleInstanceCount", 5, "Count"));
  XII_TEST_RESULT(leaf.Connect(3, "LodHistory", 5, "Lod"));
  XII_TEST_RESULT(leaf.Connect(4, "PreviousBuffer", 5, "Previous"));
  const auto         cullId = leaf.m_Passes[3].m_Id, historyId = leaf.m_Passes[4].m_Id;
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("Full candidate set", definition.m_Id);
  root.AddSubgraph("One selected component", definition.m_Id);
  const auto selectedId = root.m_Passes[1].m_Id;
  auto&      override   = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
  override.m_Path.PushBack(leaf.m_Passes[0].m_Id);
  override.m_Properties.Insert("BaseId", 1001U);
  override.m_Properties.Insert("SelectOne", true);
  xiiContiguousMemoryStreamStorage bytes;
  xiiMemoryStreamWriter            writer(&bytes);
  XII_TEST_RESULT(root.Serialize(writer));
  xiiMemoryStreamReader               reader(&bytes);
  xiiRenderPipelineResourceDescriptor decoded;
  XII_TEST_RESULT(decoded.Deserialize(reader));
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  if (!XII_TEST_RESULT(xiiRenderPipeline::Compile(decoded, compiled)))
    return;
  XII_TEST_BOOL(compiled->GetRuntimeFeatures().IsNoFlagSet() && compiled->GetExtractionRequirements().IsNoFlagSet());
  xiiRenderPipelineInstance instances[2];
  xiiRenderGraphContext     contexts[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    if (!XII_TEST_RESULT(instances[i].Initialize(compiled)) || !XII_TEST_RESULT(contexts[i].Initialize("Native culling view")))
      return;
  }
  for (xiiUInt32 phase = 0; phase < 8; ++phase)
  {
    if (phase == 4)
      instances[0].InvalidateTemporalHistory();
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    xiiSharedPtr<xiiGALBuffer> histories[4];
    xiiUInt32                  retained = 0;
    xiiUInt32                  checked  = 0;
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      auto& graph = contexts[view].GetGraph();
      auto& cache = contexts[view].GetResourceCache();
      cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      XII_SCOPE_EXIT(cache.EndFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      xiiVariantDictionary parameters;
      parameters.Insert("Phase", phase);
      if (!XII_TEST_RESULT(instances[view].Build(nullptr, graph, contexts[view].GetBlackboard(), parameters)))
        return;
      graph.EndSetup();
      xiiStringBuilder error;
      if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &contexts[view].GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
        return;
      for (xiiUInt32 i = 0; i < compiled->GetDescriptor().m_Passes.GetCount(); ++i)
      {
        const auto& source   = compiled->GetProvenance()[i];
        const bool  selected = source.m_InstancePath[0] == selectedId;
        if (source.m_Element == cullId)
        {
          ++checked;
          auto      count        = graph.GetExportedBuffer(instances[view].GetOutputBinding(i, "VisibleInstanceCount")->m_hBuffer);
          auto      lod          = graph.GetExportedBuffer(instances[view].GetOutputBinding(i, "LodHistory")->m_hBuffer);
          xiiUInt32 visibleCount = xiiInvalidIndex;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), count.Borrow(), 0, visibleCount))
            return;
          const xiiUInt32 expectedCount = phase == 5 ? 0 : selected ? 1 :
            phase >= 6                                              ? 126 :
                                                                      64;
          XII_TEST_INT(visibleCount, expectedCount);
          XII_TEST_INT(lod->GetDescription().m_uiSize, (phase >= 6 ? 129 : 67) * sizeof(xiiVec4U32));
          xiiVec4U32 choice;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), lod.Borrow(), 0, choice))
            return;
          const auto expected = phase == 5 ? xiiInvalidIndex : phase <= 1 || phase == 3 || (phase == 4 && view == 1) ? 1U :
                                                                                                                       0U;
          XII_TEST_INT(choice.x, expected);
          if (phase != 5)
          {
            XII_TEST_INT(choice.y, phase >= 2 ? 8 : 7);
            XII_TEST_INT(choice.z, (selected ? 1001 : 41) + (phase >= 3 ? 100 : 0));
            XII_TEST_INT(choice.w, frame.GetFrameIndex() % 65535U);
          }
          if (selected)
          {
            if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), lod.Borrow(), sizeof(xiiVec4U32), choice))
              return;
            XII_TEST_INT(choice.x, xiiInvalidIndex);
          }
          else if (phase >= 6)
          {
            if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), lod.Borrow(), 128 * sizeof(xiiVec4U32), choice))
              return;
            XII_TEST_INT(choice.x, 0);
            XII_TEST_INT(choice.z, 41 + 128 + 100);
          }
        }
        if (source.m_Element == historyId)
        {
          const auto* binding = instances[view].GetOutputBinding(i, "PreviousBuffer");
          const bool  missing = phase == 0 || (phase == 4 && view == 0);
          XII_TEST_BOOL(binding && binding->m_hBuffer.IsValid() != missing);
          if (missing)
            continue;
          histories[retained] = graph.GetExportedBuffer(binding->m_hBuffer);
          if (!XII_TEST_BOOL(histories[retained] != nullptr))
            return;
          XII_TEST_INT(histories[retained]->GetDescription().m_uiSize, (phase >= 7 ? 129 : 67) * sizeof(xiiVec4U32));
          ++retained;
        }
      }
    }
    XII_TEST_INT(checked, 4);
    XII_TEST_INT(retained, phase == 0 ? 0 : phase == 4 ? 2 :
                                                         4);
    for (xiiUInt32 i = 0; i < retained; ++i)
      for (xiiUInt32 j = 0; j < i; ++j)
        XII_TEST_BOOL(histories[i] != histories[j]);
    frame.End();
    environment.GetDevice()->WaitIdle();
    XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
  }
}
