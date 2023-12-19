#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtBlackboardTemplateAssetDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtBlackboardTemplateAssetDocumentWindow(xiiDocument* pDocument);
  ~xiiQtBlackboardTemplateAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "BlackboardTemplateAsset"; }

private:
  void UpdatePreview();
  void RestoreResource();

  void PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const xiiDocumentObjectStructureEvent& e);
};
