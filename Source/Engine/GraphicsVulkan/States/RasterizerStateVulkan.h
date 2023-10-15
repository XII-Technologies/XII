#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/RasterizerState.h>

class XII_GRAPHICSVULKAN_DLL xiiGALRasterizerStateVulkan : public xiiGALRasterizerState
{
public:
  XII_ALWAYS_INLINE const Diligent::RasterizerStateDesc* GetRasterizerState() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateVulkan(const xiiGALRasterizerStateCreationDescription& creationDescription);

  virtual ~xiiGALRasterizerStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);

protected:
  Diligent::RasterizerStateDesc m_RasterizerState = {};
};

#include <GraphicsVulkan/States/Implementation/RasterizerStateVulkan_inl.h>
