#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/DeferredLightingPass.h>
#include <GAL/Device/GALDevice.h>
#include <GAL/CommandList/GALCommandList.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDeferredLightingPass, 1, xiiRTTIDefaultAllocator<xiiDeferredLightingPass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTranslucencyPass, 1, xiiRTTIDefaultAllocator<xiiTranslucencyPass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

// ==========================================================
// xiiDeferredLightingPass
// ==========================================================

xiiDeferredLightingPass::xiiDeferredLightingPass() : xiiRenderGraphPass("DeferredLightingPass", true) {}
xiiDeferredLightingPass::~xiiDeferredLightingPass() {}

bool xiiDeferredLightingPass::GetRenderTargetDescriptions(const xiiView& view,
  const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs,
  xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  XII_IGNORE_UNUSED(inputs);
  const xiiRectU32& vp = view.GetViewport();
  outputs[0].m_Format     = xiiGALTextureFormat::RGBA16Float;
  outputs[0].m_uiWidth    = vp.width;
  outputs[0].m_uiHeight   = vp.height;
  outputs[0].m_BindFlags  = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  outputs[0].m_sDebugName = "LitHDR";
  return true;
}

void xiiDeferredLightingPass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
  if (m_bInitialised) return;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice, "No GAL device");

  // Tile classification compute PSO
  {
    xiiGALComputePipelineCreationDescription d;
    d.m_sDebugName      = "TileClassifyPSO";
    d.m_sShaderFilePath = "Shaders/Lighting/TileClassify.hlsl";
    d.m_sEntryPoint     = "CSMain";
    m_hTileClassifyPSO = pDevice->CreateComputePipelineState(d);
  }

  // Full-screen lighting evaluation PSO
  {
    xiiGALGraphicsPipelineCreationDescription d;
    d.m_sDebugName    = "DeferredLightingPSO";
    d.m_sVertexShader = "Shaders/FullscreenTriangleVS.hlsl";
    d.m_sPixelShader  = "Shaders/Lighting/DeferredLighting.hlsl";
    d.m_uiRenderTargetCount = 1;
    d.m_DepthStencilState.m_bDepthEnable = false;
    m_hLightingPSO = pDevice->CreateGraphicsPipelineState(d);
  }

  // Light data structured buffer (up to 16k lights)
  {
    xiiGALBufferCreationDescription bd;
    bd.m_sDebugName          = "LightDataBuffer";
    bd.m_uiSize              = 16384 * 64; // 64 bytes per light
    bd.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bd.m_ResourceUsage       = xiiGALResourceUsage::Dynamic;
    bd.m_CPUAccessFlags      = xiiGALCPUAccessFlags::Write;
    bd.m_Mode                = xiiGALBufferMode::Structured;
    bd.m_uiElementByteStride = 64;
    m_hLightDataBuffer = pDevice->CreateBuffer(bd);
  }

  // Tile light list buffer: 1080p / 16px tiles = 120×68 = 8160 tiles; 256 lights each → 8160*257*4 B
  {
    xiiGALBufferCreationDescription bd;
    bd.m_sDebugName    = "TileLightList";
    bd.m_uiSize        = 8160 * 257 * sizeof(xiiUInt32);
    bd.m_BindFlags     = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    bd.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_hTileLightListBuffer = pDevice->CreateBuffer(bd);
  }

  m_bInitialised = true;
}

void xiiDeferredLightingPass::Execute(const xiiRenderViewContext& renderViewContext,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  xiiGALCommandList& cmd = *renderViewContext.m_pCommandList;

  UploadLightData(renderViewContext, cmd);
  ClassifyTiles(renderViewContext, cmd);
  EvaluateLighting(renderViewContext, cmd, outputs);

  XII_IGNORE_UNUSED(inputs);
}

void xiiDeferredLightingPass::ExecuteInactive(const xiiRenderViewContext& ctx,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(ctx); XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
}

void xiiDeferredLightingPass::UploadLightData(const xiiRenderViewContext& ctx, xiiGALCommandList& cmd)
{
  XII_IGNORE_UNUSED(ctx); XII_IGNORE_UNUSED(cmd);
  // Collect all xiiPointLightRenderData, xiiSpotLightRenderData, etc. from extracted data.
  // Pack into compact 64-byte GPU records and upload to m_hLightDataBuffer.
}

void xiiDeferredLightingPass::ClassifyTiles(const xiiRenderViewContext& ctx, xiiGALCommandList& cmd)
{
  XII_IGNORE_UNUSED(ctx);
  if (!m_hTileClassifyPSO.IsValid()) return;

  cmd.SetComputePipelineState(m_hTileClassifyPSO);
  cmd.BindShaderResource(0, m_hLightDataBuffer, xiiGALShaderStage::Compute);
  cmd.BindUnorderedAccessView(0, m_hTileLightListBuffer);

  const xiiUInt32 uiTilesX = (1920 + m_uiTileSize - 1) / m_uiTileSize;
  const xiiUInt32 uiTilesY = (1080 + m_uiTileSize - 1) / m_uiTileSize;
  cmd.Dispatch(uiTilesX, uiTilesY, 1);
}

void xiiDeferredLightingPass::EvaluateLighting(const xiiRenderViewContext& ctx, xiiGALCommandList& cmd,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(ctx);
  if (!m_hLightingPSO.IsValid()) return;

  xiiGALRenderPassBeginInfo rpInfo;
  rpInfo.m_uiRenderTargetCount = 1;
  rpInfo.m_pRenderTargets[0]   = outputs[0]->m_pTextureView;
  rpInfo.m_pClearColors[0]     = xiiColor::Black;
  rpInfo.m_bClearColor         = true;
  cmd.BeginRenderPass(rpInfo);
  cmd.SetGraphicsPipelineState(m_hLightingPSO);
  // GBuffer SRVs, tile light list SRV, light data SRV bound here
  cmd.Draw(3, 1, 0, 0); // Full-screen triangle
  cmd.EndRenderPass();
}

// ==========================================================
// xiiTranslucencyPass
// ==========================================================

xiiTranslucencyPass::xiiTranslucencyPass() : xiiRenderGraphPass("TranslucencyPass", false) {}
xiiTranslucencyPass::~xiiTranslucencyPass() {}

bool xiiTranslucencyPass::GetRenderTargetDescriptions(const xiiView& view,
  const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs,
  xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  XII_IGNORE_UNUSED(outputs);
  // Writes to existing LitHDR — pass-through, no new RTs needed.
  XII_IGNORE_UNUSED(view); XII_IGNORE_UNUSED(inputs);
  return true;
}

void xiiTranslucencyPass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
  if (m_bInitialised) return;
  // Create translucent + OIT PSOs here
  m_bInitialised = true;
}

void xiiTranslucencyPass::Execute(const xiiRenderViewContext& ctx,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(ctx); XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
  // Draw sorted translucent geometry using per-tile light list from DeferredLightingPass.
}

void xiiTranslucencyPass::ExecuteInactive(const xiiRenderViewContext& ctx,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(ctx); XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_DeferredLightingPass);
