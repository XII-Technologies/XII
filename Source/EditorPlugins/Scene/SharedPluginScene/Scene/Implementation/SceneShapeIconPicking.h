/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <SharedPluginScene/Scene/Implementation/ScenePickingReadback.h>
#include <SharedPluginScene/Scene/SceneShapeIconPass.h>

/// Shape-icon identity validation and native metadata snapshots. Pixel transport and view/camera
/// invalidation are shared with native mesh picking; neither path submits or waits on GPU work.
class xiiSceneShapeIconPicking
{
public:
  xiiResult AddCapture(xiiRenderPipelinePassContext& context, xiiRenderGraphTextureHandle texture, xiiArrayPtr<const xiiSceneShapeIconPass::Icon> icons);
  void      Invalidate();
  xiiResult Pick(const xiiView& view, xiiUInt32 x, xiiUInt32 y, xiiSceneShapeIconPass::PickingResult& out_hit);
  xiiResult Marquee(const xiiView& view, const xiiRectU32& rectangle, xiiDynamicArray<xiiSceneShapeIconPass::PickingResult>& out_hits);

private:
  xiiResult Poll();
  bool      Resolve(const xiiView& view, xiiUInt32 index, xiiSceneShapeIconPass::PickingResult& out_hit) const;

  xiiScenePickingReadback                               m_Readback;
  xiiDynamicArray<xiiSceneShapeIconPass::PickingResult> m_SlotHits[xiiScenePickingReadback::SlotCount];
  xiiDynamicArray<xiiSceneShapeIconPass::PickingResult> m_Hits;
};
