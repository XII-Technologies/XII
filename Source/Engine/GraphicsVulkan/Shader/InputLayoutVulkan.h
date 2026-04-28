/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>

class XII_GRAPHICSVULKAN_DLL xiiGALInputLayoutVulkan final : public xiiGALInputLayout
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALInputLayoutVulkan, xiiGALInputLayout);

public:
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const vk::VertexInputAttributeDescription> GetVulkanVertexAttributes() const { return m_vkVertexAttributes; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const vk::VertexInputBindingDescription> GetVulkanVertexInputBindings() const { return m_vkVertexInputBindings; }

protected:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceVulkan;
  friend class xiiGALShaderVulkan;

  xiiGALInputLayoutVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALInputLayoutCreationDescription& creationDescription);

  virtual ~xiiGALInputLayoutVulkan();

  virtual xiiResult InitPlatform(xiiGALShader* pShader) override final;

private:
  xiiHybridArray<vk::VertexInputAttributeDescription, 2U> m_vkVertexAttributes;
  xiiHybridArray<vk::VertexInputBindingDescription, 2U>   m_vkVertexInputBindings;
};
