#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>

namespace vk
{
  class ShaderModule;
}

class XII_GRAPHICSVULKAN_DLL xiiGALShaderVulkan final : public xiiGALShader
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALShaderVulkan, xiiGALShader);

public:
  XII_ALWAYS_INLINE vk::ShaderModule GetVulkanShaderModule() const { return m_vkShaderModule; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALShaderVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShaderVulkan();

  virtual xiiResult InitPlatform();

  virtual xiiResult DeInitPlatform();

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
  vk::ShaderModule m_vkShaderModule;
};
