#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/DepthStencilState.h>

class XII_GRAPHICSVULKAN_DLL xiiGALDepthStencilStateVulkan : xiiGALDepthStencilState
{
public:
protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateVulkan(const xiiGALDepthStencilStateCreationDescription& creationDescription);

  virtual ~xiiGALDepthStencilStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;
};

#include <GraphicsVulkan/States/Implementation/DepthStencilStateVulkan_inl.h>
