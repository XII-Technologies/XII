#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>

#include <QLayout>
#include <QStackedWidget>

namespace
{
  std::unique_ptr<xiiQtDocumentTreeModel> CreateGameObjectTreeModel(xiiSceneDocument* pDocument)
  {
    std::unique_ptr<xiiQtDocumentTreeModel> pModel(new xiiQtScenegraphModel(pDocument->GetObjectManager()));
    pModel->AddAdapter(new xiiQtDummyAdapter(pDocument->GetObjectManager(), xiiGetStaticRTTI<xiiDocumentRoot>(), "Children"));
    pModel->AddAdapter(new xiiQtGameObjectAdapter(pDocument->GetObjectManager()));
    return std::move(pModel);
  }

  std::unique_ptr<xiiQtDocumentTreeModel> CreateSceneTreeModel(xiiScene2Document* pDocument)
  {
    std::unique_ptr<xiiQtDocumentTreeModel> pModel(new xiiQtScenegraphModel(pDocument->GetSceneObjectManager()));
    pModel->AddAdapter(new xiiQtDummyAdapter(pDocument->GetSceneObjectManager(), xiiGetStaticRTTI<xiiDocumentRoot>(), "Children"));
    pModel->AddAdapter(new xiiQtGameObjectAdapter(pDocument->GetSceneObjectManager(), pDocument->GetSceneDocumentObjectMetaData(), pDocument->GetSceneGameObjectMetaData()));
    return std::move(pModel);
  }
} // namespace

xiiQtScenegraphPanel::xiiQtScenegraphPanel(QWidget* pParent, xiiSceneDocument* pDocument) :
  xiiQtDocumentPanel(pParent, pDocument)
{
  setObjectName("xiiQtScenegraphPanel");
  setWindowTitle("Scenegraph");
  m_pSceneDocument = pDocument;

  m_pStack = new QStackedWidget(this);
  m_pStack->setObjectName("QStackedWidget");
  m_pStack->setContentsMargins(0, 0, 0, 0);
  m_pStack->layout()->setContentsMargins(0, 0, 0, 0);
  setWidget(m_pStack);

  auto pCustomModel       = CreateGameObjectTreeModel(pDocument);
  m_pMainGameObjectWidget = new xiiQtGameObjectWidget(this, pDocument, "EditorPluginScene_ScenegraphContextMenu", std::move(pCustomModel));
  m_pStack->addWidget(m_pMainGameObjectWidget);
}

xiiQtScenegraphPanel::xiiQtScenegraphPanel(QWidget* pParent, xiiScene2Document* pDocument) :
  xiiQtDocumentPanel(pParent, pDocument)
{
  setObjectName("xiiQtScenegraphPanel");
  setWindowTitle("Scenegraph");
  m_pSceneDocument = pDocument;

  m_pStack = new QStackedWidget(this);
  m_pStack->setObjectName("QStackedWidget");
  m_pStack->setContentsMargins(0, 0, 0, 0);
  m_pStack->layout()->setContentsMargins(0, 0, 0, 0);
  setWidget(m_pStack);

  auto pCustomModel                    = CreateSceneTreeModel(pDocument);
  m_pMainGameObjectWidget              = new xiiQtGameObjectWidget(this, pDocument, "EditorPluginScene_ScenegraphContextMenu", std::move(pCustomModel), pDocument->GetSceneSelectionManager());
  m_LayerWidgets[pDocument->GetGuid()] = m_pMainGameObjectWidget;
  m_pStack->addWidget(m_pMainGameObjectWidget);

  pDocument->m_LayerEvents.AddEventHandler(xiiMakeDelegate(&xiiQtScenegraphPanel::LayerEventHandler, this), m_LayerEventUnsubscriber);
  xiiHybridArray<xiiSceneDocument*, 16> layers;
  pDocument->GetLoadedLayers(layers);
  for (xiiSceneDocument* pLayer : layers)
  {
    if (pLayer != pDocument)
      LayerLoaded(pLayer->GetGuid());
  }
  ActiveLayerChanged(pDocument->GetActiveLayer());
}

xiiQtScenegraphPanel::~xiiQtScenegraphPanel() = default;

void xiiQtScenegraphPanel::LayerEventHandler(const xiiScene2LayerEvent& e)
{
  switch (e.m_Type)
  {
    case xiiScene2LayerEvent::Type::LayerLoaded:
      LayerLoaded(e.m_layerGuid);
      break;
    case xiiScene2LayerEvent::Type::LayerUnloaded:
      LayerUnloaded(e.m_layerGuid);
      break;
    case xiiScene2LayerEvent::Type::ActiveLayerChanged:
    {
      ActiveLayerChanged(e.m_layerGuid);
    }
    default:
      break;
  }
}

void xiiQtScenegraphPanel::LayerLoaded(const xiiUuid& layerGuid)
{
  XII_ASSERT_DEV(!m_LayerWidgets.Contains(layerGuid), "LayerLoaded was fired twice for the same layer.");

  auto pScene2              = static_cast<xiiScene2Document*>(m_pSceneDocument);
  auto pLayer               = pScene2->GetLayerDocument(layerGuid);
  auto pCustomModel         = CreateGameObjectTreeModel(pLayer);
  m_pMainGameObjectWidget   = new xiiQtGameObjectWidget(this, pLayer, "EditorPluginScene_ScenegraphContextMenu", std::move(pCustomModel));
  m_LayerWidgets[layerGuid] = m_pMainGameObjectWidget;
  m_pStack->addWidget(m_pMainGameObjectWidget);
  ActiveLayerChanged(pScene2->GetActiveLayer());
}

void xiiQtScenegraphPanel::LayerUnloaded(const xiiUuid& layerGuid)
{
  XII_ASSERT_DEV(m_LayerWidgets.Contains(layerGuid), "LayerUnloaded was fired without the layer being loaded first.");

  xiiQtGameObjectWidget* pWidget = m_LayerWidgets[layerGuid];
  m_pStack->removeWidget(pWidget);
  m_LayerWidgets.Remove(layerGuid);
  delete pWidget;

  auto pScene2 = static_cast<xiiScene2Document*>(m_pSceneDocument);
  ActiveLayerChanged(pScene2->GetActiveLayer());
}

void xiiQtScenegraphPanel::ActiveLayerChanged(const xiiUuid& layerGuid)
{
  // migrate the search filter text to the other layer
  if (xiiQtGameObjectWidget* pPrev = qobject_cast<xiiQtGameObjectWidget*>(m_pStack->currentWidget()))
  {
    QString sText = pPrev->GetFilterWidget().text();
    m_LayerWidgets[layerGuid]->GetFilterWidget().setText(sText);
  }

  m_pStack->setCurrentWidget(m_LayerWidgets[layerGuid]);
}
