/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class xiiSkeletonContext;

class xiiSkeletonViewContext : public xiiEngineProcessViewContext
{
public:
  xiiSkeletonViewContext(xiiSkeletonContext* pContext);
  ~xiiSkeletonViewContext();

  bool UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds);

  virtual void Redraw(bool bRenderEditorGizmos) override;

protected:
  virtual xiiViewHandle CreateView() override;
  virtual void          SetCamera(const xiiViewRedrawMsgToEngine* pMsg) override;

  virtual void HandleViewMessage(const xiiEditorEngineViewMsg* pMsg) override;

  void PickObjectAt(xiiUInt16 x, xiiUInt16 y);

  xiiSkeletonContext* m_pContext = nullptr;
};
