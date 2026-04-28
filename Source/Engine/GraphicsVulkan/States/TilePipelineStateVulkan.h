/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

namespace vk
{
  class Pipeline;
}

class XII_GRAPHICSVULKAN_DLL xiiGALTilePipelineStateVulkan final : public xiiGALTilePipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTilePipelineStateVulkan, xiiGALTilePipelineState);

public:
  [[nodiscard]] XII_ALWAYS_INLINE vk::Pipeline GetVulkanPipeline() const { return m_vkPipeline; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::PipelineCache GetVulkanPipelineCache() const { return m_vkPipelineCache; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::PipelineLayout GetVulkanPipelineLayout() const { return m_vkPipelineLayout; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::PipelineBindPoint GetVulkanPipelineBindPoint() const { return m_vkPipelineBindPoint; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTilePipelineStateVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALTilePipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALTilePipelineStateVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  vk::Pipeline          m_vkPipeline;
  vk::PipelineCache     m_vkPipelineCache;
  vk::PipelineLayout    m_vkPipelineLayout;
  vk::PipelineBindPoint m_vkPipelineBindPoint;
};
