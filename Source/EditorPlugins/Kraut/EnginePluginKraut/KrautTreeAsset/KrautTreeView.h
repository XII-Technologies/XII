#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class xiiKrautTreeContext;

class xiiKrautTreeViewContext : public xiiEngineProcessViewContext
{
public:
  xiiKrautTreeViewContext(xiiKrautTreeContext* pKrautTreeContext);
  ~xiiKrautTreeViewContext();

  bool UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds);

protected:
  virtual xiiViewHandle CreateView() override;
  virtual void          SetCamera(const xiiViewRedrawMsgToEngine* pMsg) override;

  xiiKrautTreeContext* m_pKrautTreeContext;
};
