#pragma once

#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiProcGenGraphAssetDocument;

class xiiQtNodeScene;
class xiiQtNodeView;
struct xiiCommandHistoryEvent;

class xiiProcGenGraphAssetDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiProcGenGraphAssetDocumentWindow(xiiProcGenGraphAssetDocument* pDocument);
  ~xiiProcGenGraphAssetDocumentWindow();

  xiiProcGenGraphAssetDocument* GetProcGenGraphDocument();

  virtual const char* GetWindowLayoutGroupName() const override { return "ProcGenAsset"; }

private Q_SLOTS:


private:
  void UpdatePreview();
  void RestoreResource();

  // needed for setting the debug pin
  void PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void TransationEventHandler(const xiiCommandHistoryEvent& e);

  xiiQtNodeScene* m_pScene;
  xiiQtNodeView*  m_pView;
};
