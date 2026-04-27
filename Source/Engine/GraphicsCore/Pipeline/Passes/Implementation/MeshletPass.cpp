#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/MeshletPass.h>
#include <GAL/Device/GALDevice.h>
#include <GAL/CommandList/GALCommandList.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshletPass, 1, xiiRTTIDefaultAllocator<xiiMeshletPass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshletShadowPass, 1, xiiRTTIDefaultAllocator<xiiMeshletShadowPass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

// ============================================================
// xiiMeshletPass
// ============================================================

xiiMeshletPass::xiiMeshletPass() : xiiRenderGraphPass("MeshletPass", true) {}
xiiMeshletPass::~xiiMeshletPass() {}

bool xiiMeshletPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  XII_IGNORE_UNUSED(view); XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
  // Relies on existing render targets (e.g. from GBuffer or Forward pass).
  return true;
}

void xiiMeshletPass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice && !m_hMeshletPSO.IsValid())
  {
    xiiGALMeshShaderPipelineCreationDescription desc;
    desc.m_sDebugName  = "MeshletPassPSO";
    desc.m_sTaskShader = "Shaders/Meshlets/MeshletCull.hlsl";
    desc.m_sMeshShader = "Shaders/Meshlets/MeshletForward.hlsl";
    desc.m_sPixelShader= "Shaders/Meshlets/MeshletPS.hlsl";
    desc.m_uiRenderTargetCount = 1;
    desc.m_DepthStencilFormat  = xiiGALTextureFormat::D32Float;
    desc.m_DepthStencilState.m_bDepthEnable = true;
    m_hMeshletPSO = pDevice->CreateMeshShaderPipelineState(desc);
  }
}

void xiiMeshletPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
  if (!m_hMeshletPSO.IsValid()) return;
  xiiGALCommandList& cmd = *renderViewContext.m_pCommandList;
  cmd.SetComputePipelineState(m_hMeshletPSO); // Or appropriate GAL call for mesh shader pipelines
  // DispatchMesh would go here, driven by indirect draw args
}

// ============================================================
// xiiMeshletShadowPass
// ============================================================

xiiMeshletShadowPass::xiiMeshletShadowPass() : xiiRenderGraphPass("MeshletShadowPass", true) {}
xiiMeshletShadowPass::~xiiMeshletShadowPass() {}

bool xiiMeshletShadowPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  XII_IGNORE_UNUSED(view); XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
  return true;
}

void xiiMeshletShadowPass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice && !m_hMeshletShadowPSO.IsValid())
  {
    xiiGALMeshShaderPipelineCreationDescription desc;
    desc.m_sDebugName  = "MeshletShadowPSO";
    desc.m_sTaskShader = "Shaders/Meshlets/MeshletCull.hlsl";
    desc.m_sMeshShader = "Shaders/Meshlets/MeshletShadow.hlsl";
    // No pixel shader for pure depth shadows
    desc.m_uiRenderTargetCount = 0;
    desc.m_DepthStencilFormat  = xiiGALTextureFormat::D32Float;
    desc.m_DepthStencilState.m_bDepthEnable = true;
    m_hMeshletShadowPSO = pDevice->CreateMeshShaderPipelineState(desc);
  }
}

void xiiMeshletShadowPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  XII_IGNORE_UNUSED(inputs); XII_IGNORE_UNUSED(outputs);
  if (!m_hMeshletShadowPSO.IsValid()) return;
  xiiGALCommandList& cmd = *renderViewContext.m_pCommandList;
  cmd.SetComputePipelineState(m_hMeshletShadowPSO); // Or appropriate GAL call
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_MeshletPass);
