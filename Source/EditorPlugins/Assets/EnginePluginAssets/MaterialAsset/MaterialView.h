/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class xiiMaterialContext;

class xiiMaterialViewContext : public xiiEngineProcessViewContext
{
public:
  xiiMaterialViewContext(xiiMaterialContext* pMaterialContext);
  ~xiiMaterialViewContext();

  void PositionThumbnailCamera();
  void Redraw(bool bRenderEditorGizmos) override;

protected:
  virtual xiiViewHandle CreateView() override;

  xiiMaterialContext* m_pMaterialContext;
};
