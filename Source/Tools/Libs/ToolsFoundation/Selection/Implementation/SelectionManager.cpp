#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

xiiSelectionManager::xiiSelectionManager(const xiiDocumentObjectManager* pObjectManager)
{
  auto pStorage              = XII_DEFAULT_NEW(Storage);
  pStorage->m_pObjectManager = pObjectManager;
  SwapStorage(pStorage);
}

xiiSelectionManager::~xiiSelectionManager()
{
  m_ObjectStructureUnsubscriber.Unsubscribe();
  m_EventsUnsubscriber.Unsubscribe();
}

void xiiSelectionManager::TreeEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case xiiDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
      RemoveObject(e.m_pObject, true);
      break;
    default:
      return;
  }
}

bool xiiSelectionManager::RecursiveRemoveFromSelection(const xiiDocumentObject* pObject)
{
  auto it = m_pSelectionStorage->m_SelectionSet.Find(pObject->GetGuid());

  bool bRemoved = false;
  if (it.IsValid())
  {
    m_pSelectionStorage->m_SelectionSet.Remove(it);
    m_pSelectionStorage->m_SelectionList.RemoveAndCopy(pObject);
    bRemoved = true;
  }

  for (const xiiDocumentObject* pChild : pObject->GetChildren())
  {
    bRemoved = bRemoved || RecursiveRemoveFromSelection(pChild);
  }
  return bRemoved;
}

void xiiSelectionManager::Clear()
{
  if (!m_pSelectionStorage->m_SelectionList.IsEmpty() || !m_pSelectionStorage->m_SelectionSet.IsEmpty())
  {
    m_pSelectionStorage->m_SelectionList.Clear();
    m_pSelectionStorage->m_SelectionSet.Clear();

    xiiSelectionManagerEvent e;
    e.m_pDocument = GetDocument();
    e.m_pObject   = nullptr;
    e.m_Type      = xiiSelectionManagerEvent::Type::SelectionCleared;

    m_pSelectionStorage->m_Events.Broadcast(e);
  }
}

void xiiSelectionManager::AddObject(const xiiDocumentObject* pObject)
{
  XII_ASSERT_DEBUG(pObject, "Object must be valid");

  if (IsSelected(pObject))
    return;

  XII_ASSERT_DEV(pObject->GetDocumentObjectManager() == m_pSelectionStorage->m_pObjectManager, "Passed in object does not belong to same object manager.");
  xiiStatus res = m_pSelectionStorage->m_pObjectManager->CanSelect(pObject);
  if (res.m_Result.Failed())
  {
    xiiLog::Error("{0}", res.m_sMessage);
    return;
  }

  m_pSelectionStorage->m_SelectionList.PushBack(pObject);
  m_pSelectionStorage->m_SelectionSet.Insert(pObject->GetGuid());

  xiiSelectionManagerEvent e;
  e.m_pDocument = GetDocument();
  e.m_pObject   = pObject;
  e.m_Type      = xiiSelectionManagerEvent::Type::ObjectAdded;

  m_pSelectionStorage->m_Events.Broadcast(e);
}

void xiiSelectionManager::RemoveObject(const xiiDocumentObject* pObject, bool bRecurseChildren)
{
  if (bRecurseChildren)
  {
    // We only want one message for the change in selection so we first everything and then fire
    // SelectionSet instead of multiple ObjectRemoved messages.
    if (RecursiveRemoveFromSelection(pObject))
    {
      xiiSelectionManagerEvent e;
      e.m_pDocument = GetDocument();
      e.m_pObject   = nullptr;
      e.m_Type      = xiiSelectionManagerEvent::Type::SelectionSet;
      m_pSelectionStorage->m_Events.Broadcast(e);
    }
  }
  else
  {
    auto it = m_pSelectionStorage->m_SelectionSet.Find(pObject->GetGuid());

    if (!it.IsValid())
      return;

    m_pSelectionStorage->m_SelectionSet.Remove(it);
    m_pSelectionStorage->m_SelectionList.RemoveAndCopy(pObject);

    xiiSelectionManagerEvent e;
    e.m_pDocument = GetDocument();
    e.m_pObject   = pObject;
    e.m_Type      = xiiSelectionManagerEvent::Type::ObjectRemoved;

    m_pSelectionStorage->m_Events.Broadcast(e);
  }
}

void xiiSelectionManager::SetSelection(const xiiDocumentObject* pSingleObject)
{
  xiiDeque<const xiiDocumentObject*> objs;
  objs.PushBack(pSingleObject);
  SetSelection(objs);
}

void xiiSelectionManager::SetSelection(const xiiDeque<const xiiDocumentObject*>& selection)
{
  if (selection.IsEmpty())
  {
    Clear();
    return;
  }

  if (m_pSelectionStorage->m_SelectionList == selection)
    return;

  m_pSelectionStorage->m_SelectionList.Clear();
  m_pSelectionStorage->m_SelectionSet.Clear();

  m_pSelectionStorage->m_SelectionList.Reserve(selection.GetCount());

  for (xiiUInt32 i = 0; i < selection.GetCount(); ++i)
  {
    // actually == nullptr should never happen, unless we have an error somewhere else
    if (selection[i] != nullptr)
    {
      XII_ASSERT_DEV(selection[i]->GetDocumentObjectManager() == m_pSelectionStorage->m_pObjectManager, "Passed in object does not belong to same object manager.");
      xiiStatus res = m_pSelectionStorage->m_pObjectManager->CanSelect(selection[i]);
      if (res.m_Result.Failed())
      {
        xiiLog::Error("{0}", res.m_sMessage);
        continue;
      }

      if (!m_pSelectionStorage->m_SelectionSet.Contains(selection[i]->GetGuid()))
      {
        m_pSelectionStorage->m_SelectionList.PushBack(selection[i]);
        m_pSelectionStorage->m_SelectionSet.Insert(selection[i]->GetGuid());
      }
    }
  }

  {
    // Sync selection model.
    xiiSelectionManagerEvent e;
    e.m_pDocument = GetDocument();
    e.m_pObject   = nullptr;
    e.m_Type      = xiiSelectionManagerEvent::Type::SelectionSet;
    m_pSelectionStorage->m_Events.Broadcast(e);
  }
}

void xiiSelectionManager::ToggleObject(const xiiDocumentObject* pObject)
{
  if (IsSelected(pObject))
    RemoveObject(pObject);
  else
    AddObject(pObject);
}

void xiiSelectionManager::SetRuntimeOverrideSelection(const xiiDeque<const xiiDocumentObject*>& selection)
{
  if (m_RuntimeOverrideSelection == selection)
    return;

  m_RuntimeOverrideSelection.Clear();
  m_RuntimeOverrideSelection.Reserve(selection.GetCount());

  for (xiiUInt32 i = 0; i < selection.GetCount(); ++i)
  {
    // actually == nullptr should never happen, unless we have an error somewhere else
    if (selection[i] != nullptr)
    {
      if (!m_RuntimeOverrideSelection.Contains(selection[i]))
      {
        m_RuntimeOverrideSelection.PushBack(selection[i]);
      }
    }
  }

  {
    xiiSelectionManagerEvent e;
    e.m_pDocument = GetDocument();
    e.m_pObject   = nullptr;
    e.m_Type      = xiiSelectionManagerEvent::Type::ChangedRuntimeOverrideSelection;
    m_pSelectionStorage->m_Events.Broadcast(e);
  }
}

const xiiDocumentObject* xiiSelectionManager::GetCurrentObject() const
{
  return m_pSelectionStorage->m_SelectionList.IsEmpty() ? nullptr : m_pSelectionStorage->m_SelectionList.PeekBack();
}

bool xiiSelectionManager::IsSelected(const xiiDocumentObject* pObject) const
{
  return m_pSelectionStorage->m_SelectionSet.Find(pObject->GetGuid()).IsValid();
}

bool xiiSelectionManager::IsParentSelected(const xiiDocumentObject* pObject) const
{
  const xiiDocumentObject* pParent = pObject->GetParent();

  while (pParent != nullptr)
  {
    if (m_pSelectionStorage->m_SelectionSet.Find(pParent->GetGuid()).IsValid())
      return true;

    pParent = pParent->GetParent();
  }

  return false;
}

const xiiDocument* xiiSelectionManager::GetDocument() const
{
  return m_pSelectionStorage->m_pObjectManager->GetDocument();
}

xiiSharedPtr<xiiSelectionManager::Storage> xiiSelectionManager::SwapStorage(xiiSharedPtr<xiiSelectionManager::Storage> pNewStorage)
{
  XII_ASSERT_ALWAYS(pNewStorage != nullptr, "Need a valid history storage object");

  auto retVal = m_pSelectionStorage;

  m_ObjectStructureUnsubscriber.Unsubscribe();
  m_EventsUnsubscriber.Unsubscribe();

  m_pSelectionStorage = pNewStorage;

  m_pSelectionStorage->m_pObjectManager->m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiSelectionManager::TreeEventHandler, this), m_ObjectStructureUnsubscriber);
  m_pSelectionStorage->m_Events.AddEventHandler([this](const xiiSelectionManagerEvent& e) { m_Events.Broadcast(e); }, m_EventsUnsubscriber);

  return retVal;
}

struct xiiObjectHierarchyComparor
{
  using Tree = xiiHybridArray<const xiiDocumentObject*, 4>;

  xiiObjectHierarchyComparor(xiiArrayPtr<xiiSelectionEntry> items)
  {
    for (const xiiSelectionEntry& e : items)
    {
      const xiiDocumentObject* pObject = e.m_pObject;

      Tree& tree = lookup[pObject];
      while (pObject)
      {
        tree.PushBack(pObject);
        pObject = pObject->GetParent();
      }
      std::reverse(begin(tree), end(tree));
    }
  }

  XII_ALWAYS_INLINE bool Less(const xiiSelectionEntry& lhs, const xiiSelectionEntry& rhs) const
  {
    const Tree& A = *lookup.GetValue(lhs.m_pObject);
    const Tree& B = *lookup.GetValue(rhs.m_pObject);

    const xiiUInt32 minSize = xiiMath::Min(A.GetCount(), B.GetCount());
    for (xiiUInt32 i = 0; i < minSize; i++)
    {
      // The first element in the loop should always be the root so there is not risk that there is no common parent.
      if (A[i] != B[i])
      {
        // These elements are the first different ones so they share the same parent.
        // We just assume that the hierarchy is integer-based for now.
        return A[i]->GetPropertyIndex().ConvertTo<xiiUInt32>() < B[i]->GetPropertyIndex().ConvertTo<xiiUInt32>();
      }
    }

    return A.GetCount() < B.GetCount();
  }

  XII_ALWAYS_INLINE bool Equal(const xiiSelectionEntry& lhs, const xiiSelectionEntry& rhs) const { return lhs.m_pObject == rhs.m_pObject; }

  xiiMap<const xiiDocumentObject*, Tree> lookup;
};

void xiiSelectionManager::GetTopLevelSelection(xiiDynamicArray<xiiSelectionEntry>& out_entries) const
{
  out_entries.Clear();
  out_entries.Reserve(m_pSelectionStorage->m_SelectionList.GetCount());

  xiiUInt32 order = 0;

  for (const auto* pObj : m_pSelectionStorage->m_SelectionList)
  {
    if (!IsParentSelected(pObj))
    {
      auto& e              = out_entries.ExpandAndGetRef();
      e.m_pObject          = pObj;
      e.m_uiSelectionOrder = order++;
    }
  }

  xiiObjectHierarchyComparor c(out_entries);
  out_entries.Sort(c);
}

void xiiSelectionManager::GetTopLevelSelectionOfType(const xiiRTTI* pBase, xiiDynamicArray<xiiSelectionEntry>& out_entries) const
{
  out_entries.Clear();
  out_entries.Reserve(m_pSelectionStorage->m_SelectionList.GetCount());

  xiiUInt32 order = 0;

  for (const auto* pObj : m_pSelectionStorage->m_SelectionList)
  {
    if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom(pBase))
      continue;

    if (!IsParentSelected(pObj))
    {
      auto& e              = out_entries.ExpandAndGetRef();
      e.m_pObject          = pObj;
      e.m_uiSelectionOrder = order++;
    }
  }

  xiiObjectHierarchyComparor c(out_entries);
  out_entries.Sort(c);
}
