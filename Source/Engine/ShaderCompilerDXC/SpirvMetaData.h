#pragma once
#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

struct xiiVulkanDescriptorSetLayoutBinding
{
  enum ResourceType : xiiUInt8
  {
    ConstantBuffer,
    ResourceView,
    UAV,
    Sampler,
  };

  XII_DECLARE_POD_TYPE();
  xiiStringView               m_sName;                                              ///< Used to match the same descriptor use across multiple stages.
  xiiUInt8                    m_uiBinding         = 0;                              ///< Target descriptor binding slot.
  xiiUInt8                    m_uiVirtualBinding  = 0;                              ///< Virtual binding slot in the high level renderer interface.
  xiiShaderResourceType::Enum m_xiiType           = xiiShaderResourceType::Unknown; ///< XII shader resource type, needed to find compatible fallback resources.
  ResourceType                m_Type              = ResourceType::ConstantBuffer;   ///< Resource type, used to map to the correct XII resource type.
  xiiUInt16                   m_uiDescriptorType  = 0;                              ///< Maps to vk::DescriptorType
  xiiUInt32                   m_uiDescriptorCount = 1;                              ///< For now, this must be 1 as XII does not support descriptor arrays right now.
  xiiUInt32                   m_uiWordOffset      = 0;                              ///< Offset of the location in the spirv code where the binding index is located to allow changing it at runtime.
};

struct xiiVulkanDescriptorSetLayout
{
  xiiUInt32                                              m_uiSet = 0;
  xiiHybridArray<xiiVulkanDescriptorSetLayoutBinding, 6> bindings;
};

struct xiiVulkanVertexInputAttribute
{
  xiiGALVertexAttributeSemantic::Enum m_eSemantic  = xiiGALVertexAttributeSemantic::Position;
  xiiUInt8                            m_uiLocation = 0;
  xiiGALResourceFormat::Enum          m_eFormat    = xiiGALResourceFormat::XYZFloat;
};

namespace xiiSpirvMetaData
{
  constexpr xiiUInt32 s_uiSpirvMetaDataMagicNumber = 0x4B565A45; //XIIVK

  enum MetaDataVersion
  {
    Version1 = 1,
    Version2 = 2, ///< m_uiVirtualBinding, m_xiiType added
    Version3 = 3, ///< Vertex input binding
  };

  void Write(xiiStreamWriter& stream, const xiiArrayPtr<xiiUInt8>& shaderCode, const xiiDynamicArray<xiiVulkanDescriptorSetLayout>& sets, const xiiDynamicArray<xiiVulkanVertexInputAttribute>& vertexInputAttributes)
  {
    stream << s_uiSpirvMetaDataMagicNumber;
    stream.WriteVersion(MetaDataVersion::Version3);
    const xiiUInt32 uiSize = shaderCode.GetCount();
    stream << uiSize;
    stream.WriteBytes(shaderCode.GetPtr(), uiSize).AssertSuccess();

    const xiiUInt8 uiSets = sets.GetCount();
    stream << uiSets;
    for (xiiUInt8 i = 0; i < uiSets; i++)
    {
      const xiiVulkanDescriptorSetLayout& set = sets[i];
      stream << set.m_uiSet;
      const xiiUInt8 uiBindings = set.bindings.GetCount();
      stream << uiBindings;
      for (xiiUInt8 j = 0; j < uiBindings; j++)
      {
        const xiiVulkanDescriptorSetLayoutBinding& binding = set.bindings[j];
        stream.WriteString(binding.m_sName).AssertSuccess();
        stream << binding.m_uiBinding;
        stream << binding.m_uiVirtualBinding;
        stream << static_cast<xiiUInt8>(binding.m_xiiType);
        stream << static_cast<xiiUInt8>(binding.m_Type);
        stream << binding.m_uiDescriptorType;
        stream << binding.m_uiDescriptorCount;
        stream << binding.m_uiWordOffset;
      }
    }

    const xiiUInt8 uiVIA = vertexInputAttributes.GetCount();
    stream << uiVIA;
    for (xiiUInt8 i = 0; i < uiVIA; i++)
    {
      const xiiVulkanVertexInputAttribute& via = vertexInputAttributes[i];
      stream << static_cast<xiiUInt8>(via.m_eSemantic);
      stream << via.m_uiLocation;
      stream << static_cast<xiiUInt8>(via.m_eFormat);
    }
  }

  /// \brief Reads Vulkan shader code and meta data from a data buffer. Note that 'data' must be kept alive for the lifetime of the shader as this functions stores views into this memory in its out parameters.
  /// \param data Raw data buffer to read the shader code and meta data from.
  /// \param out_shaderCode Will be filled with a view into data that contains the shader byte code.
  /// \param out_sets Will be filled with shader meta data. Note that this array contains string views into 'data'.
  void Read(const xiiArrayPtr<const xiiUInt8> data, xiiArrayPtr<const xiiUInt8>& out_shaderCode, xiiDynamicArray<xiiVulkanDescriptorSetLayout>& out_sets, xiiDynamicArray<xiiVulkanVertexInputAttribute>& out_vertexInputAttributes)
  {
    xiiRawMemoryStreamReader stream(data.GetPtr(), data.GetCount());

    xiiUInt32 uiMagicNumber;
    stream >> uiMagicNumber;
    XII_ASSERT_DEV(uiMagicNumber == s_uiSpirvMetaDataMagicNumber, "Vulkan shader does not start with s_uiSpirvMetaDataMagicNumber");
    xiiTypeVersion uiVersion = stream.ReadVersion(MetaDataVersion::Version3);

    xiiUInt32 uiSize = 0;
    stream >> uiSize;
    out_shaderCode = xiiArrayPtr<const xiiUInt8>(&data[(xiiUInt32)stream.GetReadPosition()], uiSize);
    stream.SkipBytes(uiSize);

    xiiUInt8 uiSets = 0;
    stream >> uiSets;
    out_sets.Reserve(uiSets);

    for (xiiUInt8 i = 0; i < uiSets; i++)
    {
      xiiVulkanDescriptorSetLayout& set = out_sets.ExpandAndGetRef();
      stream >> set.m_uiSet;
      xiiUInt8 uiBindings = 0;
      stream >> uiBindings;
      set.bindings.Reserve(uiBindings);

      for (xiiUInt8 j = 0; j < uiBindings; j++)
      {
        xiiVulkanDescriptorSetLayoutBinding& binding = set.bindings.ExpandAndGetRef();

        xiiUInt32 uiStringElements = 0;
        stream >> uiStringElements;
        binding.m_sName = xiiStringView(reinterpret_cast<const char*>(&data[(xiiUInt32)stream.GetReadPosition()]), uiStringElements);
        stream.SkipBytes(uiStringElements);
        stream >> binding.m_uiBinding;
        if (uiVersion >= MetaDataVersion::Version2)
        {
          stream >> binding.m_uiVirtualBinding;
          stream >> reinterpret_cast<xiiUInt8&>(binding.m_xiiType);
        }
        else
        {
          binding.m_uiVirtualBinding = binding.m_uiBinding;
          binding.m_xiiType          = xiiShaderResourceType::Texture2D;
        }
        stream >> reinterpret_cast<xiiUInt8&>(binding.m_Type);
        stream >> binding.m_uiDescriptorType;
        stream >> binding.m_uiDescriptorCount;
        stream >> binding.m_uiWordOffset;
      }
    }

    if (uiVersion >= MetaDataVersion::Version3)
    {
      xiiUInt8 uiVIA = 0;
      stream >> uiVIA;
      out_vertexInputAttributes.Reserve(uiVIA);
      for (xiiUInt8 i = 0; i < uiVIA; i++)
      {
        xiiVulkanVertexInputAttribute& via = out_vertexInputAttributes.ExpandAndGetRef();
        stream >> reinterpret_cast<xiiUInt8&>(via.m_eSemantic);
        stream >> via.m_uiLocation;
        stream >> reinterpret_cast<xiiUInt8&>(via.m_eFormat);
      }
    }
  }
} // namespace xiiSpirvMetaData
