#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

namespace vk
{
  class Pipeline;
}

class XII_GRAPHICSVULKAN_DLL xiiGALPipelineStateVulkan final : public xiiGALPipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALPipelineStateVulkan, xiiGALPipelineState);

public:
  XII_ALWAYS_INLINE vk::Pipeline GetVulkanPipeline() const { return m_vkPipeline; }
  XII_ALWAYS_INLINE vk::PipelineCache GetVulkanPipelineCache() const { return m_vkPipelineCache; }
  XII_ALWAYS_INLINE vk::PipelineLayout GetVulkanPipelineLayout() const { return m_vkPipelineLayout; }
  XII_ALWAYS_INLINE vk::PipelineBindPoint GetVulkanPipelineBindPoint() const { return m_vkPipelineBindPoint; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALPipelineStateVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALPipelineStateVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

private:
  vk::Pipeline          m_vkPipeline;
  vk::PipelineCache     m_vkPipelineCache;
  vk::PipelineLayout    m_vkPipelineLayout;
  vk::PipelineBindPoint m_vkPipelineBindPoint;
};
