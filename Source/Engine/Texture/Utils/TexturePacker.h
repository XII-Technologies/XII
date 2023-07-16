#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec2.h>
#include <Texture/TextureDLL.h>

class XII_TEXTURE_DLL xiiTexturePacker
{
public:
  struct Texture
  {
    XII_DECLARE_POD_TYPE();

    xiiVec2U32 m_Size;
    xiiVec2U32 m_Position;
    xiiInt32   m_Priority = 0;
  };

  xiiTexturePacker()  = default;
  ~xiiTexturePacker() = default;

  void SetTextureSize(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiReserveTextures = 0);

  void AddTexture(xiiUInt32 uiWidth, xiiUInt32 uiHeight);

  const xiiDynamicArray<Texture>& GetTextures() const { return m_Textures; }

  xiiResult PackTextures();

private:
  bool      CanPlaceAt(xiiVec2U32 pos, xiiVec2U32 size);
  bool      TryPlaceAt(xiiVec2U32 pos, xiiVec2U32 size);
  xiiUInt32 PosToIndex(xiiUInt32 x, xiiUInt32 y) const;
  bool      TryPlaceTexture(xiiUInt32 idx);

  xiiUInt32 m_uiWidth  = 0;
  xiiUInt32 m_uiHeight = 0;

  xiiDynamicArray<Texture> m_Textures;
  xiiDynamicArray<bool>    m_Grid;
};
