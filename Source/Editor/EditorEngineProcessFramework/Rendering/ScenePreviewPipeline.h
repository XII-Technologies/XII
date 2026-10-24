/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

/// Adds Editor aids to the selected scene renderer using its explicit display-linear and depth interfaces.
/// Loading or invalid source resources remain selected so the ordinary resource diagnostics stay observable.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiScenePreviewPipeline
{
public:
  static xiiRenderPipelineResourceHandle GetResource(const xiiRenderPipelineResourceHandle& scene);
};
