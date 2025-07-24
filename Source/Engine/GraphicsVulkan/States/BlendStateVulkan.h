#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/BlendState.h>

namespace vk
{
  struct PipelineColorBlendStateCreateInfo;
  struct PipelineColorBlendAttachmentState;
} // namespace vk

class XII_GRAPHICSVULKAN_DLL xiiGALBlendStateVulkan final : public xiiGALBlendState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBlendStateVulkan, xiiGALBlendState);

public:
  [[nodiscard]] XII_ALWAYS_INLINE const vk::PipelineColorBlendStateCreateInfo* GetBlendState() const { return &m_BlendState; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const vk::PipelineColorBlendAttachmentState> GetBlendAttachmentStates() const { return m_BlendAttachmentState; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBlendStateVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALBlendStateCreationDescription& creationDescription);

  virtual ~xiiGALBlendStateVulkan();

  virtual xiiResult InitPlatform() override final;

private:
  vk::PipelineColorBlendStateCreateInfo                                                 m_BlendState = {};
  xiiStaticArray<vk::PipelineColorBlendAttachmentState, 2U> m_BlendAttachmentState;
};
