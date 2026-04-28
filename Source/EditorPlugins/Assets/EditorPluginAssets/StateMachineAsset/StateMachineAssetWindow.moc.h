/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtStateMachineAssetScene;
class xiiQtNodeView;

class xiiQtStateMachineAssetDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtStateMachineAssetDocumentWindow(xiiDocument* pDocument);
  ~xiiQtStateMachineAssetDocumentWindow();

  virtual xiiStringView GetWindowLayoutGroupName() const override { return "StateMachineAsset"; }

private Q_SLOTS:

private:
  xiiQtStateMachineAssetScene* m_pScene;
  xiiQtNodeView*               m_pView;
};
