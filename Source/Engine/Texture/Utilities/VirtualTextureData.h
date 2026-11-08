/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/RefCounted.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <Texture/TextureDLL.h>

class xiiImageView;
class xiiStreamReader;
class xiiStreamWriter;

struct XII_TEXTURE_DLL xiiVirtualTextureTile
{
  XII_DECLARE_POD_TYPE();
  xiiUInt32 m_uiMip = 0U, m_uiX = 0U, m_uiY = 0U, m_uiOffset = 0U, m_uiStoredSize = 0U;
  xiiResult Serialize(xiiStreamWriter& stream) const;
  xiiResult Deserialize(xiiStreamReader& stream);
};

/// Compiled independently addressable, bordered texture pages. The coarse mip is
/// also a page, and every virtual address has a resident ancestor.
class XII_TEXTURE_DLL xiiVirtualTextureData : public xiiRefCounted
{
public:
  static constexpr xiiUInt32 s_uiMarker       = 0x58565431U;
  static constexpr xiiUInt32 s_uiTileSize     = 128U;
  static constexpr xiiUInt32 s_uiBorder       = 2U;
  static constexpr xiiUInt32 s_uiPhysicalSize = s_uiTileSize + 2U * s_uiBorder;
  static constexpr xiiUInt32 s_uiTileBytes    = s_uiPhysicalSize * s_uiPhysicalSize * 4U;

  xiiGALTextureCreationDescription       m_Description;
  xiiString                              m_sFile;
  xiiUInt64                              m_uiFileOffset = 0U, m_uiFileHash = 0U, m_uiPayloadBytes = 0U;
  xiiUInt16                              m_uiAssetVersion = 0U;
  xiiUInt32                              m_uiAddressFlags = 0U;
  xiiDynamicArray<xiiVirtualTextureTile> m_Tiles;
  xiiDynamicArray<xiiUInt8>              m_Data;

  [[nodiscard]] xiiResult Build(const xiiImageView& image, const xiiGALSamplerCreationDescription& sampler);
  [[nodiscard]] xiiResult Serialize(xiiStreamWriter& stream) const;
  [[nodiscard]] xiiResult Deserialize(xiiStreamReader& stream, xiiStringView sFile = {}, xiiUInt64 uiFileOffset = 0U);
  [[nodiscard]] xiiResult ReadTile(xiiUInt32 tile, xiiDynamicArray<xiiUInt8>& out_data) const;
};
