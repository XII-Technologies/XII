#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/States/PipelineStateD3D11.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>

xiiGALPipelineStateD3D11::xiiGALPipelineStateD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALPipelineStateCreationDescription& creationDescription) :
  xiiGALPipelineState(pDeviceD3D11, creationDescription)
{
}

xiiGALPipelineStateD3D11::~xiiGALPipelineStateD3D11() = default;

xiiResult xiiGALPipelineStateD3D11::InitPlatform()
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(m_pDevice);

  xiiGALShaderD3D11* pShaderD3D11 = static_cast<xiiGALShaderD3D11*>(pDeviceD3D11->GetShader(m_Description.m_hShader));
  if (pShaderD3D11->GetVertexShader() == nullptr)
  {
    xiiLog::Error("The given shader has an invalidated vertex shader!");
    return XII_FAILURE;
  }
  m_pShaderD3D11 = pShaderD3D11;

  m_pPipelineResourceSignatureD3D11 = static_cast<xiiGALPipelineResourceSignatureD3D11*>(pDeviceD3D11->GetPipelineResourceSignature(m_Description.m_hPipelineResourceSignature));

  switch (m_Description.m_PipelineType)
  {
    case xiiGALPipelineType::Graphics:
    case xiiGALPipelineType::Mesh:
    {
      m_pRenderPassD3D11        = static_cast<xiiGALRenderPassD3D11*>(pDeviceD3D11->GetRenderPass(m_Description.m_GraphicsPipeline.m_hRenderPass));
      m_pBlendStateD3D11        = static_cast<xiiGALBlendStateD3D11*>(pDeviceD3D11->GetBlendState(m_Description.m_GraphicsPipeline.m_hBlendState));
      m_pInputLayoutD3D11       = static_cast<xiiGALInputLayoutD3D11*>(pDeviceD3D11->GetInputLayout(m_Description.m_GraphicsPipeline.m_hInputLayout));
      m_pRasterizerStateD3D11   = static_cast<xiiGALRasterizerStateD3D11*>(pDeviceD3D11->GetRasterizerState(m_Description.m_GraphicsPipeline.m_hRasterizerState));
      m_pDepthStencilStateD3D11 = static_cast<xiiGALDepthStencilStateD3D11*>(pDeviceD3D11->GetDepthStencilState(m_Description.m_GraphicsPipeline.m_hDepthStencilState));
    }
    break;
    case xiiGALPipelineType::Compute:
    {
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALPipelineStateD3D11::DeInitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_States_Implementation_PipelineStateD3D11);
