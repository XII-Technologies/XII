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

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALPipelineStateVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALPipelineStateVulkan::xiiGALPipelineStateVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALPipelineStateCreationDescription& creationDescription) :
  xiiGALPipelineState(pDeviceVulkan, creationDescription)
{
}

xiiGALPipelineStateVulkan::~xiiGALPipelineStateVulkan() = default;

xiiResult xiiGALPipelineStateVulkan::InitPlatform()
{
  xiiGALDeviceVulkan*                    pDeviceVulkan            = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Device                             vkLogicalDevice          = pDeviceVulkan->GetVulkanLogicalDevice();
  xiiGALPipelineResourceSignatureVulkan* pResourceSignatureVulkan = static_cast<xiiGALPipelineResourceSignatureVulkan*>(pDeviceVulkan->GetPipelineResourceSignature(m_Description.m_hPipelineResourceSignature));

  switch (m_Description.m_PipelineType)
  {
    case xiiGALPipelineType::Graphics:
    case xiiGALPipelineType::Mesh:
    {
      vk::GraphicsPipelineCreateInfo vkGraphicsPipelineCreateInfo = {};
      vkGraphicsPipelineCreateInfo.pNext                          = nullptr;
      vkGraphicsPipelineCreateInfo.flags                          = {};
      vkGraphicsPipelineCreateInfo.basePipelineHandle             = nullptr; // A pipeline to derive from.
      vkGraphicsPipelineCreateInfo.basePipelineIndex              = -1;      // An index into the pCreateInfos parameter to use as a pipeline to derive from.

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      vkGraphicsPipelineCreateInfo.flags |= vk::PipelineCreateFlagBits::eDisableOptimization;
#endif

      VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createGraphicsPipelines(m_vkPipelineCache, 1U, &vkGraphicsPipelineCreateInfo, nullptr, &m_vkPipeline, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
    }
    break;
    case xiiGALPipelineType::Compute:
    {
      vk::ComputePipelineCreateInfo vkComputePipelineCreateInfo = {};
      vkComputePipelineCreateInfo.pNext                         = nullptr;
      vkComputePipelineCreateInfo.flags                         = {};
      vkComputePipelineCreateInfo.basePipelineHandle            = nullptr; // A pipeline to derive from.
      vkComputePipelineCreateInfo.basePipelineIndex             = -1;      // An index into the pCreateInfos parameter to use as a pipeline to derive from.

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      vkComputePipelineCreateInfo.flags |= vk::PipelineCreateFlagBits::eDisableOptimization;
#endif

      VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createComputePipelines(m_vkPipelineCache, 1U, &vkComputePipelineCreateInfo, nullptr, &m_vkPipeline, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
    }
    break;
    case xiiGALPipelineType::RayTracing:
    {
      vk::RayTracingPipelineCreateInfoKHR vkRayTracingPipelineCreateInfo = {};
      vkRayTracingPipelineCreateInfo.pNext                               = nullptr;
      vkRayTracingPipelineCreateInfo.flags                               = {};
      vkRayTracingPipelineCreateInfo.basePipelineHandle                  = nullptr; // A pipeline to derive from.
      vkRayTracingPipelineCreateInfo.basePipelineIndex                   = -1;      // An index into the pCreateInfos parameter to use as a pipeline to derive from.

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      vkRayTracingPipelineCreateInfo.flags |= vk::PipelineCreateFlagBits::eDisableOptimization;
#endif

      VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createRayTracingPipelinesKHR(VK_NULL_HANDLE, m_vkPipelineCache, 1U, &vkRayTracingPipelineCreateInfo, nullptr, &m_vkPipeline, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
    }
    break;
    case xiiGALPipelineType::Tile:
    {
      XII_ASSERT_NOT_IMPLEMENTED;
      return XII_FAILURE;
    }
    break;

    default:
      xiiLog::Error("Unknown pipeline type.");
      return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALPipelineStateVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  if (m_vkPipelineCache != VK_NULL_HANDLE)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(m_vkPipelineCache);

    m_vkPipelineCache = nullptr;
  }
  if (m_vkPipeline != VK_NULL_HANDLE)
  {
    pDeviceVulkan->SafeReleaseDeviceObject(m_vkPipeline);

    m_vkPipeline = VK_NULL_HANDLE;
  }
  return XII_SUCCESS;
}

void xiiGALPipelineStateVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  if (m_vkPipeline == VK_NULL_HANDLE)
    return;

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkPipeline, sName.GetData(tmp));
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
