#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtCollectionAssetDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtCollectionAssetDocumentWindow(xiiDocument* pDocument);
  ~xiiQtCollectionAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "CollectionAsset"; }
};
