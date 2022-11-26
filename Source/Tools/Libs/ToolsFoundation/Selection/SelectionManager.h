#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiDocument;
struct xiiDocumentObjectStructureEvent;

struct xiiSelectionManagerEvent
{
  enum class Type
  {
    SelectionCleared,
    SelectionSet,
    ObjectAdded,
    ObjectRemoved,
  };

  Type                     m_Type;
  const xiiDocument*       m_pDocument;
  const xiiDocumentObject* m_pObject;
};

/// \brief Selection Manager stores a set of selected document objects.
class XII_TOOLSFOUNDATION_DLL xiiSelectionManager
{
public:
  xiiCopyOnBroadcastEvent<const xiiSelectionManagerEvent&> m_Events;

  // \brief Storage for the selection so it can be swapped when using multiple sub documents.
  class Storage : public xiiRefCounted
  {
  public:
    xiiDeque<const xiiDocumentObject*>                       m_SelectionList;
    xiiSet<xiiUuid>                                          m_SelectionSet;
    const xiiDocumentObjectManager*                          m_pObjectManager = nullptr;
    xiiCopyOnBroadcastEvent<const xiiSelectionManagerEvent&> m_Events;
  };

public:
  xiiSelectionManager(const xiiDocumentObjectManager* pObjectManager);
  ~xiiSelectionManager();

  void Clear();
  void AddObject(const xiiDocumentObject* pObject);
  void RemoveObject(const xiiDocumentObject* pObject, bool bRecurseChildren = false);
  void SetSelection(const xiiDocumentObject* pSingleObject);
  void SetSelection(const xiiDeque<const xiiDocumentObject*>& Selection);
  void ToggleObject(const xiiDocumentObject* pObject);

  /// \brief Returns the last selected object in the selection or null if empty.
  const xiiDocumentObject* GetCurrentObject() const;

  /// \brief Returns the selection in the same order the objects were added to the list.
  const xiiDeque<const xiiDocumentObject*>& GetSelection() const { return m_pSelectionStorage->m_SelectionList; }

  bool IsSelectionEmpty() const { return m_pSelectionStorage->m_SelectionList.IsEmpty(); }

  /// \brief Returns the subset of selected items which have no parent selected. I.e. if an object is selected and one of its ancestors is selected, it is culled from the list. Items are returned in the order of appearance in an expanded scene tree.
  const xiiDeque<const xiiDocumentObject*> GetTopLevelSelection() const;

  /// \brief Same as GetTopLevelSelection() but additionally requires that all objects are derived from type pBase.
  const xiiDeque<const xiiDocumentObject*> GetTopLevelSelection(const xiiRTTI* pBase) const;

  bool IsSelected(const xiiDocumentObject* pObject) const;
  bool IsParentSelected(const xiiDocumentObject* pObject) const;

  const xiiDocument* GetDocument() const;

  xiiSharedPtr<xiiSelectionManager::Storage> SwapStorage(xiiSharedPtr<xiiSelectionManager::Storage> pNewStorage);
  xiiSharedPtr<xiiSelectionManager::Storage> GetStorage() { return m_pSelectionStorage; }

private:
  void TreeEventHandler(const xiiDocumentObjectStructureEvent& e);
  bool RecursiveRemoveFromSelection(const xiiDocumentObject* pObject);

  friend class xiiDocument;

  xiiSharedPtr<xiiSelectionManager::Storage> m_pSelectionStorage;

  xiiCopyOnBroadcastEvent<const xiiDocumentObjectStructureEvent&>::Unsubscriber m_ObjectStructureUnsubscriber;
  xiiCopyOnBroadcastEvent<const xiiSelectionManagerEvent&>::Unsubscriber        m_EventsUnsubscriber;
};
