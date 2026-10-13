/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/Status.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

/// Image file inputs, independent of Qt documents and the asset curator. Material
/// sources use the existing asset's ORM order; output uses native roughness/metallic/AO.
struct XII_SHAREDPLUGINASSETS_DLL xiiDecalAtlasSource
{
  xiiHashedString                  m_sDecalId;
  xiiString                        m_sAlbedo, m_sNormal, m_sORM, m_sEmissive, m_sAlphaMask;
  xiiBitflags<xiiDecalChannelMask> m_ChannelMask = xiiDecalChannelMask::Albedo;
  xiiUInt8                         m_uiPriority  = 128;
};

struct XII_SHAREDPLUGINASSETS_DLL xiiDecalAtlasBuildSettings
{
  xiiUInt32 m_uiMaxEntrySize   = 1024;
  xiiUInt32 m_uiMaxAtlasSize   = 4096;
  xiiUInt32 m_uiMaxSourceSize  = 4096; ///< Bound the transient float conversion allocation before loading pixels.
  xiiUInt8  m_uiPadding        = 4;
  bool      m_bGenerateMipMaps = true;
};

/// Packs one deterministic layout shared by all four native decal channels.
/// Sources are sorted by full ID, never truncated hashes. Source images are loaded
/// one at a time; channels are packed and encoded one at a time.
/// Failure leaves output unchanged. The renderer currently samples atlas mip zero.
class XII_SHAREDPLUGINASSETS_DLL xiiDecalAtlasBuilder
{
public:
  static xiiStatus Build(xiiArrayPtr<const xiiDecalAtlasSource> sources, const xiiDecalAtlasBuildSettings& settings,
                         xiiDecalAtlasResourceDescriptor& output);
};
