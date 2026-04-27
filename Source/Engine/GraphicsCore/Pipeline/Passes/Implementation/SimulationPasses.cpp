#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/SimulationPasses.h>
#include <GAL/Device/GALDevice.h>
#include <GAL/CommandList/GALCommandList.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDepthNoiseModel,       1, xiiRTTINoAllocator)                                  XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSegmentationPass,      1, xiiRTTIDefaultAllocator<xiiSegmentationPass>)        XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDepthGroundTruthPass,  1, xiiRTTIDefaultAllocator<xiiDepthGroundTruthPass>)    XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiOpticalFlowPass,       1, xiiRTTIDefaultAllocator<xiiOpticalFlowPass>)         XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNormalGroundTruthPass, 1, xiiRTTIDefaultAllocator<xiiNormalGroundTruthPass>)   XII_END_DYNAMIC_REFLECTED_TYPE;

// ---- Helper: full-screen PSO creation ----
static xiiGALPipelineStateHandle CreateFullscreenPSO(const char* dbg, const char* ps)
{
  xiiGALDevice* pDev = xiiGALDevice::GetDefaultDevice();
  if (!pDev) return {};
  xiiGALGraphicsPipelineCreationDescription d;
  d.m_sDebugName    = dbg;
  d.m_sVertexShader = "Shaders/FullscreenTriangleVS.hlsl";
  d.m_sPixelShader  = ps;
  d.m_uiRenderTargetCount = 1;
  d.m_DepthStencilState.m_bDepthEnable = false;
  return pDev->CreateGraphicsPipelineState(d);
}

// ============================================================
// xiiSegmentationPass
// ============================================================

xiiSegmentationPass::xiiSegmentationPass()  : xiiRenderGraphPass("SegmentationPass", true) {}
xiiSegmentationPass::~xiiSegmentationPass() {}

bool xiiSegmentationPass::GetRenderTargetDescriptions(const xiiView& view,
  const xiiArrayPtr<xiiGALTextureCreationDescription* const>, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  const xiiRectU32& vp = view.GetViewport();
  outputs[0].m_Format     = xiiGALTextureFormat::R32Uint;
  outputs[0].m_uiWidth    = vp.width;
  outputs[0].m_uiHeight   = vp.height;
  outputs[0].m_BindFlags  = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  outputs[0].m_sDebugName = "SegmentationID";
  return true;
}

void xiiSegmentationPass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const>,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const>)
{
  if (m_bInitialised) return;
  m_hSegmentPSO = CreateFullscreenPSO("SegmentationPSO", "Shaders/Simulation/Segmentation.hlsl");
  m_bInitialised = true;
}

void xiiSegmentationPass::Execute(const xiiRenderViewContext& ctx,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  xiiGALCommandList& cmd = *ctx.m_pCommandList;
  xiiGALRenderPassBeginInfo rp; rp.m_uiRenderTargetCount=1; rp.m_pRenderTargets[0]=outputs[0]->m_pTextureView; rp.m_pClearColors[0]=xiiColor(0,0,0,0); rp.m_bClearColor=true;
  cmd.BeginRenderPass(rp);
  if (m_hSegmentPSO.IsValid()) { cmd.SetGraphicsPipelineState(m_hSegmentPSO); cmd.Draw(3,1,0,0); }
  cmd.EndRenderPass();
  XII_IGNORE_UNUSED(inputs);
}

void xiiSegmentationPass::ExecuteInactive(const xiiRenderViewContext& ctx,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> i, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> o)
{ XII_IGNORE_UNUSED(ctx); XII_IGNORE_UNUSED(i); XII_IGNORE_UNUSED(o); }

// ============================================================
// xiiDepthGroundTruthPass
// ============================================================

xiiDepthGroundTruthPass::xiiDepthGroundTruthPass()  : xiiRenderGraphPass("DepthGroundTruthPass", true) {}
xiiDepthGroundTruthPass::~xiiDepthGroundTruthPass() {}

bool xiiDepthGroundTruthPass::GetRenderTargetDescriptions(const xiiView& view,
  const xiiArrayPtr<xiiGALTextureCreationDescription* const>, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  const xiiRectU32& vp = view.GetViewport();
  outputs[0].m_Format     = xiiGALTextureFormat::R32Float;
  outputs[0].m_uiWidth    = vp.width; outputs[0].m_uiHeight = vp.height;
  outputs[0].m_BindFlags  = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  outputs[0].m_sDebugName = "LinearDepth";
  return true;
}

void xiiDepthGroundTruthPass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const>,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const>)
{
  if (m_bInitialised) return;
  m_hLinearisePSO = CreateFullscreenPSO("DepthLinearisePSO", "Shaders/Simulation/LineariseDepth.hlsl");
  m_bInitialised = true;
}

void xiiDepthGroundTruthPass::Execute(const xiiRenderViewContext& ctx,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  xiiGALCommandList& cmd = *ctx.m_pCommandList;
  xiiGALRenderPassBeginInfo rp; rp.m_uiRenderTargetCount=1; rp.m_pRenderTargets[0]=outputs[0]->m_pTextureView; rp.m_pClearColors[0]=xiiColor(m_fMaxDepth,0,0,0); rp.m_bClearColor=true;
  cmd.BeginRenderPass(rp);
  if (m_hLinearisePSO.IsValid())
  {
    // Bind depth SRV from inputs[0], push near/far constants, noise params
    cmd.SetGraphicsPipelineState(m_hLinearisePSO);
    cmd.Draw(3,1,0,0);
  }
  cmd.EndRenderPass();
  XII_IGNORE_UNUSED(inputs);
}

void xiiDepthGroundTruthPass::ExecuteInactive(const xiiRenderViewContext& ctx,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> i, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> o)
{ XII_IGNORE_UNUSED(ctx); XII_IGNORE_UNUSED(i); XII_IGNORE_UNUSED(o); }

// ============================================================
// xiiOpticalFlowPass
// ============================================================

xiiOpticalFlowPass::xiiOpticalFlowPass()  : xiiRenderGraphPass("OpticalFlowPass", true) {}
xiiOpticalFlowPass::~xiiOpticalFlowPass() {}

bool xiiOpticalFlowPass::GetRenderTargetDescriptions(const xiiView& view,
  const xiiArrayPtr<xiiGALTextureCreationDescription* const>, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  const xiiRectU32& vp = view.GetViewport();
  outputs[0].m_Format     = xiiGALTextureFormat::RG16Float;
  outputs[0].m_uiWidth    = vp.width; outputs[0].m_uiHeight = vp.height;
  outputs[0].m_BindFlags  = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  outputs[0].m_sDebugName = "OpticalFlow";
  return true;
}

void xiiOpticalFlowPass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const>,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const>)
{
  if (m_bInitialised) return;
  m_hFlowPSO = CreateFullscreenPSO("OpticalFlowPSO", "Shaders/Simulation/OpticalFlow.hlsl");
  m_bInitialised = true;
}

void xiiOpticalFlowPass::Execute(const xiiRenderViewContext& ctx,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  xiiGALCommandList& cmd = *ctx.m_pCommandList;
  xiiGALRenderPassBeginInfo rp; rp.m_uiRenderTargetCount=1; rp.m_pRenderTargets[0]=outputs[0]->m_pTextureView; rp.m_bClearColor=true; rp.m_pClearColors[0]=xiiColor::Black;
  cmd.BeginRenderPass(rp);
  if (m_hFlowPSO.IsValid()) { cmd.SetGraphicsPipelineState(m_hFlowPSO); cmd.Draw(3,1,0,0); }
  cmd.EndRenderPass();
  XII_IGNORE_UNUSED(inputs);
}

void xiiOpticalFlowPass::ExecuteInactive(const xiiRenderViewContext& ctx,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> i, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> o)
{ XII_IGNORE_UNUSED(ctx); XII_IGNORE_UNUSED(i); XII_IGNORE_UNUSED(o); }

// ============================================================
// xiiNormalGroundTruthPass
// ============================================================

xiiNormalGroundTruthPass::xiiNormalGroundTruthPass()  : xiiRenderGraphPass("NormalGroundTruthPass", true) {}
xiiNormalGroundTruthPass::~xiiNormalGroundTruthPass() {}

bool xiiNormalGroundTruthPass::GetRenderTargetDescriptions(const xiiView& view,
  const xiiArrayPtr<xiiGALTextureCreationDescription* const>, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  const xiiRectU32& vp = view.GetViewport();
  for (xiiUInt32 i=0; i<2; ++i)
  {
    outputs[i].m_Format     = xiiGALTextureFormat::RGBA16Float;
    outputs[i].m_uiWidth    = vp.width; outputs[i].m_uiHeight = vp.height;
    outputs[i].m_BindFlags  = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  }
  outputs[0].m_sDebugName = "WorldNormals";
  outputs[1].m_sDebugName = "CameraNormals";
  return true;
}

void xiiNormalGroundTruthPass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const>,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const>)
{
  if (m_bInitialised) return;
  xiiGALDevice* pDev = xiiGALDevice::GetDefaultDevice(); if (!pDev) return;
  xiiGALGraphicsPipelineCreationDescription d;
  d.m_sDebugName = "NormalGTPSO";
  d.m_sVertexShader = "Shaders/FullscreenTriangleVS.hlsl";
  d.m_sPixelShader  = "Shaders/Simulation/NormalGroundTruth.hlsl";
  d.m_uiRenderTargetCount = 2;
  d.m_DepthStencilState.m_bDepthEnable = false;
  m_hNormalPSO = pDev->CreateGraphicsPipelineState(d);
  m_bInitialised = true;
}

void xiiNormalGroundTruthPass::Execute(const xiiRenderViewContext& ctx,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  xiiGALCommandList& cmd = *ctx.m_pCommandList;
  xiiGALRenderPassBeginInfo rp; rp.m_uiRenderTargetCount=2;
  rp.m_pRenderTargets[0]=outputs[0]->m_pTextureView; rp.m_pRenderTargets[1]=outputs[1]->m_pTextureView;
  rp.m_pClearColors[0]=rp.m_pClearColors[1]=xiiColor::Black; rp.m_bClearColor=true;
  cmd.BeginRenderPass(rp);
  if (m_hNormalPSO.IsValid()) { cmd.SetGraphicsPipelineState(m_hNormalPSO); cmd.Draw(3,1,0,0); }
  cmd.EndRenderPass();
  XII_IGNORE_UNUSED(inputs);
}

void xiiNormalGroundTruthPass::ExecuteInactive(const xiiRenderViewContext& ctx,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> i, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> o)
{ XII_IGNORE_UNUSED(ctx); XII_IGNORE_UNUSED(i); XII_IGNORE_UNUSED(o); }

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_SimulationPasses);
