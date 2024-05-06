#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/RenderPassD3D12.h>
#include <GraphicsD3D12/Shader/InputLayoutD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>
#include <GraphicsD3D12/States/BlendStateD3D12.h>
#include <GraphicsD3D12/States/DepthStencilStateD3D12.h>
#include <GraphicsD3D12/States/PipelineResourceSignatureD3D12.h>
#include <GraphicsD3D12/States/PipelineStateD3D12.h>
#include <GraphicsD3D12/States/RasterizerStateD3D12.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALPipelineStateD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALPipelineStateD3D12::xiiGALPipelineStateD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALPipelineStateCreationDescription& creationDescription) :
  xiiGALPipelineState(pDeviceD3D12, creationDescription)
{
}

xiiGALPipelineStateD3D12::~xiiGALPipelineStateD3D12() = default;

xiiResult xiiGALPipelineStateD3D12::InitPlatform()
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

  xiiGALShaderD3D12*                    pShaderD3D12                    = static_cast<xiiGALShaderD3D12*>(pDeviceD3D12->GetShader(m_Description.m_hShader));
  xiiGALPipelineResourceSignatureD3D12* pPipelineResourceSignatureD3D12 = static_cast<xiiGALPipelineResourceSignatureD3D12*>(pDeviceD3D12->GetPipelineResourceSignature(m_Description.m_hPipelineResourceSignature));

  return XII_SUCCESS;
}

xiiResult xiiGALPipelineStateD3D12::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

void xiiGALPipelineStateD3D12::SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBuffer* pConstantBuffer)
{
}

void xiiGALPipelineStateD3D12::SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
}

void xiiGALPipelineStateD3D12::SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
}

void xiiGALPipelineStateD3D12::SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
}

void xiiGALPipelineStateD3D12::SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
}

void xiiGALPipelineStateD3D12::SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler)
{
}

void xiiGALPipelineStateD3D12::ResetBoundResources()
{
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_PipelineStateD3D12);
