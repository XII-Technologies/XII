/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Foundation/Math/Transform.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Shaders/Editor/EditorGridConstants.h>

/// Procedural, depth-aware grid composited in display-linear space before presentation.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiInfiniteGridPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInfiniteGridPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputColor{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::RenderTarget};
  xiiRenderPipelineTextureInputPin  m_InputDepth;
  xiiRenderPipelineTextureOutputPin m_OutputColor;
  float                             m_fLineWidth    = 1.0f;
  float                             m_fMinorOpacity = 0.28f;
  float                             m_fMajorOpacity = 0.52f;
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override;

  /// Camera-relative snapshot; computation and storage cost are independent of line count.
  static xiiResult ComputeSettings(const xiiView& view, float spacing, const xiiTransform& transform, bool global, xiiEditorGridConstants& out_settings);
  /// Zero spacing/transform scale disables the view-local grid. Failure also clears stale settings.
  static xiiResult ConfigureView(xiiView& view, float spacing, const xiiTransform& transform = xiiTransform::MakeIdentity(), bool global = true);
};
