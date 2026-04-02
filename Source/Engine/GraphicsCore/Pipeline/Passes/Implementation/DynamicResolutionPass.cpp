#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/DynamicResolutionPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicResolutionPass, 1, xiiRTTIDefaultAllocator<xiiDynamicResolutionPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Active",          m_bActive)          ->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("Name",            m_sName)            ->AddAttributes(new xiiDefaultValueAttribute("DynamicResolutionPass")),
    XII_MEMBER_PROPERTY("MinScale",        m_fMinScale)        ->AddAttributes(new xiiDefaultValueAttribute(0.5f)),
    XII_MEMBER_PROPERTY("MaxScale",        m_fMaxScale)        ->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("TargetFrameMs",   m_fTargetFrameTimeMs)->AddAttributes(new xiiDefaultValueAttribute(16.667f)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  struct xiiDynamicResolutionPassAutoReg
  {
    xiiDynamicResolutionPassAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiDynamicResolutionPass)); }
  };
  static xiiDynamicResolutionPassAutoReg s_AutoReg;
}

xiiDynamicResolutionPass::xiiDynamicResolutionPass() : xiiRenderPipelinePass("DynamicResolutionPass") {}
xiiDynamicResolutionPass::~xiiDynamicResolutionPass() = default;

namespace
{
  // Shader-side constants for DynamicResolution.xiiShader
  struct alignas(16) DynResConstants
  {
    float fFrameDeltaTimeMs        = 16.667f;
    float fTargetFrameTimeMs       = 16.667f;
    float fMinDynamicResolutionScale = 0.5f;
    float fMaxDynamicResolutionScale = 1.0f;
  };

  struct DynResPassData
  {
    xiiRGBufferHandle hTimingInput;
    xiiRGBufferHandle hVelocityInput;
    xiiRGBufferHandle hResolutionOutput;
  };
}

void xiiDynamicResolutionPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Lazy-create persistent structured buffers.
  if (!m_pFrameTimingBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(float) * 4u;
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer;
    desc.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_pFrameTimingBuffer = pDevice->CreateBuffer(desc);
  }
  if (!m_pCameraVelocityBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(float) * 4u;
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer;
    desc.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
    m_pCameraVelocityBuffer = pDevice->CreateBuffer(desc);
  }
  if (!m_pResolutionOutputBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(float) * 4u;
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_pResolutionOutputBuffer = pDevice->CreateBuffer(desc);
  }
  if (!m_pFrameConstantBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(DynResConstants);
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::ConstantBuffer;
    desc.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
    m_pFrameConstantBuffer = pDevice->CreateBuffer(desc);
  }

  // Upload latest timing constants to the constant buffer.
  DynResConstants constants;
  float fGPUMs = m_fTargetFrameTimeMs;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GPUFrameTimeMs), fGPUMs);
  constants.fFrameDeltaTimeMs         = fGPUMs;
  constants.fTargetFrameTimeMs        = m_fTargetFrameTimeMs;
  constants.fMinDynamicResolutionScale = m_fMinScale;
  constants.fMaxDynamicResolutionScale = m_fMaxScale;

  auto [pData, hPass] = graph.AddPass<DynResPassData>(
    GetName(),
    xiiGALCommandQueueFlags::Compute,
    [this](DynResPassData& data, xiiRGBuilder& builder)
    {
      data.hTimingInput    = builder.ImportBuffer("DynRes_Timing", m_pFrameTimingBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.hTimingInput    = builder.ReadBuffer(data.hTimingInput, xiiGALResourceStateFlags::ShaderResource);
      data.hVelocityInput  = builder.ImportBuffer("DynRes_Velocity", m_pCameraVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.hVelocityInput  = builder.ReadBuffer(data.hVelocityInput, xiiGALResourceStateFlags::ShaderResource);
      data.hResolutionOutput = builder.ImportBuffer("DynRes_Output", m_pResolutionOutputBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hResolutionOutput = builder.WriteBuffer(data.hResolutionOutput, xiiGALResourceStateFlags::UnorderedAccess);
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [this, constants](const DynResPassData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Dynamic Resolution");

      // Upload constants.
      cmd.UpdateBuffer(m_pFrameConstantBuffer.Borrow(), 0u, &constants, sizeof(DynResConstants));

      // Dispatch the single-thread resolution compute shader (DynamicResolution.xiiShader).
      // The shader reads FrameTimingData + CameraVelocityData and writes DynamicResolutionData[0].x = scale.
      cmd.Dispatch(1u, 1u, 1u);

      cmd.PopDebugGroup();
    },
    /*bHasSideEffects=*/true);

  // CPU-side readback: sample the output buffer from the previous frame and publish to blackboard.
  // This introduces a 1-frame latency between scale computation and resource sizing — acceptable.
  // On the very first frame, default to 1.0.
  float fScale = m_fMaxScale;
  const float* pScaleData = static_cast<const float*>(
    pDevice->MapBuffer(m_pResolutionOutputBuffer.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait));
  if (pScaleData)
  {
    fScale = xiiMath::Clamp(pScaleData[0], m_fMinScale, m_fMaxScale);
    pDevice->UnmapBuffer(m_pResolutionOutputBuffer.Borrow(), xiiGALMapType::Read);
  }

  const xiiViewData& viewData = view.GetData();
  const xiiUInt32 uiRenderWidth  = xiiMath::Max(1u, static_cast<xiiUInt32>(viewData.m_ViewPortRect.width  * fScale));
  const xiiUInt32 uiRenderHeight = xiiMath::Max(1u, static_cast<xiiUInt32>(viewData.m_ViewPortRect.height * fScale));

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DynamicResolutionScale), fScale);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),            uiRenderWidth);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight),           uiRenderHeight);
}
