#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerPanel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <QVBoxLayout>

xiiQtLayerPanel::xiiQtLayerPanel(QWidget* pParent, xiiScene2Document* pDocument) :
  xiiQtDocumentPanel(pParent, pDocument)
{
  setObjectName("LayerPanel");
  setWindowTitle("Layers");
  m_pSceneDocument = pDocument;
  m_pDelegate      = new xiiQtLayerDelegate(this, pDocument);

  std::unique_ptr<xiiQtLayerModel> pModel(new xiiQtLayerModel(m_pSceneDocument));
  pModel->AddAdapter(new xiiQtDummyAdapter(pDocument->GetSceneObjectManager(), xiiGetStaticRTTI<xiiSceneDocumentSettings>(), "Layers"));
  pModel->AddAdapter(new xiiQtLayerAdapter(pDocument));

  m_pTreeWidget = new xiiQtDocumentTreeView(this, pDocument, std::move(pModel), m_pSceneDocument->GetLayerSelectionManager());
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pTreeWidget->SetAllowDeleteObjects(false);
  m_pTreeWidget->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  m_pTreeWidget->setItemDelegate(m_pDelegate);

  m_pTreeWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  XII_VERIFY(connect(m_pTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnRequestContextMenu(QPoint))) != nullptr,
             "signal/slot connection failed");

  setWidget(m_pTreeWidget);
}

xiiQtLayerPanel::~xiiQtLayerPanel() {}

void xiiQtLayerPanel::OnRequestContextMenu(QPoint pos)
{
  xiiQtMenuActionMapView menu(nullptr);

  xiiActionContext context;
  context.m_sMapping  = "EditorPluginScene_LayerContextMenu";
  context.m_pDocument = m_pSceneDocument;
  context.m_pWindow   = this;
  menu.SetActionContext(context);

  menu.exec(m_pTreeWidget->mapToGlobal(pos));
}
