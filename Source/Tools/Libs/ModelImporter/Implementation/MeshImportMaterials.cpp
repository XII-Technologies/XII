/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ModelImporter/ModelImporterPCH.h>

#include <ModelImporter/Implementation/MeshImportInternal.h>
#include <assimp/GltfMaterial.h>
#include <assimp/material.h>

namespace
{
  using namespace xiiMeshImportInternal;

  xiiResult ReadEmbedded(const aiScene& scene, xiiStringView key, xiiDynamicArray<xiiModelEmbeddedTexture>& textures, xiiStringBuilder* error)
  {
    for (const auto& texture : textures)
      if (texture.m_sKey == key)
        return XII_SUCCESS;
    const xiiString path(key);
    const auto*     source = scene.GetEmbeddedTexture(path.GetData());
    if (source == nullptr)
    {
      if (key.StartsWith("*"))
        return Fail(error, "Material references an invalid embedded texture.");
      return XII_SUCCESS; // External source-relative texture; the asset layer resolves it.
    }
    if (source->pcData == nullptr || source->mWidth == 0)
      return Fail(error, "Source contains empty embedded texture data.");
    auto& texture  = textures.ExpandAndGetRef();
    texture.m_sKey = key;
    if (source->mHeight == 0)
    {
      texture.m_sFileExtension = source->achFormatHint;
      texture.m_Data.SetCountUninitialized(source->mWidth);
      xiiMemoryUtils::Copy(texture.m_Data.GetData(), reinterpret_cast<const xiiUInt8*>(source->pcData), source->mWidth);
    }
    else
    {
      const auto pixels = static_cast<xiiUInt64>(source->mWidth) * source->mHeight;
      if (pixels > xiiMath::MaxValue<xiiUInt32>() / 4)
        return Fail(error, "Embedded texture exceeds native storage capacity.");
      texture.m_uiWidth  = source->mWidth;
      texture.m_uiHeight = source->mHeight;
      texture.m_Data.SetCountUninitialized(static_cast<xiiUInt32>(pixels * 4));
      for (xiiUInt32 p = 0; p < pixels; ++p)
      {
        const auto& pixel         = source->pcData[p];
        texture.m_Data[p * 4]     = pixel.r;
        texture.m_Data[p * 4 + 1] = pixel.g;
        texture.m_Data[p * 4 + 2] = pixel.b;
        texture.m_Data[p * 4 + 3] = pixel.a;
      }
    }
    return XII_SUCCESS;
  }

  xiiGALTextureAddressMode::Enum ConvertAddress(aiTextureMapMode mode)
  {
    switch (mode)
    {
      case aiTextureMapMode_Wrap:
        return xiiGALTextureAddressMode::Wrap;
      case aiTextureMapMode_Clamp:
        return xiiGALTextureAddressMode::Clamp;
      case aiTextureMapMode_Mirror:
        return xiiGALTextureAddressMode::Mirror;
      case aiTextureMapMode_Decal:
        return xiiGALTextureAddressMode::Border;
      default:
        return xiiGALTextureAddressMode::Unknown;
    }
  }

  xiiResult ReadTextures(const aiScene& scene, const aiMaterial& material, aiTextureType type, xiiModelTextureUsage usage, xiiModelMeshMaterial& output, xiiDynamicArray<xiiModelEmbeddedTexture>& embedded, xiiStringBuilder* error)
  {
    for (xiiUInt32 i = 0; i < material.GetTextureCount(type); ++i)
    {
      xiiModelMaterialTexture texture;
      texture.m_Usage = usage;
      xiiStringBuilder textureError;
      const auto       read = [&]() -> xiiResult {
        auto*            error = &textureError;
        aiString         path;
        aiTextureMapping mapping    = aiTextureMapping_UV;
        aiTextureMapMode address[3] = {aiTextureMapMode_Wrap, aiTextureMapMode_Wrap, aiTextureMapMode_Wrap};
        aiTextureOp      operation  = aiTextureOp_Multiply;
        if (material.GetTexture(type, i, &path, &mapping, &texture.m_uiUVChannel, &texture.m_fBlend, &operation, address) != AI_SUCCESS || path.length == 0 || !xiiUnicodeUtils::IsValidUtf8(path.C_Str()) || mapping != aiTextureMapping_UV || texture.m_uiUVChannel > 1 || !xiiMath::IsFinite(texture.m_fBlend) || operation < aiTextureOp_Multiply || operation > aiTextureOp_SignedAdd)
          return Fail(error, "Source material contains invalid texture metadata or unsupported UV mapping.");
        texture.m_sPath = path.C_Str();
        if (usage == xiiModelTextureUsage::Normal)
          material.Get(AI_MATKEY_GLTF_TEXTURE_SCALE(type, i), texture.m_fStrength);
        else if (usage == xiiModelTextureUsage::Occlusion)
          material.Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(type, i), texture.m_fStrength);
        if (!xiiMath::IsFinite(texture.m_fStrength))
          return Fail(error, "Source texture uses a non-finite strength.");
        texture.m_Operation          = static_cast<xiiModelMaterialTexture::Operation>(operation);
        texture.m_Sampler.m_AddressU = ConvertAddress(address[0]);
        texture.m_Sampler.m_AddressV = ConvertAddress(address[1]);
        texture.m_Sampler.m_AddressW = ConvertAddress(address[2]);
        if (texture.m_Sampler.m_AddressU == xiiGALTextureAddressMode::Unknown || texture.m_Sampler.m_AddressV == xiiGALTextureAddressMode::Unknown || texture.m_Sampler.m_AddressW == xiiGALTextureAddressMode::Unknown)
          return Fail(error, "Source texture uses an unknown address mode.");
        aiUVTransform uv;
        if (material.Get(AI_MATKEY_UVTRANSFORM(type, i), uv) == AI_SUCCESS)
        {
          texture.m_vUVOffset   = xiiVec2(uv.mTranslation.x, uv.mTranslation.y);
          texture.m_vUVScale    = xiiVec2(uv.mScaling.x, uv.mScaling.y);
          texture.m_fUVRotation = uv.mRotation;
          if (!texture.m_vUVOffset.IsValid() || !texture.m_vUVScale.IsValid() || !xiiMath::IsFinite(texture.m_fUVRotation))
            return Fail(error, "Source texture uses a non-finite UV transform.");
        }
        int filter;
        if (material.Get(AI_MATKEY_GLTF_MAPPINGFILTER_MAG(type, i), filter) == AI_SUCCESS)
        {
          if (filter != 9728 && filter != 9729)
            return Fail(error, "Source texture uses an invalid magnification filter.");
          texture.m_Sampler.m_MagFilter = filter == 9728 ? xiiGALFilterType::Point : xiiGALFilterType::Linear;
        }
        if (material.Get(AI_MATKEY_GLTF_MAPPINGFILTER_MIN(type, i), filter) == AI_SUCCESS)
        {
          if (filter == 9728 || filter == 9729)
            texture.m_Sampler.m_fMaxLOD = 0; // glTF non-mipmapped filters sample the base level only.
          switch (filter)
          {
            case 9728:
            case 9984:
              texture.m_Sampler.m_MinFilter = xiiGALFilterType::Point;
              texture.m_Sampler.m_MipFilter = xiiGALFilterType::Point;
              break;
            case 9729:
            case 9985:
              texture.m_Sampler.m_MinFilter = xiiGALFilterType::Linear;
              texture.m_Sampler.m_MipFilter = xiiGALFilterType::Point;
              break;
            case 9986:
              texture.m_Sampler.m_MinFilter = xiiGALFilterType::Point;
              texture.m_Sampler.m_MipFilter = xiiGALFilterType::Linear;
              break;
            case 9987:
              texture.m_Sampler.m_MinFilter = xiiGALFilterType::Linear;
              texture.m_Sampler.m_MipFilter = xiiGALFilterType::Linear;
              break;
            default:
              return Fail(error, "Source texture uses an invalid minification filter.");
          }
        }
        XII_SUCCEED_OR_RETURN(ReadEmbedded(scene, texture.m_sPath, embedded, error));
        return XII_SUCCESS;
      };
      if (read().Failed())
        xiiLog::Warning("Material '{}', texture type {} layer {} ('{}'): {} Skipping this texture.", output.m_sName, static_cast<xiiUInt32>(type), i, texture.m_sPath, textureError);
      else
        output.m_Textures.PushBack(std::move(texture));
    }
    return XII_SUCCESS;
  }
} // namespace

xiiResult xiiMeshImportInternal::ReadMaterial(const aiScene& scene, const aiMaterial& material, bool bGltf, bool bFlipTextureV, xiiModelMeshMaterial& out_material, xiiDynamicArray<xiiModelEmbeddedTexture>& inout_textures, xiiStringBuilder* error)
{
  aiString name;
  material.Get(AI_MATKEY_NAME, name);
  if (!xiiUnicodeUtils::IsValidUtf8(name.C_Str()))
    return Fail(error, "Source material has an invalid UTF-8 name.");
  out_material.m_sName = name.C_Str();
  aiColor4D color(1, 1, 1, 1);
  if (material.Get(AI_MATKEY_BASE_COLOR, color) != AI_SUCCESS)
    material.Get(AI_MATKEY_COLOR_DIFFUSE, color);
  out_material.m_BaseColor = xiiColor(color.r, color.g, color.b, color.a);
  color                    = aiColor4D(0, 0, 0, 1);
  material.Get(AI_MATKEY_COLOR_EMISSIVE, color);
  out_material.m_EmissiveColor = xiiColor(color.r, color.g, color.b, color.a);
  float emissiveIntensity      = 1;
  material.Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveIntensity);
  if (!xiiMath::IsFinite(emissiveIntensity) || emissiveIntensity < 0)
    return Fail(error, "Source material uses an invalid emissive intensity.");
  out_material.m_EmissiveColor.r *= emissiveIntensity;
  out_material.m_EmissiveColor.g *= emissiveIntensity;
  out_material.m_EmissiveColor.b *= emissiveIntensity;
  material.Get(AI_MATKEY_METALLIC_FACTOR, out_material.m_fMetallic);
  material.Get(AI_MATKEY_ROUGHNESS_FACTOR, out_material.m_fRoughness);
  material.Get(AI_MATKEY_OPACITY, out_material.m_fOpacity);
  material.Get(AI_MATKEY_GLTF_ALPHACUTOFF, out_material.m_fAlphaCutoff);
  material.Get(AI_MATKEY_CLEARCOAT_FACTOR, out_material.m_fClearCoat);
  material.Get(AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, out_material.m_fClearCoatRoughness);
  material.Get(AI_MATKEY_TRANSMISSION_FACTOR, out_material.m_fTransmission);
  material.Get(AI_MATKEY_VOLUME_THICKNESS_FACTOR, out_material.m_fThickness);
  material.Get(AI_MATKEY_REFRACTI, out_material.m_fIndexOfRefraction);
  material.Get(AI_MATKEY_ANISOTROPY_FACTOR, out_material.m_fAnisotropy);
  material.Get(AI_MATKEY_SHEEN_ROUGHNESS_FACTOR, out_material.m_fSheenRoughness);
  for (float value : {out_material.m_fClearCoat, out_material.m_fClearCoatRoughness, out_material.m_fTransmission, out_material.m_fThickness, out_material.m_fIndexOfRefraction, out_material.m_fAnisotropy, out_material.m_fSheenRoughness})
    if (!xiiMath::IsFinite(value))
      return Fail(error, "Source material contains non-finite PBR layer properties.");
  int twoSided = 0;
  material.Get(AI_MATKEY_TWOSIDED, twoSided);
  out_material.m_bTwoSided = twoSided != 0;
  aiString alpha;
  if (material.Get(AI_MATKEY_GLTF_ALPHAMODE, alpha) == AI_SUCCESS)
  {
    // Assimp mirrors glTF baseColorFactor.a into OPACITY. It is not an independent
    // multiplier; applying both would square alpha when authoring native materials.
    out_material.m_fOpacity = 1.0f;
    if (xiiStringView(alpha.C_Str()) == "MASK")
      out_material.m_AlphaMode = xiiModelMeshMaterial::AlphaMode::Mask;
    else if (xiiStringView(alpha.C_Str()) == "BLEND")
      out_material.m_AlphaMode = xiiModelMeshMaterial::AlphaMode::Blend;
    else if (xiiStringView(alpha.C_Str()) != "OPAQUE")
      return Fail(error, "Source material has an unknown alpha mode.");
  }
  else if (out_material.m_fOpacity < 1)
    out_material.m_AlphaMode = xiiModelMeshMaterial::AlphaMode::Blend;
  if (!out_material.m_BaseColor.IsValid() || !out_material.m_EmissiveColor.IsValid() || !xiiMath::IsFinite(out_material.m_fMetallic) || !xiiMath::IsFinite(out_material.m_fRoughness) || !xiiMath::IsFinite(out_material.m_fOpacity) || !xiiMath::IsFinite(out_material.m_fAlphaCutoff))
    return Fail(error, "Source material contains non-finite rendering properties.");
  out_material.m_fMetallic    = xiiMath::Clamp(out_material.m_fMetallic, 0.0f, 1.0f);
  out_material.m_fRoughness   = xiiMath::Clamp(out_material.m_fRoughness, 0.0f, 1.0f);
  out_material.m_fOpacity     = xiiMath::Clamp(out_material.m_fOpacity, 0.0f, 1.0f);
  out_material.m_fAlphaCutoff = xiiMath::Clamp(out_material.m_fAlphaCutoff, 0.0f, 1.0f);

  const bool baseColor = material.GetTextureCount(aiTextureType_BASE_COLOR) != 0;
  XII_SUCCEED_OR_RETURN(ReadTextures(scene, material, baseColor ? aiTextureType_BASE_COLOR : aiTextureType_DIFFUSE, xiiModelTextureUsage::BaseColor, out_material, inout_textures, error));
  XII_SUCCEED_OR_RETURN(ReadTextures(scene, material, aiTextureType_NORMALS, xiiModelTextureUsage::Normal, out_material, inout_textures, error));
  XII_SUCCEED_OR_RETURN(ReadTextures(scene, material, aiTextureType_HEIGHT, xiiModelTextureUsage::Height, out_material, inout_textures, error));
  XII_SUCCEED_OR_RETURN(ReadTextures(scene, material, aiTextureType_AMBIENT_OCCLUSION, xiiModelTextureUsage::Occlusion, out_material, inout_textures, error));
  // Assimp's glTF reader identifies occlusion through strength metadata on LIGHTMAP.
  // Do not reinterpret lightmaps from other formats as ambient occlusion.
  float occlusionStrength;
  if (material.Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_LIGHTMAP, 0), occlusionStrength) == AI_SUCCESS)
    XII_SUCCEED_OR_RETURN(ReadTextures(scene, material, aiTextureType_LIGHTMAP, xiiModelTextureUsage::Occlusion, out_material, inout_textures, error));
  const bool emissive = material.GetTextureCount(aiTextureType_EMISSION_COLOR) != 0;
  XII_SUCCEED_OR_RETURN(ReadTextures(scene, material, emissive ? aiTextureType_EMISSION_COLOR : aiTextureType_EMISSIVE, xiiModelTextureUsage::Emissive, out_material, inout_textures, error));
  if (material.GetTextureCount(aiTextureType_GLTF_METALLIC_ROUGHNESS) != 0)
    XII_SUCCEED_OR_RETURN(ReadTextures(scene, material, aiTextureType_GLTF_METALLIC_ROUGHNESS, xiiModelTextureUsage::MetallicRoughness, out_material, inout_textures, error));
  else
  {
    XII_SUCCEED_OR_RETURN(ReadTextures(scene, material, aiTextureType_DIFFUSE_ROUGHNESS, xiiModelTextureUsage::Roughness, out_material, inout_textures, error));
    XII_SUCCEED_OR_RETURN(ReadTextures(scene, material, aiTextureType_METALNESS, xiiModelTextureUsage::Metallic, out_material, inout_textures, error));
  }
  for (auto& texture : out_material.m_Textures)
  {
    if (bGltf)
    {
      // Undo Assimp glTF's image-center/vertical-origin conversion (and the
      // optional FlipUVs postprocess) to recover KHR_texture_transform exactly.
      // Native mappings rotate scaled coordinates about the texture origin.
      const float   rotation   = bFlipTextureV ? texture.m_fUVRotation : -texture.m_fUVRotation;
      const float   cosine     = xiiMath::Cos(xiiAngle::MakeFromRadian(rotation));
      const float   sine       = xiiMath::Sin(xiiAngle::MakeFromRadian(rotation));
      const auto    scale      = texture.m_vUVScale;
      const float   canonicalY = bFlipTextureV ? -texture.m_vUVOffset.y : texture.m_vUVOffset.y;
      const xiiVec2 offset(texture.m_vUVOffset.x - 0.5f * scale.x * (-cosine + sine + 1),
                           0.5f * scale.y * (sine + cosine - 1) + 1 - scale.y - canonicalY);
      texture.m_fUVRotation = bFlipTextureV ? rotation : -rotation;
      texture.m_vUVOffset   = bFlipTextureV ? offset : xiiVec2(offset.x - sine * scale.y, 1 - cosine * scale.y - offset.y);
    }
    else
    {
      const float cosine  = xiiMath::Cos(xiiAngle::MakeFromRadian(texture.m_fUVRotation));
      const float sine    = xiiMath::Sin(xiiAngle::MakeFromRadian(texture.m_fUVRotation));
      const auto  offset  = texture.m_vUVOffset - xiiVec2(0.5f);
      texture.m_vUVOffset = xiiVec2(0.5f) + xiiVec2(cosine * offset.x - sine * offset.y, sine * offset.x + cosine * offset.y).CompMul(texture.m_vUVScale);
    }
  }
  return XII_SUCCESS;
}
