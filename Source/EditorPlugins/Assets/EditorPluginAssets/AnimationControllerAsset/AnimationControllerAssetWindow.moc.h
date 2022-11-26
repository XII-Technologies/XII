#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtAnimationControllerAssetScene;
class xiiQtNodeView;

class xiiQtAnimationControllerAssetDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtAnimationControllerAssetDocumentWindow(xiiDocument* pDocument);
  ~xiiQtAnimationControllerAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "AnimationControllerAsset"; }

private Q_SLOTS:

private:
  xiiQtAnimationControllerAssetScene* m_pScene;
  xiiQtNodeView*                      m_pView;
};
