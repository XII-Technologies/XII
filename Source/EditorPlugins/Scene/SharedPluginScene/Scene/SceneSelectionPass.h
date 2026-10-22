/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <SharedPluginScene/SharedPluginSceneDLL.h>

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Composites selected mesh surfaces onto display-linear scene color. SelectionSurface is the
/// native RGBA32UInt nearest-surface output from a selected-only scene mesh query. Comparing its
/// reversed depth with SceneDepth distinguishes visible and occluded outlines, including objects
/// fully behind other geometry. Editor.SelectionOverlay controls the pass. No CPU readback.
class XII_SHAREDPLUGINSCENE_DLL xiiSceneSelectionPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneSelectionPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin    m_Before;
  xiiRenderPipelineOrderOutputPin   m_After;
  xiiRenderPipelineTextureInputPin  m_InputColor;
  xiiRenderPipelineTextureInputPin  m_InputSceneDepth;
  xiiRenderPipelineTextureInputPin  m_InputSelectionSurface{xiiGALResourceFormat::RGBA32UInt, true};
  xiiRenderPipelineTextureOutputPin m_OutputColor;


  xiiResult Build(xiiRenderPipelinePassContext& context) override;

  xiiColor  m_VisibleColor   = xiiColor(1.0f, 0.5f, 0.0f, 1.0f);
  xiiColor  m_OccludedColor  = xiiColor(0.2f, 0.4f, 1.0f, 0.5f);
  float     m_fFillOpacity   = 0.1f;
  xiiUInt32 m_uiOutlineWidth = 1;
};
