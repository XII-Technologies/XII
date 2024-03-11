#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/DepthStencilState.h>

class XII_GRAPHICSVULKAN_DLL xiiGALDepthStencilStateVulkan final : public xiiGALDepthStencilState
{
public:
  const Diligent::DepthStencilStateDesc* GetDepthStencilState() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateVulkan(const xiiGALDepthStencilStateCreationDescription& creationDescription);

  virtual ~xiiGALDepthStencilStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::DepthStencilStateDesc m_DepthStencilState = {};
};

#include <GraphicsVulkan/States/Implementation/DepthStencilStateVulkan_inl.h>
