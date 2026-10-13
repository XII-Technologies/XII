/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Rect.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Scene/SceneMeshSource.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

class xiiSceneMeshPickingState;

/// Resource-defined, view-local nearest mesh queries using the Engine's native visibility and
/// material rasterization. Consumes PrepareFrame's geometry/material buffer pins and frame-table
/// blackboard keys. Owns no mesh objects; selected-object filtering uses an isolated scene context.
/// Editor.Picking, Editor.PickSelected and Editor.PickTransparent default to true.
/// Editor.SelectedObjects is a VariantArray of UInt64 native game-object handle IDs.
/// Build and queries require world read access at a CPU frame boundary. No GPU wait or submission.
class XII_SHAREDPLUGINSCENE_DLL xiiSceneMeshPickingPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneMeshPickingPass, xiiRenderPipelinePass);

public:
  xiiSceneMeshPickingPass();
  ~xiiSceneMeshPickingPass() override;

  struct PickingResult
  {
    xiiSceneMeshSource m_Source;
    xiiVec3            m_vWorldPosition = xiiVec3::MakeZero();
    xiiVec3            m_vWorldNormal   = xiiVec3::MakeZero();
    xiiVec3            m_vRayStart      = xiiVec3::MakeZero();
  };

  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
  void      InvalidateTemporalHistory() override;

  /// Window pixels, including the viewport origin. Outputs are cleared first. Success with an
  /// invalid owner means background; failure means unavailable/stale capture or invalid input.
  /// Captures may lag rendering. Validates captured owners, component configuration and transform.
  /// Section identity is full-width; no application picking ID or component pointer is retained.
  xiiResult Pick(const xiiView& view, xiiUInt32 x, xiiUInt32 y, PickingResult& out_hit) const;
  /// Exclusive right/bottom edges. Unique native owners in first visible pixel order.
  xiiResult PickObjectsInRect(const xiiView& view, const xiiRectU32& rectangle, xiiDynamicArray<xiiGameObjectHandle>& out_objects) const;

  /// A selected-only surface producer for selection compositing. Uses Editor.SelectionOverlay
  /// and Editor.SelectedObjects, emits no CPU captures, and does not provide Pick queries.
  bool m_bSelectionOnly = false;

  /// Prefer mesh shading when available. False selects indexed vertex pulling on all devices.
  bool m_bUseMeshShaders = true;

private:
  xiiUniquePtr<xiiSceneMeshPickingState> m_pState;
};
