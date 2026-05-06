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

/// \brief G-Buffer channels a decal may write.
struct XII_GRAPHICSCORE_DLL xiiDecalChannelMask
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    None     = 0U,
    Albedo   = XII_BIT(0),
    Normal   = XII_BIT(1),
    Material = XII_BIT(2),
    Emissive = XII_BIT(3),

    All = Albedo | Normal | Material | Emissive,

    Default = Albedo | Normal | Material
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

/// \brief Renderer-facing decal classification.
struct XII_GRAPHICSCORE_DLL xiiDecalProjectionMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Projected, ///< Deferred projected decal volume reconstructed from depth.
    Mesh,      ///< Pre-baked mesh decal drawn into the G-Buffer.

    ENUM_COUNT,

    Default = Projected
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDecalProjectionMode);

/// \brief One atlas source entry and its packed UV metadata.
struct XII_GRAPHICSCORE_DLL xiiDecalAtlasEntry
{
  xiiHashedString m_sDecalId;

  xiiTexture2DResourceHandle m_hAlbedo;
  xiiTexture2DResourceHandle m_hNormal;
  xiiTexture2DResourceHandle m_hMaterial;
  xiiTexture2DResourceHandle m_hEmissive;

  xiiUInt16 m_uiWidth  = 256U;
  xiiUInt16 m_uiHeight = 256U;
  xiiUInt8  m_uiPadding = 2U;
  xiiUInt8  m_uiPriority = 128U;

  xiiBitflags<xiiDecalChannelMask> m_ChannelMask = xiiDecalChannelMask::Default;

  /// \brief xy = UV minimum, zw = UV size inside the packed atlas.
  xiiVec4 m_vUVRect = xiiVec4(0.0f, 0.0f, 1.0f, 1.0f);

  /// \brief xy = texel size, zw = packed pixel size.
  xiiVec4 m_vTextureMetrics = xiiVec4(1.0f / 256.0f, 1.0f / 256.0f, 256.0f, 256.0f);
};

/// \brief Descriptor used to build a packed decal atlas resource at runtime or by tools.
struct XII_GRAPHICSCORE_DLL xiiDecalAtlasResourceDescriptor
{
  xiiUInt16 m_uiAtlasWidth  = 2048U;
  xiiUInt16 m_uiAtlasHeight = 2048U;
  xiiUInt8  m_uiPadding     = 2U;
  bool      m_bGenerateMipMaps = true;

  xiiDynamicArray<xiiDecalAtlasEntry> m_Entries;

  void Save(xiiStreamWriter& ref_stream) const;
  void Load(xiiStreamReader& ref_stream);
};

/// \brief Packed decal atlas with metadata mapping decal IDs to UV rectangles.
class XII_GRAPHICSCORE_DLL xiiDecalAtlasResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalAtlasResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiDecalAtlasResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiDecalAtlasResource, xiiDecalAtlasResourceDescriptor);

public:
  xiiDecalAtlasResource();

  const xiiDecalAtlasResourceDescriptor& GetDescriptor() const { return m_Descriptor; }
  xiiArrayPtr<const xiiDecalAtlasEntry>  GetEntries() const { return m_Descriptor.m_Entries; }

  bool TryGetAtlasEntry(const xiiTempHashedString& sDecalId, const xiiDecalAtlasEntry*& out_pEntry) const;
  bool TryGetAtlasEntry(const xiiHashedString& sDecalId, const xiiDecalAtlasEntry*& out_pEntry) const;

  xiiSharedPtr<xiiGALTexture> GetGALTexture() const { return m_pAtlasTexture; }
  xiiSharedPtr<xiiGALSampler> GetGALSampler() const { return m_pAtlasSampler; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void PackAtlas();
  void CreateGPUAtlas();
  void RebuildLookup();

private:
  xiiDecalAtlasResourceDescriptor m_Descriptor;
  xiiHashTable<xiiHashedString, xiiUInt32> m_IdToEntryIndex;

  xiiSharedPtr<xiiGALTexture> m_pAtlasTexture;
  xiiSharedPtr<xiiGALSampler> m_pAtlasSampler;
  xiiUInt32                   m_uiMemoryGPU = 0U;
};

/// \brief Per-decal material defaults shared by projected and mesh decals.
struct XII_GRAPHICSCORE_DLL xiiDecalResourceDescriptor
{
  xiiHashedString m_sDecalId;

  xiiDecalAtlasResourceHandle m_hAtlas;

  xiiTexture2DResourceHandle m_hAlbedo;
  xiiTexture2DResourceHandle m_hNormal;
  xiiTexture2DResourceHandle m_hMaterial;
  xiiTexture2DResourceHandle m_hEmissive;

  xiiVec2 m_vUVOffset = xiiVec2::MakeZero();
  xiiVec2 m_vUVScale  = xiiVec2(1.0f);
  xiiColor m_Tint     = xiiColor::White;

  xiiBitflags<xiiDecalChannelMask> m_ChannelMask = xiiDecalChannelMask::Default;

  float    m_fOpacity     = 1.0f;
  float    m_fNormalBlend = 1.0f;
  float    m_fRoughness   = 0.5f;
  float    m_fMetallic    = 0.0f;
  float    m_fEmissive    = 0.0f;
  xiiUInt8 m_uiPriority   = 128U;

  void Save(xiiStreamWriter& ref_stream) const;
  void Load(xiiStreamReader& ref_stream);
};

/// \brief Decal material resource. Components can override any per-instance parameter after referencing this resource.
class XII_GRAPHICSCORE_DLL xiiDecalResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiDecalResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiDecalResource, xiiDecalResourceDescriptor);

public:
  xiiDecalResource();

  const xiiDecalResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiDecalResourceDescriptor m_Descriptor;
};
