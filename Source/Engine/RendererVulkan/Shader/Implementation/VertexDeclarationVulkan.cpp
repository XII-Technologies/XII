#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>

xiiGALVertexDeclarationVulkan::xiiGALVertexDeclarationVulkan(const xiiGALVertexDeclarationCreationDescription& Description) :
  xiiGALVertexDeclaration(Description)
{
}

xiiGALVertexDeclarationVulkan::~xiiGALVertexDeclarationVulkan() = default;

xiiResult xiiGALVertexDeclarationVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pVulkanDevice = static_cast<xiiGALDeviceVulkan*>(pDevice);

  const xiiGALShaderVulkan* pShader = static_cast<const xiiGALShaderVulkan*>(pDevice->GetShader(m_Description.m_hShader));

  if (pShader == nullptr || !pShader->GetDescription().HasByteCodeForStage(xiiGALShaderStage::VertexShader))
  {
    return XII_FAILURE;
  }

  xiiHybridArray<xiiGALShaderVulkan::VertexInputAttribute, 8> vias(pShader->GetVertexInputAttributes());
  auto                                                        FindLocation = [&](xiiGALVertexAttributeSemantic::Enum sematic, xiiGALResourceFormat::Enum format) -> xiiUInt32 {
    for (xiiUInt32 i = 0; i < vias.GetCount(); i++)
    {
      if (vias[i].m_eSemantic == sematic)
      {
        //XII_ASSERT_DEBUG(vias[i].m_eFormat == format, "Found matching sematic {} but format differs: {} : {}", sematic, format, vias[i].m_eFormat);
        xiiUInt32 uiLocation = vias[i].m_uiLocation;
        vias.RemoveAtAndSwap(i);
        return uiLocation;
      }
    }
    return xiiMath::MaxValue<xiiUInt32>();
  };

  // Copy attribute descriptions
  xiiUInt32 usedBindings = 0;
  for (xiiUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); i++)
  {
    const xiiGALVertexAttribute& Current = m_Description.m_VertexAttributes[i];

    const xiiUInt32 uiLocation = FindLocation(Current.m_eSemantic, Current.m_eFormat);
    if (uiLocation == xiiMath::MaxValue<xiiUInt32>())
    {
      xiiLog::Warning("Vertex buffer semantic {} not used by shader", Current.m_eSemantic);
      continue;
    }
    vk::VertexInputAttributeDescription& attrib = m_attributes.ExpandAndGetRef();
    attrib.binding                              = Current.m_uiVertexBufferSlot;
    attrib.location                             = uiLocation;
    attrib.format                               = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType;
    attrib.offset                               = Current.m_uiOffset;

    if (attrib.format == vk::Format::eUndefined)
    {
      xiiLog::Error("Vertex attribute format {0} of attribute at index {1} is undefined!", Current.m_eFormat, i);
      return XII_FAILURE;
    }

    usedBindings |= XII_BIT(Current.m_uiVertexBufferSlot);
    if (Current.m_uiVertexBufferSlot >= m_bindings.GetCount())
    {
      m_bindings.SetCount(Current.m_uiVertexBufferSlot + 1);
    }
    vk::VertexInputBindingDescription& binding = m_bindings[Current.m_uiVertexBufferSlot];
    binding.binding                            = Current.m_uiVertexBufferSlot;
    binding.stride                             = 0;
    binding.inputRate                          = Current.m_bInstanceData ? vk::VertexInputRate::eInstance : vk::VertexInputRate::eVertex;
  }
  for (xiiInt32 i = (xiiInt32)m_bindings.GetCount() - 1; i >= 0; --i)
  {
    if ((usedBindings & XII_BIT(i)) == 0)
    {
      m_bindings.RemoveAtAndCopy(i);
    }
  }

  if (!vias.IsEmpty())
  {
    xiiLog::Error("Vertex buffers do not cover all vertex attributes defined in the shader!");
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiGALVertexDeclarationVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererVulkan, RendererVulkan_Shader_Implementation_VertexDeclarationVulkan);
