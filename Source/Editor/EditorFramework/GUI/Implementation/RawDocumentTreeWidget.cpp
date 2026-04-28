/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiQtDocumentTreeView::xiiQtDocumentTreeView(QWidget* pParent) :
  xiiQtItemView<QTreeView>(pParent)
{
  setObjectName("xiiQtDocumentTreeView");
}

xiiQtDocumentTreeView::xiiQtDocumentTreeView(QWidget* pParent, xiiDocument* pDocument, std::unique_ptr<xiiQtDocumentTreeModel> pModel, xiiSelectionManager* pSelection) :
  xiiQtItemView<QTreeView>(pParent)
{
  setObjectName("xiiQtDocumentTreeView");

  Initialize(pDocument, std::move(pModel), pSelection);
}

static bool GameObjectFilterFunc(QModelIndex index, const xiiSearchPatternFilter& filter)
{
  if (const xiiQtDocumentTreeModel* pModel = qobject_cast<const xiiQtDocumentTreeModel*>(index.model()))
  {
    auto pObj = pModel->GetObject(index);
    auto pAcc = pModel->GetDocumentTree()->GetDocument()->GetObjectAccessor();

    xiiVariant comp;

    const xiiInt32 iNum = pAcc->GetCountByName(pObj, "Components");
    for (xiiInt32 i = 0; i < iNum; ++i)
    {
      if (pAcc->GetValueByName(pObj, "Components", comp, i).Succeeded())
      {
        if (filter.PassesFilters(pAcc->GetObject(comp.Get<xiiUuid>())->GetType()->GetTypeName()))
          return true;
      }
    }
  }

  return false;
}

void xiiQtDocumentTreeView::Initialize(xiiDocument* pDocument, std::unique_ptr<xiiQtDocumentTreeModel> pModel, xiiSelectionManager* pSelection)
{
  m_pDocument         = pDocument;
  m_pModel            = std::move(pModel);
  m_pSelectionManager = pSelection;
  if (m_pSelectionManager == nullptr)
  {
    // If no selection manager is provided, fall back to the default selection.
    m_pSelectionManager = m_pDocument->GetSelectionManager();
  }

  m_pFilterModel.reset(new xiiQtTreeSearchFilterModel(this));
  m_pFilterModel->setSourceModel(m_pModel.get());
  m_pFilterModel->SetCustomFilterFunc(GameObjectFilterFunc);

  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setModel(m_pFilterModel.get());
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);
  setHeaderHidden(true);
  setExpandsOnDoubleClick(true);
  setEditTriggers(QAbstractItemView::EditTrigger::EditKeyPressed);
  setUniformRowHeights(true);

  XII_VERIFY(connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(on_selectionChanged_triggered(const QItemSelection&, const QItemSelection&))) != nullptr, "signal/slot connection failed");
  m_pSelectionManager->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtDocumentTreeView::SelectionEventHandler, this));

  xiiSelectionManagerEvent e;
  e.m_pDocument = m_pDocument;
  e.m_pObject   = nullptr;
  e.m_Type      = xiiSelectionManagerEvent::Type::SelectionSet;
  SelectionEventHandler(e);
}

xiiQtDocumentTreeView::~xiiQtDocumentTreeView()
{
  m_pSelectionManager->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtDocumentTreeView::SelectionEventHandler, this));
}

void xiiQtDocumentTreeView::on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (m_bBlockSelectionSignal)
    return;

  QModelIndexList selection = selectionModel()->selectedIndexes();

  xiiDeque<const xiiDocumentObject*> sel;

  foreach (QModelIndex index, selection)
  {
    if (index.isValid() && index.column() == 0)
    {
      index = m_pFilterModel->mapToSource(index);

      if (index.isValid())
        sel.PushBack((const xiiDocumentObject*)index.internalPointer());
    }
  }

  // TODO const cast
  ((xiiSelectionManager*)m_pSelectionManager)->SetSelection(sel);
}

void xiiQtDocumentTreeView::SelectionEventHandler(const xiiSelectionManagerEvent& e)
{
  switch (e.m_Type)
  {
    case xiiSelectionManagerEvent::Type::SelectionCleared:
    {
      // Can't block signals on selection model or view won't update.
      m_bBlockSelectionSignal = true;
      selectionModel()->clear();
      m_bBlockSelectionSignal = false;
    }
    break;

    case xiiSelectionManagerEvent::Type::SelectionSet:
    case xiiSelectionManagerEvent::Type::ObjectAdded:
    case xiiSelectionManagerEvent::Type::ObjectRemoved:
    {
      // Can't block signals on selection model or view won't update.
      m_bBlockSelectionSignal = true;
      QItemSelection selection;
      QModelIndex    currentIndex;
      for (const xiiDocumentObject* pObject : m_pSelectionManager->GetSelection())
      {
        currentIndex = m_pModel->ComputeModelIndex(pObject);
        currentIndex = m_pFilterModel->mapFromSource(currentIndex);

        if (currentIndex.isValid())
          selection.select(currentIndex, currentIndex);
      }
      if (currentIndex.isValid())
      {
        // We need to change the current index as well because the current index can trigger side effects. E.g. deleting the current index row triggers a selection change event.
        selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::SelectCurrent);
      }
      selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows | QItemSelectionModel::NoUpdate);
      m_bBlockSelectionSignal = false;
    }
    break;

    case xiiSelectionManagerEvent::Type::ChangedRuntimeOverrideSelection:
      // ignore
      break;
  }
}

void xiiQtDocumentTreeView::EnsureLastSelectedItemVisible()
{
  if (m_pSelectionManager->GetSelection().IsEmpty())
    return;

  const xiiDocumentObject* pObject = m_pSelectionManager->GetSelection().PeekBack();
  XII_ASSERT_DEBUG(m_pModel->GetDocumentTree()->GetDocument() == pObject->GetDocumentObjectManager()->GetDocument(), "Selection is from a different document.");

  auto index = m_pModel->ComputeModelIndex(pObject);
  index      = m_pFilterModel->mapFromSource(index);

  if (index.isValid())
    scrollTo(index, QAbstractItemView::EnsureVisible);
}

void xiiQtDocumentTreeView::SetAllowDragDrop(bool bAllow)
{
  m_pModel->SetAllowDragDrop(bAllow);
}

void xiiQtDocumentTreeView::SetAllowDeleteObjects(bool bAllow)
{
  m_bAllowDeleteObjects = bAllow;
}

bool xiiQtDocumentTreeView::event(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::ShortcutOverride || pEvent->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
    if (xiiQtProxy::TriggerDocumentAction(m_pDocument, keyEvent, pEvent->type() == QEvent::ShortcutOverride))
      return true;

    if (pEvent->type() == QEvent::KeyPress && keyEvent == QKeySequence::Delete)
    {
      if (m_bAllowDeleteObjects)
      {
        m_pDocument->DeleteSelectedObjects();
      }
      pEvent->accept();
      return true;
    }
  }

  return QTreeView::event(pEvent);
}
