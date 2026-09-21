/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiDocument;
struct xiiDocumentObjectStructureEvent;

/// Event describing changes to the selection in the selection manager.
struct xiiSelectionManagerEvent
{
  enum class Type
  {
    SelectionCleared,
    SelectionSet,
    ObjectAdded,
    ObjectRemoved,
    ChangedRuntimeOverrideSelection, ///< Broadcast by SetRuntimeOverrideSelection().
  };

  Type                     m_Type;
  const xiiDocument*       m_pDocument;
  const xiiDocumentObject* m_pObject;
};

struct xiiSelectionEntry
{
  const xiiDocumentObject* m_pObject;
  xiiUInt32                m_uiSelectionOrder = 0; // the index at which this item was in the selection
};

/// Selection Manager stores a set of selected document objects.
class XII_TOOLSFOUNDATION_DLL xiiSelectionManager
{
public:
  /// Event that is broadcast when the selection changes.
  xiiCopyOnBroadcastEvent<const xiiSelectionManagerEvent&> m_Events;

  /// Storage for the selection so it can be swapped when using multiple sub documents.
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
  void SetSelection(const xiiDeque<const xiiDocumentObject*>& selection);
  void ToggleObject(const xiiDocumentObject* pObject);

  /// Sets a separate selection (temporarily), which is sent to the engine but not propagated to the editor.
  ///
  /// This is used for cases where temporarily the engine should use a different selection than the editor.
  /// Currently this is used during drag-and-drop, to already show the dragged object as selected and especially to exclude it from picking,
  /// but not yet show the new object as selected in the property grids, such that users can interact with the previously selected object.
  ///
  /// To clear a runtime override selection, simply set an empty selection.
  void SetRuntimeOverrideSelection(const xiiDeque<const xiiDocumentObject*>& selection);

  /// Returns the current runtime override selection.
  ///
  /// Valid, if the selection is non-empty.
  /// See SetRuntimeOverrideSelection() for details.
  const xiiDeque<const xiiDocumentObject*>& GetRuntimeOverrideSelection() const { return m_RuntimeOverrideSelection; }

  /// Returns the last selected object in the selection or null if empty.
  const xiiDocumentObject* GetCurrentObject() const;

  /// Returns the selection in the same order the objects were added to the list.
  const xiiDeque<const xiiDocumentObject*>& GetSelection() const { return m_pSelectionStorage->m_SelectionList; }

  bool IsSelectionEmpty() const { return m_pSelectionStorage->m_SelectionList.IsEmpty(); }

  /// Returns the subset of selected items which have no parent selected.
  ///
  /// I.e. if an object is selected and one of its ancestors is selected, it is culled from the list.
  /// Items are returned in the order of appearance in an expanded scene tree.
  /// Their order in the selection is returned through xiiSelectionEntry.
  void GetTopLevelSelection(xiiDynamicArray<xiiSelectionEntry>& out_entries) const;

  /// Same as GetTopLevelSelection() but additionally requires that all objects are derived from type pBase.
  void GetTopLevelSelectionOfType(const xiiRTTI* pBase, xiiDynamicArray<xiiSelectionEntry>& out_entries) const;

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
  xiiDeque<const xiiDocumentObject*>         m_RuntimeOverrideSelection;

  xiiCopyOnBroadcastEvent<const xiiDocumentObjectStructureEvent&>::Unsubscriber m_ObjectStructureUnsubscriber;
  xiiCopyOnBroadcastEvent<const xiiSelectionManagerEvent&>::Unsubscriber        m_EventsUnsubscriber;
};
