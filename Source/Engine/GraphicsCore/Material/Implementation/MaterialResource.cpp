/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsCore/Textures/TextureLoader.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <Texture/Image/Formats/DdsFileFormat.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

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

  static bool ReadFloatChild(const xiiOpenDdlReaderElement& block, xiiStringView sName, float& ref_fValue)
  {
    if (const xiiOpenDdlReaderElement* pValue = block.FindChildOfType(xiiOpenDdlPrimitiveType::Float, sName))
    {
      ref_fValue = pValue->GetPrimitivesFloat()[0];
      return true;
    }

    if (const xiiOpenDdlReaderElement* pValue = block.FindChildOfType(xiiOpenDdlPrimitiveType::Double, sName))
    {
      ref_fValue = static_cast<float>(pValue->GetPrimitivesDouble()[0]);
      return true;
    }

    return false;
  }

  static bool ReadBoolChild(const xiiOpenDdlReaderElement& block, xiiStringView sName, bool& ref_bValue)
  {
    if (const xiiOpenDdlReaderElement* pValue = block.FindChildOfType(xiiOpenDdlPrimitiveType::Bool, sName))
    {
      ref_bValue = pValue->GetPrimitivesBool()[0];
      return true;
    }

    return false;
  }

  static bool ReadInt16Child(const xiiOpenDdlReaderElement& block, xiiStringView sName, xiiInt16& ref_iValue)
  {
    if (const xiiOpenDdlReaderElement* pValue = block.FindChildOfType(xiiOpenDdlPrimitiveType::Int16, sName))
    {
      ref_iValue = pValue->GetPrimitivesInt16()[0];
      return true;
    }

    if (const xiiOpenDdlReaderElement* pValue = block.FindChildOfType(xiiOpenDdlPrimitiveType::Int32, sName))
    {
      ref_iValue = static_cast<xiiInt16>(pValue->GetPrimitivesInt32()[0]);
      return true;
    }

    return false;
  }

  static bool ReadStringChild(const xiiOpenDdlReaderElement& block, xiiStringView sName, xiiStringView& ref_sValue)
  {
    if (const xiiOpenDdlReaderElement* pValue = block.FindChildOfType(xiiOpenDdlPrimitiveType::String, sName))
    {
      ref_sValue = pValue->GetPrimitivesString()[0];
      return true;
    }

    return false;
  }

  static bool TryParseShadingModel(xiiStringView sValue, xiiEnum<xiiMaterialShadingModel>& ref_value)
  {
    if (sValue.IsEqual_NoCase("Lit"))
      ref_value = xiiMaterialShadingModel::Lit;
    else if (sValue.IsEqual_NoCase("Subsurface"))
      ref_value = xiiMaterialShadingModel::Subsurface;
    else if (sValue.IsEqual_NoCase("ClearCoat"))
      ref_value = xiiMaterialShadingModel::ClearCoat;
    else if (sValue.IsEqual_NoCase("Cloth"))
      ref_value = xiiMaterialShadingModel::Cloth;
    else if (sValue.IsEqual_NoCase("Hair"))
      ref_value = xiiMaterialShadingModel::Hair;
    else if (sValue.IsEqual_NoCase("Eye"))
      ref_value = xiiMaterialShadingModel::Eye;
    else if (sValue.IsEqual_NoCase("Unlit"))
      ref_value = xiiMaterialShadingModel::Unlit;
    else if (sValue.IsEqual_NoCase("ParticipatingMedia"))
      ref_value = xiiMaterialShadingModel::ParticipatingMedia;
    else if (sValue.IsEqual_NoCase("XRayAttenuation"))
      ref_value = xiiMaterialShadingModel::XRayAttenuation;
    else if (sValue.IsEqual_NoCase("SensorResponse"))
      ref_value = xiiMaterialShadingModel::SensorResponse;
    else if (sValue.IsEqual_NoCase("Custom"))
      ref_value = xiiMaterialShadingModel::Custom;
    else
      return false;

    return true;
  }

  static bool TryParseMaterialDomain(xiiStringView sValue, xiiEnum<xiiMaterialDomain>& ref_value)
  {
    if (sValue.IsEqual_NoCase("Surface"))
      ref_value = xiiMaterialDomain::Surface;
    else if (sValue.IsEqual_NoCase("Decal"))
      ref_value = xiiMaterialDomain::Decal;
    else if (sValue.IsEqual_NoCase("Volume"))
      ref_value = xiiMaterialDomain::Volume;
    else if (sValue.IsEqual_NoCase("PostProcess"))
      ref_value = xiiMaterialDomain::PostProcess;
    else if (sValue.IsEqual_NoCase("Sensor"))
      ref_value = xiiMaterialDomain::Sensor;
    else if (sValue.IsEqual_NoCase("Compute"))
      ref_value = xiiMaterialDomain::Compute;
    else
      return false;

    return true;
  }

  static bool TryGetMaterialParameterType(const xiiVariant& value, xiiMaterialParameterType::Enum& out_type)
  {
    switch (value.GetType())
    {
      case xiiVariantType::Bool: out_type = xiiMaterialParameterType::Bool; return true;
      case xiiVariantType::Int8:
      case xiiVariantType::Int16:
      case xiiVariantType::Int32:
      case xiiVariantType::Int64: out_type = xiiMaterialParameterType::Int; return true;
      case xiiVariantType::UInt8:
      case xiiVariantType::UInt16:
      case xiiVariantType::UInt32:
      case xiiVariantType::UInt64: out_type = xiiMaterialParameterType::UInt; return true;
      case xiiVariantType::Float:
      case xiiVariantType::Double: out_type = xiiMaterialParameterType::Float; return true;
      case xiiVariantType::Vector2: out_type = xiiMaterialParameterType::Float2; return true;
      case xiiVariantType::Vector3: out_type = xiiMaterialParameterType::Float3; return true;
      case xiiVariantType::Vector4: out_type = xiiMaterialParameterType::Float4; return true;
      case xiiVariantType::Color:
      case xiiVariantType::ColorGamma: out_type = xiiMaterialParameterType::Color; return true;
      case xiiVariantType::Matrix3: out_type = xiiMaterialParameterType::Matrix3; return true;
      case xiiVariantType::Matrix4: out_type = xiiMaterialParameterType::Matrix4; return true;
      default: return false;
    }
  }

  static bool TryParseBlendMode(xiiStringView sValue, xiiEnum<xiiMaterialBlendMode>& ref_value)
  {
    if (sValue.IsEqual_NoCase("Opaque"))
      ref_value = xiiMaterialBlendMode::Opaque;
    else if (sValue.IsEqual_NoCase("Masked"))
      ref_value = xiiMaterialBlendMode::Masked;
    else if (sValue.IsEqual_NoCase("Translucent"))
      ref_value = xiiMaterialBlendMode::Translucent;
    else if (sValue.IsEqual_NoCase("Additive"))
      ref_value = xiiMaterialBlendMode::Additive;
    else if (sValue.IsEqual_NoCase("Modulate"))
      ref_value = xiiMaterialBlendMode::Modulate;
    else
      return false;

    return true;
  }

  static bool TryParseAlphaMode(xiiStringView sValue, xiiEnum<xiiMaterialAlphaMode>& ref_value)
  {
    if (sValue.IsEqual_NoCase("Opaque"))
      ref_value = xiiMaterialAlphaMode::Opaque;
    else if (sValue.IsEqual_NoCase("Mask") || sValue.IsEqual_NoCase("Masked"))
      ref_value = xiiMaterialAlphaMode::Mask;
    else if (sValue.IsEqual_NoCase("Blend") || sValue.IsEqual_NoCase("Translucent"))
      ref_value = xiiMaterialAlphaMode::Blend;
    else
      return false;

    return true;
  }

  static bool TryAddMaterialFeature(xiiStringView sValue, xiiBitflags<xiiMaterialFeatureFlags>& ref_flags)
  {
    if (sValue.IsEqual_NoCase("NormalTexture"))
      ref_flags.Add(xiiMaterialFeatureFlags::NormalTexture);
    else if (sValue.IsEqual_NoCase("MetallicRoughnessTexture"))
      ref_flags.Add(xiiMaterialFeatureFlags::MetallicRoughnessTexture);
    else if (sValue.IsEqual_NoCase("OcclusionTexture"))
      ref_flags.Add(xiiMaterialFeatureFlags::OcclusionTexture);
    else if (sValue.IsEqual_NoCase("EmissiveTexture"))
      ref_flags.Add(xiiMaterialFeatureFlags::EmissiveTexture);
    else if (sValue.IsEqual_NoCase("HeightTexture"))
      ref_flags.Add(xiiMaterialFeatureFlags::HeightTexture);
    else if (sValue.IsEqual_NoCase("ClearCoat"))
      ref_flags.Add(xiiMaterialFeatureFlags::ClearCoat);
    else if (sValue.IsEqual_NoCase("Transmission"))
      ref_flags.Add(xiiMaterialFeatureFlags::Transmission);
    else if (sValue.IsEqual_NoCase("Sheen"))
      ref_flags.Add(xiiMaterialFeatureFlags::Sheen);
    else if (sValue.IsEqual_NoCase("Anisotropy"))
      ref_flags.Add(xiiMaterialFeatureFlags::Anisotropy);
    else if (sValue.IsEqual_NoCase("VertexColor"))
      ref_flags.Add(xiiMaterialFeatureFlags::VertexColor);
    else if (sValue.IsEqual_NoCase("TwoSided"))
      ref_flags.Add(xiiMaterialFeatureFlags::TwoSided);
    else if (sValue.IsEqual_NoCase("RuntimeGenerated"))
      ref_flags.Add(xiiMaterialFeatureFlags::RuntimeGenerated);
    else if (sValue.IsEqual_NoCase("ReceivesLighting"))
      ref_flags.Add(xiiMaterialFeatureFlags::ReceivesLighting);
    else if (sValue.IsEqual_NoCase("CastsShadows"))
      ref_flags.Add(xiiMaterialFeatureFlags::CastsShadows);
    else if (sValue.IsEqual_NoCase("WritesVelocity"))
      ref_flags.Add(xiiMaterialFeatureFlags::WritesVelocity);
    else if (sValue.IsEqual_NoCase("UsesBindlessResources"))
      ref_flags.Add(xiiMaterialFeatureFlags::UsesBindlessResources);
    else
      return false;

    return true;
  }

  static void ReadPbrTextureChild(const xiiOpenDdlReaderElement& block, xiiStringView sName, xiiTexture2DResourceHandle& ref_hTexture)
  {
    xiiStringView sTexture;
    if (ReadStringChild(block, sName, sTexture) && !sTexture.IsEmpty())
    {
      ref_hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(sTexture);
    }
  }

  static void ReadPbrMaterialBlock(const xiiOpenDdlReaderElement& block, xiiMaterialResourceDescriptor& ref_desc)
  {
    xiiStringView sValue;
    if (ReadStringChild(block, "Domain", sValue))
      TryParseMaterialDomain(sValue, ref_desc.m_Domain);
    if (ReadStringChild(block, "ShadingModel", sValue))
      TryParseShadingModel(sValue, ref_desc.m_ShadingModel);
    if (ReadStringChild(block, "BlendMode", sValue))
      TryParseBlendMode(sValue, ref_desc.m_BlendMode);
    if (ReadStringChild(block, "AlphaMode", sValue))
      TryParseAlphaMode(sValue, ref_desc.m_AlphaMode);

    if (const xiiOpenDdlReaderElement* pBaseColor = block.FindChild("BaseColor"))
      xiiOpenDdlUtils::ConvertToColor(pBaseColor, ref_desc.m_BaseColor).IgnoreResult();
    if (const xiiOpenDdlReaderElement* pEmissiveColor = block.FindChild("EmissiveColor"))
      xiiOpenDdlUtils::ConvertToColor(pEmissiveColor, ref_desc.m_EmissiveColor).IgnoreResult();

    ReadFloatChild(block, "Metallic", ref_desc.m_fMetallic);
    ReadFloatChild(block, "Roughness", ref_desc.m_fRoughness);
    ReadFloatChild(block, "OcclusionStrength", ref_desc.m_fOcclusionStrength);
    ReadFloatChild(block, "AlphaCutoff", ref_desc.m_fAlphaCutoff);
    ReadFloatChild(block, "NormalScale", ref_desc.m_fNormalScale);
    ReadFloatChild(block, "DisplacementScale", ref_desc.m_fDisplacementScale);
    ReadFloatChild(block, "ClearCoat", ref_desc.m_fClearCoat);
    ReadFloatChild(block, "ClearCoatRoughness", ref_desc.m_fClearCoatRoughness);
    ReadFloatChild(block, "Transmission", ref_desc.m_fTransmission);
    ReadFloatChild(block, "Thickness", ref_desc.m_fThickness);
    ReadFloatChild(block, "IndexOfRefraction", ref_desc.m_fIndexOfRefraction);
    ReadFloatChild(block, "Anisotropy", ref_desc.m_fAnisotropy);
    ReadFloatChild(block, "SheenRoughness", ref_desc.m_fSheenRoughness);
    ReadInt16Child(block, "SortPriority", ref_desc.m_iSortPriority);

    bool bFlag = false;
    if (ReadBoolChild(block, "TwoSided", bFlag))
      ref_desc.m_FeatureFlags.AddOrRemove(xiiMaterialFeatureFlags::TwoSided, bFlag);
    if (ReadBoolChild(block, "VertexColor", bFlag))
      ref_desc.m_FeatureFlags.AddOrRemove(xiiMaterialFeatureFlags::VertexColor, bFlag);

    if (const xiiOpenDdlReaderElement* pFeatures = block.FindChildOfType(xiiOpenDdlPrimitiveType::String, "Features"))
    {
      for (xiiUInt32 i = 0; i < pFeatures->GetNumPrimitives(); ++i)
      {
        TryAddMaterialFeature(pFeatures->GetPrimitivesString()[i], ref_desc.m_FeatureFlags);
      }
    }

    ReadPbrTextureChild(block, "BaseColorTexture", ref_desc.m_hBaseColorTexture);
    ReadPbrTextureChild(block, "NormalTexture", ref_desc.m_hNormalTexture);
    ReadPbrTextureChild(block, "MetallicRoughnessTexture", ref_desc.m_hMetallicRoughnessTexture);
    ReadPbrTextureChild(block, "OcclusionTexture", ref_desc.m_hOcclusionTexture);
    ReadPbrTextureChild(block, "EmissiveTexture", ref_desc.m_hEmissiveTexture);
    ReadPbrTextureChild(block, "HeightTexture", ref_desc.m_hHeightTexture);
    ReadPbrTextureChild(block, "ClearCoatTexture", ref_desc.m_hClearCoatTexture);
    ReadPbrTextureChild(block, "TransmissionTexture", ref_desc.m_hTransmissionTexture);
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
    XII_RESOURCE_MEMBER_PROPERTY("Texture", m_Value),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialResourceDescriptor::TextureCubeBinding, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialResourceDescriptor::TextureCubeBinding>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_Name),
    XII_RESOURCE_MEMBER_PROPERTY("Texture", m_Value),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialResourceDescriptor, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiMaterialResourceDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_MEMBER_PROPERTY("BaseMaterial", m_hBaseMaterial),
    XII_MEMBER_PROPERTY("Surface", m_sSurface),
    XII_RESOURCE_MEMBER_PROPERTY("Shader", m_hShader),
    XII_ENUM_MEMBER_PROPERTY("Domain", xiiMaterialDomain, m_Domain),
    XII_ENUM_MEMBER_PROPERTY("ShadingModel", xiiMaterialShadingModel, m_ShadingModel),
    XII_ENUM_MEMBER_PROPERTY("BlendMode", xiiMaterialBlendMode, m_BlendMode),
    XII_ENUM_MEMBER_PROPERTY("AlphaMode", xiiMaterialAlphaMode, m_AlphaMode),
    XII_BITFLAGS_MEMBER_PROPERTY("Features", xiiMaterialFeatureFlags, m_FeatureFlags),
    XII_MEMBER_PROPERTY("BaseColor", m_BaseColor),
    XII_MEMBER_PROPERTY("EmissiveColor", m_EmissiveColor),
    XII_MEMBER_PROPERTY("Metallic", m_fMetallic),
    XII_MEMBER_PROPERTY("Roughness", m_fRoughness),
    XII_MEMBER_PROPERTY("OcclusionStrength", m_fOcclusionStrength),
    XII_MEMBER_PROPERTY("AlphaCutoff", m_fAlphaCutoff),
    XII_MEMBER_PROPERTY("NormalScale", m_fNormalScale),
    XII_MEMBER_PROPERTY("DisplacementScale", m_fDisplacementScale),
    XII_MEMBER_PROPERTY("ClearCoat", m_fClearCoat),
    XII_MEMBER_PROPERTY("ClearCoatRoughness", m_fClearCoatRoughness),
    XII_MEMBER_PROPERTY("Transmission", m_fTransmission),
    XII_MEMBER_PROPERTY("Thickness", m_fThickness),
    XII_MEMBER_PROPERTY("IndexOfRefraction", m_fIndexOfRefraction),
    XII_MEMBER_PROPERTY("Anisotropy", m_fAnisotropy),
    XII_MEMBER_PROPERTY("SheenRoughness", m_fSheenRoughness),
    XII_MEMBER_PROPERTY("SortPriority", m_iSortPriority),
    XII_RESOURCE_MEMBER_PROPERTY("BaseColorTexture", m_hBaseColorTexture),
    XII_RESOURCE_MEMBER_PROPERTY("NormalTexture", m_hNormalTexture),
    XII_RESOURCE_MEMBER_PROPERTY("MetallicRoughnessTexture", m_hMetallicRoughnessTexture),
    XII_RESOURCE_MEMBER_PROPERTY("OcclusionTexture", m_hOcclusionTexture),
    XII_RESOURCE_MEMBER_PROPERTY("EmissiveTexture", m_hEmissiveTexture),
    XII_RESOURCE_MEMBER_PROPERTY("HeightTexture", m_hHeightTexture),
    XII_RESOURCE_MEMBER_PROPERTY("ClearCoatTexture", m_hClearCoatTexture),
    XII_RESOURCE_MEMBER_PROPERTY("TransmissionTexture", m_hTransmissionTexture),
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
  return m_Description.m_ShadingModel;
}

xiiEnum<xiiMaterialBlendMode> xiiMaterialResource::GetBlendMode() const
{
  return m_Description.m_BlendMode;
}

xiiEnum<xiiMaterialAlphaMode> xiiMaterialResource::GetAlphaMode() const
{
  return m_Description.m_AlphaMode;
}

xiiBitflags<xiiMaterialFeatureFlags> xiiMaterialResource::GetFeatureFlags() const
{
  return m_Description.m_FeatureFlags;
}

const xiiMaterialRuntimeState& xiiMaterialResource::GetRuntimeState() const
{
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
  return m_RuntimeState.IsTranslucent();
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

  for (const auto& parameter : m_ResolvedValues.m_Parameters)
    pInstance->SetParameter(xiiMaterialParameterId::Make(parameter.Key().GetString()), parameter.Value()).IgnoreResult();
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

xiiStringView xiiMaterialResource::GetDefaultMaterialFileName(DefaultMaterialType materialType)
{
  switch (materialType)
  {
    case DefaultMaterialType::Fullbright:
      return "Base/Materials/BaseMaterials/Fullbright.xiiMaterialAsset";
    case DefaultMaterialType::FullbrightAlphaTest:
      return "Base/Materials/BaseMaterials/FullbrightAlphaTest.xiiMaterialAsset";
    case DefaultMaterialType::Lit:
      return "Base/Materials/BaseMaterials/Lit.xiiMaterialAsset";
    case DefaultMaterialType::LitAlphaTest:
      return "Base/Materials/BaseMaterials/LitAlphaTest.xiiMaterialAsset";
    case DefaultMaterialType::Sky:
      return "Base/Materials/BaseMaterials/Sky.xiiMaterialAsset";
    case DefaultMaterialType::MissingMaterial:
      return "Base/Materials/Common/MissingMaterial.xiiMaterialAsset";
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return "";
  }
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

  if (sAbsFilePath.HasExtension("xiiBinMaterial"))
  {
    xiiStringBuilder sTemp, sTemp2;

    xiiAssetFileHeader AssetHash;
    AssetHash.Read(*pOuterStream).IgnoreResult();

    xiiUInt8 uiVersion = 0;
    (*pOuterStream) >> uiVersion;
    XII_ASSERT_DEV(uiVersion >= 4 && uiVersion <= 7, "Unknown xiiBinMaterial version {0}", uiVersion);

    xiiUInt8 uiCompressionMode = 0;
    if (uiVersion >= 6)
    {
      *pOuterStream >> uiCompressionMode;
    }

    xiiStreamReader* pInnerStream = pOuterStream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    xiiCompressedStreamReaderZstd decompressorZstd;
#endif

    switch (uiCompressionMode)
    {
      case 0:
        break;

      case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        decompressorZstd.SetInputStream(pOuterStream);
        pInnerStream = &decompressorZstd;
        break;
#else
        xiiLog::Error("Material resource is compressed with zstandard, but support for this compressor is not compiled in.");
        res.m_State = xiiResourceState::LoadedResourceMissing;
        return res;
#endif

      default:
        xiiLog::Error("Material resource is compressed with an unknown algorithm.");
        res.m_State = xiiResourceState::LoadedResourceMissing;
        return res;
    }

    xiiStreamReader& s = *pInnerStream;

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

    // Reserved legacy field.
    if (uiVersion >= 7)
    {
      xiiStringBuilder sUnusedLegacyField;
      s >> sUnusedLegacyField;
    }

    if (uiVersion >= 5)
    {
      xiiStreamReader& s = *pInnerStream;

      xiiStringBuilder sResourceName;
      s >> sResourceName;

      xiiTextureResourceLoader::LoadedData embedded;

      while (!sResourceName.IsEmpty())
      {
        xiiUInt32 dataSize = 0;
        s >> dataSize;

        xiiTextureResourceLoader::LoadTexFile(s, embedded).IgnoreResult();
        embedded.m_bIsFallback = true;

        xiiDefaultMemoryStreamStorage storage;
        xiiMemoryStreamWriter         loadStreamWriter(&storage);
        xiiTextureResourceLoader::WriteTextureLoadStream(loadStreamWriter, embedded);

        xiiMemoryStreamReader loadStreamReader(&storage);

        xiiTexture2DResourceHandle hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(sResourceName);
        xiiResourceManager::SetResourceLowResData(hTexture, &loadStreamReader);

        s >> sResourceName;
      }
    }
  }
  else if (sAbsFilePath.HasExtension("xiiMaterial"))
  {
    xiiOpenDdlReader reader;

    if (reader.ParseDocument(*pOuterStream, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
    {
      res.m_State = xiiResourceState::LoadedResourceMissing;
      return res;
    }

    const xiiOpenDdlReaderElement* pRoot = reader.GetRootElement();

    // Read the base material
    if (const xiiOpenDdlReaderElement* pBase = pRoot->FindChildOfType(xiiOpenDdlPrimitiveType::String, "BaseMaterial"))
    {
      m_Description.m_hBaseMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>(pBase->GetPrimitivesString()[0]);
    }

    // Read the shader
    if (const xiiOpenDdlReaderElement* pShader = pRoot->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Shader"))
    {
      m_Description.m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>(pShader->GetPrimitivesString()[0]);
    }

    for (const xiiOpenDdlReaderElement* pChild = pRoot->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
    {
      // Read explicit PBR authoring state. Legacy Constant/Texture2D blocks below still override shader-facing bindings.
      if (pChild->IsCustomType("PBR") || pChild->IsCustomType("Pbr") || pChild->IsCustomType("PbrMaterial"))
      {
        ReadPbrMaterialBlock(*pChild, m_Description);
      }

      // Read the shader permutation variables
      if (pChild->IsCustomType("Permutation"))
      {
        const xiiOpenDdlReaderElement* pName  = pChild->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Variable");
        const xiiOpenDdlReaderElement* pValue = pChild->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          AddPermutationVariable(pName->GetPrimitivesString()[0], pValue->GetPrimitivesString()[0]);
        }
      }

      // Read the shader constants
      if (pChild->IsCustomType("Constant"))
      {
        const xiiOpenDdlReaderElement* pName  = pChild->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Variable");
        const xiiOpenDdlReaderElement* pValue = pChild->FindChild("Value");

        xiiVariant value;
        if (pName && pValue && xiiOpenDdlUtils::ConvertToVariant(pValue, value).Succeeded())
        {
          xiiMaterialResourceDescriptor::Parameter& sc = m_Description.m_Parameters.ExpandAndGetRef();
          sc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          sc.m_Value = value;
        }
      }

      // Read the texture references
      if (pChild->IsCustomType("Texture2D"))
      {
        const xiiOpenDdlReaderElement* pName  = pChild->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Variable");
        const xiiOpenDdlReaderElement* pValue = pChild->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          xiiMaterialResourceDescriptor::Texture2DBinding& tc = m_Description.m_Texture2DBindings.ExpandAndGetRef();
          tc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          tc.m_Value = xiiResourceManager::LoadResource<xiiTexture2DResource>(pValue->GetPrimitivesString()[0]);
        }
      }

      // Read the texture references
      if (pChild->IsCustomType("TextureCube"))
      {
        const xiiOpenDdlReaderElement* pName  = pChild->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Variable");
        const xiiOpenDdlReaderElement* pValue = pChild->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          xiiMaterialResourceDescriptor::TextureCubeBinding& tc = m_Description.m_TextureCubeBindings.ExpandAndGetRef();
          tc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          tc.m_Value = xiiResourceManager::LoadResource<xiiTextureCubeResource>(pValue->GetPrimitivesString()[0]);
        }
      }
    }
  }
  else
  {
    xiiLog::Error("Unknown material file type: '{}'", sAbsFilePath);
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
  schemaDescription.m_sName = GetResourceID();
  if (schemaDescription.m_sName.IsEmpty())
    schemaDescription.m_sName = "Runtime Material";
  schemaDescription.m_hShader      = m_ResolvedValues.m_hShader;
  schemaDescription.m_Domain       = m_Description.m_Domain;
  schemaDescription.m_ShadingModel = m_Description.m_ShadingModel;

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
