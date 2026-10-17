/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

namespace
{
  float DepthValue(xiiUInt32 x, xiiUInt32 y, xiiUInt32 width, xiiUInt32 height, float base, bool uniform)
  {
    return uniform ? base : x + 1 == width || y + 1 == height ? base - 0.03f :
                                                                base + ((x + y * 3) % 7) * 0.01f;
  }
} // namespace

class xiiNativePyramidSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNativePyramidSource, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureOutputPin m_Output;
  float                             m_fBase            = 0.2f;
  bool                              m_bDepthAttachment = false;
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    const auto*      bias   = context.m_Parameters.GetValue("Native.DepthBias");
    const auto*      width  = context.m_Parameters.GetValue("Native.Width");
    const auto*      height = context.m_Parameters.GetValue("Native.Height");
    const float      base   = m_fBase + (bias ? bias->ConvertTo<float>() : 0);
    const xiiSizeU32 size{width ? width->ConvertTo<xiiUInt32>() : 7, height ? height->ConvertTo<xiiUInt32>() : 5};
    struct Data
    {
      xiiRenderGraphTextureHandle m_Texture;
      xiiDynamicArray<float>      m_Pixels;
      xiiSizeU32                  m_Size;
      float                       m_fBase;
      bool                        m_bDepth;
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bDepthAttachment ? xiiGALCommandQueueFlags::Graphics : xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription description;
      description.m_Type = xiiGALResourceDimension::Texture2D; description.m_Size = size;
      description.m_Format = m_bDepthAttachment ? xiiGALResourceFormat::D32Float : xiiGALResourceFormat::R32Float;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource;
      if (m_bDepthAttachment) description.m_BindFlags.Add(xiiGALBindFlags::DepthStencil);
      data.m_Texture = builder.WriteTexture(xiiStringBuilder(context.m_sPassName, " Synthetic depth"), description, m_bDepthAttachment ? xiiGALResourceStateFlags::DepthWrite : xiiGALResourceStateFlags::CopyDestination);
      data.m_Size = size; data.m_fBase = base; data.m_bDepth = m_bDepthAttachment;
      if (m_bDepthAttachment) builder.SetPassRenderPassManaged();
      else { const xiiUInt32 rowElements = xiiMemoryUtils::AlignSize(size.width * 4U, 16U) / 4U;
        data.m_Pixels.SetCount(rowElements * size.height); for (xiiUInt32 y = 0; y < size.height; ++y) for (xiiUInt32 x = 0; x < size.width; ++x) data.m_Pixels[y * rowElements + x] = DepthValue(x, y, size.width, size.height, base, false); } }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      if (data.m_bDepth) execute.GetCommandList().ClearDepthStencilView(execute.GetTexture(data.m_Texture)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, data.m_fBase, 0);
      else {
        xiiGALTextureSubResourceData upload;
        upload.m_pData = xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(data.m_Pixels.GetData()), data.m_Pixels.GetCount() * sizeof(float));
        upload.m_uiStride = xiiMemoryUtils::AlignSize(data.m_Size.width * 4U, 16U);
        execute.GetCommandList().UpdateTexture(execute.GetTexture(data.m_Texture), {}, xiiBoundingBoxU32::MakeFromMinMax({0, 0, 0}, {data.m_Size.width, data.m_Size.height, 1}), upload);
      } });
    return context.SetOutput(m_Output, emitted.first->m_Texture);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNativePyramidSource, 1, xiiRTTIDefaultAllocator<xiiNativePyramidSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Depth", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Base", m_fBase),
      XII_MEMBER_PROPERTY("DepthAttachment", m_bDepthAttachment),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiNativePyramidExport : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNativePyramidExport, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin m_Current{xiiGALResourceFormat::R32Float}, m_Previous{xiiGALResourceFormat::R32Float, true};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override
  {
    context.m_Graph.AddPass<xiiUInt32>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto&, xiiRenderGraphBuilder& builder) {
      builder.ExportTexture(builder.ReadTexture(context.GetInputTexture(m_Current), xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource);
      if (context.GetInputTexture(m_Previous).IsValid()) builder.ExportTexture(builder.ReadTexture(context.GetInputTexture(m_Previous), xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNativePyramidExport, 1, xiiRTTIDefaultAllocator<xiiNativePyramidExport>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Current", m_Current)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Previous", m_Previous)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  bool CheckPyramid(xiiGALDevice& device, xiiGALTexture* texture, xiiSizeU32 size, float base, bool uniform)
  {
    if (!XII_TEST_BOOL(texture && texture->GetDescription().m_Size == size))
      return false;
    xiiDynamicArray<float> expected;
    expected.SetCount(size.width * size.height);
    for (xiiUInt32 y = 0; y < size.height; ++y)
      for (xiiUInt32 x = 0; x < size.width; ++x)
        expected[y * size.width + x] = DepthValue(x, y, size.width, size.height, base, uniform);
    xiiGALTextureReadback readback(&device);
    auto                  commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    if (!XII_TEST_BOOL(commands != nullptr))
      return false;
    for (xiiUInt32 mip = 0; mip < texture->GetDescription().m_uiMipLevels; ++mip)
    {
      commands->Begin();
      const auto before = texture->GetResourceState();
      if (!XII_TEST_RESULT(readback.Enqueue(commands, {.m_pTexture = texture, .m_uiMipLevel = mip})))
        return false;
      xiiGALStateTransitionDescription transition;
      transition.m_pResource       = texture;
      transition.m_NewState        = before;
      transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
      commands->TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
      commands->End();
      device.GetCommandQueue()->WaitForFenceValue(device.GetCommandQueue()->Submit(commands));
      readback.WaitForNextCompleted();
      const auto capture = readback.GetCompleted();
      if (!XII_TEST_BOOL(static_cast<bool>(capture)))
        return false;
      XII_SCOPE_EXIT(readback.RecycleStagingTexture(capture.m_pStagingTexture));
      XII_TEST_INT(capture.m_uiWidth, size.width);
      XII_TEST_INT(capture.m_uiHeight, size.height);
      commands->Begin();
      xiiGALMappedTextureSubresource mapped;
      if (!XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
        return false;
      for (xiiUInt32 y = 0; y < size.height; ++y)
      {
        const auto* row = xiiMemoryUtils::AddByteOffset(static_cast<const float*>(mapped.m_pData), y * mapped.m_uiStride);
        for (xiiUInt32 x = 0; x < size.width; ++x)
          XII_TEST_FLOAT(row[x], expected[y * size.width + x], 0.00001f);
      }
      commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}).AssertSuccess();
      commands->End();
      commands->Reset();
      const xiiSizeU32       next{xiiMath::Max(size.width >> 1U, 1U), xiiMath::Max(size.height >> 1U, 1U)};
      xiiDynamicArray<float> reduced;
      reduced.SetCount(next.width * next.height);
      for (xiiUInt32 y = 0; y < next.height; ++y)
        for (xiiUInt32 x = 0; x < next.width; ++x)
        {
          float depth = 1;
          // Reference footprints are normalized pixel intervals, independent of the shader's odd-edge implementation.
          const xiiUInt32 beginX = static_cast<xiiUInt32>(xiiUInt64(x) * size.width / next.width);
          const xiiUInt32 beginY = static_cast<xiiUInt32>(xiiUInt64(y) * size.height / next.height);
          const xiiUInt32 endX   = static_cast<xiiUInt32>((xiiUInt64(x + 1) * size.width + next.width - 1) / next.width);
          const xiiUInt32 endY   = static_cast<xiiUInt32>((xiiUInt64(y + 1) * size.height + next.height - 1) / next.height);
          for (xiiUInt32 sy = beginY; sy < endY; ++sy)
            for (xiiUInt32 sx = beginX; sx < endX; ++sx)
              depth = xiiMath::Min(depth, expected[sy * size.width + sx]);
          reduced[y * next.width + x] = depth;
        }
      expected = std::move(reduced);
      size     = next;
    }
    XII_TEST_INT(expected.GetCount(), 1);
    return true;
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(NativeDepthPyramid);
XII_CREATE_SIMPLE_TEST(NativeDepthPyramid, AuthoredInterleavedHistory)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 asset = xiiLoadPipelineLibraryResource("DepthPyramid");
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  const auto                          definition = loaded->GetAuthoring();
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({definition->m_Descriptor.m_Id, asset.GetResourceID()});
  xiiUInt32 subgraphs[2];
  xiiUuid   subgraphIds[2], reduceId;
  for (const auto& pass : definition->m_Descriptor.m_Passes)
    if (pass.m_sType == "xiiDepthPyramidPass")
      reduceId = pass.m_Id;
  if (!XII_TEST_BOOL(reduceId.IsValid()))
    return;
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    xiiVariantDictionary properties;
    properties.Insert("Base", i ? 0.65f : 0.2f);
    properties.Insert("DepthAttachment", i != 0);
    const auto source = root.AddPass(i ? "Depth attachment" : "Odd-edge depth", "xiiNativePyramidSource", properties);
    subgraphs[i]      = root.AddSubgraph(i ? "Graphics depth history" : "Compute depth history", definition->m_Descriptor.m_Id);
    subgraphIds[i]    = root.m_Passes[subgraphs[i]].m_Id;
    const auto sink   = root.AddPass(i ? "Depth history readback" : "Odd-edge history readback", "xiiNativePyramidExport");
    XII_TEST_RESULT(root.Connect(source, "Depth", subgraphs[i], "SceneDepth"));
    XII_TEST_RESULT(root.Connect(subgraphs[i], "DepthPyramid", sink, "Current"));
    XII_TEST_RESULT(root.Connect(subgraphs[i], "PreviousDepthPyramid", sink, "Previous"));
    if (i)
    {
      auto& override = root.m_Passes[subgraphs[i]].m_Overrides.ExpandAndGetRef();
      override.m_Path.PushBack(reduceId);
      override.m_Properties.Insert("AsyncCompute", false);
    }
  }
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Native depth history instances", std::move(root));
  xiiRenderPipelineInstance instances[2];
  xiiRenderGraphContext     contexts[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
    if (!XII_TEST_RESULT(instances[i].Synchronize(resource)) || !XII_TEST_RESULT(contexts[i].Initialize("Interleaved native depth")))
      return;
  xiiUInt32   currentIndices[2] = {xiiInvalidIndex, xiiInvalidIndex}, historyIndices[2] = {xiiInvalidIndex, xiiInvalidIndex};
  const auto* pipeline = instances[0].GetPipeline();
  for (xiiUInt32 index = 0; index < pipeline->GetDescriptor().m_Passes.GetCount(); ++index)
  {
    const auto& source = pipeline->GetProvenance()[index];
    if (source.m_InstancePath.IsEmpty())
      continue;
    for (xiiUInt32 i = 0; i < 2; ++i)
      if (source.m_InstancePath[0] == subgraphIds[i])
      {
        if (pipeline->GetDescriptor().m_Passes[index].m_sType == "xiiDepthPyramidPass")
          currentIndices[i] = index;
        if (pipeline->GetDescriptor().m_Passes[index].m_sType == "xiiTextureHistoryPass")
          historyIndices[i] = index;
      }
  }
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    XII_TEST_BOOL(currentIndices[i] != xiiInvalidIndex && historyIndices[i] != xiiInvalidIndex);
  }
  bool       valid[2]    = {};
  float      previous[2] = {};
  xiiSizeU32 previousSize[2];
  const auto render = [&](xiiUInt32 view, float bias, xiiSizeU32 size, bool execute = true, bool readback = true) {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    auto& graph = contexts[view].GetGraph();
    auto& cache = contexts[view].GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    xiiVariantDictionary parameters;
    parameters.Insert("Native.DepthBias", bias);
    parameters.Insert("Native.Width", size.width);
    parameters.Insert("Native.Height", size.height);
    if (!XII_TEST_RESULT(instances[view].Build(nullptr, graph, contexts[view].GetBlackboard(), parameters)))
      return;
    graph.EndSetup();
    if (!XII_TEST_RESULT(graph.Compile()))
      return;
    if (!execute)
    {
      frame.End();
      return;
    }
    if (!XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &contexts[view].GetBlackboard(), &cache)))
      return;
    frame.End();
    xiiSharedPtr<xiiGALTexture> histories[2];
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      const auto current = instances[view].GetOutputBinding(currentIndices[i], "DepthPyramid");
      const auto history = instances[view].GetOutputBinding(historyIndices[i], "PreviousTexture");
      if (!XII_TEST_BOOL(current && history))
        return;
      XII_TEST_BOOL(history->m_hTexture.IsValid() == valid[view]);
      if (readback)
        CheckPyramid(*environment.GetDevice(), graph.GetExportedTexture(current->m_hTexture).Borrow(), size, (i ? 0.65f : 0.2f) + bias, i != 0);
      if (valid[view])
      {
        histories[i] = graph.GetExportedTexture(history->m_hTexture);
        if (readback)
          CheckPyramid(*environment.GetDevice(), histories[i].Borrow(), previousSize[view], (i ? 0.65f : 0.2f) + previous[view], i != 0);
      }
    }
    if (valid[view])
    {
      XII_TEST_BOOL(histories[0] != histories[1]);
    }
    valid[view]        = true;
    previous[view]     = bias;
    previousSize[view] = size;
  };
  for (xiiUInt32 iteration = 0; iteration < 4; ++iteration)
  {
    render(0, iteration * 0.01f, {7, 5});
    render(1, 0.1f + iteration * 0.01f, {7, 5});
    render(0, 0.15f, {7, 5}, false);
  }
  // Submit a burst with no device-idle or readback wait between frames.
  for (xiiUInt32 iteration = 0; iteration < 7; ++iteration)
  {
    render(0, iteration * 0.01f, {7, 5}, true, false);
    render(1, 0.1f + iteration * 0.01f, {7, 5}, true, false);
  }
  render(0, 0.08f, {7, 5});
  render(1, 0.18f, {7, 5});
  instances[0].InvalidateTemporalHistory();
  valid[0] = false;
  render(0, 0.03f, {9, 1});
  render(1, 0.05f, {7, 5});
  render(0, 0.06f, {9, 1});
}
