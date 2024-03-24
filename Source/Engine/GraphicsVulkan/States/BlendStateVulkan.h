#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/States/BlendState.h>

class XII_GRAPHICSVULKAN_DLL xiiGALBlendStateVulkan final : public xiiGALBlendState
{
public:
  const Diligent::BlendStateDesc* GetBlendState() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALBlendStateVulkan(const xiiGALBlendStateCreationDescription& creationDescription);

  virtual ~xiiGALBlendStateVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::BlendStateDesc m_BlendState = {};
};

#include <GraphicsVulkan/States/Implementation/BlendStateVulkan_inl.h>
