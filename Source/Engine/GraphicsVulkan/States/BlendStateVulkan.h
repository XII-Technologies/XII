#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/BlendState.h>

/// \brief Interface that defines methods to manipulate a blend state object.
class XII_GRAPHICSVULKAN_DLL xiiGALBlendStateVulkan : xiiGALBlendState
{
public:
protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALBlendStateVulkan(const xiiGALBlendStateCreationDescription& creationDescription);

  virtual ~xiiGALBlendStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;
};

#include <GraphicsVulkan/States/Implementation/BlendStateVulkan_inl.h>
