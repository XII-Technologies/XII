#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/ReadbackAndTelemetryPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReadbackAndTelemetryPass, 1, xiiRTTIDefaultAllocator<xiiReadbackAndTelemetryPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("ReadbackAndTelemetryPass")), XII_MEMBER_PROPERTY("ReadbackInterval", m_uiReadbackIntervalFrames)->AddAttributes(new xiiDefaultValueAttribute(4u)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiReadbackAndTelemetryPass::xiiReadbackAndTelemetryPass() : xiiRenderPipelinePass("ReadbackAndTelemetryPass") {}
xiiReadbackAndTelemetryPass::~xiiReadbackAndTelemetryPass() = default;

namespace { struct ReadbackData { bool bDoReadback = false; xiiUInt32 uiSlot = 0u; }; }

void xiiReadbackAndTelemetryPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Lazy-create readback ring: GPU timestamp + per-frame stats.
  for (xiiUInt32 i = 0u; i < k_uiReadbackRingSize; ++i)
  {
    if (!m_pStatsReadback[i])
    {
      xiiGALBufferCreationDescription desc;
      desc.m_uiSize        = sizeof(xiiUInt64) * 16u; // up to 16 GPU timestamps
      desc.m_BufferFlags   = xiiGALBufferUsageFlags::None;
      desc.m_ResourceUsage = xiiGALResourceUsage::Readback;
      m_pStatsReadback[i] = pDevice->CreateBuffer(desc);
    }
  }

  xiiUInt32 uiFrame = 0u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_FrameIndex), uiFrame);

  const bool bDoReadback = (uiFrame % m_uiReadbackIntervalFrames) == 0u;
  const xiiUInt32 uiSlot = uiFrame % k_uiReadbackRingSize;

  auto [pData, hPass] = graph.AddPass<ReadbackData>(
    GetName(), xiiGALCommandQueueFlags::Graphics,
    [bDoReadback, uiSlot](ReadbackData& data, xiiRGBuilder& builder)
    {
      data.bDoReadback = bDoReadback;
      data.uiSlot      = uiSlot;
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [this](const ReadbackData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Readback + Telemetry");
      if (data.bDoReadback)
      {
        // Write end-of-frame timestamp into readback buffer for this slot.
        if (xiiGALBuffer* pBuf = m_pStatsReadback[data.uiSlot].Borrow())
          cmd.WriteTimestamp(pBuf, 0u);
      }
      // GPU pipeline statistics query resolve (draw/dispatch counts, primitive counts).
      cmd.ResolveQueryHeap();
      cmd.PopDebugGroup();
    },
    /*bHasSideEffects=*/true);
}
