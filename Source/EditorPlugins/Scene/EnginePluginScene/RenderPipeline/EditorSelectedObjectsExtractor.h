#pragma once

#include <Core/Graphics/Camera.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsCore/Pipeline/Extractor.h>

class xiiSceneContext;
class xiiCameraComponent;

class xiiEditorSelectedObjectsExtractor : public xiiSelectedObjectsExtractorBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorSelectedObjectsExtractor, xiiSelectedObjectsExtractorBase);

public:
  xiiEditorSelectedObjectsExtractor();
  ~xiiEditorSelectedObjectsExtractor();

  virtual const xiiDeque<xiiGameObjectHandle>* GetSelection() override;

  virtual void      Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  void             SetSceneContext(xiiSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  xiiSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  void CreateRenderTargetTexture(const xiiView& view);
  void CreateRenderTargetView(const xiiView& view);
  void UpdateRenderTargetCamera(const xiiCameraComponent* pCamComp);

  xiiSceneContext*                   m_pSceneContext;
  xiiViewHandle                      m_hRenderTargetView;
  xiiRenderToTexture2DResourceHandle m_hRenderTarget;
  xiiCamera                          m_RenderTargetCamera;
};
