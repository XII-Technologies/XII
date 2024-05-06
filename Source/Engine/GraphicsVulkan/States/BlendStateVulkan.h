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
  const vk::PipelineColorBlendStateCreateInfo* GetBlendState() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBlendStateVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALBlendStateCreationDescription& creationDescription);

  virtual ~xiiGALBlendStateVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
  vk::PipelineColorBlendStateCreateInfo                                                 m_BlendState = {};
  xiiStaticArray<vk::PipelineColorBlendAttachmentState, XII_GAL_MAX_RENDERTARGET_COUNT> m_BlendAttachmentState;
};

#include <GraphicsVulkan/States/Implementation/BlendStateVulkan_inl.h>
