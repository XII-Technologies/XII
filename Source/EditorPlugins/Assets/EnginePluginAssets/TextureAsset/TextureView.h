#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class xiiTextureContext;

class xiiTextureViewContext : public xiiEngineProcessViewContext
{
public:
  xiiTextureViewContext(xiiTextureContext* pMaterialContext);
  ~xiiTextureViewContext();

protected:
  virtual xiiViewHandle CreateView() override;
  virtual void          SetCamera(const xiiViewRedrawMsgToEngine* pMsg) override;

  xiiTextureContext* m_pTextureContext;
};
