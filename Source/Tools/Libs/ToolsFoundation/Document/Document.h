/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Implementation/Declarations.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

class xiiObjectAccessorBase;
class xiiObjectCommandAccessor;
class xiiEditorInputContext;
class xiiAbstractObjectNode;

struct XII_TOOLSFOUNDATION_DLL xiiObjectAccessorChangeEvent
{
  xiiDocument*           m_pDocument          = nullptr; ///< The document in which the accessor change occurred.
  xiiObjectAccessorBase* m_pOldObjectAccessor = nullptr;
  xiiObjectAccessorBase* m_pNewObjectAccessor = nullptr;
};

/// Stores meta data for document objects, such as prefab information and visibility in the editor.
class XII_TOOLSFOUNDATION_DLL xiiDocumentObjectMetaData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDocumentObjectMetaData, xiiReflectedClass);

public:
  enum ModifiedFlags : xiiUInt32
  {
    HiddenFlag       = XII_BIT(0),
    PrefabFlag       = XII_BIT(1),
    ActiveParentFlag = XII_BIT(2), ///< This flag is used to update an entry, even though there is no meta data for it.

    AllFlags = 0xFFFFFFFFU
  };

  bool      m_bHidden = false;  ///< Whether the object should be rendered in the editor view (no effect on the runtime)
  xiiUuid   m_CreateFromPrefab; ///< The asset GUID of the prefab from which this object was created. Invalid GUID, if this is not a prefab instance.
  xiiUuid   m_PrefabSeedGuid;   ///< The seed GUID used to remap the object GUIDs from the prefab asset into this instance.
  xiiString m_sBasePrefab;      ///< The prefab from which this instance was created as complete DDL text (this describes the entire object!). Necessary for
                                ///< three-way-merging the prefab instances.
};

enum class xiiManipulatorSearchStrategy : xiiUInt8
{
  None = 0U,               ///< No manipulator search.
  SelectedObject,          ///< Search for manipulators on the selected object.
  ChildrenOfSelectedObject ///< Search for manipulators on the children of the selected object.
};

/// Base class for all editable documents in the editor. Handles state, object management, undo/redo, and more.
class XII_TOOLSFOUNDATION_DLL xiiDocument : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDocument, xiiReflectedClass);

public:
  xiiDocument(xiiStringView sPath, xiiDocumentObjectManager* pDocumentObjectManagerImpl);
  virtual ~xiiDocument();

  /// \name Document State Functions
  ///@{

  bool IsModified() const { return m_bModified; }
  bool IsReadOnly() const { return m_bReadOnly; }

  /// Returns when the document was last marked as modified. Invalid if the document is not modified.
  xiiTime GetModifiedTime() const { return m_ModifiedTime; }

  const xiiUuid GetGuid() const { return m_pDocumentInfo ? m_pDocumentInfo->m_DocumentID : xiiUuid(); }

  const xiiDocumentObjectManager* GetObjectManager() const { return m_pObjectManager.Borrow(); }
  xiiDocumentObjectManager*       GetObjectManager() { return m_pObjectManager.Borrow(); }
  xiiSelectionManager*            GetSelectionManager() const { return m_pSelectionManager.Borrow(); }
  xiiCommandHistory*              GetCommandHistory() const { return m_pCommandHistory.Borrow(); }
  virtual xiiObjectAccessorBase*  GetObjectAccessor() const;

  ///@}
  /// \name Main / Sub-Document Functions
  ///@{

  /// Returns whether this document is a main document, i.e. self contained.
  bool IsMainDocument() const { return m_pHostDocument == this; }
  /// Returns whether this document is a sub-document, i.e. is part of another document.
  bool IsSubDocument() const { return m_pHostDocument != this; }
  /// In case this is a sub-document, returns the main document this belongs to. Otherwise 'this' is returned.
  const xiiDocument* GetMainDocument() const { return m_pHostDocument; }
  /// At any given time, only the active sub-document can be edited. This returns the active sub-document which can also be this document itself. Changes to the active sub-document are generally triggered by xiiDocumentObjectStructureEvent::Type::AfterReset.
  const xiiDocument* GetActiveSubDocument() const { return m_pActiveSubDocument; }
  xiiDocument*       GetMainDocument() { return m_pHostDocument; }
  xiiDocument*       GetActiveSubDocument() { return m_pActiveSubDocument; }

protected:
  xiiDocument* m_pHostDocument      = nullptr; ///< Pointer to the main document if this is a sub-document, otherwise self.
  xiiDocument* m_pActiveSubDocument = nullptr; ///< Pointer to the currently active sub-document.

  ///@}
  /// \name Document Management Functions
  ///@{

public:
  /// Returns the absolute path to the document.
  xiiStringView GetDocumentPath() const { return m_sDocumentPath; }

  /// Saves the document, if it is modified.
  /// If bForce is true, the document will be written, even if it is not considered modified.
  xiiStatus SaveDocument(bool bForce = false);
  /// Callback type for asynchronous save operations.
  using AfterSaveCallback = xiiDelegate<void(xiiDocument*, xiiStatus)>;
  /// Saves the document asynchronously. Calls the callback when done.
  xiiTaskGroupID SaveDocumentAsync(AfterSaveCallback callback, bool bForce = false);
  /// Updates the document path after a rename operation.
  void DocumentRenamed(xiiStringView sNewDocumentPath);

  /// Reads a document from disk and parses its header, objects, and types.
  static xiiStatus ReadDocument(xiiStringView sDocumentPath, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pHeader, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pObjects, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pTypes);
  /// Reads and registers types from the given object graph.
  static xiiStatus ReadAndRegisterTypes(const xiiAbstractObjectGraph& types);

  /// Loads the document from disk.
  xiiStatus LoadDocument() { return InternalLoadDocument(); }

  /// Brings the corresponding window to the front.
  void EnsureVisible();

  /// Returns the document manager that owns this document.
  xiiDocumentManager* GetDocumentManager() const { return m_pDocumentManager; }

  bool HasWindowBeenRequested() const { return m_bWindowRequested; }

  const xiiDocumentTypeDescriptor* GetDocumentTypeDescriptor() const { return m_pTypeDescriptor; }

  /// Returns the document's type name. Same as GetDocumentTypeDescriptor()->m_sDocumentTypeName.
  xiiStringView GetDocumentTypeName() const
  {
    if (m_pTypeDescriptor == nullptr)
    {
      // if this is a document without a type descriptor, use the RTTI type name as a fallback
      return GetDynamicRTTI()->GetTypeName();
    }

    return m_pTypeDescriptor->m_sDocumentTypeName;
  }

  const xiiDocumentInfo* GetDocumentInfo() const { return m_pDocumentInfo; }

  /// Asks the document whether a restart of the engine process is allowed at this time.
  ///
  /// Documents that are currently interacting with the engine process (active play-the-game mode) should return false.
  /// All others should return true.
  /// As long as any document returns false, automatic engine process reload is suppressed.
  virtual bool CanEngineProcessBeRestarted() const { return true; }

  ///@}
  /// \name Clipboard Functions
  ///@{

  /// Information about a pasted object, including its parent and index.
  struct PasteInfo
  {
    XII_DECLARE_POD_TYPE();

    xiiDocumentObject* m_pObject = nullptr; ///< The object being pasted.
    xiiDocumentObject* m_pParent = nullptr; ///< The parent object to paste into.
    xiiInt32           m_Index   = -1;      ///< The index at which to insert the object.
  };

  /// Whether this document supports pasting the given mime format into it
  virtual void GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_mimeTypes) const {}
  /// Creates the abstract graph of data to be copied and returns the mime type for the clipboard to identify the data
  virtual bool CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_sMimeType) const { return false; };
  /// Pastes objects from the given object graph into the document.
  virtual bool Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType)
  {
    return false;
  };

  ///@}
  /// \name Inter Document Communication
  ///@{

  /// This will deliver the message to all open documents. The documents may respond, e.g. by modifying the content of the message.
  void BroadcastInterDocumentMessage(xiiReflectedClass* pMessage, xiiDocument* pSender);

  /// Called on all documents when BroadcastInterDocumentMessage() is called.
  ///
  /// Use the RTTI information to identify whether the message is of interest.
  virtual void OnInterDocumentMessage(xiiReflectedClass* pMessage, xiiDocument* pSender) {}

  ///@}
  /// \name Editing Functionality
  ///@{

  /// Allows to return a single input context that currently overrides all others (in priority).
  ///
  /// Used to implement custom tools that need to have priority over selection and camera movement.
  virtual xiiEditorInputContext* GetEditorInputContextOverride() { return nullptr; }

  ///@}
  /// \name Misc Functions
  ///@{

  /// Deletes all currently selected objects in the document.
  virtual void DeleteSelectedObjects() const;

  /// Returns the set of unknown object types encountered during loading.
  const xiiSet<xiiString>& GetUnknownObjectTypes() const { return m_UnknownObjectTypes; }
  /// Returns the number of unknown object type instances encountered during loading.
  xiiUInt32 GetUnknownObjectTypeInstances() const { return m_uiUnknownObjectTypeInstances; }

  /// If disabled, this document will not be put into the recent files list.
  void SetAddToResetFilesList(bool b) { m_bAddToRecentFilesList = b; }

  /// Whether this document shall be put into the recent files list.
  bool GetAddToRecentFilesList() const { return m_bAddToRecentFilesList; }

  /// Broadcasts a status message event. The window that displays the document may show this in some form, e.g. in the status bar.
  void ShowDocumentStatus(const xiiFormatString& msg) const;

  /// Tries to compute the position and rotation for an object in the document. Returns XII_SUCCESS if it was possible.
  virtual xiiResult ComputeObjectTransformation(const xiiDocumentObject* pObject, xiiTransform& out_result) const;

  /// Needed by xiiManipulatorManager to know where to look for the manipulator attributes.
  ///
  /// Override this function for document types that use manipulators.
  /// The xiiManipulatorManager will assert that the document type doesn't return 'None' once it is in use.
  virtual xiiManipulatorSearchStrategy GetManipulatorSearchStrategy() const { return xiiManipulatorSearchStrategy::None; }

  ///@}
  /// \name Prefab Functions
  ///@{

  /// Whether the document allows to create prefabs in it. This may not be allowed for prefab documents themselves, to prevent nested prefabs.
  virtual bool ArePrefabsAllowed() const { return true; }

  /// Updates ALL prefabs in the document with the latest changes. Merges the current prefab templates with the instances in the document.
  virtual void UpdatePrefabs();

  /// Resets the given objects to their template prefab state, if they have local modifications.
  void RevertPrefabs(xiiArrayPtr<const xiiDocumentObject*> selection);

  /// Removes the link between a prefab instance and its template, turning the instance into a regular object.
  virtual void UnlinkPrefabs(xiiArrayPtr<const xiiDocumentObject*> selection);

  /// Creates a prefab document from the current selection.
  virtual xiiStatus CreatePrefabDocumentFromSelection(xiiStringView sFile, const xiiRTTI* pRootType, xiiDelegate<void(xiiAbstractObjectNode*)> adjustGraphNodeCB = {}, xiiDelegate<void(xiiDocumentObject*)> adjustNewNodesCB = {}, xiiDelegate<void(xiiAbstractObjectGraph& graph, xiiDynamicArray<xiiAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {});
  /// Creates a prefab document from the given root objects.
  virtual xiiStatus CreatePrefabDocument(xiiStringView sFile, xiiArrayPtr<const xiiDocumentObject*> rootObjects, const xiiUuid& invPrefabSeed, xiiUuid& out_newDocumentGuid, xiiDelegate<void(xiiAbstractObjectNode*)> adjustGraphNodeCB = {}, bool bKeepOpen = false, xiiDelegate<void(xiiAbstractObjectGraph& graph, xiiDynamicArray<xiiAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {});

  /// Replaces the given object by a prefab instance. Returns new guid of replaced object.
  virtual xiiUuid ReplaceByPrefab(const xiiDocumentObject* pRootObject, xiiStringView sPrefabFile, const xiiUuid& prefabAsset, const xiiUuid& prefabSeed, bool bEnginePrefab);
  /// Reverts the given object to its prefab state. Returns new guid of reverted object.
  virtual xiiUuid RevertPrefab(const xiiDocumentObject* pObject);

  ///@}

public:
  /// Meta data for all document objects.
  xiiUniquePtr<xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>> m_DocumentObjectMetaData;

  /// Event for document-specific notifications.
  mutable xiiEvent<const xiiDocumentEvent&> m_EventsOne;
  /// Static event for notifications across all documents.
  static xiiEvent<const xiiDocumentEvent&> s_EventsAny;

  /// Event for object accessor change notifications.
  mutable xiiEvent<const xiiObjectAccessorChangeEvent&> m_ObjectAccessorChangeEvents;

protected:
  void SetModified(bool b);
  void SetReadOnly(bool b);
  /// Internal save implementation. Returns a task group ID for async save.
  virtual xiiTaskGroupID InternalSaveDocument(AfterSaveCallback callback);
  /// Internal load implementation. Loads the document from disk.
  virtual xiiStatus InternalLoadDocument();
  /// Creates the document info structure. Must be implemented by derived classes.
  virtual xiiDocumentInfo* CreateDocumentInfo() = 0;

  /// Hook to execute additional code after successfully saving a document. E.g. manual asset transform can be done here.
  virtual void InternalAfterSaveDocument() {}

  virtual void AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const;
  virtual void RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable);

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) {}
  virtual void InitializeAfterLoadingAndSaving() {}

  virtual void BeforeClosing();

  void SetUnknownObjectTypes(const xiiSet<xiiString>& Types, xiiUInt32 uiInstances);

  /// \name Prefab Functions
  ///@{

  /// Recursively updates all prefab instances starting from the given object.
  virtual void UpdatePrefabsRecursive(xiiDocumentObject* pObject);
  virtual void UpdatePrefabObject(xiiDocumentObject* pObject, const xiiUuid& PrefabAsset, const xiiUuid& PrefabSeed, xiiStringView sBasePrefab);

  ///@}

  xiiUniquePtr<xiiDocumentObjectManager>         m_pObjectManager;
  mutable xiiUniquePtr<xiiCommandHistory>        m_pCommandHistory;
  mutable xiiUniquePtr<xiiSelectionManager>      m_pSelectionManager;
  mutable xiiUniquePtr<xiiObjectCommandAccessor> m_pObjectAccessor; ///< Default object accessor used by every doc.

  xiiDocumentInfo*                 m_pDocumentInfo   = nullptr;
  const xiiDocumentTypeDescriptor* m_pTypeDescriptor = nullptr;

private:
  friend class xiiDocumentManager;
  friend class xiiCommandHistory;
  friend class xiiSaveDocumentTask;
  friend class xiiAfterSaveDocumentTask;

  void SetupDocumentInfo(const xiiDocumentTypeDescriptor* pTypeDescriptor);

  /// The document manager that owns this document.
  xiiDocumentManager* m_pDocumentManager = nullptr;

  /// The absolute path to the document file.
  xiiString m_sDocumentPath;
  bool      m_bModified             = true;
  bool      m_bReadOnly             = false;
  bool      m_bWindowRequested      = false;
  bool      m_bAddToRecentFilesList = true;
  xiiTime   m_ModifiedTime;

  /// Set of unknown object types encountered during loading.
  xiiSet<xiiString> m_UnknownObjectTypes;
  /// Number of unknown object type instances encountered during loading.
  xiiUInt32 m_uiUnknownObjectTypeInstances = 0U;

  xiiTaskGroupID m_ActiveSaveTask;
  xiiStatus      m_LastSaveResult = XII_SUCCESS;
};
