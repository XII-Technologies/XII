#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtOrbitCamViewWidget;
class xiiSelectionContext;

class xiiQtSkeletonAssetDocumentWindow : public xiiQtEngineDocumentWindow
{
  Q_OBJECT

public:
  xiiQtSkeletonAssetDocumentWindow(xiiSkeletonAssetDocument* pDocument);
  ~xiiQtSkeletonAssetDocumentWindow();

  xiiSkeletonAssetDocument* GetSkeletonDocument();
  virtual const char*       GetWindowLayoutGroupName() const override { return "SkeletonAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg) override;

private:
  void SendRedrawMsg();
  void QueryObjectBBox(xiiInt32 iPurpose = 0);
  void SelectionEventHandler(const xiiSelectionManagerEvent& e);
  void SkeletonAssetEventHandler(const xiiSkeletonAssetEvent& e);

  void PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void CommandEventHandler(const xiiCommandHistoryEvent&);

  void SendLiveResourcePreview();
  void RestoreResource();

  xiiEngineViewConfig      m_ViewConfig;
  xiiQtOrbitCamViewWidget* m_pViewWidget = nullptr;
};
