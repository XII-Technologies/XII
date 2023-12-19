#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

#include <QPointer>

class xiiQtOrbitCamViewWidget;

class xiiQtAnimatedMeshAssetDocumentWindow : public xiiQtEngineDocumentWindow
{
  Q_OBJECT

public:
  xiiQtAnimatedMeshAssetDocumentWindow(xiiAnimatedMeshAssetDocument* pDocument);
  ~xiiQtAnimatedMeshAssetDocumentWindow();

  xiiAnimatedMeshAssetDocument* GetMeshDocument();
  virtual const char*           GetWindowLayoutGroupName() const override { return "AnimatedMeshAsset"; }

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
