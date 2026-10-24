/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Shaders/Editor/EditorGizmoData.h>

/// Batched, shader-expanded Editor handles with private antialiased color/depth targets.
/// Immutable primitive buffers are cached by this view's pass instance and retained by graph imports.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEditorGizmoPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorGizmoPass, xiiRenderPipelinePass);

public:
  xiiEditorGizmoPass();
  ~xiiEditorGizmoPass();
  xiiRenderPipelineTextureInputPin  m_InputColor{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::RenderTarget};
  xiiRenderPipelineTextureInputPin  m_InputDepth;
  xiiRenderPipelineTextureOutputPin m_OutputColor;
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override;
  /// One owned immutable snapshot per redraw. Empty submission clears stale handles.
  /// Custom mesh paths correspond one-to-one with instances; built-in entries are empty.
  static xiiResult Submit(xiiView& view, xiiArrayPtr<const xiiEditorGizmoInstance> instances, const xiiVariantArray& customMeshes);

private:
  struct State;
  xiiUniquePtr<State> m_pState;
};
