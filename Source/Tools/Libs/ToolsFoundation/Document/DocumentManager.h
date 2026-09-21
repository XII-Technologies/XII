/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class XII_TOOLSFOUNDATION_DLL xiiDocumentManager : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDocumentManager, xiiReflectedClass);

public:
  virtual ~xiiDocumentManager() = default;

  static const xiiHybridArray<xiiDocumentManager*, 16>& GetAllDocumentManagers() { return s_AllDocumentManagers; }

  static xiiResult FindDocumentTypeFromPath(xiiStringView sPath, bool bForCreation, const xiiDocumentTypeDescriptor*& out_pTypeDesc);

  xiiStatus CanOpenDocument(xiiStringView sFilePath) const;

  /// Creates a new document.
  /// \param szDocumentTypeName Document type to create. See xiiDocumentTypeDescriptor.
  /// \param szPath Absolute path to the document to be created.
  /// \param out_pDocument Out parameter for the resulting xiiDocument. Will be nullptr on failure.
  /// \param flags Flags to define various options like whether a window should be created.
  /// \param pOpenContext An generic context object. Allows for custom data to be passed along into the construction. E.g. inform a sub-document which main document it belongs to.
  /// \return Returns the error in case the operations failed.
  xiiStatus CreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, xiiDocument*& out_pDocument, xiiBitflags<xiiDocumentFlags> flags = xiiDocumentFlags::None, const xiiDocumentObject* pOpenContext = nullptr);

  /// Opens an existing document.
  /// \param szDocumentTypeName Document type to open. See xiiDocumentTypeDescriptor.
  /// \param szPath Absolute path to the document to be opened.
  /// \param out_pDocument Out parameter for the resulting xiiDocument. Will be nullptr on failure.
  /// \param flags Flags to define various options like whether a window should be created.
  /// \param pOpenContext  An generic context object. Allows for custom data to be passed along into the construction. E.g. inform a sub-document which main document it belongs to.
  /// \return Returns the error in case the operations failed.
  /// \return Returns the error in case the operations failed.
  xiiStatus         OpenDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, xiiDocument*& out_pDocument, xiiBitflags<xiiDocumentFlags> flags = xiiDocumentFlags::AddToRecentFilesList | xiiDocumentFlags::RequestWindow, const xiiDocumentObject* pOpenContext = nullptr);
  virtual xiiStatus CloneDocument(xiiStringView sPath, xiiStringView sClonePath, xiiUuid& inout_cloneGuid);
  void              CloseDocument(xiiDocument* pDocument);
  void              EnsureWindowRequested(xiiDocument* pDocument, const xiiDocumentObject* pOpenContext = nullptr);

  /// Returns a list of all currently open documents that are managed by this document manager
  const xiiDynamicArray<xiiDocument*>& GetAllOpenDocuments() const { return m_AllOpenDocuments; }

  xiiDocument* GetDocumentByPath(xiiStringView sPath) const;

  static xiiDocument* GetDocumentByGuid(const xiiUuid& guid);

  /// If the given document is open, it will be closed. User is not asked about it, unsaved changes are discarded. Returns true if the document
  /// was open and needed to be closed.
  static bool EnsureDocumentIsClosedInAllManagers(xiiStringView sPath);

  /// If the given document is open, it will be closed. User is not asked about it, unsaved changes are discarded. Returns true if the document
  /// was open and needed to be closed. This function only operates on documents opened by this manager. Use EnsureDocumentIsClosedInAllManagers() to
  /// close documents of any type.
  bool EnsureDocumentIsClosed(xiiStringView sPath);

  void        CloseAllDocumentsOfManager();
  static void CloseAllDocuments();

  struct Event
  {
    enum class Type
    {
      DocumentTypesRemoved,
      DocumentTypesAdded,
      DocumentOpened,
      DocumentWindowRequested,      ///< Sent when the window for a document is needed. Each plugin should check this and see if it can create the desired
                                    ///< window type
      AfterDocumentWindowRequested, ///< Sent after a document window was requested. Can be used to do things after the new window has been opened
      DocumentClosing,
      DocumentClosing2, // sent after DocumentClosing but before removing the document, use this to do stuff that depends on code executed during
                        // DocumentClosing
      DocumentClosed,   // this will not point to a valid document anymore, as the document is deleted, use DocumentClosing to get the event before it
                        // is deleted
    };

    Type                     m_Type;
    xiiDocument*             m_pDocument    = nullptr;
    const xiiDocumentObject* m_pOpenContext = nullptr;
  };

  struct Request
  {
    enum class Type
    {
      DocumentAllowedToOpen,
    };

    Type      m_Type;
    xiiString m_sDocumentType;
    xiiString m_sDocumentPath;
    xiiStatus m_RequestStatus = XII_SUCCESS;
  };

  static xiiCopyOnBroadcastEvent<const Event&> s_Events;
  static xiiEvent<Request&>                    s_Requests;

  static const xiiDocumentTypeDescriptor*                           GetDescriptorForDocumentType(xiiStringView sDocumentType);
  static const xiiMap<xiiString, const xiiDocumentTypeDescriptor*>& GetAllDocumentDescriptors();

  void GetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_documentTypes) const;

  using CustomAction = xiiVariant (*)(const xiiDocument*);
  static xiiMap<xiiString, CustomAction> s_CustomActions;

protected:
  virtual void InternalCloneDocument(xiiStringView sPath, xiiStringView sClonePath, const xiiUuid& documentId, const xiiUuid& seedGuid, const xiiUuid& cloneGuid, xiiAbstractObjectGraph* pHeader, xiiAbstractObjectGraph* pObjects, xiiAbstractObjectGraph* pTypes);

private:
  virtual void InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext) = 0;
  virtual void InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const                                                           = 0;

private:
  xiiStatus CreateOrOpenDocument(bool bCreate, xiiStringView sDocumentTypeName, xiiStringView sPath, xiiDocument*& out_pDocument, xiiBitflags<xiiDocumentFlags> flags, const xiiDocumentObject* pOpenContext = nullptr);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, DocumentManager);

  static void OnPluginEvent(const xiiPluginEvent& e);

  static void UpdateBeforeUnloadingPlugins(const xiiPluginEvent& e);
  static void UpdatedAfterLoadingPlugins();

  xiiDynamicArray<xiiDocument*> m_AllOpenDocuments;

  static xiiSet<const xiiRTTI*>                  s_KnownManagers;
  static xiiHybridArray<xiiDocumentManager*, 16> s_AllDocumentManagers;

  static xiiMap<xiiString, const xiiDocumentTypeDescriptor*> s_AllDocumentDescriptors;
};
