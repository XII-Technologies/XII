#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/ExposurePass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExposurePass, 1, xiiRTTIDefaultAllocator<xiiExposurePass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("ExposurePass")), XII_MEMBER_PROPERTY("MinEV100", m_fMinEV100)->AddAttributes(new xiiDefaultValueAttribute(-4.0f)), XII_MEMBER_PROPERTY("MaxEV100", m_fMaxEV100)->AddAttributes(new xiiDefaultValueAttribute(16.0f)), XII_MEMBER_PROPERTY("AdaptationSpeed", m_fAdaptationSpeed)->AddAttributes(new xiiDefaultValueAttribute(2.0f)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiExposureAutoReg { xiiExposureAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiExposurePass)); } }; static xiiExposureAutoReg s_AutoReg; }

xiiExposurePass::xiiExposurePass() : xiiRenderPipelinePass("ExposurePass") {}
xiiExposurePass::~xiiExposurePass() = default;

namespace
{
  struct alignas(16) ExposureConstants { float MinEV100, MaxEV100, LowPercent, HighPercent, AdaptationSpeed, DeltaTimeS, _pad[2]; };
  struct ExposureData { xiiRGTextureHandle hHDRScene; xiiRGBufferHandle hHistogram; xiiRGBufferHandle hExposure; xiiUInt32 uiW = 1u, uiH = 1u; };
}

void xiiExposurePass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (!m_pHistogramBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(xiiUInt32) * 256u;
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_pHistogramBuffer = pDevice->CreateBuffer(desc);
  }
  if (!m_pExposureBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(float) * 4u; // (exposure, avgLuminance, minLum, maxLum)
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_pExposureBuffer = pDevice->CreateBuffer(desc);
  }

  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);
  xiiRGTextureHandle hHDR;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), hHDR);

  const float fDelta = static_cast<float>(view.GetData().m_fDeltaTime);
  const ExposureConstants consts = { m_fMinEV100, m_fMaxEV100, m_fLowPercent, m_fHighPercent, m_fAdaptationSpeed, fDelta };

  auto [pData, hPass] = graph.AddPass<ExposureData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hHDR, this](ExposureData& data, xiiRGBuilder& builder)
    {
      if (hHDR.IsValid()) data.hHDRScene = builder.ReadTexture(hHDR, xiiGALResourceStateFlags::ShaderResource);
      data.hHistogram = builder.ImportBuffer("LuminanceHistogram", m_pHistogramBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hHistogram = builder.WriteBuffer(data.hHistogram, xiiGALResourceStateFlags::UnorderedAccess);
      data.hExposure  = builder.ImportBuffer("CurrentExposure",    m_pExposureBuffer,   xiiGALResourceStateFlags::UnorderedAccess);
      data.hExposure  = builder.WriteBuffer(data.hExposure,  xiiGALResourceStateFlags::UnorderedAccess);
      builder.SetPassSideEffects(true);
    },
    [consts](const ExposureData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Exposure + Eye Adaptation");
      // Pass 53: ExposureHistogram.xiiShader — atomic histogram of log2(luminance) per pixel.
      cmd.Dispatch((data.uiW + 15u) / 16u, (data.uiH + 15u) / 16u, 1u);
      // Pass 54: ExposureAdaptation.xiiShader — reads histogram, emits adapted exposure.
      cmd.Dispatch(1u, 1u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_LuminanceHistogram), pData->hHistogram);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_CurrentExposure),    pData->hExposure);
}
