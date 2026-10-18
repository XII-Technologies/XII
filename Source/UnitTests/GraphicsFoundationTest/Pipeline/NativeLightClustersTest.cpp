/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterFrameData.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
XII_DEFINE_AS_POD_TYPE(xiiGpuClusterBounds);
namespace
{
  xiiDynamicArray<xiiGpuLightData> ClusterLights(xiiUInt32 phase)
  {
    xiiDynamicArray<xiiGpuLightData> result;
    result.SetCount(phase == 1 ? 1 : phase == 2 ? 137 :
                                                  8);
    xiiMemoryUtils::ZeroFill(result.GetData(), result.GetCount());
    for (xiiUInt32 i = 0; i < result.GetCount(); ++i)
    {
      auto& light                   = result[i];
      light.m_DirectionAndType.w    = i ? float(xiiGpuLightType::Point) : float(xiiGpuLightType::Directional);
      light.m_ColorAndIntensity     = xiiVec4(1, 1, 1, 1);
      light.m_BoundsCenterAndRadius = xiiVec4(0.25f, 0.25f, 3, 0.01f);
      light.m_Metadata              = xiiVec4U32(1000 + i, i, xiiUInt32(i ? xiiGpuLightType::Point : xiiGpuLightType::Directional), 0);
    }
    if (result.GetCount() > 1)
    {
      result[1].m_BoundsCenterAndRadius = xiiVec4(0, 0, 3, 50);
      result[2].m_BoundsCenterAndRadius = xiiVec4(0, 0, 3, 60);
      result[3].m_BoundsCenterAndRadius = xiiVec4(100, 100, 3, 1);
      result[4].m_BoundsCenterAndRadius = xiiVec4(0, 0, -20, 1);
      result[5].m_BoundsCenterAndRadius = xiiVec4(0.5f, 0.25f, 3, 0);
      result[6].m_BoundsCenterAndRadius = xiiVec4(0, 0, 3, 50);
      result[6].m_ColorAndIntensity.w   = 0;
      result[7].m_BoundsCenterAndRadius = xiiVec4(0, 0, 10, 0.01f);
    }
    return result;
  }
  xiiGpuClusterBounds AnalyticCluster(xiiUInt32 index, xiiUInt32 width, xiiUInt32 height, xiiUInt32 tile, xiiUInt32 slices, bool logarithmic, bool perspective, xiiVec3 position)
  {
    const auto columns = (width + tile - 1) / tile, rows = (height + tile - 1) / tile;
    const auto x = index % columns, y = (index / columns) % rows, z = index / (columns * rows);
    const auto depth = [&](xiiUInt32 slice) {
      const float t = float(slice) / slices;
      return logarithmic ? xiiMath::Pow(9.0f, t) : 1.0f + 8.0f * t;
    };
    const float nearDepth = depth(z), farDepth = depth(z + 1);
    // Independently derived from the asymmetric near plane [-1,2] x [-0.5,1].
    const float         left = -1 + 3.0f * float(x * tile) / width, right = -1 + 3.0f * float(xiiMath::Min((x + 1) * tile, width)) / width;
    const float         bottom = 1 - 1.5f * float(xiiMath::Min((y + 1) * tile, height)) / height, top = 1 - 1.5f * float(y * tile) / height;
    xiiGpuClusterBounds result;
    result.MinBounds = xiiVec4(xiiMath::Min(left * (perspective ? nearDepth : 1), left * (perspective ? farDepth : 1)), xiiMath::Min(bottom * (perspective ? nearDepth : 1), bottom * (perspective ? farDepth : 1)), nearDepth, 0) + xiiVec4(position, 0);
    result.MaxBounds = xiiVec4(xiiMath::Max(right * (perspective ? nearDepth : 1), right * (perspective ? farDepth : 1)), xiiMath::Max(top * (perspective ? nearDepth : 1), top * (perspective ? farDepth : 1)), farDepth, 0) + xiiVec4(position, 0);
    return result;
  }
  bool Intersects(const xiiGpuLightData& light, const xiiGpuClusterBounds& bounds)
  {
    if (light.m_DirectionAndType.w == 0 || light.m_ColorAndIntensity.w <= 0)
      return false;
    const auto    center  = light.m_BoundsCenterAndRadius.GetAsVec3();
    const auto    minimum = bounds.MinBounds.GetAsVec3(), maximum = bounds.MaxBounds.GetAsVec3();
    const xiiVec3 closest(xiiMath::Clamp(center.x, minimum.x, maximum.x), xiiMath::Clamp(center.y, minimum.y, maximum.y), xiiMath::Clamp(center.z, minimum.z, maximum.z));
    return (center - closest).GetLengthSquared() <= xiiMath::Square(light.m_BoundsCenterAndRadius.w);
  }
} // namespace
class xiiNativeClusterLightsSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNativeClusterLightsSource, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_Lights{false, xiiGALBindFlags::ShaderResource};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override
  {
    const auto* phase = context.m_Parameters.GetValue("ClusterTestPhase");
    struct Data
    {
      xiiRenderGraphBufferHandle       m_Lights;
      xiiDynamicArray<xiiGpuLightData> m_Records;
    };
    const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Records=ClusterLights(phase ? phase->Get<xiiUInt32>() : 0U);
      xiiGALBufferCreationDescription description; description.m_uiSize=data.m_Records.GetCount()*sizeof(xiiGpuLightData); description.m_uiElementByteStride=sizeof(xiiGpuLightData);
      description.m_Mode=xiiGALBufferMode::Structured; description.m_Usage=xiiGALResourceUsage::Mutable; description.m_BindFlags=xiiGALBindFlags::ShaderResource;
      data.m_Lights=builder.WriteBuffer("Explicit cluster test lights",description,xiiGALResourceStateFlags::CopyDestination); }, [](const Data& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Lights), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Records.GetData()), data.m_Records.GetCount() * sizeof(xiiGpuLightData))); });
    return context.SetOutput(m_Lights, pass.first->m_Lights);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNativeClusterLightsSource, 1, xiiRTTIDefaultAllocator<xiiNativeClusterLightsSource>)
  {
    XII_BEGIN_PROPERTIES
    {XII_MEMBER_PROPERTY("Lights", m_Lights)->AddFlags(xiiPropertyFlags::ReadOnly)} XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_CREATE_SIMPLE_TEST_GROUP(NativeLightClusters);
XII_CREATE_SIMPLE_TEST(NativeLightClusters, ExplicitProjectionAssignmentOverflowAndRetirement)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 asset = xiiLoadPipelineLibraryResource("ClusterLightLists");
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  const auto authored = loaded->GetAuthoring();
  xiiUuid    layoutNode;
  for (const auto& node : authored->m_Descriptor.m_Passes)
    if (node.m_sType == "xiiLightClusterViewPass")
      layoutNode = node.m_Id;
  if (!XII_TEST_BOOL(layoutNode.IsValid()))
    return;
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({authored->m_Descriptor.m_Id, asset.GetResourceID()});
  const auto lights = root.AddPass("Explicit synthetic light stream", "xiiNativeClusterLightsSource");
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto node      = root.AddSubgraph(i ? "Small lists" : "Large lists", authored->m_Descriptor.m_Id);
    auto&      overrides = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    overrides.m_Path.PushBack(layoutNode);
    overrides.m_Properties.Insert("UseViewCamera", false);
    overrides.m_Properties.Insert("Width", i ? 9U : 7U);
    overrides.m_Properties.Insert("Height", i ? 3U : 5U);
    overrides.m_Properties.Insert("TileSize", i ? 4U : 3U);
    overrides.m_Properties.Insert("DepthSlices", i ? 4U : 3U);
    overrides.m_Properties.Insert("NearDistance", 1.0f);
    overrides.m_Properties.Insert("FarDistance", 9.0f);
    overrides.m_Properties.Insert("MaxLightsPerCluster", i ? 1U : 16U);
    overrides.m_Properties.Insert("LogarithmicDepth", i != 0);
    XII_TEST_RESULT(root.Connect(lights, "Lights", node, "Lights"));
    for (const xiiStringView pin : {"Clusters", "LightGrid", "LightIndices", "Overflow", "PackedIndices", "SamplingTotals"})
      root.m_Interface.PushBack({xiiStringBuilder(i ? "B" : "A", pin), false, root.m_Passes[node].m_Id, pin});
  }
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Repeated authored cluster lists", std::move(root));
  xiiRenderGraphContext     contexts[2];
  xiiRenderPipelineInstance instances[2];
  for (xiiUInt32 view = 0; view < 2; ++view)
    if (!XII_TEST_RESULT(contexts[view].Initialize("Native clustered lighting")) || !XII_TEST_RESULT(instances[view].Synchronize(resource)))
      return;
  for (xiiUInt32 phase = 0; phase < 4; ++phase)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      auto& graph = contexts[view].GetGraph();
      auto& cache = contexts[view].GetResourceCache();
      cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      XII_SCOPE_EXIT(cache.EndFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      const auto           position    = view ? xiiVec3(10, -5, 0) : xiiVec3::MakeZero();
      const bool           perspective = view == 0;
      const auto           projection  = perspective ? xiiGraphicsUtils::CreatePerspectiveProjectionMatrix(-1, 2, -0.5f, 1, 1, phase == 3 ? xiiMath::Infinity<float>() : 10.0f, xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded) : xiiGraphicsUtils::CreateOrthographicProjectionMatrix(-1, 2, -0.5f, 1, 1, 10, xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
      const auto           inverse     = xiiMat4::MakeTranslation(position) * projection.GetInverse();
      xiiVariantDictionary parameters;
      parameters.Insert("ClusterTestPhase", phase);
      parameters.Insert("ClusterInverseViewProjection", inverse);
      parameters.Insert("ClusterCameraPosition", position);
      const auto built = instances[view].Build(nullptr, graph, contexts[view].GetBlackboard(), parameters);
      if (!XII_TEST_RESULT_MSG(built, "%s", instances[view].GetLastBuildError().GetStartPointer()))
        return;
      xiiRenderGraphBufferHandle bounds[2], grids[2], indices[2], overflows[2], packedIndices[2], totals[2];
      for (xiiUInt32 i = 0; i < 2; ++i)
      {
        const auto output = [&](xiiStringView pin) {
          return instances[view].GetInterfaceBinding(instances[view].GetPipeline()->FindInterface(xiiStringBuilder(i ? "B" : "A", pin), false))->m_hBuffer;
        };
        bounds[i]        = output("Clusters");
        grids[i]         = output("LightGrid");
        indices[i]       = output("LightIndices");
        overflows[i]     = output("Overflow");
        packedIndices[i] = output("PackedIndices");
        totals[i]        = output("SamplingTotals");
      }
      graph.AddPass<xiiUInt32>("Test explicit cluster readback exports", xiiGALCommandQueueFlags::Graphics, [&](auto&, xiiRenderGraphBuilder& builder) {for(xiiUInt32 i=0;i<2;++i){builder.ExportBuffer(builder.ReadBuffer(bounds[i],xiiGALResourceStateFlags::ShaderResource),xiiGALResourceStateFlags::ShaderResource);builder.ExportBuffer(builder.ReadBuffer(grids[i],xiiGALResourceStateFlags::ShaderResource),xiiGALResourceStateFlags::ShaderResource);builder.ExportBuffer(builder.ReadBuffer(indices[i],xiiGALResourceStateFlags::ShaderResource),xiiGALResourceStateFlags::ShaderResource);builder.ExportBuffer(builder.ReadBuffer(overflows[i],xiiGALResourceStateFlags::ShaderResource),xiiGALResourceStateFlags::ShaderResource); builder.ExportBuffer(builder.ReadBuffer(packedIndices[i],xiiGALResourceStateFlags::ShaderResource),xiiGALResourceStateFlags::ShaderResource); builder.ExportBuffer(builder.ReadBuffer(totals[i],xiiGALResourceStateFlags::ShaderResource),xiiGALResourceStateFlags::ShaderResource);} }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
      if (phase == 3)
        instances[view].Clear();
      graph.EndSetup();
      xiiStringBuilder error;
      const auto       compiled = graph.Compile({.m_bEnableAsyncQueues = false}, &error);
      if (!XII_TEST_RESULT_MSG(compiled, "%s", error.GetData()))
        return;
      const auto executed = graph.Execute(environment.GetDevice(), nullptr, &contexts[view].GetBlackboard(), &cache, nullptr, &error);
      if (!XII_TEST_RESULT_MSG(executed, "%s", error.GetData()))
        return;
      XII_TEST_INT(graph.GetStatistics().m_uiQueueSubmissionCount, 1);
      environment.GetDevice()->WaitIdle();
      const auto source = ClusterLights(phase);
      XII_TEST_BOOL(graph.GetExportedBuffer(grids[0]) != graph.GetExportedBuffer(grids[1]));
      for (xiiUInt32 i = 0; i < 2; ++i)
      {
        const xiiUInt32 width = i ? 9 : 7, height = i ? 3 : 5, tile = i ? 4 : 3, slices = i ? 4 : 3, capacity = i ? 1 : 16;
        const xiiUInt32 count            = ((width + tile - 1) / tile) * ((height + tile - 1) / tile) * slices;
        bool            expectedOverflow = false;
        for (xiiUInt32 c = 0; c < count; ++c)
        {
          const auto          expected = AnalyticCluster(c, width, height, tile, slices, i != 0, perspective, position);
          xiiGpuClusterBounds actual;
          xiiVec2U32          grid;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(bounds[i]).Borrow(), c * sizeof(actual), actual) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(grids[i]).Borrow(), c * sizeof(grid), grid))
            return;
          XII_TEST_BOOL(actual.MinBounds.IsEqual(expected.MinBounds, 0.001f));
          XII_TEST_BOOL(actual.MaxBounds.IsEqual(expected.MaxBounds, 0.001f));
          xiiDynamicArray<xiiUInt32> candidates;
          for (xiiUInt32 light = 0; light < source.GetCount(); ++light)
            if (Intersects(source[light], expected))
              candidates.PushBack(light);
          XII_TEST_INT(grid.x, c * capacity);
          XII_TEST_INT(grid.y, xiiMath::Min(candidates.GetCount(), capacity));
          expectedOverflow |= candidates.GetCount() > capacity;
          float totalPower = 0;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(totals[i]).Borrow(), c * sizeof(float), totalPower))
            return;
          XII_TEST_FLOAT(totalPower, grid.y * 4.0f * xiiMath::Pi<float>(), 0.001f);
          xiiDynamicArray<xiiUInt32> seen;
          xiiUInt32                  previousCDF = 0;
          for (xiiUInt32 slot = 0; slot < capacity; ++slot)
          {
            xiiUInt32 index = 0;
            if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(indices[i]).Borrow(), (grid.x + slot) * sizeof(index), index))
              return;
            xiiUInt32 packed = 0;
            if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(packedIndices[i]).Borrow(), (grid.x + slot) * sizeof(packed), packed))
              return;
            if (slot < grid.y)
            {
              XII_TEST_INT(packed & 0xFFFFFU, index);
              const auto cdf = xiiUInt32(xiiMath::Round(float(slot + 1) / grid.y * 4095.0f));
              // FP32 prefix sums can move a halfway rounding by one 12-bit quantization step.
              const auto actualCDF = packed >> 20U;
              XII_TEST_BOOL(xiiMath::Abs(xiiInt32(actualCDF) - xiiInt32(cdf)) <= 1);
              XII_TEST_BOOL(actualCDF >= previousCDF);
              previousCDF = actualCDF;
              if (slot + 1 == grid.y)
                XII_TEST_INT(actualCDF, 4095);
            }
            else
              XII_TEST_INT(packed, xiiInvalidIndex);
            if (slot >= grid.y)
            {
              XII_TEST_INT(index, xiiInvalidIndex);
              continue;
            }
            XII_TEST_BOOL(candidates.Contains(index));
            XII_TEST_BOOL(!seen.Contains(index));
            seen.PushBack(index);
          }
        }
        xiiUInt32 overflow = 0;
        if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(overflows[i]).Borrow(), 0, overflow))
          return;
        XII_TEST_INT(overflow, expectedOverflow ? 1 : 0);
      }
      xiiTestFramework::Output(xiiTestOutput::Details, "Native cluster phase %u view %u: %u compiled operations, one submission, %u barriers.", phase, view, graph.GetStatistics().m_uiCompiledPassCount, graph.GetStatistics().m_uiTotalBarrierCount);
    }
    frame.End();
  }
  // Invalid runtime policy is diagnosed at its original nested occurrence and clears old bindings.
  for (xiiUInt32 invalidCase = 0; invalidCase < 3; ++invalidCase)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    auto& graph = contexts[0].GetGraph();
    auto& cache = contexts[0].GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    if (!XII_TEST_RESULT(instances[0].Synchronize(resource)))
      return;
    xiiVariantDictionary parameters;
    if (invalidCase == 0)
      parameters.Insert("ClusterNear", -1.0f);
    else if (invalidCase == 2)
    {
      auto crossing = xiiMat4::MakeIdentity();
      crossing.SetRow(3, xiiVec4(2, 0, 0, 1));
      parameters.Insert("ClusterInverseViewProjection", crossing);
    }
    else
    {
      parameters.Insert("ClusterWidth", 65536U);
      parameters.Insert("ClusterHeight", 65536U);
    }
    const auto built = instances[0].Build(nullptr, graph, contexts[0].GetBlackboard(), parameters);
    XII_TEST_BOOL(built.Failed());
    XII_TEST_BOOL(instances[0].GetLastBuildError().FindSubString(invalidCase == 0 ? "Invalid cluster" : invalidCase == 1 ? "budget" :
                                                                                                                           "horizon") != nullptr);
    const auto* output = instances[0].GetInterfaceBinding(instances[0].GetPipeline()->FindInterface("ALightGrid", false));
    XII_TEST_BOOL(!output || !output->m_hBuffer.IsValid());
    graph.EndSetup();
    frame.End();
  }
}

XII_CREATE_SIMPLE_TEST(NativeLightClusters, LargeListDispatchAndPrivateInitialization)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  constexpr xiiUInt32                 count = 2049U * 2048U;
  xiiRenderPipelineResourceDescriptor descriptor;
  const auto                          layout     = descriptor.AddPass("Large explicit layout", "xiiLightClusterViewPass");
  auto&                               properties = descriptor.m_Passes[layout].m_Properties;
  properties.Insert("UseViewCamera", false);
  properties.Insert("Width", 2049U);
  properties.Insert("Height", 2048U);
  properties.Insert("TileSize", 1U);
  properties.Insert("DepthSlices", 1U);
  properties.Insert("MaxLightsPerCluster", 1U);
  properties.Insert("MaximumClusters", 8388608U);
  properties.Insert("MaximumIndices", 8388608U);
  const auto clear = descriptor.AddPass("Large list initialization", "xiiClusterListClearPass");
  XII_TEST_RESULT(descriptor.Connect(layout, "Layout", clear, "Layout"));
  for (const xiiStringView name : {"LightGrid", "LightIndices", "Overflow"})
    descriptor.m_Interface.PushBack({name, false, descriptor.m_Passes[clear].m_Id, name});
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Large explicit cluster initialization", std::move(descriptor));
  xiiRenderGraphContext     context;
  xiiRenderPipelineInstance instance;
  if (!XII_TEST_RESULT(context.Initialize("Large cluster dispatch")) || !XII_TEST_RESULT(instance.Synchronize(resource)))
    return;
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  auto& graph = context.GetGraph();
  auto& cache = context.GetResourceCache();
  cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame());
  graph.BeginSetup(frame.GetFrameIndex());
  const auto built = instance.Build(nullptr, graph, context.GetBlackboard());
  if (!XII_TEST_RESULT_MSG(built, "%s", instance.GetLastBuildError().GetStartPointer()))
    return;
  const auto output = [&](xiiStringView name) {
    return instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false))->m_hBuffer;
  };
  const auto grid = output("LightGrid"), indices = output("LightIndices"), overflow = output("Overflow");
  graph.AddPass<xiiUInt32>("Large initialized list readback", xiiGALCommandQueueFlags::Graphics, [&](auto&, xiiRenderGraphBuilder& builder) {builder.ExportBuffer(builder.ReadBuffer(grid,xiiGALResourceStateFlags::ShaderResource),xiiGALResourceStateFlags::ShaderResource);builder.ExportBuffer(builder.ReadBuffer(indices,xiiGALResourceStateFlags::ShaderResource),xiiGALResourceStateFlags::ShaderResource);builder.ExportBuffer(builder.ReadBuffer(overflow,xiiGALResourceStateFlags::ShaderResource),xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
  instance.Clear();
  graph.EndSetup();
  xiiStringBuilder error;
  const auto       compiled = graph.Compile({.m_bEnableAsyncQueues = false}, &error);
  if (!XII_TEST_RESULT_MSG(compiled, "%s", error.GetData()))
    return;
  const auto executed = graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error);
  if (!XII_TEST_RESULT_MSG(executed, "%s", error.GetData()))
    return;
  XII_TEST_INT(graph.GetStatistics().m_uiQueueSubmissionCount, 1);
  frame.End();
  environment.GetDevice()->WaitIdle();
  const auto groupsX     = environment.GetDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties.m_uiMaxThreadGroupCountX;
  const auto rowBoundary = xiiMath::Min(groupsX, (count + 63U) / 64U) * 64U;
  for (const auto index : {0U, xiiMath::Min(rowBoundary - 1, count - 1), xiiMath::Min(rowBoundary, count - 1), count - 1})
  {
    xiiVec2U32 range;
    xiiUInt32  value = 0;
    if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(grid).Borrow(), index * sizeof(range), range) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(indices).Borrow(), index * sizeof(value), value))
      return;
    XII_TEST_INT(range.x, index);
    XII_TEST_INT(range.y, 0);
    XII_TEST_INT(value, xiiInvalidIndex);
  }
  xiiUInt32 flag = 1;
  if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(overflow).Borrow(), 0, flag))
    return;
  XII_TEST_INT(flag, 0);
  xiiTestFramework::Output(xiiTestOutput::Details, "Initialized %u clusters/indices; device group-X limit %u, flattened row boundary %u; one submission.", count, groupsX, rowBoundary);
}
