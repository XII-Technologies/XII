#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>

class XII_GRAPHICSVULKAN_DLL xiiGALShaderVulkan final : public xiiGALShader
{
public:
protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALShaderVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShaderVulkan();

  virtual xiiResult InitPlatform();

  virtual xiiResult DeInitPlatform();

protected:
};
