/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtVisualScriptNodeScene;
class xiiQtNodeView;

class xiiQtVisualScriptWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtVisualScriptWindow(xiiDocument* pDocument);
  ~xiiQtVisualScriptWindow();

  virtual xiiStringView GetWindowLayoutGroupName() const override { return "VisualScriptGraph"; }

private Q_SLOTS:

private:
  void SelectionEventHandler(const xiiSelectionManagerEvent& e);

  xiiQtVisualScriptNodeScene* m_pScene;
  xiiQtNodeView*              m_pView;
};
