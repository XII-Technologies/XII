#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/RasterizerState.h>

class XII_GRAPHICSVULKAN_DLL xiiGALRasterizerStateVulkan : public xiiGALRasterizerState
{
public:
protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateVulkan(const xiiGALRasterizerStateCreationDescription& creationDescription);

  virtual ~xiiGALRasterizerStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;
};

#include <GraphicsVulkan/States/Implementation/RasterizerStateVulkan_inl.h>
