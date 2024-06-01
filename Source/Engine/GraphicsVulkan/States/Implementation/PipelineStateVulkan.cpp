#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>
#include <GraphicsVulkan/Shader/InputLayoutVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>
#include <GraphicsVulkan/States/BlendStateVulkan.h>
#include <GraphicsVulkan/States/DepthStencilStateVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>
#include <GraphicsVulkan/States/PipelineStateVulkan.h>
#include <GraphicsVulkan/States/RasterizerStateVulkan.h>

xiiGALPipelineStateVulkan::xiiGALPipelineStateVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALPipelineStateCreationDescription& creationDescription) :
  xiiGALPipelineState(pDeviceVulkan, creationDescription)
{
}

xiiGALPipelineStateVulkan::~xiiGALPipelineStateVulkan() = default;

xiiResult xiiGALPipelineStateVulkan::InitPlatform()
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_FAILURE;
}

xiiResult xiiGALPipelineStateVulkan::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

void xiiGALPipelineStateVulkan::SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBuffer* pConstantBuffer)
{
}

void xiiGALPipelineStateVulkan::SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
}

void xiiGALPipelineStateVulkan::SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
}

void xiiGALPipelineStateVulkan::SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
}

void xiiGALPipelineStateVulkan::SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
}

void xiiGALPipelineStateVulkan::SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler)
{
}

void xiiGALPipelineStateVulkan::ResetBoundResources()
{
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_PipelineStateVulkan);
