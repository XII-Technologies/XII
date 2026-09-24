/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Material/MaterialCompiler.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMaterialCompilationSeverity, 1)
  XII_ENUM_CONSTANTS(xiiMaterialCompilationSeverity::Info, xiiMaterialCompilationSeverity::Warning, xiiMaterialCompilationSeverity::Error)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialCompilationMessage, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialCompilationMessage>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Severity", xiiMaterialCompilationSeverity, m_Severity),
    XII_MEMBER_PROPERTY("Parameter", m_sParameter),
    XII_MEMBER_PROPERTY("Message", m_sMessage),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiCompiledMaterialLayout, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiCompiledMaterialLayout>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SchemaHash", m_uiSchemaHash),
    XII_MEMBER_PROPERTY("ShaderBlockSize", m_uiShaderBlockSize),
    XII_MEMBER_PROPERTY("ParameterCount", m_uiParameterCount),
    XII_MEMBER_PROPERTY("TextureCount", m_uiTextureCount),
    XII_ARRAY_MEMBER_PROPERTY("Messages", m_Messages),
    XII_MEMBER_PROPERTY("Valid", m_bValid),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  static constexpr xiiGALShaderType::Enum s_ShaderStages[] = {
    xiiGALShaderType::Vertex, xiiGALShaderType::Pixel, xiiGALShaderType::Geometry, xiiGALShaderType::Hull,
    xiiGALShaderType::Domain, xiiGALShaderType::Compute, xiiGALShaderType::Amplification, xiiGALShaderType::Mesh,
    xiiGALShaderType::RayGeneration, xiiGALShaderType::RayMiss, xiiGALShaderType::RayClosestHit, xiiGALShaderType::RayAnyHit,
    xiiGALShaderType::RayIntersection, xiiGALShaderType::Callable, xiiGALShaderType::Tile};

  static void AddMessage(xiiCompiledMaterialLayout& out_layout, xiiMaterialCompilationSeverity::Enum severity, xiiStringView sParameter, xiiStringView sMessage)
  {
    xiiMaterialCompilationMessage& message = out_layout.m_Messages.ExpandAndGetRef();
    message.m_Severity                     = severity;
    message.m_sParameter                  = sParameter;
    message.m_sMessage                    = sMessage;
  }

  static const xiiGALShaderResourceDescription* FindResource(const xiiShaderPermutationResource& permutation, const xiiTempHashedString& sName)
  {
    for (xiiGALShaderType::Enum stage : s_ShaderStages)
    {
      if (!permutation.GetActiveShaderStages().IsSet(stage))
        continue;

      const xiiGALShaderByteCode* pByteCode = permutation.GetShaderByteCode(stage);
      if (pByteCode != nullptr)
      {
        if (const xiiGALShaderResourceDescription* pResource = pByteCode->GetDescription(sName))
          return pResource;
      }
    }
    return nullptr;
  }

  static const xiiGALShaderVariableDescription* FindVariable(const xiiGALShaderResourceDescription& resource, const xiiTempHashedString& sName)
  {
    for (const xiiGALShaderVariableDescription& variable : resource.m_Variables)
    {
      if (variable.m_sName == sName)
        return &variable;
    }
    return nullptr;
  }

  static bool IsShaderTypeCompatible(xiiMaterialParameterType::Enum materialType, const xiiGALShaderVariableDescription& shaderType)
  {
    switch (materialType)
    {
      case xiiMaterialParameterType::Bool:
        return shaderType.m_PrimitiveType == xiiGALShaderPrimitiveType::Bool && shaderType.m_uiColumnCount <= 1U;
      case xiiMaterialParameterType::Int:
        return shaderType.m_PrimitiveType == xiiGALShaderPrimitiveType::Int32 && shaderType.m_uiColumnCount <= 1U;
      case xiiMaterialParameterType::UInt:
        return shaderType.m_PrimitiveType == xiiGALShaderPrimitiveType::UInt32 && shaderType.m_uiColumnCount <= 1U;
      case xiiMaterialParameterType::Float:
        return shaderType.m_PrimitiveType == xiiGALShaderPrimitiveType::Float32 && shaderType.m_uiColumnCount <= 1U;
      case xiiMaterialParameterType::Float2:
        return shaderType.m_PrimitiveType == xiiGALShaderPrimitiveType::Float32 && shaderType.m_uiRowCount <= 1U && shaderType.m_uiColumnCount == 2U;
      case xiiMaterialParameterType::Float3:
        return shaderType.m_PrimitiveType == xiiGALShaderPrimitiveType::Float32 && shaderType.m_uiRowCount <= 1U && shaderType.m_uiColumnCount == 3U;
      case xiiMaterialParameterType::Float4:
      case xiiMaterialParameterType::Color:
        return shaderType.m_PrimitiveType == xiiGALShaderPrimitiveType::Float32 && shaderType.m_uiRowCount <= 1U && shaderType.m_uiColumnCount == 4U;
      case xiiMaterialParameterType::Matrix3:
        return shaderType.m_PrimitiveType == xiiGALShaderPrimitiveType::Float32 && shaderType.m_uiRowCount == 3U && shaderType.m_uiColumnCount == 3U;
      case xiiMaterialParameterType::Matrix4:
        return shaderType.m_PrimitiveType == xiiGALShaderPrimitiveType::Float32 && shaderType.m_uiRowCount == 4U && shaderType.m_uiColumnCount == 4U;
      default: return false;
    }
  }
} // namespace

xiiResult xiiMaterialCompiler::ValidateShaderLayout(const xiiMaterialSchema& schema, const xiiShaderPermutationResource& permutation, xiiCompiledMaterialLayout& out_layout, xiiStringView sParameterBlockName)
{
  out_layout = {};
  out_layout.m_uiSchemaHash     = schema.GetLayoutHash();
  out_layout.m_uiParameterCount = schema.GetParameters().GetCount();
  out_layout.m_uiTextureCount   = schema.GetTextures().GetCount();

  if (!schema.IsValid() || !permutation.IsShaderValid())
  {
    AddMessage(out_layout, xiiMaterialCompilationSeverity::Error, {}, "The material schema or shader permutation is invalid.");
    return XII_FAILURE;
  }

  const xiiTempHashedString blockName(sParameterBlockName);
  const xiiGALShaderResourceDescription* pParameterBlock = FindResource(permutation, blockName);
  if (pParameterBlock == nullptr)
  {
    AddMessage(out_layout, xiiMaterialCompilationSeverity::Error, sParameterBlockName, "The shader does not declare the material parameter block.");
    return XII_FAILURE;
  }

  out_layout.m_uiShaderBlockSize = pParameterBlock->m_uiTotalSize;
  bool bValid = pParameterBlock->m_Type == xiiGALShaderResourceType::ConstantBuffer || pParameterBlock->m_Type == xiiGALShaderResourceType::BufferSRV;
  if (!bValid)
    AddMessage(out_layout, xiiMaterialCompilationSeverity::Error, sParameterBlockName, "The material parameter block must be a constant buffer or read-only buffer.");

  if (pParameterBlock->m_uiTotalSize < schema.GetParameterBlockSize())
  {
    AddMessage(out_layout, xiiMaterialCompilationSeverity::Error, sParameterBlockName, "The reflected shader block is smaller than the schema's canonical parameter block.");
    bValid = false;
  }

  for (const xiiMaterialParameterDefinition& parameter : schema.GetParameters())
  {
    const xiiGALShaderVariableDescription* pVariable = FindVariable(*pParameterBlock, xiiTempHashedString(parameter.m_sName.GetString()));
    if (pVariable == nullptr)
    {
      AddMessage(out_layout, xiiMaterialCompilationSeverity::Error, parameter.m_sName.GetString(), "The schema parameter is missing from shader reflection.");
      bValid = false;
      continue;
    }

    if (pVariable->m_uiOffset != parameter.m_uiOffset)
    {
      AddMessage(out_layout, xiiMaterialCompilationSeverity::Error, parameter.m_sName.GetString(), "The shader offset does not match the canonical schema offset.");
      bValid = false;
    }
    if (!IsShaderTypeCompatible(parameter.m_Type, *pVariable))
    {
      AddMessage(out_layout, xiiMaterialCompilationSeverity::Error, parameter.m_sName.GetString(), "The shader variable type does not match the material parameter type.");
      bValid = false;
    }
  }

  for (const xiiMaterialTextureDefinition& texture : schema.GetTextures())
  {
    const xiiGALShaderResourceDescription* pTexture = FindResource(permutation, xiiTempHashedString(texture.m_sName.GetString()));
    if (pTexture == nullptr)
    {
      if (texture.m_bRequired)
      {
        AddMessage(out_layout, xiiMaterialCompilationSeverity::Error, texture.m_sName.GetString(), "A required material texture is missing from shader reflection.");
        bValid = false;
      }
      else
      {
        AddMessage(out_layout, xiiMaterialCompilationSeverity::Warning, texture.m_sName.GetString(), "An optional material texture is not used by this permutation.");
      }
      continue;
    }

    if (pTexture->m_TextureType != texture.m_TextureType)
    {
      AddMessage(out_layout, xiiMaterialCompilationSeverity::Error, texture.m_sName.GetString(), "The shader texture dimension does not match the material schema.");
      bValid = false;
    }
  }

  out_layout.m_bValid = bValid;
  return bValid ? XII_SUCCESS : XII_FAILURE;
}

