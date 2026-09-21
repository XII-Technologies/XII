/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiToolsProject;
class xiiDocument;

struct xiiToolsProjectEvent
{
  enum class Type
  {
    ProjectCreated,
    ProjectOpened,
    ProjectFirstSetup,
    ProjectSaveState,
    ProjectClosing,
    ProjectClosed,
    ProjectConfigChanged, ///< Sent when global project configuration data was changed and thus certain menus would need to update their content (or
                          ///< just deselect any item, forcing the user to reselect and thus update state)
    SaveAll,              ///< When sent, this shall save all outstanding modifications
  };

  xiiToolsProject* m_pProject;
  Type             m_Type;
};

struct xiiToolsProjectRequest
{
  xiiToolsProjectRequest();

  enum class Type
  {
    CanCloseProject,        ///< Can we close the project? Listener needs to set m_bCanClose if not.
    CanCloseDocuments,      ///< Can we close the documents in m_Documents? Listener needs to set m_bCanClose if not.
    SuggestContainerWindow, ///< m_Documents contains one element that a container window should be suggested for and written to
                            ///< m_iContainerWindowUniqueIdentifier.
    GetPathForDocumentGuid,
  };

  Type                          m_Type;
  bool                          m_bCanClose; ///< When the event is sent, interested code can set this to false to prevent closing.
  xiiDynamicArray<xiiDocument*> m_Documents; ///< In case of 'CanCloseDocuments', these will be the documents in question.
  xiiInt32
    m_iContainerWindowUniqueIdentifier; ///< In case of 'SuggestContainerWindow', the ID of the container to be used for the docs in m_Documents.

  xiiUuid          m_documentGuid;
  xiiStringBuilder m_sAbsDocumentPath;
};

class XII_TOOLSFOUNDATION_DLL xiiToolsProject
{
  XII_DECLARE_SINGLETON(xiiToolsProject);

public:
  static xiiEvent<const xiiToolsProjectEvent&, xiiMutex> s_Events;
  static xiiEvent<xiiToolsProjectRequest&>               s_Requests;

public:
  static bool IsProjectOpen() { return GetSingleton() != nullptr; }
  static bool IsProjectClosing() { return (GetSingleton() != nullptr && GetSingleton()->m_bIsClosing); }
  static void CloseProject();
  static void SaveProjectState();
  /// Returns true when the project can be closed. Uses xiiToolsProjectRequest::Type::CanCloseProject event.
  static bool CanCloseProject();
  /// Returns true when the given list of documents can be closed. Uses xiiToolsProjectRequest::Type::CanCloseDocuments event.
  static bool CanCloseDocuments(xiiArrayPtr<xiiDocument*> documents);
  /// Returns the unique ID of the container window this document should use for its window. Uses
  /// xiiToolsProjectRequest::Type::SuggestContainerWindow event.
  static xiiInt32 SuggestContainerWindow(xiiDocument* pDoc);
  /// Resolve document GUID into an absolute path.
  xiiStringBuilder GetPathForDocumentGuid(const xiiUuid& guid);
  static xiiStatus OpenProject(xiiStringView sProjectPath);
  static xiiStatus CreateProject(xiiStringView sProjectPath);

  /// Broadcasts the SaveAll event, though otherwise has no direct effect.
  static void BroadcastSaveAll();

  /// Sent when global project configuration data was changed and thus certain menus would need to update their content (or just deselect any
  /// item, forcing the user to reselect and thus update state)
  static void BroadcastConfigChanged();

  /// Returns the path to the 'xiiProject' file
  const xiiString& GetProjectFile() const { return m_sProjectPath; }

  /// Returns the short name of the project (extracted from the path).
  ///
  /// \param bSanitize Whether to replace whitespace and other problematic characters, such that it can be used in code.
  const xiiString GetProjectName(bool bSanitize) const;

  /// Returns the path in which the 'xiiProject' file is stored
  xiiString GetProjectDirectory() const;

  /// Returns the directory path in which project settings etc. should be stored
  xiiString GetProjectDataFolder() const;

  /// Starts at the  given document and then searches the tree upwards until it finds a xiiProject file.
  static xiiString FindProjectDirectoryForDocument(xiiStringView sDocumentPath);

  bool IsDocumentInAllowedRoot(xiiStringView sDocumentPath, xiiString* out_pRelativePath = nullptr) const;

  void AddAllowedDocumentRoot(xiiStringView sPath);

  /// Makes sure the given sub-folder exists inside the project directory
  void CreateSubFolder(xiiStringView sFolder) const;

private:
  static xiiStatus CreateOrOpenProject(xiiStringView sProjectPath, bool bCreate);

private:
  xiiToolsProject(xiiStringView sProjectPath);
  ~xiiToolsProject();

  xiiStatus Create();
  xiiStatus Open();

private:
  bool                         m_bIsClosing;
  xiiString                    m_sProjectPath;
  xiiHybridArray<xiiString, 4> m_AllowedDocumentRoots;
};
