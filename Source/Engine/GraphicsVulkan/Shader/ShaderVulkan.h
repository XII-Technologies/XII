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
  [[nodiscard]] XII_ALWAYS_INLINE vk::ShaderModule GetVulkanShaderModule() const { return m_vkShaderModule; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALShaderVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShaderVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiInternal::NewInstance<xiiGALInputLayout> CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description) override;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  vk::ShaderModule m_vkShaderModule;
};
