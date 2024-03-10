#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Shader/ShaderResourceVariable.h>

class XII_GRAPHICSVULKAN_DLL xiiGALShaderResourceVariableVulkan final : public xiiGALShaderResourceVariable
{
public:
protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALShaderResourceVariableVulkan();

  virtual ~xiiGALShaderResourceVariableVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);
};

#include <GraphicsVulkan/Shader/Implementation/ShaderResourceVariableVulkan_inl.h>
