#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class xiiView;
class xiiViewRedrawMsgToEngine;
class xiiEngineProcessDocumentContext;
class xiiEditorEngineDocumentMsg;
class xiiSelectedObjectsExtractorBase;
class xiiSceneContext;
using xiiRenderPipelineResourceHandle = xiiTypedResourceHandle<class xiiRenderPipelineResource>;
class xiiViewMarqueePickingMsgToEngine;

struct ObjectData
{
  xiiMat4 m_ModelView;
  float   m_PickingID[4];
};

class xiiSceneViewContext : public xiiEngineProcessViewContext
{
public:
  xiiSceneViewContext(xiiSceneContext* pSceneContext);
  ~xiiSceneViewContext();

  virtual void HandleViewMessage(const xiiEditorEngineViewMsg* pMsg) override;
  virtual void SetupRenderTarget(xiiGALSwapChainHandle hSwapChain, const xiiGALRenderTargets* pRenderTargets, xiiUInt16 uiWidth, xiiUInt16 uiHeight) override;

  bool UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds);
  void SetInvisibleLayerTags(const xiiArrayPtr<xiiTag> removeTags, const xiiArrayPtr<xiiTag> addTags);

protected:
  virtual void          Redraw(bool bRenderEditorGizmos) override;
  virtual void          SetCamera(const xiiViewRedrawMsgToEngine* pMsg) override;
  virtual xiiViewHandle CreateView() override;

  void PickObjectAt(xiiUInt16 x, xiiUInt16 y);
  void MarqueePickObjects(const xiiViewMarqueePickingMsgToEngine* pMsg);

private:
  xiiSceneContext* m_pSceneContext = nullptr;

  bool m_bUpdatePickingData;

  xiiCamera m_CullingCamera;
};
