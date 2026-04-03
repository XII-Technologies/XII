#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/FrameSetupPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Device/Device.h>

namespace
{
  struct FrameSetupPassData
  {
    xiiUInt32 uiTimestampSlot = 0u;
  };
} // namespace

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiFrameSetupPass, 1, xiiRTTIDefaultAllocator<xiiFrameSetupPass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiFrameSetupPass::xiiFrameSetupPass() :
  xiiRenderPipelinePass("FrameSetupPass")
{
}

xiiFrameSetupPass::~xiiFrameSetupPass() = default;

void xiiFrameSetupPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  // Lazy-create the timestamp readback ring.
  for (xiiUInt32 i = 0; i < s_uiTimestampRingSize; ++i)
  {
    if (!m_pTimestampBuffers[i])
    {
      xiiGALBufferCreationDescription description;
      description.m_uiSize         = sizeof(xiiUInt64) * 2U; // Begin + end GPU timestamps.
      description.m_Usage          = xiiGALResourceUsage::Staging;
      description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;

      m_pTimestampBuffers[i] = pDevice->CreateBuffer(description);
    }
  }

  const xiiUInt32 uiSlot   = m_uiCurrentTimestampSlot;
  m_uiCurrentTimestampSlot = (m_uiCurrentTimestampSlot + 1U) % s_uiTimestampRingSize;

  // Read GPU frame time written two frames ago (safe, since ring size ensures no GPU stall).
  xiiGALBuffer* pReadbackBuffer = m_pTimestampBuffers[(uiSlot + 1U) % s_uiTimestampRingSize].Borrow();
  float         fGPUFrameTimeMs = 0.0f;
  if (pReadbackBuffer)
  {
    auto pGraphicsQueue = pDevice->GetCommandQueue();
    xiiSharedPtr<xiiGALCommandList> pCommandList = pDevice->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});


    const xiiUInt64* pData = static_cast<const xiiUInt64*>(pDevice->MapBuffer(pReadbackBuffer, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait));
    if (pData)
    {
      const xiiUInt64 uiFreq = pDevice->GetTimestampFrequency();
      if (uiFreq > 0 && pData[1] > pData[0])
        fGPUFrameTimeMs = static_cast<float>((pData[1] - pData[0]) * 1000.0 / static_cast<double>(uiFreq));
      pDevice->UnmapBuffer(pReadbackBuffer, xiiGALMapType::Read);
    }
  }

  // Publish GPU timing to blackboard so DynamicResolutionPass can consume it.
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_GPUFrameTimeMs), fGPUFrameTimeMs);

  auto [pData, hPass] = graph.AddPass<FrameSetupPassData>(
    GetName(),
    xiiGALCommandQueueFlags::Graphics,
    [](FrameSetupPassData& data, xiiRGBuilder& builder) {
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [this, uiSlot](const FrameSetupPassData&, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("XII Frame");

      // Write begin timestamp into this frame's readback slot.
      xiiGALBuffer* pDst = m_pTimestampBuffers[uiSlot].Borrow();
      if (pDst)
      {
        cmd.WriteTimestamp(pDst, 0u);
      }
    },
    /*bHasSideEffects=*/true);

  pData->uiTimestampSlot = uiSlot;
}
