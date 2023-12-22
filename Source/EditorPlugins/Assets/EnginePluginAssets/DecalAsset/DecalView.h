#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class xiiDecalContext;

class xiiDecalViewContext : public xiiEngineProcessViewContext
{
public:
  xiiDecalViewContext(xiiDecalContext* pDecalContext);
  ~xiiDecalViewContext();

protected:
  virtual xiiViewHandle CreateView() override;

  xiiDecalContext* m_pDecalContext = nullptr;
};
