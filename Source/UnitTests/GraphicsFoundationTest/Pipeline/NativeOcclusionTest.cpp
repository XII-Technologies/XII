/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>

namespace
{
  xiiRenderGraphBufferHandle Upload(xiiRenderGraph& graph, xiiStringView name, xiiUInt32 stride, xiiArrayPtr<const xiiUInt8> bytes)
  {
    struct Data
    {
      xiiRenderGraphBufferHandle m_Buffer;
      xiiDynamicArray<xiiUInt8>  m_Bytes;
    };
    const auto emitted = graph.AddPass<Data>(name, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription description;
      description.m_uiSize = bytes.GetCount(); description.m_uiElementByteStride = stride;
      description.m_Mode = xiiGALBufferMode::Structured; description.m_BindFlags = xiiGALBindFlags::ShaderResource;
      data.m_Buffer = builder.WriteBuffer(name, description, xiiGALResourceStateFlags::CopyDestination);
      data.m_Bytes = bytes; }, [](const Data& data, xiiRenderGraphPassContext& context) { context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_Buffer), 0, data.m_Bytes); });
    return emitted.first->m_Buffer;
  }

  template <typename T> xiiRenderGraphBufferHandle Upload(xiiRenderGraph& graph, xiiStringView name, xiiArrayPtr<T> values)
  {
    return Upload(graph, name, sizeof(T), xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(values.GetPtr()), values.GetCount() * sizeof(T)));
  }

  xiiRenderPipelineResourceHandle MakeOcclusionGraph(bool pyramid, const xiiRenderPipelineResourceHandle& asset, const xiiRenderPipelineAuthoring& authoring)
  {
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({authoring.m_Descriptor.m_Id, asset.GetResourceID()});
    xiiUuid filterId;
    for (const auto& pass : authoring.m_Descriptor.m_Passes)
      if (pass.m_sType == "xiiInstanceOcclusionPass")
        filterId = pass.m_Id;
    const auto reduce = pyramid ? root.AddPass("Reduce supplied depth", "xiiDepthPyramidPass") : xiiInvalidIndex;
    if (pyramid)
      root.m_Interface.PushBack({"DepthPyramid", true, root.m_Passes[reduce].m_Id, "SceneDepth"});
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    xiiGetRenderPipelineInterface(authoring.m_Descriptor, pins).AssertSuccess();
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      const auto node     = root.AddSubgraph(i ? "Strict comparison" : "Biased comparison", authoring.m_Descriptor.m_Id);
      auto&      override = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      override.m_Path.PushBack(filterId);
      override.m_Properties.Insert("DepthBias", i ? 0.0f : 0.0005f);
      override.m_Properties.Insert("AsyncCompute", i == 0);
      for (const auto& pin : pins)
      {
        if (pin.m_Type == xiiRenderPipelinePin::Order)
          continue;
        if (pyramid && pin.m_bInput && pin.m_sName == "DepthPyramid")
        {
          root.Connect(reduce, "DepthPyramid", node, pin.m_sName).AssertSuccess();
          continue;
        }
        root.m_Interface.PushBack({pin.m_bInput ? xiiStringBuilder(pin.m_sName) : xiiStringBuilder(i ? "Strict/" : "Biased/", pin.m_sName), pin.m_bInput, root.m_Passes[node].m_Id, pin.m_sName});
      }
    }
    return xiiResourceManager::CreateResource<xiiRenderPipelineResource>(pyramid ? "Native occlusion with reduction" : "Native occlusion with incomplete pyramid", std::move(root));
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(NativeOcclusion);
XII_CREATE_SIMPLE_TEST(NativeOcclusion, AuthoredConservativeFiltering)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 asset = xiiLoadPipelineLibraryResource("InstanceOcclusion");
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  const auto                full       = MakeOcclusionGraph(true, asset, *loaded->GetAuthoring());
  const auto                incomplete = MakeOcclusionGraph(false, asset, *loaded->GetAuthoring());
  xiiRenderPipelineInstance pipelines[2];
  xiiRenderGraphContext     contexts[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
    if (!XII_TEST_RESULT(pipelines[i].Synchronize(i ? incomplete : full)) || !XII_TEST_RESULT(contexts[i].Initialize("Native occlusion regression")))
      return;

  for (xiiUInt32 phase = 0; phase < 6; ++phase)
  {
    const bool          hole = phase == 1, empty = phase == 3, invalidProjection = phase == 4;
    const xiiUInt32     graphIndex = phase == 2 ? 1 : 0;
    auto&               pipeline   = pipelines[graphIndex];
    auto&               context    = contexts[graphIndex];
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    auto& graph = context.GetGraph();
    auto& cache = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    xiiGpuSceneInstance records[11];
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(records); ++i)
    {
      records[i].m_uiFlags            = xiiGpuInstanceFlags::Enabled;
      records[i].m_uiObjectId         = 1 + i;
      records[i].m_BoundsCenterRadius = xiiVec4(0, 0, 0.2f, 0.04f);
      records[i].m_BoundsExtents      = xiiVec4(0.02f, 0.02f, 0.02f, 0);
    }
    records[1].m_BoundsCenterRadius = xiiVec4(0.4f, 0, 0.9f, 0.04f);
    records[2].m_BoundsCenterRadius = xiiVec4(-0.4f, 0, 0.8f, 0.04f);
    records[2].m_BoundsExtents.z    = 0;
    records[3].m_BoundsCenterRadius = xiiVec4(-0.6f, 0, 0.7797f, 0.04f);
    records[4].m_uiSkinningCount    = 1;
    records[5].m_GlobalTransform.SetTranslationVector(xiiVec3(0.1f, 0, 0));
    records[6].m_BoundsCenterRadius.z = -0.01f;
    records[7].m_BoundsCenterRadius.x = 2;
    // The old floor(log2(footprint)) four-corner test missed the central hole in this rectangle.
    records[8].m_BoundsCenterRadius = xiiVec4(0.015625f, -0.015625f, 0.2f, 0.1f);
    records[8].m_BoundsExtents      = xiiVec4(0.046875f, 0.046875f, 0.02f, 0);
    records[9].m_uiFlags            = 0;
    records[10].m_uiFlags |= xiiGpuInstanceFlags::AlwaysVisible;
    xiiGpuVisibilityView view;
    view.m_OcclusionViewProjectionMatrix = xiiMat4::MakeIdentity();
    if (invalidProjection)
      view.m_OcclusionViewProjectionMatrix.Element(0, 0) = xiiMath::NaN<float>();
    xiiUInt32 candidates[67];
    for (auto& index : candidates)
      index = xiiInvalidIndex;
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(records); ++i)
      candidates[i] = i;
    candidates[64]        = 1;
    candidates[66]        = 9;
    xiiUInt32  count      = empty ? 0 : phase == 5 ? 11 :
                                                     xiiMath::MaxValue<xiiUInt32>();
    const auto scene      = Upload(graph, "Occlusion test instances", xiiMakeArrayPtr(records));
    const auto projection = Upload(graph, "Occlusion test projection", xiiMakeArrayPtr(&view, 1));
    const auto list       = Upload(graph, "Occlusion test candidates", xiiMakeArrayPtr(candidates));
    const auto counter    = Upload(graph, "Occlusion test candidate count", xiiMakeArrayPtr(&count, 1));
    struct Depth
    {
      xiiRenderGraphTextureHandle m_Texture;
      xiiDynamicArray<float>      m_Pixels;
    };
    const auto                                       source   = graph.AddPass<Depth>("Occlusion test depth", xiiGALCommandQueueFlags::Transfer, [&](Depth& data, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription image;
      image.m_Type = xiiGALResourceDimension::Texture2D; image.m_Size = {64, 64}; image.m_Format = xiiGALResourceFormat::R32Float;
      image.m_BindFlags = xiiGALBindFlags::ShaderResource;
      data.m_Texture = builder.WriteTexture("Occlusion test depth", image, xiiGALResourceStateFlags::CopyDestination);
      data.m_Pixels.SetCount(4096); for (auto& pixel : data.m_Pixels) pixel = 0.8f;
      if (hole) data.m_Pixels[32 * 64 + 32] = 0; }, [](const Depth& data, xiiRenderGraphPassContext& execute) {
      xiiGALTextureSubResourceData upload;
      upload.m_pData = xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(data.m_Pixels.GetData()), data.m_Pixels.GetCount() * sizeof(float));
      upload.m_uiStride = 64 * sizeof(float);
      execute.GetCommandList().UpdateTexture(execute.GetTexture(data.m_Texture), {}, xiiBoundingBoxU32::MakeFromMinMax({0, 0, 0}, {64, 64, 1}), upload); });
    const auto*                                      compiled = pipeline.GetPipeline();
    xiiHybridArray<xiiRenderPipelineInputBinding, 5> inputs;
    inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("SceneInstances", true), scene));
    inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("OcclusionView", true), projection));
    inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("Candidates", true), list));
    inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("CandidateCount", true), counter));
    inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("DepthPyramid", true), source.first->m_Texture));
    if (!XII_TEST_RESULT(pipeline.Build(nullptr, graph, context.GetBlackboard(), {}, inputs)))
      return;
    xiiRenderGraphBufferHandle visible[2], counts[2];
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      visible[i] = pipeline.GetInterfaceBinding(compiled->FindInterface(i ? "Strict/VisibleInstances" : "Biased/VisibleInstances", false))->m_hBuffer;
      counts[i]  = pipeline.GetInterfaceBinding(compiled->FindInterface(i ? "Strict/VisibleInstanceCount" : "Biased/VisibleInstanceCount", false))->m_hBuffer;
      graph.AddPass<xiiUInt32>(i ? "Export strict" : "Export biased", xiiGALCommandQueueFlags::Graphics, [&, i](auto&, xiiRenderGraphBuilder& builder) {
        builder.ExportBuffer(builder.ReadBuffer(visible[i], xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource);
        builder.ExportBuffer(builder.ReadBuffer(counts[i], xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    }
    XII_TEST_BOOL(visible[0].m_Id != visible[1].m_Id && counts[0].m_Id != counts[1].m_Id);
    graph.EndSetup();
    xiiStringBuilder error;
    if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
      return;
    frame.End();
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      xiiUInt32 actualCount = xiiInvalidIndex;
      if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(counts[i]).Borrow(), 0, actualCount))
        return;
      xiiDynamicArray<xiiUInt32> expected;
      if (!empty)
      {
        for (xiiUInt32 index : {1U, 2U, 4U, 5U, 6U, 7U, 10U})
          expected.PushBack(index);
        if (phase != 5)
          expected.PushBack(1);
        if (!i || invalidProjection)
          expected.PushBack(3);
        if (hole || invalidProjection)
          expected.PushBack(0);
        if (hole || invalidProjection || graphIndex == 1)
          expected.PushBack(8);
      }
      XII_TEST_INT(actualCount, expected.GetCount());
      if (actualCount > XII_ARRAY_SIZE(candidates))
        return;
      struct Result
      {
        XII_DECLARE_POD_TYPE();
        xiiUInt32 m_Indices[67];
      } result;
      if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(visible[i]).Borrow(), 0, result))
        return;
      xiiDynamicArray<xiiUInt32> actual;
      for (xiiUInt32 index = 0; index < actualCount; ++index)
        actual.PushBack(result.m_Indices[index]);
      expected.Sort();
      actual.Sort();
      XII_TEST_BOOL(actual == expected);
    }
  }
}
