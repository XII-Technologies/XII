#include <ShaderCompiler/ShaderCompilerPCH.h>

#include <ShaderCompiler/ShaderMetadata.h>

namespace xiiShaderMetaData
{
  void Write(xiiStreamWriter& stream, const xiiArrayPtr<xiiUInt8>& shaderCode, const xiiDynamicArray<xiiShaderDescriptorSetLayout>& sets, const xiiDynamicArray<xiiShaderVertexInputAttribute>& vertexInputAttributes)
  {
    stream << x_uiMetaDataTag;
    stream.WriteVersion(MetaDataVersion::CurrentVerion);

    const xiiUInt32 uiSize = shaderCode.GetCount();
    stream << uiSize;
    stream.WriteBytes(shaderCode.GetPtr(), uiSize).AssertSuccess();

    const xiiUInt8 uiSets = sets.GetCount();
    stream << uiSets;
    for (xiiUInt8 i = 0; i < uiSets; ++i)
    {
      const xiiShaderDescriptorSetLayout& set = sets[i];
      stream << set.m_uiSet;

      const xiiUInt8 uiBindings = set.Bindings.GetCount();
      stream << uiBindings;

      for (xiiUInt8 j = 0; j < uiBindings; ++j)
      {
        const xiiShaderDescriptorSetLayoutBinding& binding = set.Bindings[j];
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

    for (xiiUInt8 i = 0; i < uiVIA; ++i)
    {
      const xiiShaderVertexInputAttribute& via = vertexInputAttributes[i];
      stream << static_cast<xiiUInt8>(via.m_eSemantic);
      stream << via.m_uiSemanticIndex;
      stream << static_cast<xiiUInt8>(via.m_eFormat);
    }
  }

  void Read(const xiiArrayPtr<const xiiUInt8> data, xiiArrayPtr<const xiiUInt8>& out_shaderCode, xiiDynamicArray<xiiShaderDescriptorSetLayout>& out_sets, xiiDynamicArray<xiiShaderVertexInputAttribute>& out_vertexInputAttributes)
  {
    xiiRawMemoryStreamReader stream(data.GetPtr(), data.GetCount());

    xiiUInt32 uiMetadataTag;
    stream >> uiMetadataTag;
    XII_ASSERT_DEV(uiMetadataTag == x_uiMetaDataTag, "Shader does not begin with x_uiMetaDataTag");

    xiiTypeVersion uiVersion = stream.ReadVersion(MetaDataVersion::CurrentVerion);

    xiiUInt32 uiSize = 0;
    stream >> uiSize;
    out_shaderCode = xiiArrayPtr<const xiiUInt8>(&data[(xiiUInt32)stream.GetReadPosition()], uiSize);
    stream.SkipBytes(uiSize);

    xiiUInt8 uiSets = 0;
    stream >> uiSets;
    out_sets.Reserve(uiSets);

    for (xiiUInt8 i = 0; i < uiSets; ++i)
    {
      xiiShaderDescriptorSetLayout& set = out_sets.ExpandAndGetRef();
      stream >> set.m_uiSet;

      xiiUInt8 uiBindings = 0;
      stream >> uiBindings;
      set.Bindings.Reserve(uiBindings);

      for (xiiInt8 j = 0; j < uiBindings; ++j)
      {
        xiiShaderDescriptorSetLayoutBinding& binding = set.Bindings.ExpandAndGetRef();

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
      for (xiiUInt8 i = 0; i < uiVIA; ++i)
      {
        xiiShaderVertexInputAttribute& via = out_vertexInputAttributes.ExpandAndGetRef();
        stream >> reinterpret_cast<xiiUInt8&>(via.m_eSemantic);
        stream >> via.m_uiSemanticIndex;
        stream >> reinterpret_cast<xiiUInt8&>(via.m_eFormat);
      }
    }
  }
} // namespace xiiShaderMetaData
