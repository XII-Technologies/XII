/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <Foundation/Basics.h>

class xiiQtSearchWidget;
class xiiQtDocumentTreeView;
class xiiSceneDocument;
class xiiScene2Document;
class QStackedWidget;
struct xiiScene2LayerEvent;

class xiiQtScenegraphPanel : public xiiQtDocumentPanel
{
  Q_OBJECT

public:
  xiiQtScenegraphPanel(ads::CDockManager* pDockManager, QWidget* pParent, xiiSceneDocument* pDocument);
  xiiQtScenegraphPanel(ads::CDockManager* pDockManager, QWidget* pParent, xiiScene2Document* pDocument);
  ~xiiQtScenegraphPanel();

private:
  void LayerEventHandler(const xiiScene2LayerEvent& e);
  void LayerLoaded(const xiiUuid& layerGuid);
  void LayerUnloaded(const xiiUuid& layerGuid);
  void ActiveLayerChanged(const xiiUuid& layerGuid);

private:
  xiiSceneDocument*                                  m_pSceneDocument;
  QStackedWidget*                                    m_pStack                = nullptr;
  xiiQtGameObjectWidget*                             m_pMainGameObjectWidget = nullptr;
  xiiEvent<const xiiScene2LayerEvent&>::Unsubscriber m_LayerEventUnsubscriber;
  xiiMap<xiiUuid, xiiQtGameObjectWidget*>            m_LayerWidgets;
};
