#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/VirtualGeometryPasses.h>
#include <GAL/Device/GALDevice.h>
#include <GAL/CommandList/GALCommandList.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisibilityBufferPass, 1, xiiRTTIDefaultAllocator<xiiVisibilityBufferPass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMaterialEvalPass, 1, xiiRTTIDefaultAllocator<xiiMaterialEvalPass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

// ============================================================
// xiiVisibilityBufferPass
// ============================================================

xiiVisibilityBufferPass::xiiVisibilityBufferPass() : xiiRenderGraphPass("VisibilityBufferPass", true) {}
xiiVisibilityBufferPass::~xiiVisibilityBufferPass() {}

bool xiiVisibilityBufferPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  XII_IGNORE_UNUSED(inputs);
  const xiiRectU32& vp = view.GetViewport();

  // Visibility Buffer: R32G32_UINT (Instance ID, Triangle ID)
  outputs[0].m_Format     = xiiGALTextureFormat::RG32Uint;
  outputs[0].m_uiWidth    = vp.width;
  outputs[0].m_uiHeight   = vp.height;
  outputs[0].m_BindFlags  = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  outputs[0].m_sDebugName = "VisibilityBuffer";

  return true;
}

void xiiVisibilityBufferPass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice && !m_hVisibilityPSO.IsValid())
  {
    // Real implementation would use a Task/Mesh shader pipeline with specialized SW rasterizer
    // fallback or HW rasterization. Here we stub the pipeline creation.
    xiiGALGraphicsPipelineCreationDescription desc;
    desc.m_sDebugName  = "VisibilityBufferPSO";
    desc.m_sVertexShader = "Shaders/VirtualGeometry/VisibilityVS.hlsl";
    desc.m_sPixelShader  = "Shaders/VirtualGeometry/VisibilityPS.hlsl";
    desc.m_uiRenderTargetCount = 1;
    desc.m_DepthStencilFormat  = xiiGALTextureFormat::D32Float;
    desc.m_DepthStencilState.m_bDepthEnable = true;
    m_hVisibilityPSO = pDevice->CreateGraphicsPipelineState(desc);
  }
}

void xiiVisibilityBufferPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(inputs);
  if (!m_hVisibilityPSO.IsValid()) return;

  xiiGALCommandList& cmd = *renderViewContext.m_pCommandList;
  xiiGALRenderPassBeginInfo rp;
  rp.m_uiRenderTargetCount = 1;
  rp.m_pRenderTargets[0] = outputs[0]->m_pTextureView;
  // Assumes depth is managed externally or bound here in a full implementation
  rp.m_pClearColors[0] = xiiColor(0, 0, 0, 0); // 0 = invalid ID
  rp.m_bClearColor = true;

  cmd.BeginRenderPass(rp);
  cmd.SetGraphicsPipelineState(m_hVisibilityPSO);
  // Indirect dispatch of visible DAG nodes
  cmd.EndRenderPass();
}

// ============================================================
// xiiMaterialEvalPass
// ============================================================

xiiMaterialEvalPass::xiiMaterialEvalPass() : xiiRenderGraphPass("MaterialEvalPass", true) {}
xiiMaterialEvalPass::~xiiMaterialEvalPass() {}

bool xiiMaterialEvalPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  XII_IGNORE_UNUSED(view); XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
  // Outputs to the standard GBuffer slots or Forward target.
  return true;
}

void xiiMaterialEvalPass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice && !m_hMaterialEvalPSO.IsValid())
  {
    // Fullscreen quad or compute shader to evaluate materials from VisBuffer
    xiiGALGraphicsPipelineCreationDescription desc;
    desc.m_sDebugName  = "MaterialEvalPSO";
    desc.m_sVertexShader = "Shaders/FullscreenTriangleVS.hlsl";
    desc.m_sPixelShader  = "Shaders/VirtualGeometry/MaterialEvalPS.hlsl";
    // Setup for GBuffer output layout matching the GBuffer pass
    m_hMaterialEvalPSO = pDevice->CreateGraphicsPipelineState(desc);
  }
}

void xiiMaterialEvalPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
  if (!m_hMaterialEvalPSO.IsValid()) return;

  xiiGALCommandList& cmd = *renderViewContext.m_pCommandList;
  // Bind outputs to GBuffer
  // Bind inputs: VisBuffer SRV
  // cmd.BeginRenderPass(...)
  cmd.SetGraphicsPipelineState(m_hMaterialEvalPSO);
  cmd.Draw(3, 1, 0, 0); // Fullscreen pass
  // cmd.EndRenderPass(...)
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_VirtualGeometryPasses);
