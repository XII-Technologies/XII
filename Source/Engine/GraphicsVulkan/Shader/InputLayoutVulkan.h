#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>

class XII_GRAPHICSVULKAN_DLL xiiGALInputLayoutVulkan final : public xiiGALInputLayout
{
public:
protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALInputLayoutVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALInputLayoutCreationDescription& creationDescription);

  virtual ~xiiGALInputLayoutVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};

#include <GraphicsVulkan/Shader/Implementation/InputLayoutVulkan_inl.h>
