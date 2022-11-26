#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class xiiRmlUiDocumentContext;

class xiiRmlUiViewContext : public xiiEngineProcessViewContext
{
public:
  xiiRmlUiViewContext(xiiRmlUiDocumentContext* pRmlUiContext);
  ~xiiRmlUiViewContext();

  bool UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds);

protected:
  virtual xiiViewHandle CreateView() override;
  virtual void          SetCamera(const xiiViewRedrawMsgToEngine* pMsg) override;

  xiiRmlUiDocumentContext* m_pRmlUiContext;
};
