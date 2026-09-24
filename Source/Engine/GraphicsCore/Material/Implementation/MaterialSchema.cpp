/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <GraphicsCore/Material/MaterialSchema.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialParameterDefinition, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialParameterDefinition>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Id", m_Id),
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("DisplayName", m_sDisplayName),
    XII_MEMBER_PROPERTY("Category", m_sCategory),
    XII_ENUM_MEMBER_PROPERTY("Type", xiiMaterialParameterType, m_Type),
    XII_ENUM_MEMBER_PROPERTY("UpdateFrequency", xiiMaterialUpdateFrequency, m_UpdateFrequency),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiMaterialParameterFlags, m_Flags),
    XII_MEMBER_PROPERTY("DefaultValue", m_DefaultValue),
    XII_MEMBER_PROPERTY("MinValue", m_MinValue),
    XII_MEMBER_PROPERTY("MaxValue", m_MaxValue),
    XII_MEMBER_PROPERTY("Offset", m_uiOffset),
    XII_MEMBER_PROPERTY("Size", m_uiSize),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialTextureDefinition, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialTextureDefinition>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Id", m_Id),
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("DisplayName", m_sDisplayName),
    XII_ENUM_MEMBER_PROPERTY("TextureType", xiiGALShaderTextureType, m_TextureType),
    XII_ENUM_MEMBER_PROPERTY("UpdateFrequency", xiiMaterialUpdateFrequency, m_UpdateFrequency),
    XII_MEMBER_PROPERTY("BindingSlot", m_uiBindingSlot),
    XII_MEMBER_PROPERTY("Required", m_bRequired),
    XII_MEMBER_PROPERTY("Bindless", m_bBindless),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialSchemaDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialSchemaDescription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_RESOURCE_MEMBER_PROPERTY("Shader", m_hShader),
    XII_ENUM_MEMBER_PROPERTY("Domain", xiiMaterialDomain, m_Domain),
    XII_ENUM_MEMBER_PROPERTY("ShadingModel", xiiMaterialShadingModel, m_ShadingModel),
    XII_MEMBER_PROPERTY("Version", m_uiVersion),
    XII_ARRAY_MEMBER_PROPERTY("Parameters", m_Parameters),
    XII_ARRAY_MEMBER_PROPERTY("Textures", m_Textures),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  static xiiResult SetSchemaError(xiiStringBuilder* out_pError, xiiStringView sMessage)
  {
    if (out_pError != nullptr)
      *out_pError = sMessage;

    return XII_FAILURE;
  }

  static xiiUInt32 AlignParameterOffset(xiiUInt32 uiOffset, xiiUInt32 uiSize, xiiUInt32 uiAlignment)
  {
    uiOffset = xiiMemoryUtils::AlignSize(uiOffset, uiAlignment);

    // Scalars and small vectors may share a 16-byte register, but may not straddle one.
    if (uiSize <= 16U && uiOffset / 16U != (uiOffset + uiSize - 1U) / 16U)
      uiOffset = xiiMemoryUtils::AlignSize(uiOffset, 16U);

    return uiOffset;
  }
} // namespace

xiiMaterialParameterDefinition& xiiMaterialSchemaDescription::AddParameter(xiiStringView sName, xiiMaterialParameterType::Enum type, const xiiVariant& defaultValue)
{
  xiiMaterialParameterDefinition& definition = m_Parameters.ExpandAndGetRef();
  definition.m_Id                            = xiiMaterialParameterId::Make(sName);
  definition.m_sName.Assign(sName);
  definition.m_sDisplayName = sName;
  definition.m_Type         = type;
  definition.m_DefaultValue = defaultValue;
  return definition;
}

xiiMaterialTextureDefinition& xiiMaterialSchemaDescription::AddTexture(xiiStringView sName, xiiGALShaderTextureType::Enum textureType)
{
  xiiMaterialTextureDefinition& definition = m_Textures.ExpandAndGetRef();
  definition.m_Id                          = xiiMaterialParameterId::Make(sName);
  definition.m_sName.Assign(sName);
  definition.m_sDisplayName  = sName;
  definition.m_TextureType   = textureType;
  definition.m_uiBindingSlot = m_Textures.GetCount() - 1U;
  return definition;
}

xiiResult xiiMaterialSchema::Build(const xiiMaterialSchemaDescription& description, xiiStringBuilder* out_pError)
{
  Clear();

  if (description.m_sName.IsEmpty())
    return SetSchemaError(out_pError, "Material schema name must not be empty.");

  m_sName        = description.m_sName;
  m_hShader      = description.m_hShader;
  m_Domain       = description.m_Domain;
  m_ShadingModel = description.m_ShadingModel;
  m_uiVersion    = description.m_uiVersion;
  m_Parameters   = description.m_Parameters;
  m_Textures     = description.m_Textures;

  xiiHashStreamWriter64 layoutHash;
  layoutHash << m_sName;
  layoutHash << m_uiVersion;
  layoutHash << m_Domain.GetValue();
  layoutHash << m_ShadingModel.GetValue();

  xiiUInt32 uiOffset = 0U;
  for (xiiUInt32 i = 0U; i < m_Parameters.GetCount(); ++i)
  {
    xiiMaterialParameterDefinition& parameter = m_Parameters[i];

    if (parameter.m_sName.IsEmpty())
      return SetSchemaError(out_pError, "Material parameter names must not be empty.");

    if (!parameter.m_Id.IsValid())
      parameter.m_Id = xiiMaterialParameterId::Make(parameter.m_sName.GetString());

    if (m_ParameterLookup.Contains(parameter.m_Id.m_uiValue))
      return SetSchemaError(out_pError, "Material parameter IDs must be unique. Check for duplicate names or a hash collision.");

    if (parameter.m_DefaultValue.IsValid() && !IsValueCompatible(parameter.m_Type, parameter.m_DefaultValue))
      return SetSchemaError(out_pError, "A material parameter default value is incompatible with its declared type.");

    parameter.m_uiSize   = GetPackedSize(parameter.m_Type);
    parameter.m_uiOffset = AlignParameterOffset(uiOffset, parameter.m_uiSize, GetPackedAlignment(parameter.m_Type));
    uiOffset             = parameter.m_uiOffset + parameter.m_uiSize;
    m_ParameterLookup.Insert(parameter.m_Id.m_uiValue, i);

    layoutHash << parameter.m_Id.m_uiValue;
    layoutHash << parameter.m_Type.GetValue();
    layoutHash << parameter.m_UpdateFrequency.GetValue();
    layoutHash << parameter.m_uiOffset;
    layoutHash << parameter.m_uiSize;
  }

  m_uiParameterBlockSize = xiiMemoryUtils::AlignSize(uiOffset, 16U);

  for (xiiUInt32 i = 0U; i < m_Textures.GetCount(); ++i)
  {
    xiiMaterialTextureDefinition& texture = m_Textures[i];

    if (texture.m_sName.IsEmpty())
      return SetSchemaError(out_pError, "Material texture names must not be empty.");

    if (!texture.m_Id.IsValid())
      texture.m_Id = xiiMaterialParameterId::Make(texture.m_sName.GetString());

    if (m_TextureLookup.Contains(texture.m_Id.m_uiValue))
      return SetSchemaError(out_pError, "Material texture IDs must be unique. Check for duplicate names or a hash collision.");

    if (texture.m_uiBindingSlot == xiiInvalidIndex)
      texture.m_uiBindingSlot = i;

    m_TextureLookup.Insert(texture.m_Id.m_uiValue, i);

    layoutHash << texture.m_Id.m_uiValue;
    layoutHash << texture.m_TextureType.GetValue();
    layoutHash << texture.m_UpdateFrequency.GetValue();
    layoutHash << texture.m_uiBindingSlot;
    layoutHash << texture.m_bBindless;
  }

  layoutHash << m_uiParameterBlockSize;
  m_uiLayoutHash = layoutHash.GetHashValue();
  m_bValid       = true;
  return XII_SUCCESS;
}

void xiiMaterialSchema::Clear()
{
  m_sName.Clear();
  m_hShader.Invalidate();
  m_Domain               = xiiMaterialDomain::Surface;
  m_ShadingModel         = xiiMaterialShadingModel::Lit;
  m_uiVersion            = 0U;
  m_uiParameterBlockSize = 0U;
  m_uiLayoutHash         = 0U;
  m_Parameters.Clear();
  m_Textures.Clear();
  m_ParameterLookup.Clear();
  m_TextureLookup.Clear();
  m_bValid = false;
}

const xiiMaterialParameterDefinition* xiiMaterialSchema::FindParameter(xiiMaterialParameterId id) const
{
  xiiUInt32 uiIndex = xiiInvalidIndex;
  if (m_ParameterLookup.TryGetValue(id.m_uiValue, uiIndex))
    return &m_Parameters[uiIndex];

  return nullptr;
}

const xiiMaterialParameterDefinition* xiiMaterialSchema::FindParameter(const xiiTempHashedString& sName) const
{
  xiiMaterialParameterId id;
  id.m_uiValue = sName.GetHash();
  return FindParameter(id);
}

const xiiMaterialTextureDefinition* xiiMaterialSchema::FindTexture(xiiMaterialParameterId id) const
{
  xiiUInt32 uiIndex = xiiInvalidIndex;
  if (m_TextureLookup.TryGetValue(id.m_uiValue, uiIndex))
    return &m_Textures[uiIndex];

  return nullptr;
}

const xiiMaterialTextureDefinition* xiiMaterialSchema::FindTexture(const xiiTempHashedString& sName) const
{
  xiiMaterialParameterId id;
  id.m_uiValue = sName.GetHash();
  return FindTexture(id);
}

xiiUInt32 xiiMaterialSchema::FindParameterIndex(xiiMaterialParameterId id) const
{
  xiiUInt32 uiIndex = xiiInvalidIndex;
  m_ParameterLookup.TryGetValue(id.m_uiValue, uiIndex);
  return uiIndex;
}

xiiUInt32 xiiMaterialSchema::FindTextureIndex(xiiMaterialParameterId id) const
{
  xiiUInt32 uiIndex = xiiInvalidIndex;
  m_TextureLookup.TryGetValue(id.m_uiValue, uiIndex);
  return uiIndex;
}

xiiUInt32 xiiMaterialSchema::GetPackedSize(xiiMaterialParameterType::Enum type)
{
  switch (type)
  {
    case xiiMaterialParameterType::Bool:
    case xiiMaterialParameterType::Int:
    case xiiMaterialParameterType::UInt:
    case xiiMaterialParameterType::Float:
      return 4U;
    case xiiMaterialParameterType::Float2:
      return 8U;
    case xiiMaterialParameterType::Float3:
    case xiiMaterialParameterType::Float4:
    case xiiMaterialParameterType::Color:
      return 16U;
    case xiiMaterialParameterType::Matrix3:
      return 48U;
    case xiiMaterialParameterType::Matrix4:
      return 64U;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return 0U;
  }
}

xiiUInt32 xiiMaterialSchema::GetPackedAlignment(xiiMaterialParameterType::Enum type)
{
  switch (type)
  {
    case xiiMaterialParameterType::Bool:
    case xiiMaterialParameterType::Int:
    case xiiMaterialParameterType::UInt:
    case xiiMaterialParameterType::Float:
      return 4U;
    case xiiMaterialParameterType::Float2:
      return 8U;
    default:
      return 16U;
  }
}

bool xiiMaterialSchema::IsValueCompatible(xiiMaterialParameterType::Enum type, const xiiVariant& value)
{
  if (!value.IsValid())
    return true;

  switch (type)
  {
    case xiiMaterialParameterType::Bool: return value.CanConvertTo<bool>();
    case xiiMaterialParameterType::Int: return value.CanConvertTo<xiiInt32>();
    case xiiMaterialParameterType::UInt: return value.CanConvertTo<xiiUInt32>();
    case xiiMaterialParameterType::Float: return value.CanConvertTo<float>();
    case xiiMaterialParameterType::Float2: return value.CanConvertTo<xiiVec2>();
    case xiiMaterialParameterType::Float3: return value.CanConvertTo<xiiVec3>();
    case xiiMaterialParameterType::Float4: return value.CanConvertTo<xiiVec4>();
    case xiiMaterialParameterType::Color: return value.CanConvertTo<xiiColor>();
    case xiiMaterialParameterType::Matrix3: return value.CanConvertTo<xiiMat3>();
    case xiiMaterialParameterType::Matrix4: return value.CanConvertTo<xiiMat4>();
    default: return false;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Material_Implementation_MaterialSchema);
