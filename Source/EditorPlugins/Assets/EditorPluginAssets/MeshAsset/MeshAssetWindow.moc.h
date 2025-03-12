#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

#include <QPointer>

class xiiQtOrbitCamViewWidget;

class xiiQtMeshAssetDocumentWindow : public xiiQtEngineDocumentWindow
{
  Q_OBJECT

public:
  xiiQtMeshAssetDocumentWindow(xiiMeshAssetDocument* pDocument);
  ~xiiQtMeshAssetDocumentWindow();

  xiiMeshAssetDocument* GetMeshDocument();
  virtual xiiStringView GetWindowLayoutGroupName() const override { return "MeshAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg) override;

protected Q_SLOTS:
  void HighlightTimer();

private:
  void SendRedrawMsg();
  void QueryObjectBBox(xiiInt32 iPurpose = 0);
  void PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  bool UpdatePreview();

  xiiEngineViewConfig      m_ViewConfig;
  xiiQtOrbitCamViewWidget* m_pViewWidget;
  xiiUInt32                m_uiHighlightSlots = 0;
  QPointer<QTimer>         m_pHighlightTimer;
};
