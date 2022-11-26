#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtSurfaceAssetDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtSurfaceAssetDocumentWindow(xiiDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "SurfaceAsset"; }
};
