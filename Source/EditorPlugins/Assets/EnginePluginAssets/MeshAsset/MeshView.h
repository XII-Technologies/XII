#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class xiiMeshContext;

class xiiMeshViewContext : public xiiEngineProcessViewContext
{
public:
  xiiMeshViewContext(xiiMeshContext* pMeshContext);
  ~xiiMeshViewContext();

  bool UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds);

protected:
  virtual xiiViewHandle CreateView() override;
  virtual void          SetCamera(const xiiViewRedrawMsgToEngine* pMsg) override;

  xiiMeshContext* m_pContext = nullptr;
};
