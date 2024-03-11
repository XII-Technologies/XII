#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/RasterizerState.h>

class XII_GRAPHICSVULKAN_DLL xiiGALRasterizerStateVulkan final : public xiiGALRasterizerState
{
public:
  const Diligent::RasterizerStateDesc* GetRasterizerState() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateVulkan(const xiiGALRasterizerStateCreationDescription& creationDescription);

  virtual ~xiiGALRasterizerStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::RasterizerStateDesc m_RasterizerState = {};
};

#include <GraphicsVulkan/States/Implementation/RasterizerStateVulkan_inl.h>
