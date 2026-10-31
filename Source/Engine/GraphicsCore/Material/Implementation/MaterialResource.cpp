/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Material/MaterialAssetFile.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>


namespace
{
  static xiiUInt32 FindParameter(const xiiMaterialResourceDescriptor& desc, const xiiTempHashedString& sName)
  {
    for (xiiUInt32 i = 0; i < desc.m_Parameters.GetCount(); ++i)
    {
      if (desc.m_Parameters[i].m_Name == sName)
        return i;
    }

    return xiiInvalidIndex;
  }

  static void SetPbrParameter(xiiMaterialResourceDescriptor& ref_desc, xiiStringView sName, const xiiVariant& value, bool bOnlyIfMissing)
  {
    const xiiTempHashedString sNameHash(sName);
    const xiiUInt32           uiIndex = FindParameter(ref_desc, sNameHash);

    if (uiIndex != xiiInvalidIndex)
    {
      if (!bOnlyIfMissing)
      {
        ref_desc.m_Parameters[uiIndex].m_Value = value;
      }
      return;
    }

    xiiMaterialResourceDescriptor::Parameter& param = ref_desc.m_Parameters.ExpandAndGetRef();
    param.m_Name.Assign(sName);
    param.m_Value = value;
  }

  static xiiUInt32 FindTexture2DBinding(const xiiMaterialResourceDescriptor& desc, const xiiTempHashedString& sName)
  {
    for (xiiUInt32 i = 0; i < desc.m_Texture2DBindings.GetCount(); ++i)
    {
      if (desc.m_Texture2DBindings[i].m_Name == sName)
        return i;
    }

    return xiiInvalidIndex;
  }

  static void SetPbrTexture2DBinding(xiiMaterialResourceDescriptor& ref_desc, xiiStringView sName, const xiiTexture2DResourceHandle& hTexture, bool bOnlyIfMissing)
  {
    if (!hTexture.IsValid())
      return;

    const xiiTempHashedString sNameHash(sName);
    const xiiUInt32           uiIndex = FindTexture2DBinding(ref_desc, sNameHash);

    if (uiIndex != xiiInvalidIndex)
    {
      if (!bOnlyIfMissing)
      {
        ref_desc.m_Texture2DBindings[uiIndex].m_Value = hTexture;
      }
      return;
    }

    xiiMaterialResourceDescriptor::Texture2DBinding& binding = ref_desc.m_Texture2DBindings.ExpandAndGetRef();
    binding.m_Name.Assign(sName);
    binding.m_Value = hTexture;
  }

  static constexpr xiiUInt32 TextureSlotBit(xiiMaterialTextureSlot::Enum slot)
  {
    return 1U << static_cast<xiiUInt8>(slot);
  }

  static bool TryGetMaterialParameterType(const xiiVariant& value, xiiMaterialParameterType::Enum& out_type)
  {
    switch (value.GetType())
    {
      case xiiVariantType::Bool:
        out_type = xiiMaterialParameterType::Bool;
        return true;
      case xiiVariantType::Int8:
      case xiiVariantType::Int16:
      case xiiVariantType::Int32:
      case xiiVariantType::Int64:
        out_type = xiiMaterialParameterType::Int;
        return true;
      case xiiVariantType::UInt8:
      case xiiVariantType::UInt16:
      case xiiVariantType::UInt32:
      case xiiVariantType::UInt64:
        out_type = xiiMaterialParameterType::UInt;
        return true;
      case xiiVariantType::Float:
      case xiiVariantType::Double:
        out_type = xiiMaterialParameterType::Float;
        return true;
      case xiiVariantType::Vector2:
        out_type = xiiMaterialParameterType::Float2;
        return true;
      case xiiVariantType::Vector3:
        out_type = xiiMaterialParameterType::Float3;
        return true;
      case xiiVariantType::Vector4:
        out_type = xiiMaterialParameterType::Float4;
        return true;
      case xiiVariantType::Color:
      case xiiVariantType::ColorGamma:
        out_type = xiiMaterialParameterType::Color;
        return true;
      case xiiVariantType::Matrix3:
        out_type = xiiMaterialParameterType::Matrix3;
        return true;
      case xiiVariantType::Matrix4:
        out_type = xiiMaterialParameterType::Matrix4;
        return true;
      default:
        return false;
    }
  }

} // namespace

void xiiMaterialResourceDescriptor::Clear()
{
  m_hBaseMaterial.Invalidate();
  m_sSurface.Clear();
  m_hShader.Invalidate();

  m_Domain       = xiiMaterialDomain::Surface;
  m_ShadingModel = xiiMaterialShadingModel::Lit;
  m_BlendMode    = xiiMaterialBlendMode::Opaque;
  m_AlphaMode    = xiiMaterialAlphaMode::Opaque;
  m_FeatureFlags = xiiMaterialFeatureFlags::Default;

  m_BaseColor           = xiiColor::White;
  m_EmissiveColor       = xiiColor::Black;
  m_fMetallic           = 0.0f;
  m_fRoughness          = 0.5f;
  m_fOcclusionStrength  = 1.0f;
  m_fAlphaCutoff        = 0.5f;
  m_fNormalScale        = 1.0f;
  m_fDisplacementScale  = 0.0f;
  m_fClearCoat          = 0.0f;
  m_fClearCoatRoughness = 0.0f;
  m_fTransmission       = 0.0f;
  m_fThickness          = 0.0f;
  m_fIndexOfRefraction  = 1.5f;
  m_fAnisotropy         = 0.0f;
  m_fSheenRoughness     = 0.5f;
  m_iSortPriority       = 0;

  m_hBaseColorTexture.Invalidate();
  m_hNormalTexture.Invalidate();
  m_hMetallicRoughnessTexture.Invalidate();
  m_hOcclusionTexture.Invalidate();
  m_hEmissiveTexture.Invalidate();
  m_hHeightTexture.Invalidate();
  m_hClearCoatTexture.Invalidate();
  m_hTransmissionTexture.Invalidate();

  m_uiRuntimeHash = 0U;

  m_PermutationVariables.Clear();
  m_Parameters.Clear();
  m_Texture2DBindings.Clear();
  m_TextureCubeBindings.Clear();
}

xiiUInt32 xiiMaterialResourceDescriptor::ComputeRuntimeHash() const
{
  xiiHashStreamWriter32 hashWriter;
  hashWriter << (m_hBaseMaterial.IsValid() ? m_hBaseMaterial.GetResourceIDHash() : 0ULL);
  hashWriter << (m_hShader.IsValid() ? m_hShader.GetResourceIDHash() : 0ULL);
  hashWriter << m_Domain.GetValue();
  hashWriter << m_ShadingModel.GetValue();
  hashWriter << m_BlendMode.GetValue();
  hashWriter << m_AlphaMode.GetValue();
  hashWriter << m_FeatureFlags.GetValue();
  hashWriter << m_BaseColor.r;
  hashWriter << m_BaseColor.g;
  hashWriter << m_BaseColor.b;
  hashWriter << m_BaseColor.a;
  hashWriter << m_EmissiveColor.r;
  hashWriter << m_EmissiveColor.g;
  hashWriter << m_EmissiveColor.b;
  hashWriter << m_EmissiveColor.a;
  hashWriter << m_fMetallic;
  hashWriter << m_fRoughness;
  hashWriter << m_fOcclusionStrength;
  hashWriter << m_fAlphaCutoff;
  hashWriter << m_fNormalScale;
  hashWriter << m_fDisplacementScale;
  hashWriter << m_fClearCoat;
  hashWriter << m_fClearCoatRoughness;
  hashWriter << m_fTransmission;
  hashWriter << m_fThickness;
  hashWriter << m_fIndexOfRefraction;
  hashWriter << m_fAnisotropy;
  hashWriter << m_fSheenRoughness;
  hashWriter << m_iSortPriority;
  hashWriter << (m_hBaseColorTexture.IsValid() ? m_hBaseColorTexture.GetResourceIDHash() : 0ULL);
  hashWriter << (m_hNormalTexture.IsValid() ? m_hNormalTexture.GetResourceIDHash() : 0ULL);
  hashWriter << (m_hMetallicRoughnessTexture.IsValid() ? m_hMetallicRoughnessTexture.GetResourceIDHash() : 0ULL);
  hashWriter << (m_hOcclusionTexture.IsValid() ? m_hOcclusionTexture.GetResourceIDHash() : 0ULL);
  hashWriter << (m_hEmissiveTexture.IsValid() ? m_hEmissiveTexture.GetResourceIDHash() : 0ULL);
  hashWriter << (m_hHeightTexture.IsValid() ? m_hHeightTexture.GetResourceIDHash() : 0ULL);
  hashWriter << (m_hClearCoatTexture.IsValid() ? m_hClearCoatTexture.GetResourceIDHash() : 0ULL);
  hashWriter << (m_hTransmissionTexture.IsValid() ? m_hTransmissionTexture.GetResourceIDHash() : 0ULL);
  hashWriter << m_PermutationVariables.GetCount();
  for (const xiiGALPermutationVariable& permutation : m_PermutationVariables)
  {
    hashWriter << permutation.m_sName.GetHash();
    hashWriter << permutation.m_sValue.GetHash();
  }
  hashWriter << m_Parameters.GetCount();
  for (const Parameter& parameter : m_Parameters)
  {
    hashWriter << parameter.m_Name.GetHash();
    hashWriter << parameter.m_Value;
  }
  hashWriter << m_Texture2DBindings.GetCount();
  for (const Texture2DBinding& binding : m_Texture2DBindings)
  {
    hashWriter << binding.m_Name.GetHash();
    hashWriter << (binding.m_Value.IsValid() ? binding.m_Value.GetResourceIDHash() : 0ULL);
  }
  hashWriter << m_TextureCubeBindings.GetCount();
  for (const TextureCubeBinding& binding : m_TextureCubeBindings)
  {
    hashWriter << binding.m_Name.GetHash();
    hashWriter << (binding.m_Value.IsValid() ? binding.m_Value.GetResourceIDHash() : 0ULL);
  }

  return hashWriter.GetHashValue();
}

void xiiMaterialResourceDescriptor::RecomputeRuntimeHash()
{
  m_uiRuntimeHash = ComputeRuntimeHash();
}

xiiMaterialRuntimeState xiiMaterialResourceDescriptor::BuildRuntimeState() const
{
  xiiMaterialRuntimeState state;
  state.m_Domain        = m_Domain;
  state.m_ShadingModel  = m_ShadingModel;
  state.m_BlendMode     = m_BlendMode;
  state.m_AlphaMode     = m_AlphaMode;
  state.m_FeatureFlags  = m_FeatureFlags;
  state.m_uiRuntimeHash = m_uiRuntimeHash;
  state.m_iSortPriority = m_iSortPriority;

  for (const auto& variable : m_PermutationVariables)
  {
    const auto name  = variable.m_sName.GetString();
    const auto value = variable.m_sValue.GetString();
    if (name == "BLEND_MODE")
    {
      if (value == "BLEND_MODE_OPAQUE")
      {
        state.m_BlendMode = xiiMaterialBlendMode::Opaque;
        state.m_AlphaMode = xiiMaterialAlphaMode::Opaque;
      }
      else if (value == "BLEND_MODE_MASKED")
      {
        state.m_BlendMode = xiiMaterialBlendMode::Masked;
        state.m_AlphaMode = xiiMaterialAlphaMode::Mask;
      }
      else if (value == "BLEND_MODE_TRANSPARENT")
      {
        state.m_BlendMode = xiiMaterialBlendMode::Translucent;
        state.m_AlphaMode = xiiMaterialAlphaMode::Blend;
      }
      else if (value == "BLEND_MODE_ADDITIVE")
      {
        state.m_BlendMode = xiiMaterialBlendMode::Additive;
        state.m_AlphaMode = xiiMaterialAlphaMode::Blend;
      }
      else if (value == "BLEND_MODE_MODULATE")
      {
        state.m_BlendMode = xiiMaterialBlendMode::Modulate;
        state.m_AlphaMode = xiiMaterialAlphaMode::Blend;
      }
    }
    else if (name == "TWO_SIDED")
      state.m_FeatureFlags.AddOrRemove(xiiMaterialFeatureFlags::TwoSided, value == "TRUE");
    else if (name == "SHADING_MODE")
    {
      if (value == "SHADING_MODE_FULLBRIGHT")
        state.m_ShadingModel = xiiMaterialShadingModel::Unlit;
      else if (state.m_ShadingModel == xiiMaterialShadingModel::Unlit)
        state.m_ShadingModel = xiiMaterialShadingModel::Lit;
      state.m_FeatureFlags.AddOrRemove(xiiMaterialFeatureFlags::ReceivesLighting, state.m_ShadingModel != xiiMaterialShadingModel::Unlit);
    }
  }

  if (m_hBaseColorTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("BaseColorTexture")) != xiiInvalidIndex)
    state.m_uiTextureMask |= TextureSlotBit(xiiMaterialTextureSlot::BaseColor);
  if (m_hNormalTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("NormalTexture")) != xiiInvalidIndex)
    state.m_uiTextureMask |= TextureSlotBit(xiiMaterialTextureSlot::Normal);
  if (m_hMetallicRoughnessTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("MetallicRoughnessTexture")) != xiiInvalidIndex)
    state.m_uiTextureMask |= TextureSlotBit(xiiMaterialTextureSlot::MetallicRoughness);
  if (m_hOcclusionTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("OcclusionTexture")) != xiiInvalidIndex)
    state.m_uiTextureMask |= TextureSlotBit(xiiMaterialTextureSlot::Occlusion);
  if (m_hEmissiveTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("EmissiveTexture")) != xiiInvalidIndex)
    state.m_uiTextureMask |= TextureSlotBit(xiiMaterialTextureSlot::Emissive);
  if (m_hHeightTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("HeightTexture")) != xiiInvalidIndex)
    state.m_uiTextureMask |= TextureSlotBit(xiiMaterialTextureSlot::Height);
  if (m_hClearCoatTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("ClearCoatTexture")) != xiiInvalidIndex)
    state.m_uiTextureMask |= TextureSlotBit(xiiMaterialTextureSlot::ClearCoat);
  if (m_hTransmissionTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("TransmissionTexture")) != xiiInvalidIndex)
    state.m_uiTextureMask |= TextureSlotBit(xiiMaterialTextureSlot::Transmission);

  xiiHashStreamWriter32 keyWriter;
  keyWriter << state.m_ShadingModel.GetValue();
  keyWriter << state.m_BlendMode.GetValue();
  keyWriter << state.m_AlphaMode.GetValue();
  keyWriter << state.m_FeatureFlags.GetValue();
  keyWriter << state.m_uiTextureMask;
  state.m_uiPipelineKey = keyWriter.GetHashValue();

  return state;
}

void xiiMaterialResourceDescriptor::ApplyPbrParameterDefaults(bool bOnlyIfMissing)
{
  m_FeatureFlags.AddOrRemove(xiiMaterialFeatureFlags::NormalTexture, m_hNormalTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("NormalTexture")) != xiiInvalidIndex);
  m_FeatureFlags.AddOrRemove(xiiMaterialFeatureFlags::MetallicRoughnessTexture, m_hMetallicRoughnessTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("MetallicRoughnessTexture")) != xiiInvalidIndex);
  m_FeatureFlags.AddOrRemove(xiiMaterialFeatureFlags::OcclusionTexture, m_hOcclusionTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("OcclusionTexture")) != xiiInvalidIndex);
  m_FeatureFlags.AddOrRemove(xiiMaterialFeatureFlags::EmissiveTexture, m_hEmissiveTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("EmissiveTexture")) != xiiInvalidIndex);
  m_FeatureFlags.AddOrRemove(xiiMaterialFeatureFlags::HeightTexture, m_hHeightTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("HeightTexture")) != xiiInvalidIndex);
  m_FeatureFlags.AddOrRemove(xiiMaterialFeatureFlags::ClearCoat, m_fClearCoat > 0.0f || m_hClearCoatTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("ClearCoatTexture")) != xiiInvalidIndex);
  m_FeatureFlags.AddOrRemove(xiiMaterialFeatureFlags::Transmission, m_fTransmission > 0.0f || m_hTransmissionTexture.IsValid() || FindTexture2DBinding(*this, xiiTempHashedString("TransmissionTexture")) != xiiInvalidIndex);
  m_FeatureFlags.AddOrRemove(xiiMaterialFeatureFlags::Sheen, m_fSheenRoughness < 1.0f);
  m_FeatureFlags.AddOrRemove(xiiMaterialFeatureFlags::Anisotropy, m_fAnisotropy != 0.0f);

  if (m_hBaseMaterial.IsValid())
  {
    // Inherited materials store sparse overrides. Injecting local PBR defaults
    // here would silently replace their parent's roughness, color and emission.
    SetPbrTexture2DBinding(*this, "BaseColorTexture", m_hBaseColorTexture, bOnlyIfMissing);
    SetPbrTexture2DBinding(*this, "NormalTexture", m_hNormalTexture, bOnlyIfMissing);
    SetPbrTexture2DBinding(*this, "MetallicRoughnessTexture", m_hMetallicRoughnessTexture, bOnlyIfMissing);
    SetPbrTexture2DBinding(*this, "OcclusionTexture", m_hOcclusionTexture, bOnlyIfMissing);
    SetPbrTexture2DBinding(*this, "EmissiveTexture", m_hEmissiveTexture, bOnlyIfMissing);
    SetPbrTexture2DBinding(*this, "HeightTexture", m_hHeightTexture, bOnlyIfMissing);
    SetPbrTexture2DBinding(*this, "ClearCoatTexture", m_hClearCoatTexture, bOnlyIfMissing);
    SetPbrTexture2DBinding(*this, "TransmissionTexture", m_hTransmissionTexture, bOnlyIfMissing);
    RecomputeRuntimeHash();
    return;
  }

  SetPbrParameter(*this, "BaseColor", m_BaseColor, bOnlyIfMissing);
  SetPbrParameter(*this, "EmissiveColor", m_EmissiveColor, bOnlyIfMissing);
  SetPbrParameter(*this, "Metallic", m_fMetallic, bOnlyIfMissing);
  SetPbrParameter(*this, "Roughness", m_fRoughness, bOnlyIfMissing);
  SetPbrParameter(*this, "OcclusionStrength", m_fOcclusionStrength, bOnlyIfMissing);
  SetPbrParameter(*this, "AlphaCutoff", m_fAlphaCutoff, bOnlyIfMissing);
  SetPbrParameter(*this, "NormalScale", m_fNormalScale, bOnlyIfMissing);
  SetPbrParameter(*this, "DisplacementScale", m_fDisplacementScale, bOnlyIfMissing);
  SetPbrParameter(*this, "ClearCoat", m_fClearCoat, bOnlyIfMissing);
  SetPbrParameter(*this, "ClearCoatRoughness", m_fClearCoatRoughness, bOnlyIfMissing);
  SetPbrParameter(*this, "Transmission", m_fTransmission, bOnlyIfMissing);
  SetPbrParameter(*this, "Thickness", m_fThickness, bOnlyIfMissing);
  SetPbrParameter(*this, "IndexOfRefraction", m_fIndexOfRefraction, bOnlyIfMissing);
  SetPbrParameter(*this, "Anisotropy", m_fAnisotropy, bOnlyIfMissing);
  SetPbrParameter(*this, "SheenRoughness", m_fSheenRoughness, bOnlyIfMissing);

  SetPbrTexture2DBinding(*this, "BaseColorTexture", m_hBaseColorTexture, bOnlyIfMissing);
  SetPbrTexture2DBinding(*this, "NormalTexture", m_hNormalTexture, bOnlyIfMissing);
  SetPbrTexture2DBinding(*this, "MetallicRoughnessTexture", m_hMetallicRoughnessTexture, bOnlyIfMissing);
  SetPbrTexture2DBinding(*this, "OcclusionTexture", m_hOcclusionTexture, bOnlyIfMissing);
  SetPbrTexture2DBinding(*this, "EmissiveTexture", m_hEmissiveTexture, bOnlyIfMissing);
  SetPbrTexture2DBinding(*this, "HeightTexture", m_hHeightTexture, bOnlyIfMissing);
  SetPbrTexture2DBinding(*this, "ClearCoatTexture", m_hClearCoatTexture, bOnlyIfMissing);
  SetPbrTexture2DBinding(*this, "TransmissionTexture", m_hTransmissionTexture, bOnlyIfMissing);

  RecomputeRuntimeHash();
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMaterialResource, 1, xiiRTTIDefaultAllocator<xiiMaterialResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialResourceDescriptor::Parameter, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialResourceDescriptor::Parameter>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_Name),
    XII_MEMBER_PROPERTY("Value", m_Value),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialResourceDescriptor::Texture2DBinding, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialResourceDescriptor::Texture2DBinding>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_Name),
    XII_RESOURCE_MEMBER_PROPERTY("Texture", m_Value)->AddAttributes(new xiiFileBrowserAttribute("Select Texture 2D", "*.xiiTextureAsset;*.xiiRenderTargetAsset;*.xiiBinTexture2D;*.dds")),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialResourceDescriptor::TextureCubeBinding, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialResourceDescriptor::TextureCubeBinding>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_Name),
    XII_RESOURCE_MEMBER_PROPERTY("Texture", m_Value)->AddAttributes(new xiiFileBrowserAttribute("Select Texture Cube", "*.xiiTextureCubeAsset;*.xiiBinTextureCube;*.dds")),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialResourceDescriptor, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiMaterialResourceDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_MEMBER_PROPERTY("BaseMaterial", m_hBaseMaterial)->AddAttributes(new xiiFileBrowserAttribute("Select Base Material", "*.xiiMaterialAsset;*.xiiBinMaterial")),
    XII_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new xiiFileBrowserAttribute("Select Surface", "*.xiiSurfaceAsset;*.xiiBinSurface")),
    XII_RESOURCE_MEMBER_PROPERTY("Shader", m_hShader)->AddAttributes(new xiiFileBrowserAttribute("Select Shader", "*.xiiShader")),
    XII_ENUM_MEMBER_PROPERTY("Domain", xiiMaterialDomain, m_Domain),
    XII_ENUM_MEMBER_PROPERTY("ShadingModel", xiiMaterialShadingModel, m_ShadingModel),
    XII_ENUM_MEMBER_PROPERTY("BlendMode", xiiMaterialBlendMode, m_BlendMode),
    XII_ENUM_MEMBER_PROPERTY("AlphaMode", xiiMaterialAlphaMode, m_AlphaMode),
    XII_BITFLAGS_MEMBER_PROPERTY("Features", xiiMaterialFeatureFlags, m_FeatureFlags),
    XII_MEMBER_PROPERTY("BaseColor", m_BaseColor)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::White)),
    XII_MEMBER_PROPERTY("EmissiveColor", m_EmissiveColor)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::Black)),
    XII_MEMBER_PROPERTY("Metallic", m_fMetallic)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("Roughness", m_fRoughness)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("OcclusionStrength", m_fOcclusionStrength)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("AlphaCutoff", m_fAlphaCutoff)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("NormalScale", m_fNormalScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("DisplacementScale", m_fDisplacementScale)->AddAttributes(new xiiDefaultValueAttribute(0.0f)),
    XII_MEMBER_PROPERTY("ClearCoat", m_fClearCoat)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("ClearCoatRoughness", m_fClearCoatRoughness)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("Transmission", m_fTransmission)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("IndexOfRefraction", m_fIndexOfRefraction)->AddAttributes(new xiiDefaultValueAttribute(1.5f), new xiiClampValueAttribute(1.0f, 3.0f)),
    XII_MEMBER_PROPERTY("Anisotropy", m_fAnisotropy)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(-1.0f, 1.0f)),
    XII_MEMBER_PROPERTY("SheenRoughness", m_fSheenRoughness)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("SortPriority", m_iSortPriority),
    XII_RESOURCE_MEMBER_PROPERTY("BaseColorTexture", m_hBaseColorTexture)->AddAttributes(new xiiFileBrowserAttribute("Select BaseColor Texture", "*.xiiTextureAsset;*.xiiRenderTargetAsset;*.xiiBinTexture2D;*.dds")),
    XII_RESOURCE_MEMBER_PROPERTY("NormalTexture", m_hNormalTexture)->AddAttributes(new xiiFileBrowserAttribute("Select Normal Texture", "*.xiiTextureAsset;*.xiiRenderTargetAsset;*.xiiBinTexture2D;*.dds")),
    XII_RESOURCE_MEMBER_PROPERTY("MetallicRoughnessTexture", m_hMetallicRoughnessTexture)->AddAttributes(new xiiFileBrowserAttribute("Select MetallicRoughness Texture", "*.xiiTextureAsset;*.xiiRenderTargetAsset;*.xiiBinTexture2D;*.dds")),
    XII_RESOURCE_MEMBER_PROPERTY("OcclusionTexture", m_hOcclusionTexture)->AddAttributes(new xiiFileBrowserAttribute("Select Occlusion Texture", "*.xiiTextureAsset;*.xiiRenderTargetAsset;*.xiiBinTexture2D;*.dds")),
    XII_RESOURCE_MEMBER_PROPERTY("EmissiveTexture", m_hEmissiveTexture)->AddAttributes(new xiiFileBrowserAttribute("Select Emissive Texture", "*.xiiTextureAsset;*.xiiRenderTargetAsset;*.xiiBinTexture2D;*.dds")),
    XII_RESOURCE_MEMBER_PROPERTY("HeightTexture", m_hHeightTexture)->AddAttributes(new xiiFileBrowserAttribute("Select Height Texture", "*.xiiTextureAsset;*.xiiRenderTargetAsset;*.xiiBinTexture2D;*.dds")),
    XII_RESOURCE_MEMBER_PROPERTY("ClearCoatTexture", m_hClearCoatTexture)->AddAttributes(new xiiFileBrowserAttribute("Select ClearCoat Texture", "*.xiiTextureAsset;*.xiiRenderTargetAsset;*.xiiBinTexture2D;*.dds")),
    XII_RESOURCE_MEMBER_PROPERTY("TransmissionTexture", m_hTransmissionTexture)->AddAttributes(new xiiFileBrowserAttribute("Select Transmission Texture", "*.xiiTextureAsset;*.xiiRenderTargetAsset;*.xiiBinTexture2D;*.dds")),
    XII_ARRAY_MEMBER_PROPERTY("Parameters", m_Parameters),
    XII_ARRAY_MEMBER_PROPERTY("Texture2DBindings", m_Texture2DBindings),
    XII_ARRAY_MEMBER_PROPERTY("TextureCubeBindings", m_TextureCubeBindings),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiMaterialResource);

xiiMaterialResource::xiiMaterialResource() :
  xiiResource(DoUpdate::OnAnyThread, 1), m_iLastUpdated(0)
{
  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiMaterialResource::OnResourceEvent, this));
}

xiiMaterialResource::~xiiMaterialResource()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiMaterialResource::OnResourceEvent, this));
}

xiiHashedString xiiMaterialResource::GetPermutationValue(const xiiTempHashedString& sName)
{
  EnsureRuntimeData();
  XII_LOCK(m_RuntimeDataMutex);

  xiiHashedString sResult;
  m_ResolvedValues.m_PermutationVariables.TryGetValue(sName, sResult);

  return sResult;
}

xiiHashedString xiiMaterialResource::GetSurface() const
{
  if (!m_Description.m_sSurface.IsEmpty())
    return m_Description.m_sSurface;

  if (m_Description.m_hBaseMaterial.IsValid())
  {
    xiiResourceLock<xiiMaterialResource> pBaseMaterial(m_Description.m_hBaseMaterial, xiiResourceAcquireMode::BlockTillLoaded);
    return pBaseMaterial->GetSurface();
  }

  return xiiHashedString();
}

xiiEnum<xiiMaterialDomain> xiiMaterialResource::GetDomain() const
{
  return m_Description.m_Domain;
}

xiiEnum<xiiMaterialShadingModel> xiiMaterialResource::GetShadingModel() const
{
  return GetRuntimeState().m_ShadingModel;
}

xiiEnum<xiiMaterialBlendMode> xiiMaterialResource::GetBlendMode() const
{
  return GetRuntimeState().m_BlendMode;
}

xiiEnum<xiiMaterialAlphaMode> xiiMaterialResource::GetAlphaMode() const
{
  return GetRuntimeState().m_AlphaMode;
}

xiiBitflags<xiiMaterialFeatureFlags> xiiMaterialResource::GetFeatureFlags() const
{
  return GetRuntimeState().m_FeatureFlags;
}

const xiiMaterialRuntimeState& xiiMaterialResource::GetRuntimeState() const
{
  const_cast<xiiMaterialResource*>(this)->EnsureRuntimeData();
  return m_RuntimeState;
}

xiiUInt32 xiiMaterialResource::GetRuntimeHash() const
{
  return m_RuntimeState.m_uiRuntimeHash;
}

xiiUInt32 xiiMaterialResource::GetTextureMask() const
{
  return m_RuntimeState.m_uiTextureMask;
}

bool xiiMaterialResource::IsTranslucent() const
{
  return GetRuntimeState().IsTranslucent();
}

xiiSharedPtr<const xiiMaterialSchema> xiiMaterialResource::GetSchema()
{
  EnsureRuntimeData();
  XII_LOCK(m_RuntimeDataMutex);
  return m_pSchema;
}

xiiSharedPtr<xiiMaterialInstance> xiiMaterialResource::GetDefaultInstance()
{
  EnsureRuntimeData();
  XII_LOCK(m_RuntimeDataMutex);
  return m_pDefaultInstance;
}

xiiSharedPtr<xiiMaterialInstance> xiiMaterialResource::CreateInstance()
{
  EnsureRuntimeData();
  XII_LOCK(m_RuntimeDataMutex);
  if (m_pSchema == nullptr)
    return nullptr;

  xiiSharedPtr<xiiMaterialInstance> pInstance = XII_DEFAULT_NEW(xiiMaterialInstance);
  if (pInstance->Initialize(m_pSchema, m_RuntimeState).Failed())
    return nullptr;

  // Initialize already applied the immutable schema's defaults. Reapplying them
  // through SetParameter would incorrectly mark every default as an override.
  for (const auto& texture : m_ResolvedValues.m_Texture2DBindings)
    pInstance->SetTexture2D(xiiMaterialParameterId::Make(texture.Key().GetString()), texture.Value()).IgnoreResult();
  for (const auto& texture : m_ResolvedValues.m_TextureCubeBindings)
    pInstance->SetTextureCube(xiiMaterialParameterId::Make(texture.Key().GetString()), texture.Value()).IgnoreResult();

  return pInstance;
}

void xiiMaterialResource::SetParameter(const xiiHashedString& sName, const xiiVariant& value)
{
  xiiUInt32 uiIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < m_Description.m_Parameters.GetCount(); ++i)
  {
    if (m_Description.m_Parameters[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != xiiInvalidIndex)
    {
      if (m_Description.m_Parameters[uiIndex].m_Value == value)
      {
        return;
      }

      m_Description.m_Parameters[uiIndex].m_Value = value;
    }
    else
    {
      auto& param   = m_Description.m_Parameters.ExpandAndGetRef();
      param.m_Name  = sName;
      param.m_Value = value;
    }
  }
  else
  {
    if (uiIndex == xiiInvalidIndex)
    {
      return;
    }

    m_Description.m_Parameters.RemoveAtAndSwap(uiIndex);
  }

  m_Description.RecomputeRuntimeHash();
  UpdateRuntimeState();

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void xiiMaterialResource::SetParameter(xiiStringView sName, const xiiVariant& value)
{
  xiiTempHashedString sNameHash(sName);

  xiiUInt32 uiIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < m_Description.m_Parameters.GetCount(); ++i)
  {
    if (m_Description.m_Parameters[i].m_Name == sNameHash)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != xiiInvalidIndex)
    {
      if (m_Description.m_Parameters[uiIndex].m_Value == value)
      {
        return;
      }

      m_Description.m_Parameters[uiIndex].m_Value = value;
    }
    else
    {
      auto& param = m_Description.m_Parameters.ExpandAndGetRef();
      param.m_Name.Assign(sName);
      param.m_Value = value;
    }
  }
  else
  {
    if (uiIndex == xiiInvalidIndex)
    {
      return;
    }

    m_Description.m_Parameters.RemoveAtAndSwap(uiIndex);
  }

  m_Description.RecomputeRuntimeHash();
  UpdateRuntimeState();

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

xiiVariant xiiMaterialResource::GetParameter(const xiiTempHashedString& sName)
{
  EnsureRuntimeData();
  XII_LOCK(m_RuntimeDataMutex);
  xiiVariant value;
  m_ResolvedValues.m_Parameters.TryGetValue(sName, value);
  return value;
}

void xiiMaterialResource::SetTexture2DBinding(const xiiHashedString& sName, const xiiTexture2DResourceHandle& value)
{
  xiiUInt32 uiIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < m_Description.m_Texture2DBindings.GetCount(); ++i)
  {
    if (m_Description.m_Texture2DBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_Description.m_Texture2DBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding   = m_Description.m_Texture2DBindings.ExpandAndGetRef();
      binding.m_Name  = sName;
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_Description.m_Texture2DBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_Description.ApplyPbrParameterDefaults();
  UpdateRuntimeState();

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void xiiMaterialResource::SetTexture2DBinding(xiiStringView sName, const xiiTexture2DResourceHandle& value)
{
  xiiTempHashedString sNameHash(sName);

  xiiUInt32 uiIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < m_Description.m_Texture2DBindings.GetCount(); ++i)
  {
    if (m_Description.m_Texture2DBindings[i].m_Name == sNameHash)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_Description.m_Texture2DBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_Description.m_Texture2DBindings.ExpandAndGetRef();
      binding.m_Name.Assign(sName);
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_Description.m_Texture2DBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_Description.ApplyPbrParameterDefaults();
  UpdateRuntimeState();

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

xiiTexture2DResourceHandle xiiMaterialResource::GetTexture2DBinding(const xiiTempHashedString& sName)
{
  EnsureRuntimeData();
  XII_LOCK(m_RuntimeDataMutex);

  // Use pointer to prevent ref counting
  xiiTexture2DResourceHandle* pBinding;
  if (m_ResolvedValues.m_Texture2DBindings.TryGetValue(sName, pBinding))
  {
    return *pBinding;
  }

  return xiiTexture2DResourceHandle();
}


void xiiMaterialResource::SetTextureCubeBinding(const xiiHashedString& sName, const xiiTextureCubeResourceHandle& value)
{
  xiiUInt32 uiIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < m_Description.m_TextureCubeBindings.GetCount(); ++i)
  {
    if (m_Description.m_TextureCubeBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_Description.m_TextureCubeBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding   = m_Description.m_TextureCubeBindings.ExpandAndGetRef();
      binding.m_Name  = sName;
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_Description.m_TextureCubeBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_Description.RecomputeRuntimeHash();
  UpdateRuntimeState();

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void xiiMaterialResource::SetTextureCubeBinding(xiiStringView sName, const xiiTextureCubeResourceHandle& value)
{
  xiiTempHashedString sNameHash(sName);

  xiiUInt32 uiIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < m_Description.m_TextureCubeBindings.GetCount(); ++i)
  {
    if (m_Description.m_TextureCubeBindings[i].m_Name == sNameHash)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_Description.m_TextureCubeBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_Description.m_TextureCubeBindings.ExpandAndGetRef();
      binding.m_Name.Assign(sName);
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_Description.m_TextureCubeBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_Description.RecomputeRuntimeHash();
  UpdateRuntimeState();

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

xiiTextureCubeResourceHandle xiiMaterialResource::GetTextureCubeBinding(const xiiTempHashedString& sName)
{
  EnsureRuntimeData();
  XII_LOCK(m_RuntimeDataMutex);

  // Use pointer to prevent ref counting
  xiiTextureCubeResourceHandle* pBinding;
  if (m_ResolvedValues.m_TextureCubeBindings.TryGetValue(sName, pBinding))
  {
    return *pBinding;
  }

  return xiiTextureCubeResourceHandle();
}

void xiiMaterialResource::PreserveCurrentDescription()
{
  m_LoadingDescription = m_Description;
}

void xiiMaterialResource::ResetResource()
{
  if (m_Description != m_LoadingDescription)
  {
    m_Description = m_LoadingDescription;
    UpdateRuntimeState();

    m_iLastModified.Increment();

    m_ModifiedEvent.Broadcast(this);
  }
}

const xiiMaterialResourceDescriptor& xiiMaterialResource::GetCurrentDescription() const
{
  return m_Description;
}

xiiMaterialResourceHandle xiiMaterialResource::GetDefaultMaterial(DefaultMaterialType type)
{
  xiiMaterialResourceDescriptor description;
  xiiStringView                 name;
  switch (type)
  {
    case DefaultMaterialType::Missing:
      name                    = "Native/Materials/Missing";
      description.m_BaseColor = xiiColor(1, 0, 1);
      break;
    case DefaultMaterialType::Loading:
      name                    = "Native/Materials/Loading";
      description.m_BaseColor = xiiColor(0.25f, 0.25f, 0.25f);
      break;
    case DefaultMaterialType::Highlight:
      name                        = "Native/Materials/Highlight";
      description.m_BaseColor     = xiiColor::Black;
      description.m_EmissiveColor = xiiColor(1, 5, 5);
      description.m_FeatureFlags.Add(xiiMaterialFeatureFlags::TwoSided);
      break;
    case DefaultMaterialType::Preview:
      name                    = "Native/Materials/Preview";
      description.m_BaseColor = xiiColor(0.4f, 0.4f, 0.4f);
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return {};
  }
  return xiiResourceManager::GetOrCreateResource<xiiMaterialResource>(name, std::move(description), name);
}

xiiResourceLoadDescription xiiMaterialResource::UnloadData(Unload WhatToUnload)
{
  if (m_Description.m_hBaseMaterial.IsValid())
  {
    xiiResourceLock<xiiMaterialResource> pBaseMaterial(m_Description.m_hBaseMaterial, xiiResourceAcquireMode::PointerOnly);

    auto d = xiiMakeDelegate(&xiiMaterialResource::OnBaseMaterialModified, this);
    if (pBaseMaterial->m_ModifiedEvent.HasEventHandler(d))
    {
      pBaseMaterial->m_ModifiedEvent.RemoveEventHandler(d);
    }
  }

  m_Description.Clear();
  m_LoadingDescription.Clear();
  m_RuntimeState = xiiMaterialRuntimeState();
  {
    XII_LOCK(m_RuntimeDataMutex);
    m_ResolvedValues.Clear();
    m_pSchema.Clear();
    m_pDefaultInstance.Clear();
    m_iLastUpdated = m_iLastModified;
  }

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDescription xiiMaterialResource::UpdateContent(xiiStreamReader* pOuterStream)
{
  m_Description.Clear();
  m_LoadingDescription.Clear();
  m_RuntimeState = xiiMaterialRuntimeState();
  {
    XII_LOCK(m_RuntimeDataMutex);
    m_ResolvedValues.Clear();
    m_pSchema.Clear();
    m_pDefaultInstance.Clear();
  }

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  if (pOuterStream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  xiiStringBuilder sAbsFilePath;
  (*pOuterStream) >> sAbsFilePath;

  xiiAssetFileHeader header;
  xiiUInt8           version = 0;
  if (!sAbsFilePath.HasExtension("xiiBinMaterial") || header.Read(*pOuterStream).Failed() || pOuterStream->ReadBytes(&version, 1) != 1 || version != 9)
  {
    xiiLog::Error("Material '{}' requires current native material content. Re-transform the asset.", sAbsFilePath);
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }
  xiiStringBuilder sTemp, sTemp2;
  auto&            s = *pOuterStream;
  // Base material
  {
    s >> sTemp;

    if (!sTemp.IsEmpty())
      m_Description.m_hBaseMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>(sTemp);
  }

  // Surface
  {
    s >> sTemp;
    m_Description.m_sSurface.Assign(sTemp.GetView());
  }

  // Shader
  {
    s >> sTemp;

    if (!sTemp.IsEmpty())
      m_Description.m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>(sTemp);
  }

  // Permutation Variables
  {
    xiiUInt16 uiPermVars;
    s >> uiPermVars;

    m_Description.m_PermutationVariables.Reserve(uiPermVars);

    for (xiiUInt16 i = 0; i < uiPermVars; ++i)
    {
      s >> sTemp;
      s >> sTemp2;

      if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
      {
        AddPermutationVariable(sTemp, sTemp2);
      }
    }
  }

  // 2D Textures
  {
    xiiUInt16 uiTextures = 0;
    s >> uiTextures;

    m_Description.m_Texture2DBindings.Reserve(uiTextures);

    for (xiiUInt16 i = 0; i < uiTextures; ++i)
    {
      s >> sTemp;
      s >> sTemp2;

      if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
      {
        xiiMaterialResourceDescriptor::Texture2DBinding& tc = m_Description.m_Texture2DBindings.ExpandAndGetRef();
        tc.m_Name.Assign(sTemp.GetData());
        tc.m_Value = xiiResourceManager::LoadResource<xiiTexture2DResource>(sTemp2);
      }
    }
  }

  // Cube Textures
  {
    xiiUInt16 uiTextures = 0;
    s >> uiTextures;

    m_Description.m_TextureCubeBindings.Reserve(uiTextures);

    for (xiiUInt16 i = 0; i < uiTextures; ++i)
    {
      s >> sTemp;
      s >> sTemp2;

      if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
      {
        xiiMaterialResourceDescriptor::TextureCubeBinding& tc = m_Description.m_TextureCubeBindings.ExpandAndGetRef();
        tc.m_Name.Assign(sTemp.GetView());
        tc.m_Value = xiiResourceManager::LoadResource<xiiTextureCubeResource>(sTemp2);
      }
    }
  }

  // Shader constants
  {
    xiiUInt16 uiConstants = 0;
    s >> uiConstants;

    m_Description.m_Parameters.Reserve(uiConstants);

    xiiVariant vTemp;

    for (xiiUInt16 i = 0; i < uiConstants; ++i)
    {
      s >> sTemp;
      s >> vTemp;

      if (!sTemp.IsEmpty() && vTemp.IsValid())
      {
        xiiMaterialResourceDescriptor::Parameter& tc = m_Description.m_Parameters.ExpandAndGetRef();
        tc.m_Name.Assign(sTemp.GetData());
        tc.m_Value = vTemp;
      }
    }
  }

  if (xiiMaterialAssetFile::ReadPbrBlock(s, m_Description).Failed())
  {
    m_Description.Clear();
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  m_Description.ApplyPbrParameterDefaults();
  UpdateRuntimeState();

  if (m_Description.m_hBaseMaterial.IsValid())
  {
    // Block till the base material has been fully loaded to ensure that all parameters have their final value once this material is loaded.
    xiiResourceLock<xiiMaterialResource> pBaseMaterial(m_Description.m_hBaseMaterial, xiiResourceAcquireMode::BlockTillLoaded);

    if (!pBaseMaterial->m_ModifiedEvent.HasEventHandler(xiiMakeDelegate(&xiiMaterialResource::OnBaseMaterialModified, this)))
    {
      pBaseMaterial->m_ModifiedEvent.AddEventHandler(xiiMakeDelegate(&xiiMaterialResource::OnBaseMaterialModified, this));
    }
  }

  m_LoadingDescription = m_Description;

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);

  return res;
}

void xiiMaterialResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiMaterialResource) + (xiiUInt32)(m_Description.m_PermutationVariables.GetHeapMemoryUsage() + m_Description.m_Parameters.GetHeapMemoryUsage() + m_Description.m_Texture2DBindings.GetHeapMemoryUsage() + m_Description.m_TextureCubeBindings.GetHeapMemoryUsage() + m_LoadingDescription.m_PermutationVariables.GetHeapMemoryUsage() + m_LoadingDescription.m_Parameters.GetHeapMemoryUsage() + m_LoadingDescription.m_Texture2DBindings.GetHeapMemoryUsage() + m_LoadingDescription.m_TextureCubeBindings.GetHeapMemoryUsage());

  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiMaterialResource, xiiMaterialResourceDescriptor)
{
  descriptor.ApplyPbrParameterDefaults();

  m_Description        = descriptor;
  m_LoadingDescription = descriptor;
  UpdateRuntimeState();

  xiiResourceLoadDescription res;
  res.m_State                      = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (m_Description.m_hBaseMaterial.IsValid())
  {
    // Can't block here for the base material since this would result in a deadlock
    xiiResourceLock<xiiMaterialResource> pBaseMaterial(m_Description.m_hBaseMaterial, xiiResourceAcquireMode::PointerOnly);
    pBaseMaterial->m_ModifiedEvent.AddEventHandler(xiiMakeDelegate(&xiiMaterialResource::OnBaseMaterialModified, this));
  }

  m_iLastModified.Increment();

  return res;
}

void xiiMaterialResource::OnBaseMaterialModified(const xiiMaterialResource* pModifiedMaterial)
{
  XII_ASSERT_DEV(m_Description.m_hBaseMaterial == pModifiedMaterial, "Implementation error");

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void xiiMaterialResource::OnResourceEvent(const xiiResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != xiiResourceEvent::Type::ResourceContentUpdated)
    return;

  bool bUsesShader = false;
  {
    XII_LOCK(m_RuntimeDataMutex);
    bUsesShader = m_ResolvedValues.m_hShader.IsValid() && m_ResolvedValues.m_hShader == resourceEvent.m_pResource;
  }

  if (bUsesShader)
  {
    m_iLastModified.Increment();
    m_ModifiedEvent.Broadcast(this);
  }
}

void xiiMaterialResource::AddPermutationVariable(xiiStringView sName, xiiStringView sValue)
{
  xiiHashedString sNameHashed;
  sNameHashed.Assign(sName);
  xiiHashedString sValueHashed;
  sValueHashed.Assign(sValue);

  if (xiiGALShaderManager::IsPermutationValueAllowed(sNameHashed, sValueHashed))
  {
    xiiGALPermutationVariable& permutationVariable = m_Description.m_PermutationVariables.ExpandAndGetRef();
    permutationVariable.m_sName                    = sNameHashed;
    permutationVariable.m_sValue                   = sValueHashed;
  }
}

void xiiMaterialResource::UpdateRuntimeState()
{
  m_RuntimeState = m_Description.BuildRuntimeState();
}

bool xiiMaterialResource::IsModified()
{
  return m_iLastModified != m_iLastUpdated;
}

void xiiMaterialResource::EnsureRuntimeData()
{
  if (!IsModified())
    return;

  xiiHybridArray<xiiMaterialResource*, 16> materialHierarchy;
  xiiMaterialResource*                     pCurrentMaterial = this;

  while (true)
  {
    if (materialHierarchy.Contains(pCurrentMaterial))
    {
      xiiLog::Error("Cyclic material inheritance detected while resolving '{}'.", GetResourceID());
      break;
    }

    materialHierarchy.PushBack(pCurrentMaterial);

    const xiiMaterialResourceHandle& hBaseMaterial = pCurrentMaterial->m_Description.m_hBaseMaterial;
    if (!hBaseMaterial.IsValid())
      break;

    // Ensure that the base material is loaded at this point.
    // For loaded materials this will always be the case but is still necessary for runtime created materials.
    pCurrentMaterial = xiiResourceManager::BeginAcquireResource(hBaseMaterial, xiiResourceAcquireMode::BlockTillLoaded);
  }

  XII_SCOPE_EXIT(for (xiiUInt32 i = materialHierarchy.GetCount(); i-- > 1;) {
    xiiMaterialResource* pMaterial = materialHierarchy[i];
    xiiResourceManager::EndAcquireResource(pMaterial);

    materialHierarchy[i] = nullptr;
  });

  // Read source configuration before taking the material mutex: loading the
  // shader can broadcast resource events back to this material.
  xiiHybridArray<xiiGALPermutationVariable, 16> fixedPermutations;
  for (const auto* material : materialHierarchy)
    if (material->m_Description.m_hShader.IsValid())
    {
      xiiResourceLock<xiiShaderResource> shader(material->m_Description.m_hShader, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (shader.IsValid())
        fixedPermutations = shader->GetFixedPermutationVariables();
      break;
    }

  XII_LOCK(m_RuntimeDataMutex);

  if (!IsModified())
    return;

  m_ResolvedValues.Clear();
  m_pSchema.Clear();
  m_pDefaultInstance.Clear();

  // set state of parent material first
  for (xiiUInt32 i = materialHierarchy.GetCount(); i-- > 0;)
  {
    xiiMaterialResource*                 pMaterial   = materialHierarchy[i];
    const xiiMaterialResourceDescriptor& description = pMaterial->m_Description;

    if (description.m_hShader.IsValid())
    {
      m_ResolvedValues.m_hShader = description.m_hShader;
    }

    for (const auto& permutationVar : description.m_PermutationVariables)
    {
      m_ResolvedValues.m_PermutationVariables.Insert(permutationVar.m_sName, permutationVar.m_sValue);
    }

    for (const auto& param : description.m_Parameters)
    {
      m_ResolvedValues.m_Parameters.Insert(param.m_Name, param.m_Value);
    }

    for (const auto& textureBinding : description.m_Texture2DBindings)
    {
      m_ResolvedValues.m_Texture2DBindings.Insert(textureBinding.m_Name, textureBinding.m_Value);
    }

    for (const auto& textureBinding : description.m_TextureCubeBindings)
    {
      m_ResolvedValues.m_TextureCubeBindings.Insert(textureBinding.m_Name, textureBinding.m_Value);
    }
  }

  xiiMaterialSchemaDescription schemaDescription;
  for (const auto& fixed : fixedPermutations)
    m_ResolvedValues.m_PermutationVariables.Insert(fixed.m_sName, fixed.m_sValue);
  auto resolvedDescription = m_Description;
  resolvedDescription.m_PermutationVariables.Clear();
  for (const auto& permutation : m_ResolvedValues.m_PermutationVariables)
  {
    auto& variable    = resolvedDescription.m_PermutationVariables.ExpandAndGetRef();
    variable.m_sName  = permutation.Key();
    variable.m_sValue = permutation.Value();
  }
  m_RuntimeState            = resolvedDescription.BuildRuntimeState();
  schemaDescription.m_sName = GetResourceID();
  if (schemaDescription.m_sName.IsEmpty())
    schemaDescription.m_sName = "Runtime Material";
  schemaDescription.m_hShader      = m_ResolvedValues.m_hShader;
  for (const auto& permutation : m_ResolvedValues.m_PermutationVariables)
  {
    schemaDescription.m_Permutations.Insert(permutation.Key().GetString(), permutation.Value().GetString());
  }
  schemaDescription.m_Domain       = m_Description.m_Domain;
  schemaDescription.m_ShadingModel = m_RuntimeState.m_ShadingModel;

  xiiDynamicArray<xiiHashedString> parameterNames;
  for (const auto& parameter : m_ResolvedValues.m_Parameters)
    parameterNames.PushBack(parameter.Key());
  parameterNames.Sort([](const xiiHashedString& lhs, const xiiHashedString& rhs) { return lhs.GetString().Compare(rhs.GetString()) < 0; });

  for (const xiiHashedString& name : parameterNames)
  {
    const xiiVariant*              pValue = m_ResolvedValues.m_Parameters.GetValue(name);
    xiiMaterialParameterType::Enum type;
    if (pValue == nullptr || !TryGetMaterialParameterType(*pValue, type))
    {
      xiiLog::Warning("Material '{}' parameter '{}' uses an unsupported runtime type and was omitted from its GPU schema.", GetResourceID(), name);
      continue;
    }

    schemaDescription.AddParameter(name.GetString(), type, *pValue);
  }

  xiiDynamicArray<xiiHashedString> texture2DNames;
  for (const auto& texture : m_ResolvedValues.m_Texture2DBindings)
    texture2DNames.PushBack(texture.Key());
  texture2DNames.Sort([](const xiiHashedString& lhs, const xiiHashedString& rhs) { return lhs.GetString().Compare(rhs.GetString()) < 0; });
  for (const xiiHashedString& name : texture2DNames)
    schemaDescription.AddTexture(name.GetString(), xiiGALShaderTextureType::Texture2D);

  xiiDynamicArray<xiiHashedString> textureCubeNames;
  for (const auto& texture : m_ResolvedValues.m_TextureCubeBindings)
    textureCubeNames.PushBack(texture.Key());
  textureCubeNames.Sort([](const xiiHashedString& lhs, const xiiHashedString& rhs) { return lhs.GetString().Compare(rhs.GetString()) < 0; });
  for (const xiiHashedString& name : textureCubeNames)
    schemaDescription.AddTexture(name.GetString(), xiiGALShaderTextureType::TextureCube);

  xiiSharedPtr<xiiMaterialSchema> pSchema = XII_DEFAULT_NEW(xiiMaterialSchema);
  xiiStringBuilder                schemaError;
  if (pSchema->Build(schemaDescription, &schemaError).Failed())
  {
    xiiLog::Error("Failed to build runtime schema for material '{}': {}", GetResourceID(), schemaError);
    m_iLastUpdated = m_iLastModified;
    return;
  }

  xiiSharedPtr<xiiMaterialInstance> pDefaultInstance = XII_DEFAULT_NEW(xiiMaterialInstance);
  m_RuntimeState.m_uiLayoutHash                      = pSchema->GetLayoutHash();
  if (pDefaultInstance->Initialize(pSchema, m_RuntimeState).Failed())
  {
    xiiLog::Error("Failed to initialize runtime instance for material '{}'.", GetResourceID());
    m_iLastUpdated = m_iLastModified;
    return;
  }

  for (const auto& texture : m_ResolvedValues.m_Texture2DBindings)
    pDefaultInstance->SetTexture2D(xiiMaterialParameterId::Make(texture.Key().GetString()), texture.Value()).IgnoreResult();
  for (const auto& texture : m_ResolvedValues.m_TextureCubeBindings)
    pDefaultInstance->SetTextureCube(xiiMaterialParameterId::Make(texture.Key().GetString()), texture.Value()).IgnoreResult();

  m_pSchema          = std::move(pSchema);
  m_pDefaultInstance = std::move(pDefaultInstance);
  m_iLastUpdated     = m_iLastModified;
}

void xiiMaterialResource::ResolvedValues::Clear()
{
  m_hShader.Invalidate();
  m_PermutationVariables.Clear();
  m_Parameters.Clear();
  m_Texture2DBindings.Clear();
  m_TextureCubeBindings.Clear();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Material_Implementation_MaterialResource);
