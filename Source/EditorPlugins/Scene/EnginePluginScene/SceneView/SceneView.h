/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <SharedPluginScene/Scene/SceneCameraPreview.h>

class xiiView;
class xiiViewRedrawMsgToEngine;
class xiiEngineProcessDocumentContext;
class xiiEditorEngineDocumentMsg;
class xiiEditorRenderPass;
class xiiSceneContext;
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
  virtual void SetupRenderTarget(xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiGALTextureView* pRenderTargetView, xiiUInt16 uiWidth, xiiUInt16 uiHeight) override;
  void         SuspendRendering() override;

  bool UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds);
  void SetInvisibleLayerTags(const xiiArrayPtr<xiiTag> removeTags, const xiiArrayPtr<xiiTag> addTags);

protected:
  virtual void          Redraw(bool bRenderEditorGizmos) override;
  virtual void          SetCamera(const xiiViewRedrawMsgToEngine* pMsg) override;
  virtual xiiViewHandle CreateView() override;

  void PickObjectAt(xiiUInt16 x, xiiUInt16 y);
  void MarqueePickObjects(const xiiViewMarqueePickingMsgToEngine* pMsg);

private:
  void UpdateCameraPreview(bool bRenderEditorGizmos);

  xiiSceneContext* m_pSceneContext;

  bool m_bUpdatePickingData;

  xiiCamera             m_CullingCamera;
  xiiSceneCameraPreview m_CameraPreview;
};
