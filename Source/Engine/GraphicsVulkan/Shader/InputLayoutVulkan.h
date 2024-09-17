#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>

class XII_GRAPHICSVULKAN_DLL xiiGALInputLayoutVulkan final : public xiiGALInputLayout
{
public:
  XII_ALWAYS_INLINE xiiArrayPtr<const vk::VertexInputAttributeDescription> GetVulkanVertexAttributes() const { return m_vkVertexAttributes; }
  XII_ALWAYS_INLINE xiiArrayPtr<const vk::VertexInputBindingDescription> GetVulkanVertexInputBindings() const { return m_vkVertexInputBindings; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALInputLayoutVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALInputLayoutCreationDescription& creationDescription);

  virtual ~xiiGALInputLayoutVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
  xiiHybridArray<vk::VertexInputAttributeDescription, XII_GAL_MAX_VERTEX_BUFFER_COUNT> m_vkVertexAttributes;
  xiiHybridArray<vk::VertexInputBindingDescription, XII_GAL_MAX_VERTEX_BUFFER_COUNT>   m_vkVertexInputBindings;
};
