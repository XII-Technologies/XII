#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtAnimationGraphAssetScene;
class xiiQtNodeView;

class xiiQtAnimationGraphAssetDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtAnimationGraphAssetDocumentWindow(xiiDocument* pDocument);
  ~xiiQtAnimationGraphAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "AnimationGraphAsset"; }

private Q_SLOTS:

private:
  void SelectionEventHandler(const xiiSelectionManagerEvent& e);

  xiiQtAnimationGraphAssetScene* m_pScene;
  xiiQtNodeView*                 m_pView;
};
