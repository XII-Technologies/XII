#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <GraphicsCore/Pipeline/Renderer.h>

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiGizmoRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGizmoRenderer, xiiRenderer);

public:
  xiiGizmoRenderer();
  ~xiiGizmoRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& inout_types) const override;
  virtual void GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& inout_categories) const override;
  virtual void UpdateBatch(const xiiRenderViewContext& renderContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) override;
  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;

  static float s_fGizmoScale;
};
