/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

/// Internal view-local capture transport shared by scene picking consumers. It owns only pixel
/// and view snapshots; callers retain the matching native owner metadata in three slot tables.
/// CPU frame-boundary access only. No submission or GPU wait occurs in this facility.
class XII_SHAREDPLUGINSCENE_DLL xiiScenePickingReadback
{
public:
  static constexpr xiiUInt32 SlotCount = 3;
  /// Publishes the newest completed image and returns its slot, or InvalidIndex when unchanged.
  /// Consume/swap that slot's metadata before AddCapture can reserve a slot again.
  xiiResult Poll(xiiUInt32& out_uiCompletedSlot);
  /// Requires Poll first. Supports R32UInt and RGBA32UInt, both single-sample 2D textures.
  /// Success with InvalidIndex means backpressure or a resize draining old captures. At most
  /// three captures are retained, including during repeated resizing and format changes.
  xiiResult AddCapture(xiiRenderPipelinePassContext& context, xiiRenderGraphTextureHandle texture, xiiUInt32& out_uiReservedSlot);
  void      Invalidate();

  bool                         Matches(const xiiView& view) const;
  xiiSizeU32                   GetSize() const { return m_Size; }
  xiiUInt32                    GetWordsPerPixel() const { return m_uiWordsPerPixel; }
  xiiArrayPtr<const xiiUInt32> GetPixels() const { return m_Pixels; }

private:
  struct Snapshot
  {
    const xiiWorld*                 m_pWorld = nullptr;
    xiiViewHandle                   m_hView;
    xiiSharedPtr<xiiGALTextureView> m_pTarget;
    const xiiGALSwapChain*          m_pSwapChain     = nullptr;
    xiiMat4                         m_ViewProjection = xiiMat4::MakeIdentity();
    xiiRectFloat                    m_Viewport;
    xiiUInt64                       m_uiEpoch = 0;
  };
  struct Slot
  {
    enum State
    {
      Free,
      Reserved,
      Pending
    } m_State = Free;
    Snapshot  m_Snapshot;
    xiiUInt64 m_uiSetupFrameIndex = 0;
  };

  Slot                                m_Slots[SlotCount];
  xiiUInt64                           m_uiEpoch = 1;
  bool                                m_bValid  = false;
  Snapshot                            m_Completed;
  xiiSizeU32                          m_Size            = {0, 0};
  xiiUInt32                           m_uiWordsPerPixel = 0;
  xiiDynamicArray<xiiUInt32>          m_Pixels;
  xiiUniquePtr<xiiGALTextureReadback> m_pReadback;
  xiiSharedPtr<xiiGALCommandList>     m_pMapCommands;
};
