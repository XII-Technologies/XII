/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/PipelineResourceSignature.h>

#include <GraphicsVulkan/Resources/SamplerVulkan.h>

struct xiiGALPipelineResourceDescriptionVulkan : xiiHashableStruct<xiiGALPipelineResourceDescriptionVulkan>
{
  xiiHashedString               m_sName;
  xiiGALDescriporTypeVulkan     m_DescriptorType       = xiiGALDescriporTypeVulkan::ENUM_COUNT;
  xiiUInt32                     m_uiBindingSet         = xiiInvalidIndex;
  xiiUInt32                     m_uiBindingIndex       = xiiInvalidIndex;
  xiiUInt32                     m_uiSamplerIndex       = xiiInvalidIndex;
  xiiUInt32                     m_uiArraySize          = 0U;
  xiiBitflags<xiiGALShaderType> m_ShaderStages         = xiiGALShaderType::Unknown;
  xiiBitflags<xiiGALPipelineResourceFlags> m_PipelineResourceFlags = xiiGALPipelineResourceFlags::None;
  bool                          m_bHasImmutableSampler = false;
};

class XII_GRAPHICSVULKAN_DLL xiiGALPipelineResourceSignatureVulkan final : public xiiGALPipelineResourceSignature
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALPipelineResourceSignatureVulkan, xiiGALPipelineResourceSignature);

public:
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiGALPipelineResourceDescriptionVulkan> GetPipelineResourceSetLayout(xiiUInt32 uiSet) const { return (uiSet < m_PipelineResourceSetLayouts.GetCount()) ? m_PipelineResourceSetLayouts[uiSet].GetArrayPtr() : xiiArrayPtr<const xiiGALPipelineResourceDescriptionVulkan>(); }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const vk::DescriptorSetLayout> GetVulkanDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::DescriptorSetLayout GetVulkanDescriptorSetLayout(xiiUInt32 uiSet) const { return (uiSet < m_DescriptorSetLayouts.GetCount()) ? m_DescriptorSetLayouts[uiSet] : VK_NULL_HANDLE; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32               GetVulkanDescriptorSetLayoutCount() const { return m_DescriptorSetLayouts.GetCount(); }

  struct ImmutableSamplerStorage
  {
    void Initialize(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALSamplerCreationDescription& samplerDescription);
    void DeInitialize();

    XII_ALWAYS_INLINE explicit operator bool() const { return m_pSamplerVulkan != nullptr; }

    [[nodiscard]] XII_ALWAYS_INLINE vk::Sampler GetVulkanSampler() const { return m_pSamplerVulkan->GetVulkanSampler(); }

  private:
    xiiSharedPtr<xiiGALSamplerVulkan> m_pSamplerVulkan;
  };

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALPipelineResourceSignatureVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription);

  virtual ~xiiGALPipelineResourceSignatureVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  xiiHybridArray<xiiHybridArray<xiiGALPipelineResourceDescriptionVulkan, 1U>, 1U> m_PipelineResourceSetLayouts;
  xiiHybridArray<vk::DescriptorSetLayout, 1U>                                     m_DescriptorSetLayouts;

  xiiDynamicArray<ImmutableSamplerStorage> m_ImmutableSamplers;
};
