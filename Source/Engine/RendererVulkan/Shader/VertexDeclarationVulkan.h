
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class xiiGALVertexDeclarationVulkan : public xiiGALVertexDeclaration
{
public:
  XII_ALWAYS_INLINE xiiArrayPtr<const vk::VertexInputAttributeDescription> GetAttributes() const;
  XII_ALWAYS_INLINE xiiArrayPtr<const vk::VertexInputBindingDescription> GetBindings() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  xiiGALVertexDeclarationVulkan(const xiiGALVertexDeclarationCreationDescription& Description);

  virtual ~xiiGALVertexDeclarationVulkan();

  xiiHybridArray<vk::VertexInputAttributeDescription, XII_GAL_MAX_VERTEX_BUFFER_COUNT> m_attributes;
  xiiHybridArray<vk::VertexInputBindingDescription, XII_GAL_MAX_VERTEX_BUFFER_COUNT>   m_bindings;
};

#include <RendererVulkan/Shader/Implementation/VertexDeclarationVulkan_inl.h>
