/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec4.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// G-Buffer channels a decal may write.
struct XII_GRAPHICSCORE_DLL xiiDecalChannelMask
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    None     = 0U,                                    ///< This decal does not write to any G-Buffer channel, it only has an emissive contribution.
    Albedo   = XII_BIT(0),                            ///< This decal writes to the albedo channel, which typically contains the base color of the surface.
    Normal   = XII_BIT(1),                            ///< This decal writes to the normal channel, which typically contains the surface normal in tangent space.
    Material = XII_BIT(2),                            ///< This decal writes to the material channel, which typically contains roughness, metallic and ambient occlusion packed together.
    Emissive = XII_BIT(3),                            ///< This decal writes to the emissive channel, which typically contains the emissive color of the surface.
    All      = Albedo | Normal | Material | Emissive, ///< This decal writes to all G-Buffer channels.

    Default = Albedo | Normal | Material ///< By default, decals write to all channels except emissive, as most decals do not have an emissive contribution.
  };

  struct Bits
  {
    StorageType Albedo : 1;
    StorageType Normal : 1;
    StorageType Material : 1;
    StorageType Emissive : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiDecalChannelMask);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDecalChannelMask);

/// Renderer-facing decal classification.
struct XII_GRAPHICSCORE_DLL xiiDecalProjectionMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Projected = 0U, ///< Deferred projected decal volume reconstructed from depth.
    Mesh,           ///< Pre-baked mesh decal drawn into the G-Buffer.

    ENUM_COUNT,

    Default = Projected
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDecalProjectionMode);

/// One atlas source entry and its packed UV metadata.
struct XII_GRAPHICSCORE_DLL xiiDecalAtlasEntry
{
  xiiHashedString m_sDecalId; ///< Unique identifier for the decal, used for lookup.

  xiiTexture2DResourceHandle m_hAlbedo;   ///< Handle to the albedo texture for this decal.
  xiiTexture2DResourceHandle m_hNormal;   ///< Handle to the normal texture for this decal.
  xiiTexture2DResourceHandle m_hMaterial; ///< Handle to the material texture for this decal, typically containing roughness, metallic and ambient occlusion packed together.
  xiiTexture2DResourceHandle m_hEmissive; ///< Handle to the emissive texture for this decal.

  xiiSizeU16 m_Size       = xiiSizeU16(256, 256); ///< Original size of the decal textures, used for packing and UV calculations.
  xiiUInt8   m_uiPadding  = 2U;                   ///< Padding in pixels to add around this decal in the atlas to prevent bleeding. This is typically 2 pixels for bilinear filtering, but may need to be higher for mipmapping or if the source textures have alpha coverage that extends to the edges.
  xiiUInt8   m_uiPriority = 128U;                 ///< Priority for packing and rendering. Higher priority decals are packed closer to the top-left corner of the atlas, which can result in better sampling quality due to how mipmaps are generated. This also allows controlling the rendering order of decals, as higher priority decals can be rendered after lower priority ones to reduce overdraw.

  xiiBitflags<xiiDecalChannelMask> m_ChannelMask; ///< Mask of G-Buffer channels that this decal writes to. This allows packing multiple decals into the same atlas, as decals that write to different channels can be blended together without artifacts.

  xiiVec4 m_vUVRect = xiiVec4(0.0f, 0.0f, 1.0f, 1.0f); ///< UV rectangle in the atlas where this decal is located, in normalized coordinates. xy = UV minimum, zw = UV size inside the packed atlas.

  xiiVec4 m_vTextureMetrics = xiiVec4(1.0f / 256.0f, 1.0f / 256.0f, 256.0f, 256.0f); ///< Texture metrics for this decal, used for sampling and mipmap calculations. xy = texel size (1 / original texture size), zw = packed pixel size (original texture size + padding, in pixels).
};

/// Descriptor used to build a packed decal atlas resource at runtime or by tools.
struct XII_GRAPHICSCORE_DLL xiiDecalAtlasResourceDescriptor
{
  xiiSizeU16                          m_AtlasSize        = xiiSizeU16(2048, 2048); ///< Size of the atlas texture to create. This should be a power of two for optimal mipmapping, but it is not strictly required.
  xiiUInt8                            m_uiPadding        = 2U;                     ///< Default padding in pixels to add around each decal in the atlas to prevent bleeding. This can be overridden per decal entry, but this value is used as a default for entries that do not specify their own padding.
  bool                                m_bGenerateMipMaps = true;                   ///< Whether to generate mipmaps for the atlas texture. This is typically true for projected decals, as they can be sampled at varying distances, but may be false for mesh decals that are always sampled at a fixed size.
  xiiDynamicArray<xiiDecalAtlasEntry> m_Entries;                                   ///< Array of entries to pack into the atlas, which contains the metadata for all entries. The packing algorithm will use the size and padding of each entry to determine how to pack them into the atlas, and will fill in the UVRect and TextureMetrics for each entry based on where they are packed.

  void Save(xiiStreamWriter& ref_stream) const;
  void Load(xiiStreamReader& ref_stream);
};

/// Packed decal atlas with metadata mapping decal IDs to UV rectangles.
class XII_GRAPHICSCORE_DLL xiiDecalAtlasResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalAtlasResource, xiiResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiDecalAtlasResource);

  XII_RESOURCE_DECLARE_CREATEABLE(xiiDecalAtlasResource, xiiDecalAtlasResourceDescriptor);

public:
  xiiDecalAtlasResource();

  /// Returns the descriptor used to create this atlas, which contains the metadata for all entries.
  XII_ALWAYS_INLINE const xiiDecalAtlasResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  /// Returns the array of entries in this atlas, which contains the metadata for all entries.
  XII_ALWAYS_INLINE xiiArrayPtr<const xiiDecalAtlasEntry> GetEntries() const { return m_Descriptor.m_Entries; }

  /// Returns the GPU texture for the given channel. All channels are packed into the same atlas, so this returns the same texture for all channels.
  XII_ALWAYS_INLINE xiiSharedPtr<xiiGALTexture> GetGALTexture() const { return m_pAlbedoAtlasTexture; }

  /// Returns the GPU texture for the albedo channel.
  XII_ALWAYS_INLINE xiiSharedPtr<xiiGALTexture> GetAlbedoTexture() const { return m_pAlbedoAtlasTexture; }

  /// Returns the GPU texture for the normal channel.
  XII_ALWAYS_INLINE xiiSharedPtr<xiiGALTexture> GetNormalTexture() const { return m_pNormalAtlasTexture; }

  /// Returns the GPU texture for the material channel.
  XII_ALWAYS_INLINE xiiSharedPtr<xiiGALTexture> GetMaterialTexture() const { return m_pMaterialAtlasTexture; }

  /// Returns the GPU texture for the emissive channel.
  XII_ALWAYS_INLINE xiiSharedPtr<xiiGALTexture> GetEmissiveTexture() const { return m_pEmissiveAtlasTexture; }

  /// Returns the sampler state used for sampling the atlas textures.
  XII_ALWAYS_INLINE xiiSharedPtr<xiiGALSampler> GetGALSampler() const { return m_pAtlasSampler; }

  /// Tries to find the atlas entry for the given decal ID. Returns true if found, false otherwise.
  bool TryGetAtlasEntry(const xiiTempHashedString& sDecalId, const xiiDecalAtlasEntry*& out_pEntry) const;

  /// Tries to find the atlas entry for the given decal ID. Returns true if found, false otherwise.
  bool TryGetAtlasEntry(const xiiHashedString& sDecalId, const xiiDecalAtlasEntry*& out_pEntry) const;

private:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void PackAtlas();
  void CreateGPUAtlases();
  void RebuildLookup();

private:
  xiiDecalAtlasResourceDescriptor          m_Descriptor;     ///< Descriptor used to create this atlas, which contains the metadata for all entries.
  xiiHashTable<xiiHashedString, xiiUInt32> m_IdToEntryIndex; ///< Lookup table mapping decal IDs to their index in the entries array for fast retrieval.

  xiiSharedPtr<xiiGALTexture> m_pAlbedoAtlasTexture;   ///< GPU texture for the albedo channel. All channels are packed into the same atlas, so this is also returned by GetGALTexture().
  xiiSharedPtr<xiiGALTexture> m_pNormalAtlasTexture;   ///< GPU texture for the normal channel. All channels are packed into the same atlas, so this is also returned by GetGALTexture().
  xiiSharedPtr<xiiGALTexture> m_pMaterialAtlasTexture; ///< GPU texture for the material channel. All channels are packed into the same atlas, so this is also returned by GetGALTexture().
  xiiSharedPtr<xiiGALTexture> m_pEmissiveAtlasTexture; ///< GPU texture for the emissive channel. All channels are packed into the same atlas, so this is also returned by GetGALTexture().
  xiiSharedPtr<xiiGALSampler> m_pAtlasSampler;         ///< Sampler state used for sampling the atlas textures.
  xiiUInt32                   m_uiMemoryGPU = 0U;      ///< Memory usage of the GPU resources for this atlas, used for tracking and reporting memory usage.
};

/// Per-decal material defaults shared by projected and mesh decals.
struct XII_GRAPHICSCORE_DLL xiiDecalResourceDescriptor
{
  xiiHashedString m_sDecalId; ///< Unique identifier for the decal, used for lookup in the atlas resource.

  xiiDecalAtlasResourceHandle m_hAtlas; ///< Handle to the decal atlas resource that contains the textures for this decal. The atlas resource contains the metadata mapping decal IDs to UV rectangles, as well as the GPU textures for all channels. This allows sharing the same atlas resource between multiple decals, as long as they reference different entries in the atlas.

  xiiTexture2DResourceHandle m_hAlbedo;   ///< Handle to the albedo texture for this decal, used if this decal is not packed into an atlas. If this is valid, the atlas handle is ignored and this texture is used instead. This allows using individual textures for decals that are not suitable for atlasing, such as large mesh decals or decals with unique channel masks.
  xiiTexture2DResourceHandle m_hNormal;   ///< Handle to the normal texture for this decal, used if this decal is not packed into an atlas. If this is valid, the atlas handle is ignored and this texture is used instead. This allows using individual textures for decals that are not suitable for atlasing, such as large mesh decals or decals with unique channel masks.
  xiiTexture2DResourceHandle m_hMaterial; ///< Handle to the material texture for this decal, used if this decal is not packed into an atlas. If this is valid, the atlas handle is ignored and this texture is used instead. This allows using individual textures for decals that are not suitable for atlasing, such as large mesh decals or decals with unique channel masks.
  xiiTexture2DResourceHandle m_hEmissive; ///< Handle to the emissive texture for this decal, used if this decal is not packed into an atlas. If this is valid, the atlas handle is ignored and this texture is used instead. This allows using individual textures for decals that are not suitable for atlasing, such as large mesh decals or decals with unique channel masks.

  xiiVec2  m_vUVOffset = xiiVec2::MakeZero(); ///< UV offset to apply to this decal when sampling from the atlas, in normalized coordinates. This allows adjusting the UVs for this decal without modifying the atlas entry, which can be useful for animation or variation purposes.
  xiiVec2  m_vUVScale  = xiiVec2(1.0f);       ///< UV scale to apply to this decal when sampling from the atlas, in normalized coordinates. This allows adjusting the UVs for this decal without modifying the atlas entry, which can be useful for animation or variation purposes.
  xiiColor m_Tint      = xiiColor::White;     ///< Tint color to multiply with the sampled albedo color for this decal, used for color variation without modifying the source textures.

  xiiBitflags<xiiDecalChannelMask> m_ChannelMask; ///< Mask of G-Buffer channels that this decal writes to. This allows using the same material resource for both projected and mesh decals, as well as for decals that are packed into an atlas and decals that use individual textures, as the channel mask can be specified per decal instance.

  float    m_fOpacity     = 1.0f; ///< Opacity of the decal, used for blending the decal color with the underlying surface. This is typically used for projected decals, as mesh decals can have their opacity controlled by vertex colors or material parameters, but it can also be used for mesh decals if desired.
  float    m_fNormalBlend = 1.0f; ///< Normal blend factor, used for blending the sampled normal with the underlying surface normal. This is typically used for projected decals, as mesh decals can have their normal blend controlled by vertex colors or material parameters, but it can also be used for mesh decals if desired.
  float    m_fRoughness   = 0.5f; ///< Default roughness value for this decal, used if the material texture does not contain a roughness channel or if this decal is not packed into an atlas. This is typically used for projected decals, as mesh decals can have their roughness controlled by vertex colors or material parameters, but it can also be used for mesh decals if desired.
  float    m_fMetallic    = 0.0f; ///< Default metallic value for this decal, used if the material texture does not contain a metallic channel or if this decal is not packed into an atlas. This is typically used for projected decals, as mesh decals can have their metallic controlled by vertex colors or material parameters, but it can also be used for mesh decals if desired.
  float    m_fEmissive    = 0.0f; ///< Default emissive intensity for this decal, used if the emissive texture does not contain an emissive channel or if this decal is not packed into an atlas. This is typically used for projected decals, as mesh decals can have their emissive intensity controlled by vertex colors or material parameters, but it can also be used for mesh decals if desired.
  xiiUInt8 m_uiPriority   = 128U; ///< Priority for rendering this decal. Higher priority decals are rendered after lower priority ones, which can reduce overdraw and improve performance. This is typically used for projected decals, as mesh decals can have their rendering order controlled by their position in the scene or by material parameters, but it can also be used for mesh decals if desired.

  void Save(xiiStreamWriter& ref_stream) const;
  void Load(xiiStreamReader& ref_stream);
};

/// Decal material resource. Components can override any per-instance parameter after referencing this resource.
class XII_GRAPHICSCORE_DLL xiiDecalResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalResource, xiiResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiDecalResource);

  XII_RESOURCE_DECLARE_CREATEABLE(xiiDecalResource, xiiDecalResourceDescriptor);

public:
  xiiDecalResource();

  /// Returns the descriptor used to create this decal, which contains all the parameters for this decal.
  XII_ALWAYS_INLINE const xiiDecalResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiDecalResourceDescriptor m_Descriptor; ///< Descriptor used to create this decal, which contains all the parameters for this decal.
};
