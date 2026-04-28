/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class xiiAnimatedMeshContext;

class xiiAnimatedMeshViewContext : public xiiEngineProcessViewContext
{
public:
  xiiAnimatedMeshViewContext(xiiAnimatedMeshContext* pMeshContext);
  ~xiiAnimatedMeshViewContext();

  bool UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds);

protected:
  virtual xiiViewHandle CreateView() override;
  virtual void          SetCamera(const xiiViewRedrawMsgToEngine* pMsg) override;

  xiiAnimatedMeshContext* m_pContext = nullptr;
};
