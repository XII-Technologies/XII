#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>

class xiiShaderMetaData
{
public:
  static constexpr xiiUInt32 x_uiMetaDataTag = 0x58494958U; // XIIX

  enum MetaDataVersion
  {
    Version1 = 1,

    ENUM_COUNT,

    CurrentVersion = ENUM_COUNT - 1
  };

  static void WriteShaderVariableMembers(xiiStreamWriter& stream, const xiiDynamicArray<xiiGALShaderVariableDescription>& variableDescriptions)
  {
    const xiiUInt32 uiResourceVariableCount = variableDescriptions.GetCount();
    stream << uiResourceVariableCount;

    if (variableDescriptions.IsEmpty())
      return;

    for (xiiUInt32 i = 0; i < uiResourceVariableCount; ++i)
    {
      const xiiGALShaderVariableDescription& variableDescription = variableDescriptions[i];

      stream.WriteString(variableDescription.m_sName).AssertSuccess();
      stream << variableDescription.m_Class.GetValue();
      stream << variableDescription.m_PrimitiveType.GetValue();
      stream << variableDescription.m_uiRowCount;
      stream << variableDescription.m_uiColumnCount;
      stream << variableDescription.m_uiOffset;
      stream << variableDescription.m_uiArraySize;

      WriteShaderVariableMembers(stream, variableDescription.m_Members);
    }
  }

  static void ReadShaderVariableMembers(xiiRawMemoryStreamReader& stream, const xiiArrayPtr<const xiiUInt8> pData, xiiDynamicArray<xiiGALShaderVariableDescription>& variableDescriptions)
  {
    xiiUInt32 uiResourceVariableCount = 0U;
    stream >> uiResourceVariableCount;

    if (uiResourceVariableCount == 0)
      return;

    variableDescriptions.Reserve(uiResourceVariableCount);

    for (xiiUInt32 i = 0; i < uiResourceVariableCount; ++i)
    {
      xiiGALShaderVariableDescription& variableDescription = variableDescriptions[i];

      xiiUInt32 uiStringElements = 0;
      stream >> uiStringElements;

      variableDescription.m_sName = xiiStringView(reinterpret_cast<const char*>(&pData[(xiiUInt32)stream.GetReadPosition()]), uiStringElements);
      stream.SkipBytes(uiStringElements);

      stream >> variableDescription.m_Class;
      stream >> variableDescription.m_PrimitiveType;
      stream >> variableDescription.m_uiRowCount;
      stream >> variableDescription.m_uiColumnCount;
      stream >> variableDescription.m_uiOffset;
      stream >> variableDescription.m_uiArraySize;

      ReadShaderVariableMembers(stream, pData, variableDescription.m_Members);
    }
  }

  /// \brief Writes the custom shader bytecode format to a stream.
  static void Write(xiiStreamWriter& stream, const xiiArrayPtr<xiiUInt8>& shaderByteCode, const xiiDynamicArray<xiiGALShaderResourceBinding>& shaderResourceBinding, const xiiDynamicArray<xiiGALVertexInputLayout>& vertexInputLayouts)
  {
    stream << x_uiMetaDataTag;
    stream.WriteVersion(MetaDataVersion::CurrentVersion);

    const xiiUInt32 uiSize = shaderByteCode.GetCount();
    stream << uiSize;
    stream.WriteBytes(shaderByteCode.GetPtr(), shaderByteCode.GetCount()).AssertSuccess();

    const xiiUInt32 uiBindingCount = shaderResourceBinding.GetCount();
    stream << uiBindingCount;

    for (xiiUInt32 i = 0; i < uiBindingCount; ++i)
    {
      const xiiGALShaderResourceBinding& resourceBinding = shaderResourceBinding[i];

      stream.WriteString(resourceBinding.m_sName).AssertSuccess();
      stream << resourceBinding.m_Type;
      stream << resourceBinding.m_uiSlot;
      stream << resourceBinding.m_uiSize;

      const xiiUInt32 uiResourceVariableCount = resourceBinding.m_Variables.GetCount();
      stream << uiResourceVariableCount;

      for (xiiUInt32 j = 0; j < uiResourceVariableCount; ++j)
      {
        const xiiGALShaderVariableDescription& variableDescription = resourceBinding.m_Variables[j];

        stream.WriteString(variableDescription.m_sName).AssertSuccess();
        stream << variableDescription.m_Class;
        stream << variableDescription.m_PrimitiveType;
        stream << variableDescription.m_uiRowCount;
        stream << variableDescription.m_uiColumnCount;
        stream << variableDescription.m_uiOffset;
        stream << variableDescription.m_uiArraySize;

        WriteShaderVariableMembers(stream, variableDescription.m_Members);
      }
    }

    const xiiUInt32 uiVertexInputLayoutCount = vertexInputLayouts.GetCount();
    stream << uiVertexInputLayoutCount;

    for (xiiUInt32 i = 0; i < uiVertexInputLayoutCount; ++i)
    {
      const xiiGALVertexInputLayout& vertexInputLayout = vertexInputLayouts[i];

      stream << vertexInputLayout.m_Semantic.GetValue();
      stream << vertexInputLayout.m_uiSemanticIndex;
      stream << vertexInputLayout.m_Format.GetValue();
    }
  }

  /// \brief Reads Shader code and meta data from a data buffer. Note that 'data' must be kept alive for the lifetime of the shader as this functions stores views into this memory in its out parameters.
  /// \param pData Raw data buffer to read the shader code and meta data from.
  /// \param out_shaderByteCode Will be filled with a view into data that contains the shader byte code.
  /// \param out_shaderResourceBinding Will be filled with shader meta data. Note that this array contains string views into 'data'.
  /// \param out_vertexInputLayout Contains the shader vertex input layout, if any.
  static void Read(const xiiArrayPtr<const xiiUInt8> pData, xiiArrayPtr<const xiiUInt8>& out_shaderByteCode, xiiDynamicArray<xiiGALShaderResourceBinding>& out_shaderResourceBinding, xiiDynamicArray<xiiGALVertexInputLayout>& out_vertexInputLayout)
  {
    xiiRawMemoryStreamReader stream(pData.GetPtr(), pData.GetCount());

    xiiUInt32 uiMetadataTag;
    stream >> uiMetadataTag;

    XII_ASSERT_DEV(uiMetadataTag == x_uiMetaDataTag, "Shader byte code does not begin with x_uiMetaDataTag.");

    xiiTypeVersion uiVersion = stream.ReadVersion(MetaDataVersion::CurrentVersion);

    xiiUInt32 uiSize = 0;
    stream >> uiSize;
    out_shaderByteCode = xiiArrayPtr<const xiiUInt8>(&pData[(xiiUInt32)stream.GetReadPosition()], uiSize);
    stream.SkipBytes(uiSize);

    xiiUInt32 uiBindingCount = 0;
    stream >> uiBindingCount;

    out_shaderResourceBinding.Reserve(uiBindingCount);

    for (xiiUInt32 i = 0; i < uiBindingCount; ++i)
    {
      xiiGALShaderResourceBinding& resourceBinding = out_shaderResourceBinding[i];

      xiiUInt32 uiStringElements = 0;
      stream >> uiStringElements;

      resourceBinding.m_sName.Assign(xiiStringView(reinterpret_cast<const char*>(&pData[(xiiUInt32)stream.GetReadPosition()]), uiStringElements));
      stream.SkipBytes(uiStringElements);

      stream >> resourceBinding.m_Type;
      stream >> resourceBinding.m_uiSlot;
      stream >> resourceBinding.m_uiSize;

      xiiUInt32 uiResourceVariableCount = 0;
      stream >> uiResourceVariableCount;

      resourceBinding.m_Variables.Reserve(uiResourceVariableCount);

      for (xiiUInt32 j = 0; j < uiResourceVariableCount; ++j)
      {
        xiiGALShaderVariableDescription& variableDescription = resourceBinding.m_Variables[j];

        uiStringElements = 0;
        stream >> uiStringElements;

        variableDescription.m_sName = xiiStringView(reinterpret_cast<const char*>(&pData[(xiiUInt32)stream.GetReadPosition()]), uiStringElements);
        stream.SkipBytes(uiStringElements);

        stream >> variableDescription.m_Class;
        stream >> variableDescription.m_PrimitiveType;
        stream >> variableDescription.m_uiRowCount;
        stream >> variableDescription.m_uiColumnCount;
        stream >> variableDescription.m_uiOffset;
        stream >> variableDescription.m_uiArraySize;

        ReadShaderVariableMembers(stream, pData, variableDescription.m_Members);
      }
    }

    xiiUInt32 uiVertexInputLayoutCount = 0;
    stream >> uiVertexInputLayoutCount;

    out_vertexInputLayout.Reserve(uiVertexInputLayoutCount);

    for (xiiUInt32 i = 0; i < uiVertexInputLayoutCount; ++i)
    {
      xiiGALVertexInputLayout& vertexInputLayout = out_vertexInputLayout[i];

      stream >> vertexInputLayout.m_Semantic;
      stream >> vertexInputLayout.m_uiSemanticIndex;
      stream >> vertexInputLayout.m_Format;
    }
  }
};
