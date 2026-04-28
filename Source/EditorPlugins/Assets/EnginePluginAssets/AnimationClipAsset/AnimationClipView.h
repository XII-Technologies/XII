/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class xiiAnimationClipContext;

class xiiAnimationClipViewContext : public xiiEngineProcessViewContext
{
public:
  xiiAnimationClipViewContext(xiiAnimationClipContext* pContext);
  ~xiiAnimationClipViewContext();

  bool UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds);

protected:
  virtual xiiViewHandle CreateView() override;
  virtual void          SetCamera(const xiiViewRedrawMsgToEngine* pMsg) override;

  xiiAnimationClipContext* m_pContext = nullptr;
};
