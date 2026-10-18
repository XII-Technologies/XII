/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

/// Filters the current view's world-derived candidate indices. No visibility or raster execution.
/// Editor.SelectedObjects contains full game-object handle IDs; it is never a renderer registry.
class XII_SHAREDPLUGINSCENE_DLL xiiSceneInstanceSelectionPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneInstanceSelectionPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin         m_Candidates{false, xiiGALBindFlags::ShaderResource};
  bool                                     m_bEnabled         = true;
  bool                                     m_bSelectionOnly   = false;
  bool                                     m_bPickSelected    = true;
  bool                                     m_bPickTransparent = true;
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return xiiRenderExtractionCategory::Geometry; }
  xiiBitflags<xiiRenderPipelineFeature>    GetRuntimeFeatures() const override { return xiiRenderPipelineFeature::GpuScene; }
};
