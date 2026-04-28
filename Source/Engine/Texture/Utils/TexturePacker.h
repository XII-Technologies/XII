/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Vec2.h>
#include <Texture/TextureDLL.h>

struct stbrp_node;
struct stbrp_rect;

class XII_TEXTURE_DLL xiiTexturePacker
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiTexturePacker);

public:
  struct Texture
  {
    XII_DECLARE_POD_TYPE();

    xiiVec2U32 m_Size;
    xiiVec2U32 m_Position;
  };

  xiiTexturePacker();
  ~xiiTexturePacker();

  void SetTextureSize(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiReserveTextures = 0);

  void AddTexture(xiiUInt32 uiWidth, xiiUInt32 uiHeight);

  const xiiDynamicArray<Texture>& GetTextures() const { return m_Textures; }

  xiiResult PackTextures();

private:
  xiiUInt32 m_uiWidth  = 0;
  xiiUInt32 m_uiHeight = 0;

  xiiDynamicArray<Texture> m_Textures;

  xiiDynamicArray<stbrp_node> m_Nodes;
  xiiDynamicArray<stbrp_rect> m_Rects;
};
