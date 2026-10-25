/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <DearImguiPlugin/DearImguiContext.h>
#include <DearImguiPlugin/DearImguiRenderer.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// One immutable CPU texture version. Its synchronized GAL cache is populated during graph setup.
/// Font updates replace the version rather than overwriting an image referenced by outstanding GPU frames.
class xiiImguiTexture : public xiiRefCounted
{
public:
  xiiSharedPtr<xiiGALTexture> GetTexture();
  xiiDynamicArray<xiiUInt8>   m_Pixels;
  xiiSizeU32                  m_Size;
  xiiSharedPtr<xiiGALTexture> m_pTexture;
  xiiMutex                    m_Mutex;
};

class xiiImguiContext
{
public:
  xiiImguiContext();
  ~xiiImguiContext();
  xiiResult   Snapshot(xiiSharedPtr<const xiiImguiFrame>& out_frame);
  xiiResult   UpdateTextures();
  ImTextureID AddTexture(xiiSharedPtr<xiiImguiTexture> pTexture);
  xiiResult   RemoveTexture(ImTextureID id, bool bFontTexture = false);

  struct TextureEntry
  {
    ImTextureID                   m_Id = ImTextureID_Invalid;
    xiiSharedPtr<xiiImguiTexture> m_pTexture;
    bool                          m_bFont = false;
  };
  xiiDynamicArray<TextureEntry> m_Textures;
  ImGuiContext*                 m_pContext      = nullptr;
  xiiUInt32                     m_uiScopes      = 0;
  bool                          m_bFrameActive  = false;
  bool                          m_bInputEnabled = true;
};
