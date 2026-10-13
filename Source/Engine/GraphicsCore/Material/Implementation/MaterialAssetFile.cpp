/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Material/MaterialAssetFile.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>

namespace
{
  bool Valid(const xiiMaterialResourceDescriptor& d)
  {
    if (d.m_Domain >= xiiMaterialDomain::ENUM_COUNT || d.m_ShadingModel >= xiiMaterialShadingModel::ENUM_COUNT ||
        d.m_BlendMode >= xiiMaterialBlendMode::ENUM_COUNT || d.m_AlphaMode >= xiiMaterialAlphaMode::ENUM_COUNT ||
        (d.m_FeatureFlags.GetValue() & 0xFFFF0000U) != 0 || !d.m_BaseColor.IsValid() || !d.m_EmissiveColor.IsValid())
      return false;
    for (float value : {d.m_fMetallic, d.m_fRoughness, d.m_fOcclusionStrength, d.m_fAlphaCutoff, d.m_fNormalScale,
                        d.m_fDisplacementScale, d.m_fClearCoat, d.m_fClearCoatRoughness, d.m_fTransmission, d.m_fThickness,
                        d.m_fIndexOfRefraction, d.m_fAnisotropy, d.m_fSheenRoughness})
      if (!xiiMath::IsFinite(value))
        return false;
    return true;
  }

  xiiResult WritePbrBlock(xiiStreamWriter& stream, const xiiMaterialResourceDescriptor& d)
  {
    const xiiUInt8 enums[] = {d.m_Domain.GetValue(), d.m_ShadingModel.GetValue(), d.m_BlendMode.GetValue(), d.m_AlphaMode.GetValue()};
    XII_SUCCEED_OR_RETURN(stream.WriteBytes(enums, sizeof(enums)));
    const auto flags = d.m_FeatureFlags.GetValue();
    XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&flags));
    const float values[] = {d.m_BaseColor.r, d.m_BaseColor.g, d.m_BaseColor.b, d.m_BaseColor.a,
                            d.m_EmissiveColor.r, d.m_EmissiveColor.g, d.m_EmissiveColor.b, d.m_EmissiveColor.a,
                            d.m_fMetallic, d.m_fRoughness, d.m_fOcclusionStrength, d.m_fAlphaCutoff, d.m_fNormalScale,
                            d.m_fDisplacementScale, d.m_fClearCoat, d.m_fClearCoatRoughness, d.m_fTransmission,
                            d.m_fThickness, d.m_fIndexOfRefraction, d.m_fAnisotropy, d.m_fSheenRoughness};
    for (const float& value : values)
      XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&value));
    XII_SUCCEED_OR_RETURN(stream.WriteWordValue(&d.m_iSortPriority));
    for (const auto& texture : {d.m_hBaseColorTexture, d.m_hNormalTexture, d.m_hMetallicRoughnessTexture, d.m_hOcclusionTexture,
                                d.m_hEmissiveTexture, d.m_hHeightTexture, d.m_hClearCoatTexture, d.m_hTransmissionTexture})
    {
      if (texture.IsValid() && texture.GetResourceID().GetElementCount() > 65536)
        return XII_FAILURE;
      XII_SUCCEED_OR_RETURN(stream.WriteString(texture.IsValid() ? texture.GetResourceID() : xiiStringView()));
    }
    return XII_SUCCESS;
  }

  xiiResult ReadTextureId(xiiStreamReader& stream, xiiString& out_id)
  {
    // ReadString does not check short reads in its payload. Resource IDs are bounded
    // UTF-8 text, so validate their length and bytes explicitly at this file boundary.
    xiiUInt32 length = 0;
    XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&length));
    if (length > 65536)
      return XII_FAILURE;
    xiiDynamicArray<char> text;
    text.SetCountUninitialized(length + 1);
    if (stream.ReadBytes(text.GetData(), length) != length)
      return XII_FAILURE;
    text[length] = '\0';
    if (!xiiUnicodeUtils::IsValidUtf8(text.GetData(), text.GetData() + length) ||
        xiiStringUtils::GetStringElementCount(text.GetData()) != length)
      return XII_FAILURE;
    out_id = xiiStringView(text.GetData(), length);
    return XII_SUCCESS;
  }
} // namespace

xiiResult xiiMaterialAssetFile::Write(xiiStreamWriter& stream, const xiiMaterialResourceDescriptor& d)
{
  if (!Valid(d) || d.m_PermutationVariables.GetCount() > 65535 || d.m_Parameters.GetCount() > 65535 ||
      d.m_Texture2DBindings.GetCount() > 65535 || d.m_TextureCubeBindings.GetCount() > 65535)
    return XII_FAILURE;
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  writer << xiiUInt8(8) << xiiUInt8(0); // Native PBR block, no compression.
  writer << (d.m_hBaseMaterial.IsValid() ? d.m_hBaseMaterial.GetResourceID() : xiiStringView());
  writer << d.m_sSurface.GetString();
  writer << (d.m_hShader.IsValid() ? d.m_hShader.GetResourceID() : xiiStringView());
  writer << static_cast<xiiUInt16>(d.m_PermutationVariables.GetCount());
  for (const auto& permutation : d.m_PermutationVariables)
    writer << permutation.m_sName.GetString() << permutation.m_sValue.GetString();
  writer << static_cast<xiiUInt16>(d.m_Texture2DBindings.GetCount());
  for (const auto& binding : d.m_Texture2DBindings)
    writer << binding.m_Name.GetString() << (binding.m_Value.IsValid() ? binding.m_Value.GetResourceID() : xiiStringView());
  writer << static_cast<xiiUInt16>(d.m_TextureCubeBindings.GetCount());
  for (const auto& binding : d.m_TextureCubeBindings)
    writer << binding.m_Name.GetString() << (binding.m_Value.IsValid() ? binding.m_Value.GetResourceID() : xiiStringView());
  writer << static_cast<xiiUInt16>(d.m_Parameters.GetCount());
  for (const auto& parameter : d.m_Parameters)
    writer << parameter.m_Name.GetString() << parameter.m_Value;
  writer << xiiStringView(); // Reserved field, unchanged from version 7.
  XII_SUCCEED_OR_RETURN(WritePbrBlock(writer, d));
  writer << xiiStringView(); // No embedded fallback textures; native resource references own loading.
  return storage.CopyToStream(stream);
}

xiiResult xiiMaterialAssetFile::ReadPbrBlock(xiiStreamReader& stream, xiiMaterialResourceDescriptor& inout_description)
{
  xiiMaterialResourceDescriptor d;
  xiiUInt8                      enums[4];
  xiiUInt32                     flags;
  if (stream.ReadBytes(enums, sizeof(enums)) != sizeof(enums))
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&flags));
  d.m_Domain       = static_cast<xiiMaterialDomain::Enum>(enums[0]);
  d.m_ShadingModel = static_cast<xiiMaterialShadingModel::Enum>(enums[1]);
  d.m_BlendMode    = static_cast<xiiMaterialBlendMode::Enum>(enums[2]);
  d.m_AlphaMode    = static_cast<xiiMaterialAlphaMode::Enum>(enums[3]);
  d.m_FeatureFlags.SetValue(flags);
  float values[21];
  for (float& value : values)
    XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&value));
  d.m_BaseColor           = xiiColor(values[0], values[1], values[2], values[3]);
  d.m_EmissiveColor       = xiiColor(values[4], values[5], values[6], values[7]);
  d.m_fMetallic           = values[8];
  d.m_fRoughness          = values[9];
  d.m_fOcclusionStrength  = values[10];
  d.m_fAlphaCutoff        = values[11];
  d.m_fNormalScale        = values[12];
  d.m_fDisplacementScale  = values[13];
  d.m_fClearCoat          = values[14];
  d.m_fClearCoatRoughness = values[15];
  d.m_fTransmission       = values[16];
  d.m_fThickness          = values[17];
  d.m_fIndexOfRefraction  = values[18];
  d.m_fAnisotropy         = values[19];
  d.m_fSheenRoughness     = values[20];
  XII_SUCCEED_OR_RETURN(stream.ReadWordValue(&d.m_iSortPriority));
  if (!Valid(d))
    return XII_FAILURE;
  xiiString ids[8];
  for (auto& id : ids)
    XII_SUCCEED_OR_RETURN(ReadTextureId(stream, id));
  xiiTexture2DResourceHandle* textures[] = {&d.m_hBaseColorTexture, &d.m_hNormalTexture, &d.m_hMetallicRoughnessTexture, &d.m_hOcclusionTexture,
                                            &d.m_hEmissiveTexture, &d.m_hHeightTexture, &d.m_hClearCoatTexture, &d.m_hTransmissionTexture};
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(ids); ++i)
    if (!ids[i].IsEmpty())
      *textures[i] = xiiResourceManager::LoadResource<xiiTexture2DResource>(ids[i]);
  d.m_hBaseMaterial        = inout_description.m_hBaseMaterial;
  d.m_hShader              = inout_description.m_hShader;
  d.m_sSurface             = inout_description.m_sSurface;
  d.m_PermutationVariables = std::move(inout_description.m_PermutationVariables);
  d.m_Parameters           = std::move(inout_description.m_Parameters);
  d.m_Texture2DBindings    = std::move(inout_description.m_Texture2DBindings);
  d.m_TextureCubeBindings  = std::move(inout_description.m_TextureCubeBindings);
  inout_description        = std::move(d);
  return XII_SUCCESS;
}
