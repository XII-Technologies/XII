#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class xiiJoltCollisionMeshContext;

class xiiJoltCollisionMeshViewContext : public xiiEngineProcessViewContext
{
public:
  xiiJoltCollisionMeshViewContext(xiiJoltCollisionMeshContext* pMeshContext);
  ~xiiJoltCollisionMeshViewContext();

  bool UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds);

protected:
  virtual xiiViewHandle CreateView() override;
  virtual void          SetCamera(const xiiViewRedrawMsgToEngine* pMsg) override;

  xiiJoltCollisionMeshContext* m_pContext = nullptr;
};
