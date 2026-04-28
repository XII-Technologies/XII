/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiDecalAssetDocument;
class xiiQtOrbitCamViewWidget;

class xiiQtDecalAssetDocumentWindow : public xiiQtEngineDocumentWindow
{
  Q_OBJECT

public:
  xiiQtDecalAssetDocumentWindow(xiiDecalAssetDocument* pDocument);

  virtual xiiStringView GetWindowLayoutGroupName() const override { return "DecalAsset"; }

private:
  virtual void InternalRedraw() override;
  void         SendRedrawMsg();

  xiiEngineViewConfig      m_ViewConfig;
  xiiQtOrbitCamViewWidget* m_pViewWidget;
};
