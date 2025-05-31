#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

namespace vk
{
  class Pipeline;
}

class XII_GRAPHICSVULKAN_DLL xiiGALComputePipelineStateVulkan final : public xiiGALComputePipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALComputePipelineStateVulkan, xiiGALComputePipelineState);

public:
  [[nodiscard]] XII_ALWAYS_INLINE vk::Pipeline GetVulkanPipeline() const { return m_vkPipeline; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::PipelineCache GetVulkanPipelineCache() const { return m_vkPipelineCache; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::PipelineLayout GetVulkanPipelineLayout() const { return m_vkPipelineLayout; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::PipelineBindPoint GetVulkanPipelineBindPoint() const { return m_vkPipelineBindPoint; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALComputePipelineStateVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALComputePipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALComputePipelineStateVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  vk::Pipeline          m_vkPipeline;
  vk::PipelineCache     m_vkPipelineCache;
  vk::PipelineLayout    m_vkPipelineLayout;
  vk::PipelineBindPoint m_vkPipelineBindPoint;
};
