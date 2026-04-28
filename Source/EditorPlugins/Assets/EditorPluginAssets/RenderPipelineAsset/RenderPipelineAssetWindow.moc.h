/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtRenderPipelineAssetScene;
class xiiQtNodeView;

class xiiQtRenderPipelineAssetDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtRenderPipelineAssetDocumentWindow(xiiDocument* pDocument);
  ~xiiQtRenderPipelineAssetDocumentWindow();

  virtual xiiStringView GetWindowLayoutGroupName() const override { return "RenderPipelineAsset"; }

private Q_SLOTS:

private:
  xiiQtRenderPipelineAssetScene* m_pScene;
  xiiQtNodeView*                 m_pView;
};
