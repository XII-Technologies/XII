#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/FrameSetupPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiFrameSetupPass, 1, xiiRTTIDefaultAllocator<xiiFrameSetupPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("Name",   m_sName)  ->AddAttributes(new xiiDefaultValueAttribute("FrameSetupPass")),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  struct xiiFrameSetupPassAutoReg
  {
    xiiFrameSetupPassAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiFrameSetupPass)); }
  };
  static xiiFrameSetupPassAutoReg s_AutoReg;
}

xiiFrameSetupPass::xiiFrameSetupPass() : xiiRenderPipelinePass("FrameSetupPass") {}
xiiFrameSetupPass::~xiiFrameSetupPass() = default;

namespace
{
  struct FrameSetupPassData
  {
    xiiUInt32 uiTimestampSlot = 0u;
  };
}

void xiiFrameSetupPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  // Lazy-create the timestamp readback ring.
  for (xiiUInt32 i = 0; i < k_uiTimestampRingSize; ++i)
  {
    if (!m_pTimestampBuffers[i])
    {
      xiiGALBufferCreationDescription desc;
      desc.m_uiSize        = sizeof(xiiUInt64) * 2u; // begin + end GPU timestamps
      desc.m_BufferFlags   = xiiGALBufferUsageFlags::None;
      desc.m_ResourceUsage = xiiGALResourceUsage::Readback;
      m_pTimestampBuffers[i] = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc);
    }
  }

  const xiiUInt32 uiSlot = m_uiCurrentTimestampSlot;
  m_uiCurrentTimestampSlot = (m_uiCurrentTimestampSlot + 1u) % k_uiTimestampRingSize;

  // Read GPU frame time written two frames ago (safe — ring size ensures no GPU stall).
  xiiGALBuffer* pReadbackBuffer = m_pTimestampBuffers[(uiSlot + 1u) % k_uiTimestampRingSize].Borrow();
  float fGPUFrameTimeMs = 0.0f;
  if (pReadbackBuffer)
  {
    xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
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
    [](FrameSetupPassData& data, xiiRGBuilder& builder)
    {
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [this, uiSlot](const FrameSetupPassData&, xiiRGPassContext& context)
    {
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
