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

protected:
  virtual xiiViewHandle CreateView() override;
  virtual void          SetCamera(const xiiViewRedrawMsgToEngine* pMsg) override;

  virtual void HandleViewMessage(const xiiEditorEngineViewMsg* pMsg) override;

  void PickObjectAt(xiiUInt16 x, xiiUInt16 y, xiiUInt64 uiRequestID);

  xiiSkeletonContext* m_pContext = nullptr;
};
