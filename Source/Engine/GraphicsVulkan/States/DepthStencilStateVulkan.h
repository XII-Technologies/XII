#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/DepthStencilState.h>

namespace vk
{
  struct PipelineDepthStencilStateCreateInfo;
} // namespace vk

class XII_GRAPHICSVULKAN_DLL xiiGALDepthStencilStateVulkan : public xiiGALDepthStencilState
{
public:
  XII_ALWAYS_INLINE const vk::PipelineDepthStencilStateCreateInfo* GetDepthStencilState() const;

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateVulkan(const xiiGALDepthStencilStateCreationDescription& creationDescription);

  virtual ~xiiGALDepthStencilStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  vk::PipelineDepthStencilStateCreateInfo m_DepthStencilState = {};
};

#include <GraphicsVulkan/States/Implementation/DepthStencilStateVulkan_inl.h>
