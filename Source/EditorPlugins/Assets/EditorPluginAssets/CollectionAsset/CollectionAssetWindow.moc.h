/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

  virtual xiiStringView GetWindowLayoutGroupName() const override { return "CollectionAsset"; }
};
