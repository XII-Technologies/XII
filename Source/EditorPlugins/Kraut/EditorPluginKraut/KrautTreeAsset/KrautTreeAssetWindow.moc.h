#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtOrbitCamViewWidget;

class xiiQtKrautTreeAssetDocumentWindow : public xiiQtEngineDocumentWindow
{
  Q_OBJECT

public:
  xiiQtKrautTreeAssetDocumentWindow(xiiAssetDocument* pDocument);
  ~xiiQtKrautTreeAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "KrautTreeAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg) override;
  void         PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);

private:
  void SendRedrawMsg();
  void QueryObjectBBox(xiiInt32 iPurpose = 0);

  xiiEngineViewConfig        m_ViewConfig;
  xiiQtOrbitCamViewWidget*   m_pViewWidget;
  xiiKrautTreeAssetDocument* m_pAssetDoc;
};
