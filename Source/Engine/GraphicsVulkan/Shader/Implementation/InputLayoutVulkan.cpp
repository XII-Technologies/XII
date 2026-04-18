#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Shader/InputLayoutVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALInputLayoutVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALInputLayoutVulkan::xiiGALInputLayoutVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALInputLayoutCreationDescription& creationDescription) :
  xiiGALInputLayout(std::move(pDeviceVulkan), creationDescription), m_vkVertexAttributes(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator()), m_vkVertexInputBindings(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator())
{
}

xiiGALInputLayoutVulkan::~xiiGALInputLayoutVulkan() = default;

xiiResult xiiGALInputLayoutVulkan::InitPlatform(xiiGALShader* pShader)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALShaderVulkan*              pShaderVulkan = static_cast<xiiGALShaderVulkan*>(pShader);

  xiiTemporaryHybridArray<xiiGALVertexInputLayout, 8U> vertexInputAttributes(pShaderVulkan->GetVertexInputLayout());
  auto                                                 FindLocation = [&](xiiGALInputLayoutSemantic::Enum semantic, xiiGALResourceFormat::Enum) -> xiiUInt32 {
    for (xiiUInt32 i = 0U; i < vertexInputAttributes.GetCount(); ++i)
    {
      if (vertexInputAttributes[i].m_Semantic == semantic)
      {
        // XII_ASSERT_DEBUG(vertexInputAttributes[i].m_Format == format, "Found matching semantic {}, but with differing formats. {} : {}", semantic, format, vertexInputAttributes[i].m_Format);

        xiiUInt32 uiLocation = vertexInputAttributes[i].m_uiSemanticIndex;
        vertexInputAttributes.RemoveAtAndSwap(i);

        return uiLocation;
      }
    }
    return xiiInvalidIndex;
  };

  // Copy the vertex attribute descriptions.
  xiiUInt32 uiUsedBindings = 0;
  for (xiiUInt32 i = 0; i < m_Description.m_LayoutElements.GetCount(); ++i)
  {
    const xiiGALLayoutElement& layoutElement = m_Description.m_LayoutElements[i];

    const xiiUInt32 uiLocation = FindLocation(layoutElement.m_Semantic, layoutElement.m_Format);
    if (uiLocation == xiiInvalidIndex)
    {
      // xiiLog::Warning("Vertex buffer semantic {} is not used by the shader.", layoutElement.m_Semantic);
      continue;
    }

    vk::VertexInputAttributeDescription& vertexAttributeDescription = m_vkVertexAttributes.ExpandAndGetRef();
    vertexAttributeDescription.location                             = uiLocation;
    vertexAttributeDescription.binding                              = layoutElement.m_uiBufferSlot;
    vertexAttributeDescription.format                               = xiiVulkanTypeConversions::GetFormat(layoutElement.m_Format);
    vertexAttributeDescription.offset                               = layoutElement.m_uiRelativeOffset;

    if (vertexAttributeDescription.format == vk::Format::eUndefined)
    {
      xiiLog::Error("Vertex attribute format {} of attribute at index {} is undefined!", layoutElement.m_Format, i);
      return XII_FAILURE;
    }

    uiUsedBindings |= XII_BIT(layoutElement.m_uiBufferSlot);
    if (layoutElement.m_uiBufferSlot >= m_vkVertexInputBindings.GetCount())
    {
      m_vkVertexInputBindings.SetCount(layoutElement.m_uiBufferSlot + 1);
    }

    vk::VertexInputBindingDescription& vertexInputBindingDescription = m_vkVertexInputBindings[layoutElement.m_uiBufferSlot];
    vertexInputBindingDescription.binding                            = layoutElement.m_uiBufferSlot;
    vertexInputBindingDescription.stride                             = layoutElement.m_uiStride;
    vertexInputBindingDescription.inputRate                          = xiiVulkanTypeConversions::GetFrequency(layoutElement.m_Frequency);
  }

  for (xiiInt32 i = (xiiInt32)m_vkVertexInputBindings.GetCount() - 1; i >= 0; --i)
  {
    if ((uiUsedBindings & XII_BIT(i)) == 0U)
    {
      m_vkVertexInputBindings.RemoveAtAndCopy(i);
    }
  }

  if (!vertexInputAttributes.IsEmpty())
  {
    xiiLog::Error("Vertex attributes do not cover all vertex attributes defined in the shader!");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Shader_Implementation_InputLayoutVulkan);
