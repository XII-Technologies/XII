#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class xiiTextureCubeContext;

class xiiTextureCubeViewContext : public xiiEngineProcessViewContext
{
public:
  xiiTextureCubeViewContext(xiiTextureCubeContext* pMaterialContext);
  ~xiiTextureCubeViewContext();

protected:
  virtual xiiViewHandle CreateView() override;
  virtual void          SetCamera(const xiiViewRedrawMsgToEngine* pMsg) override;

  xiiTextureCubeContext* m_pTextureContext;
};
