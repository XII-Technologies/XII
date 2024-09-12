#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

namespace vk
{
  class Pipeline;
}

class XII_GRAPHICSVULKAN_DLL xiiGALPipelineStateVulkan final : public xiiGALPipelineState
{
public:
  XII_ALWAYS_INLINE vk::Pipeline GetVulkanPipeline() const { return m_vkPipeline; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALPipelineStateVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALPipelineStateVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBuffer* pConstantBuffer) override final;
  virtual void SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView) override final;
  virtual void SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView) override final;
  virtual void SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView) override final;
  virtual void SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView) override final;
  virtual void SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler) override final;
  virtual void ResetBoundResources() override final;

protected:
  vk::Pipeline m_vkPipeline;
};
