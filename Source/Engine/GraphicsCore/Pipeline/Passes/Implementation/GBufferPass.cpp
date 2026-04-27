#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/GBufferPass.h>
#include <GAL/Device/GALDevice.h>
#include <GAL/CommandList/GALCommandList.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGBufferPass, 1, xiiRTTIDefaultAllocator<xiiGBufferPass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGBufferPass::xiiGBufferPass() : xiiRenderGraphPass("GBufferPass", true) {}
xiiGBufferPass::~xiiGBufferPass() { m_bInitialised = false; }

bool xiiGBufferPass::GetRenderTargetDescriptions(const xiiView& view,
  const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs,
  xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  XII_IGNORE_UNUSED(inputs);
  const xiiRectU32& vp = view.GetViewport();

  auto Setup = [&](xiiUInt8 slot, xiiGALTextureFormat fmt, const char* name)
  {
    outputs[slot].m_Format     = fmt;
    outputs[slot].m_uiWidth    = vp.width;
    outputs[slot].m_uiHeight   = vp.height;
    outputs[slot].m_BindFlags  = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    outputs[slot].m_sDebugName = name;
  };

  Setup(xiiGBufferLayout::Albedo_AO,          xiiGALTextureFormat::RGBA8Unorm,    "GBuffer_AlbedoAO");
  Setup(xiiGBufferLayout::Normal_ShadingModel, xiiGALTextureFormat::RGB10A2Unorm, "GBuffer_NormalSM");
  Setup(xiiGBufferLayout::ORM_Custom,          xiiGALTextureFormat::RGBA8Unorm,   "GBuffer_ORM");
  Setup(xiiGBufferLayout::Velocity,            xiiGALTextureFormat::RG16Float,    "GBuffer_Velocity");
  return true;
}

void xiiGBufferPass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
  if (m_bInitialised) return;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice, "No GAL device");

  // Static mesh PSO
  {
    xiiGALGraphicsPipelineCreationDescription d;
    d.m_sDebugName = "GBufferStaticPSO";
    d.m_sVertexShader = "Shaders/GBuffer/GBufferStaticVS.hlsl";
    d.m_sPixelShader  = "Shaders/GBuffer/GBufferPS.hlsl";
    d.m_uiRenderTargetCount = xiiGBufferLayout::RenderTargetCount;
    d.m_DepthStencilFormat  = xiiGALTextureFormat::D32Float;
    d.m_DepthStencilState.m_bDepthEnable = true;
    d.m_DepthStencilState.m_DepthFunc    = xiiGALComparisonFunc::Less;
    m_hGBufferStaticPSO = pDevice->CreateGraphicsPipelineState(d);
  }

  // Meshlet PSO
  if (m_bUseMeshShader)
  {
    xiiGALMeshShaderPipelineCreationDescription d;
    d.m_sDebugName  = "GBufferMeshletPSO";
    d.m_sTaskShader = "Shaders/Meshlets/MeshletCull.hlsl";
    d.m_sMeshShader = "Shaders/Meshlets/MeshletGBuffer.hlsl";
    d.m_sPixelShader= "Shaders/GBuffer/GBufferPS.hlsl";
    d.m_uiRenderTargetCount = xiiGBufferLayout::RenderTargetCount;
    d.m_DepthStencilFormat  = xiiGALTextureFormat::D32Float;
    m_hGBufferMeshletPSO = pDevice->CreateMeshShaderPipelineState(d);
  }

  // Skinned PSO
  {
    xiiGALGraphicsPipelineCreationDescription d;
    d.m_sDebugName = "GBufferSkinnedPSO";
    d.m_sVertexShader = "Shaders/GBuffer/GBufferSkinnedVS.hlsl";
    d.m_sPixelShader  = "Shaders/GBuffer/GBufferPS.hlsl";
    d.m_uiRenderTargetCount = xiiGBufferLayout::RenderTargetCount;
    d.m_DepthStencilFormat  = xiiGALTextureFormat::D32Float;
    m_hGBufferSkinnedPSO = pDevice->CreateGraphicsPipelineState(d);
  }

  // Material table buffer
  {
    xiiGALBufferCreationDescription bd;
    bd.m_sDebugName = "MaterialTableBuffer";
    bd.m_uiSize = 4096 * 128;
    bd.m_BindFlags = xiiGALBindFlags::ShaderResource;
    bd.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
    bd.m_CPUAccessFlags = xiiGALCPUAccessFlags::Write;
    bd.m_Mode = xiiGALBufferMode::Structured;
    bd.m_uiElementByteStride = 128;
    m_hMaterialTableBuffer = pDevice->CreateBuffer(bd);
  }

  m_bInitialised = true;
}

void xiiGBufferPass::Execute(const xiiRenderViewContext& renderViewContext,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(inputs);
  xiiGALCommandList& cmd = *renderViewContext.m_pCommandList;

  xiiGALRenderPassBeginInfo rpInfo;
  rpInfo.m_uiRenderTargetCount = xiiGBufferLayout::RenderTargetCount;
  for (xiiUInt8 i = 0; i < xiiGBufferLayout::RenderTargetCount; ++i)
  {
    rpInfo.m_pRenderTargets[i] = outputs[i]->m_pTextureView;
    rpInfo.m_pClearColors[i]   = xiiColor::Black;
  }
  rpInfo.m_pDepthStencil = outputs[xiiGBufferLayout::RenderTargetCount]->m_pTextureView;
  rpInfo.m_bClearDepth   = true;
  rpInfo.m_fClearDepth   = 1.0f;
  cmd.BeginRenderPass(rpInfo);

  UploadPerFrameData(renderViewContext, cmd);

  if (m_bUseMeshShader && m_hGBufferMeshletPSO.IsValid())
    DrawMeshletBatches(renderViewContext, cmd);

  DrawIndirectBatches(renderViewContext, cmd);
  cmd.EndRenderPass();
}

void xiiGBufferPass::ExecuteInactive(const xiiRenderViewContext& ctx,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
  const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(ctx); XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
}

void xiiGBufferPass::UploadPerFrameData(const xiiRenderViewContext& ctx, xiiGALCommandList& cmd)
{
  XII_IGNORE_UNUSED(ctx); XII_IGNORE_UNUSED(cmd);
}

void xiiGBufferPass::DrawMeshletBatches(const xiiRenderViewContext& ctx, xiiGALCommandList& cmd)
{
  XII_IGNORE_UNUSED(ctx);
  cmd.SetGraphicsPipelineState(m_hGBufferMeshletPSO);
  // Per batch: bind meshlet SRVs, DispatchMesh(meshletCount / 32, 1, 1)
}

void xiiGBufferPass::DrawIndirectBatches(const xiiRenderViewContext& ctx, xiiGALCommandList& cmd)
{
  XII_IGNORE_UNUSED(ctx);
  cmd.SetGraphicsPipelineState(m_hGBufferStaticPSO);
  // Per batch: bind VB/IB, ExecuteIndirect from indirect args buffer
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GBufferPass);
