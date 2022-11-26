#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class xiiMaterialContext;

class xiiMaterialViewContext : public xiiEngineProcessViewContext
{
public:
  xiiMaterialViewContext(xiiMaterialContext* pMaterialContext);
  ~xiiMaterialViewContext();

  void PositionThumbnailCamera();

protected:
  virtual xiiViewHandle CreateView() override;

  xiiMaterialContext* m_pMaterialContext;
};
