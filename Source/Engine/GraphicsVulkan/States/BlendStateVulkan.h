#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/BlendState.h>

namespace vk
{
  struct PipelineColorBlendStateCreateInfo;
  struct PipelineColorBlendAttachmentState;
} // namespace vk

XII_DEFINE_AS_POD_TYPE(vk::PipelineColorBlendAttachmentState);

class XII_GRAPHICSVULKAN_DLL xiiGALBlendStateVulkan : xiiGALBlendState
{
public:
  XII_ALWAYS_INLINE const vk::PipelineColorBlendStateCreateInfo* GetBlendState() const;

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALBlendStateVulkan(const xiiGALBlendStateCreationDescription& creationDescription);

  virtual ~xiiGALBlendStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  vk::PipelineColorBlendStateCreateInfo                                                 m_BlendState = {};
  xiiStaticArray<vk::PipelineColorBlendAttachmentState, XII_GAL_MAX_RENDERTARGET_COUNT> m_BlendAttachmentState;
};

#include <GraphicsVulkan/States/Implementation/BlendStateVulkan_inl.h>
