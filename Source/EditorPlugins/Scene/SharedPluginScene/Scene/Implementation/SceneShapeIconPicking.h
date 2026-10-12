/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <SharedPluginScene/Scene/SceneShapeIconPass.h>

/// Internal, view-local asynchronous readback. Uses the GAL staging pool; does not submit or wait
/// on GPU work. A resize drains outstanding captures before replacing the pool, bounding retained
/// staging textures across repeated resizes as well as bounding pending work in a steady viewport.
class xiiSceneShapeIconPicking
{
public:
  xiiResult AddCapture(xiiRenderPipelinePassContext& context, xiiRenderGraphTextureHandle texture, xiiArrayPtr<const xiiSceneShapeIconPass::Icon> icons);
  void      Invalidate();
  xiiResult Pick(const xiiView& view, xiiUInt32 x, xiiUInt32 y, xiiSceneShapeIconPass::PickingResult& out_hit);
  xiiResult Marquee(const xiiView& view, const xiiRectU32& rectangle, xiiDynamicArray<xiiSceneShapeIconPass::PickingResult>& out_hits);

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
    Snapshot                                              m_Snapshot;
    xiiDynamicArray<xiiSceneShapeIconPass::PickingResult> m_Hits;
  };
  xiiResult Poll();
  bool      Matches(const xiiView& view) const;
  bool      Resolve(const xiiView& view, xiiUInt32 index, xiiSceneShapeIconPass::PickingResult& out_hit) const;

  Slot                                                  m_Slots[3];
  xiiUInt64                                             m_uiEpoch = 1;
  bool                                                  m_bValid  = false;
  Snapshot                                              m_Completed;
  xiiSizeU32                                            m_Size = {0, 0};
  xiiDynamicArray<xiiUInt32>                            m_Pixels;
  xiiDynamicArray<xiiSceneShapeIconPass::PickingResult> m_Hits;
  xiiUniquePtr<xiiGALTextureReadback>                   m_pReadback;
  xiiSharedPtr<xiiGALCommandList>                       m_pMapCommands;
};
