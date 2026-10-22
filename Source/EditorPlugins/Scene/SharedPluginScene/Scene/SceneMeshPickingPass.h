/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <SharedPluginScene/SharedPluginSceneDLL.h>

#include <Foundation/Math/Rect.h>
#include <GraphicsCore/Meshes/GpuInstanceTypes.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class xiiSceneMeshPickingState;

/// Captures an explicitly supplied native surface and resolves asynchronous pixels against xiiWorld.
/// Owns no visibility, geometry preparation, rasterization or selected-object rendering.
/// Build and queries require world read access at a CPU frame boundary. No GPU wait or submission.
class XII_SHAREDPLUGINSCENE_DLL xiiSceneMeshPickingPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneMeshPickingPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin    m_Before;
  xiiRenderPipelineOrderOutputPin   m_After;
  xiiRenderPipelineTextureInputPin  m_InputSurface{xiiGALResourceFormat::RGBA32UInt, false, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputSurface{xiiGALResourceFormat::RGBA32UInt, true, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  bool                              m_bEnabled = true;

  xiiSceneMeshPickingPass();
  ~xiiSceneMeshPickingPass() override;

  struct PickingResult
  {
    xiiGpuInstanceSource m_Source;
    xiiVec3              m_vWorldPosition = xiiVec3::MakeZero();
    xiiVec3              m_vWorldNormal   = xiiVec3::MakeZero();
    xiiVec3              m_vRayStart      = xiiVec3::MakeZero();
  };

  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return m_bEnabled ? xiiRenderExtractionCategory::Geometry : xiiRenderExtractionCategory::None; }
  xiiBitflags<xiiRenderPipelineFeature>    GetRuntimeFeatures() const override { return m_bEnabled ? xiiRenderPipelineFeature::GpuScene : xiiRenderPipelineFeature::None; }

  xiiResult Build(xiiRenderPipelinePassContext& context) override;
  void      InvalidateTemporalHistory() override;

  /// Window pixels, including the viewport origin. Outputs are cleared first. Success with an
  /// invalid owner means background; failure means unavailable/stale capture or invalid input.
  /// Captures may lag rendering. Validates captured owners, component configuration and transform.
  /// Section identity is full-width; no application picking ID or component pointer is retained.
  xiiResult Pick(const xiiView& view, xiiUInt32 x, xiiUInt32 y, PickingResult& out_hit) const;
  /// Exclusive right/bottom edges. Unique native owners in first visible pixel order.
  xiiResult PickObjectsInRect(const xiiView& view, const xiiRectU32& rectangle, xiiDynamicArray<xiiGameObjectHandle>& out_objects) const;

private:
  xiiUniquePtr<xiiSceneMeshPickingState> m_pState;
};
