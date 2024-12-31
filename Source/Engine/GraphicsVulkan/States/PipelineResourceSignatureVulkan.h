#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/PipelineResourceSignature.h>

class XII_GRAPHICSVULKAN_DLL xiiGALPipelineResourceSignatureVulkan final : public xiiGALPipelineResourceSignature
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALPipelineResourceSignatureVulkan, xiiGALPipelineResourceSignature);

public:
  virtual bool IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const override final;

  XII_ALWAYS_INLINE vk::DescriptorSetLayout GetVulkanDescriptorSetLayout() const { return m_vkDescriptorSetLayout; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALPipelineResourceSignatureVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription);

  virtual ~xiiGALPipelineResourceSignatureVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
  vk::DescriptorSetLayout m_vkDescriptorSetLayout = VK_NULL_HANDLE;
};
