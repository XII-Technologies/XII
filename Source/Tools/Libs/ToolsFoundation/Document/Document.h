#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Implementation/Declarations.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiObjectAccessorBase;
class xiiObjectCommandAccessor;
class xiiEditorInputContext;
class xiiAbstractObjectNode;

struct XII_TOOLSFOUNDATION_DLL xiiObjectAccessorChangeEvent
{
  xiiDocument*           m_pDocument;
  xiiObjectAccessorBase* m_pOldObjectAccessor;
  xiiObjectAccessorBase* m_pNewObjectAccessor;
};

class XII_TOOLSFOUNDATION_DLL xiiDocumentObjectMetaData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDocumentObjectMetaData, xiiReflectedClass);

public:
  enum ModifiedFlags : unsigned int
  {
    HiddenFlag = XII_BIT(0),
    PrefabFlag = XII_BIT(1),

    AllFlags = 0xFFFFFFFF
  };

  xiiDocumentObjectMetaData() { m_bHidden = false; }

  bool      m_bHidden;          /// Whether the object should be rendered in the editor view (no effect on the runtime)
  xiiUuid   m_CreateFromPrefab; /// The asset GUID of the prefab from which this object was created. Invalid GUID, if this is not a prefab instance.
  xiiUuid   m_PrefabSeedGuid;   /// The seed GUID used to remap the object GUIDs from the prefab asset into this instance.
  xiiString m_sBasePrefab;      /// The prefab from which this instance was created as complete DDL text (this describes the entire object!). Necessary for
                                /// three-way-merging the prefab instances.
};

enum class xiiManipulatorSearchStrategy
{
  None,
  SelectedObject,
  ChildrenOfSelectedObject
};

class XII_TOOLSFOUNDATION_DLL xiiDocument : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDocument, xiiReflectedClass);

public:
  xiiDocument(const char* szPath, xiiDocumentObjectManager* pDocumentObjectManagerImpl);
  virtual ~xiiDocument();

  /// \name Document State Functions
  ///@{

  bool          IsModified() const { return m_bModified; }
  bool          IsReadOnly() const { return m_bReadOnly; }
  const xiiUuid GetGuid() const { return m_pDocumentInfo ? m_pDocumentInfo->m_DocumentID : xiiUuid(); }

  const xiiDocumentObjectManager* GetObjectManager() const { return m_pObjectManager.Borrow(); }
  xiiDocumentObjectManager*       GetObjectManager() { return m_pObjectManager.Borrow(); }
  xiiSelectionManager*            GetSelectionManager() const { return m_pSelectionManager.Borrow(); }
  xiiCommandHistory*              GetCommandHistory() const { return m_pCommandHistory.Borrow(); }
  virtual xiiObjectAccessorBase*  GetObjectAccessor() const;

  ///@}
  /// \name Main / Sub-Document Functions
  ///@{

  /// \brief Returns whether this document is a main document, i.e. self contained.
  bool IsMainDocument() const { return m_pHostDocument == this; }
  /// \brief Returns whether this document is a sub-document, i.e. is part of another document.
  bool IsSubDocument() const { return m_pHostDocument != this; }
  /// \brief In case this is a sub-document, returns the main document this belongs to. Otherwise 'this' is returned.
  const xiiDocument* GetMainDocument() const { return m_pHostDocument; }
  /// @brief At any given time, only the active sub-document can be edited. This returns the active sub-document which can also be this document itself. Changes to the active sub-document are generally triggered by xiiDocumentObjectStructureEvent::Type::AfterReset.
  const xiiDocument* GetActiveSubDocument() const { return m_pActiveSubDocument; }
  xiiDocument*       GetMainDocument() { return m_pHostDocument; }
  xiiDocument*       GetActiveSubDocument() { return m_pActiveSubDocument; }

protected:
  xiiDocument* m_pHostDocument      = nullptr;
  xiiDocument* m_pActiveSubDocument = nullptr;

  ///@}
  /// \name Document Management Functions
  ///@{

public:
  /// \brief Returns the absolute path to the document.
  const char* GetDocumentPath() const { return m_sDocumentPath; }

  /// \brief Saves the document, if it is modified.
  /// If bForce is true, the document will be written, even if it is not considered modified.
  xiiStatus SaveDocument(bool bForce = false);
  using AfterSaveCallback = xiiDelegate<void(xiiDocument*, xiiStatus)>;
  xiiTaskGroupID SaveDocumentAsync(AfterSaveCallback callback, bool bForce = false);

  static xiiStatus ReadDocument(const char* szDocumentPath, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pHeader, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pObjects, xiiUniquePtr<xiiAbstractObjectGraph>& ref_pTypes);
  static xiiStatus ReadAndRegisterTypes(const xiiAbstractObjectGraph& types);

  xiiStatus LoadDocument() { return InternalLoadDocument(); }

  /// \brief Brings the corresponding window to the front.
  void EnsureVisible();

  xiiDocumentManager* GetDocumentManager() const { return m_pDocumentManager; }

  bool HasWindowBeenRequested() const { return m_bWindowRequested; }

  const xiiDocumentTypeDescriptor* GetDocumentTypeDescriptor() const { return m_pTypeDescriptor; }

  /// \brief Returns the document's type name. Same as GetDocumentTypeDescriptor()->m_sDocumentTypeName.
  const char* GetDocumentTypeName() const { return m_pTypeDescriptor->m_sDocumentTypeName; }

  const xiiDocumentInfo* GetDocumentInfo() const { return m_pDocumentInfo; }

  /// \brief Asks the document whether a restart of the engine process is allowed at this time.
  ///
  /// Documents that are currently interacting with the engine process (active play-the-game mode) should return false.
  /// All others should return true.
  /// As long as any document returns false, automatic engine process reload is suppressed.
  virtual bool CanEngineProcessBeRestarted() const { return true; }

  ///@}
  /// \name Clipboard Functions
  ///@{

  struct PasteInfo
  {
    XII_DECLARE_POD_TYPE();

    xiiDocumentObject* m_pObject = nullptr;
    xiiDocumentObject* m_pParent = nullptr;
    xiiInt32           m_Index   = -1;
  };

  /// \brief Whether this document supports pasting the given mime format into it
  virtual void GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_mimeTypes) const {}
  /// \brief Creates the abstract graph of data to be copied and returns the mime type for the clipboard to identify the data
  virtual bool CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_sMimeType) const { return false; };
  virtual bool Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
  {
    return false;
  };

  ///@}
  /// \name Inter Document Communication
  ///@{

  /// \brief This will deliver the message to all open documents. The documents may respond, e.g. by modifying the content of the message.
  void BroadcastInterDocumentMessage(xiiReflectedClass* pMessage, xiiDocument* pSender);

  /// \brief Called on all documents when BroadcastInterDocumentMessage() is called.
  ///
  /// Use the RTTI information to identify whether the message is of interest.
  virtual void OnInterDocumentMessage(xiiReflectedClass* pMessage, xiiDocument* pSender) {}

  ///@}
  /// \name Editing Functionality
  ///@{

  /// \brief Allows to return a single input context that currently overrides all others (in priority).
  ///
  /// Used to implement custom tools that need to have priority over selection and camera movement.
  virtual xiiEditorInputContext* GetEditorInputContextOverride() { return nullptr; }

  ///@}
  /// \name Misc Functions
  ///@{

  virtual void DeleteSelectedObjects() const;

  const xiiSet<xiiString>& GetUnknownObjectTypes() const { return m_UnknownObjectTypes; }
  xiiUInt32                GetUnknownObjectTypeInstances() const { return m_uiUnknownObjectTypeInstances; }

  /// \brief If disabled, this document will not be put into the recent files list.
  void SetAddToResetFilesList(bool b) { m_bAddToRecentFilesList = b; }

  /// \brief Whether this document shall be put into the recent files list.
  bool GetAddToRecentFilesList() const { return m_bAddToRecentFilesList; }

  /// \brief Broadcasts a status message event. The window that displays the document may show this in some form, e.g. in the status bar.
  void ShowDocumentStatus(const xiiFormatString& msg) const;

  /// \brief Tries to compute the position and rotation for an object in the document. Returns XII_SUCCESS if it was possible.
  virtual xiiResult ComputeObjectTransformation(const xiiDocumentObject* pObject, xiiTransform& out_result) const;

  /// \brief Needed by xiiManipulatorManager to know where to look for the manipulator attributes.
  ///
  /// Override this function for document types that use manipulators.
  /// The xiiManipulatorManager will assert that the document type doesn't return 'None' once it is in use.
  virtual xiiManipulatorSearchStrategy GetManipulatorSearchStrategy() const { return xiiManipulatorSearchStrategy::None; }

  ///@}
  /// \name Prefab Functions
  ///@{

  /// \brief Whether the document allows to create prefabs in it. This may note be allowed for prefab documents themselves, to prevent nested prefabs.
  virtual bool ArePrefabsAllowed() const { return true; }

  /// \brief Updates ALL prefabs in the document with the latest changes. Merges the current prefab templates with the instances in the document.
  virtual void UpdatePrefabs();

  /// \brief Resets the given objects to their template prefab state, if they have local modifications.
  void RevertPrefabs(const xiiDeque<const xiiDocumentObject*>& selection);

  /// \brief Removes the link between a prefab instance and its template, turning the instance into a regular object.
  virtual void UnlinkPrefabs(const xiiDeque<const xiiDocumentObject*>& selection);

  virtual xiiStatus CreatePrefabDocumentFromSelection(const char* szFile, const xiiRTTI* pRootType, xiiDelegate<void(xiiAbstractObjectNode*)> adjustGraphNodeCB = {}, xiiDelegate<void(xiiDocumentObject*)> adjustNewNodesCB = {}, xiiDelegate<void(xiiAbstractObjectGraph& graph, xiiDynamicArray<xiiAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {});
  virtual xiiStatus CreatePrefabDocument(const char* szFile, xiiArrayPtr<const xiiDocumentObject*> rootObjects, const xiiUuid& invPrefabSeed, xiiUuid& out_newDocumentGuid, xiiDelegate<void(xiiAbstractObjectNode*)> adjustGraphNodeCB = {}, bool bKeepOpen = false, xiiDelegate<void(xiiAbstractObjectGraph& graph, xiiDynamicArray<xiiAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {});

  // Returns new guid of reverted object.
  virtual xiiUuid ReplaceByPrefab(const xiiDocumentObject* pRootObject, const char* szPrefabFile, const xiiUuid& prefabAsset, const xiiUuid& prefabSeed, bool bEnginePrefab);
  // Returns new guid of reverted object.
  virtual xiiUuid RevertPrefab(const xiiDocumentObject* pObject);

  ///@}

public:
  xiiUniquePtr<xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>> m_DocumentObjectMetaData;

  mutable xiiEvent<const xiiDocumentEvent&> m_EventsOne;
  static xiiEvent<const xiiDocumentEvent&>  s_EventsAny;

  mutable xiiEvent<const xiiObjectAccessorChangeEvent&> m_ObjectAccessorChangeEvents;

protected:
  void                     SetModified(bool b);
  void                     SetReadOnly(bool b);
  virtual xiiTaskGroupID   InternalSaveDocument(AfterSaveCallback callback);
  virtual xiiStatus        InternalLoadDocument();
  virtual xiiDocumentInfo* CreateDocumentInfo() = 0;

  /// \brief A hook to execute additional code after SUCCESSFULLY saving a document. E.g. manual asset transform can be done here.
  virtual void InternalAfterSaveDocument() {}

  virtual void AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const;
  virtual void RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable);

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) {}
  virtual void InitializeAfterLoadingAndSaving() {}

  virtual void BeforeClosing();

  void SetUnknownObjectTypes(const xiiSet<xiiString>& Types, xiiUInt32 uiInstances);

  /// \name Prefab Functions
  ///@{

  virtual void UpdatePrefabsRecursive(xiiDocumentObject* pObject);
  virtual void UpdatePrefabObject(xiiDocumentObject* pObject, const xiiUuid& PrefabAsset, const xiiUuid& PrefabSeed, const char* szBasePrefab);

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

  xiiDocumentManager* m_pDocumentManager = nullptr;

  xiiString m_sDocumentPath;
  bool      m_bModified;
  bool      m_bReadOnly;
  bool      m_bWindowRequested;
  bool      m_bAddToRecentFilesList;

  xiiSet<xiiString> m_UnknownObjectTypes;
  xiiUInt32         m_uiUnknownObjectTypeInstances;

  xiiTaskGroupID m_ActiveSaveTask;
  xiiStatus      m_LastSaveResult;
};
