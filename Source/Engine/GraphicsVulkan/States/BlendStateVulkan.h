#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/BlendState.h>

class XII_GRAPHICSVULKAN_DLL xiiGALBlendStateVulkan : public xiiGALBlendState
{
public:
  XII_ALWAYS_INLINE const Diligent::BlendStateDesc* GetBlendState() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBlendStateVulkan(const xiiGALBlendStateCreationDescription& creationDescription);

  virtual ~xiiGALBlendStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);

protected:
  Diligent::BlendStateDesc m_BlendState = {};
};

#include <GraphicsVulkan/States/Implementation/BlendStateVulkan_inl.h>
