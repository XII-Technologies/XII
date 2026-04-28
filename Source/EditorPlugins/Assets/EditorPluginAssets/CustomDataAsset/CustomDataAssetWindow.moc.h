/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtCustomDataAssetDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtCustomDataAssetDocumentWindow(xiiDocument* pDocument);

  virtual xiiStringView GetWindowLayoutGroupName() const override { return "CustomDataAsset"; }
};
