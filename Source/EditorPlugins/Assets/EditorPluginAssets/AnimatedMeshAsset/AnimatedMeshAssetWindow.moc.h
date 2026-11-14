/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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
  ~xiiQtAnimatedMeshAssetDocumentWindow() override;

  xiiAnimatedMeshAssetDocument* GetMeshDocument();
  xiiStringView                 GetWindowLayoutGroupName() const override { return "AnimatedMeshAsset"; }

protected:
  void InternalRedraw() override;
  void ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg) override;

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
