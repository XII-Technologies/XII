#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/PipelineResourceSignature.h>

#include <GraphicsVulkan/Resources/SamplerVulkan.h>

class XII_GRAPHICSVULKAN_DLL xiiGALPipelineResourceSignatureVulkan final : public xiiGALPipelineResourceSignature
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALPipelineResourceSignatureVulkan, xiiGALPipelineResourceSignature);

public:
  virtual bool IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const override final;

  XII_ALWAYS_INLINE xiiArrayPtr<const vk::DescriptorSetLayout> GetVulkanDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }

  struct ImmutableSamplerStorage
  {
    XII_DECLARE_POD_TYPE();

    void Initialize(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALSamplerCreationDescription& samplerDescription);
    void DeInitialize(xiiGALDeviceVulkan* pDeviceVulkan);

    XII_ALWAYS_INLINE explicit operator bool() const { return m_pSamplerVulkan != nullptr; }

    XII_ALWAYS_INLINE vk::Sampler GetVulkanSampler() const { return m_pSamplerVulkan->GetVulkanSampler(); }

  private:
    xiiGALSamplerVulkan* m_pSamplerVulkan = nullptr;
  };

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALPipelineResourceSignatureVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription);

  virtual ~xiiGALPipelineResourceSignatureVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

private:
  xiiHybridArray<vk::DescriptorSetLayout, 1U> m_DescriptorSetLayouts;

  xiiDynamicArray<ImmutableSamplerStorage> m_ImmutableSamplers;
};
