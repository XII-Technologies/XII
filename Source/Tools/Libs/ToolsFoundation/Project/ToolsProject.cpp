/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Project/ToolsProject.h>

XII_IMPLEMENT_SINGLETON(xiiToolsProject);

std::atomic<xiiUInt64> xiiToolsProject::s_uiProjectGeneration = 0;

namespace
{
  bool s_bOpeningProject = false;
}


xiiEvent<const xiiToolsProjectEvent&, xiiMutex> xiiToolsProject::s_Events;
xiiEvent<xiiToolsProjectRequest&>               xiiToolsProject::s_Requests;


xiiToolsProjectRequest::xiiToolsProjectRequest()
{
  m_Type                             = Type::CanCloseProject;
  m_bCanClose                        = true;
  m_iContainerWindowUniqueIdentifier = 0;
}

xiiToolsProject::xiiToolsProject(xiiStringView sProjectPath) :
  m_SingletonRegistrar(this)
{
  ++s_uiProjectGeneration;

  m_sProjectPath = sProjectPath;
  XII_ASSERT_DEV(!m_sProjectPath.IsEmpty(), "Path cannot be empty.");
}

xiiToolsProject::~xiiToolsProject() = default;

xiiStatus xiiToolsProject::Create()
{
  if (xiiOSFile::ExistsFile(m_sProjectPath))
    return xiiStatus("Refusing to overwrite an existing project. Open it instead.");
  m_Manifest.Initialize(m_sProjectPath);
  XII_SUCCEED_OR_RETURN(m_Manifest.Save(m_sProjectPath));

  {
    xiiToolsProjectEvent e;
    e.m_pProject = this;
    e.m_Type     = xiiToolsProjectEvent::Type::ProjectCreated;
    s_Events.Broadcast(e);
  }

  XII_SUCCEED_OR_RETURN(Open());

  // if this file already exists, the project was created from a template and should not get additional setup
  xiiStringBuilder path(GetProjectDirectory(), "/Scenes/Main.xiiScene");
  if (!xiiOSFile::ExistsFile(path))
  {
    xiiToolsProjectEvent e;
    e.m_pProject = this;
    e.m_Type     = xiiToolsProjectEvent::Type::ProjectFirstSetup;
    s_Events.Broadcast(e);
  }

  return XII_SUCCESS;
}

xiiStatus xiiToolsProject::Open()
{
  XII_SUCCEED_OR_RETURN(m_Manifest.Load(m_sProjectPath));

  xiiStatus            status(XII_SUCCESS);
  xiiToolsProjectEvent e;
  e.m_pProject              = this;
  e.m_Type                  = xiiToolsProjectEvent::Type::ProjectOpening;
  e.m_pInitializationStatus = &status;
  s_Events.Broadcast(e);
  if (status.Failed())
  {
    m_State  = State::Failed;
    e.m_Type = xiiToolsProjectEvent::Type::ProjectOpenFailed;
    s_Events.Broadcast(e);
    return status;
  }

  m_State                   = State::Ready;
  e.m_pInitializationStatus = nullptr;
  e.m_Type                  = xiiToolsProjectEvent::Type::ProjectOpened;
  s_Events.Broadcast(e);

  return XII_SUCCESS;
}

xiiStatus xiiToolsProject::SaveManifest(const xiiProjectManifest& manifest)
{
  if (!IsProjectOpen() || manifest.m_ProjectID != m_Manifest.m_ProjectID)
    return xiiStatus("Cannot save a manifest outside a ready project or change its persistent identity.");
  XII_SUCCEED_OR_RETURN(manifest.CheckEngineCompatibility());
  XII_SUCCEED_OR_RETURN(manifest.Save(m_sProjectPath));
  m_Manifest = manifest;
  return XII_SUCCESS;
}

void xiiToolsProject::CreateSubFolder(xiiStringView sFolder) const
{
  xiiStringBuilder sPath;

  sPath = m_sProjectPath;
  sPath.PathParentDirectory();
  sPath.AppendPath(sFolder);

  xiiOSFile::CreateDirectoryStructure(sPath).IgnoreResult();
}

void xiiToolsProject::CloseProject()
{
  if (!s_bOpeningProject && GetSingleton() && GetSingleton()->m_State != State::Opening && !IsProjectClosing())
  {
    GetSingleton()->m_State = State::Closing;
    ++s_uiProjectGeneration;

    xiiToolsProjectEvent e;
    e.m_pProject = GetSingleton();
    e.m_Type     = xiiToolsProjectEvent::Type::ProjectClosing;
    s_Events.Broadcast(e);

    xiiDocumentManager::CloseAllDocuments();

    delete GetSingleton();

    e.m_pProject = nullptr;
    e.m_Type     = xiiToolsProjectEvent::Type::ProjectClosed;
    s_Events.Broadcast(e);
  }
}

void xiiToolsProject::SaveProjectState()
{
  if (IsProjectOpen())
  {

    xiiToolsProjectEvent e;
    e.m_pProject = GetSingleton();
    e.m_Type     = xiiToolsProjectEvent::Type::ProjectSaveState;
    s_Events.Broadcast(e, 1);
  }
}

bool xiiToolsProject::CanCloseProject()
{
  if (GetSingleton() == nullptr)
    return true;

  xiiToolsProjectRequest e;
  e.m_Type      = xiiToolsProjectRequest::Type::CanCloseProject;
  e.m_bCanClose = true;
  s_Requests.Broadcast(e, 1); // when the save dialog pops up and the user presses 'Save' we need to allow one more recursion

  return e.m_bCanClose;
}

bool xiiToolsProject::CanCloseDocuments(xiiArrayPtr<xiiDocument*> documents)
{
  if (GetSingleton() == nullptr)
    return true;

  xiiToolsProjectRequest e;
  e.m_Type      = xiiToolsProjectRequest::Type::CanCloseDocuments;
  e.m_bCanClose = true;
  e.m_Documents = documents;
  s_Requests.Broadcast(e);

  return e.m_bCanClose;
}

xiiInt32 xiiToolsProject::SuggestContainerWindow(xiiDocument* pDoc)
{
  if (pDoc == nullptr)
  {
    return 0;
  }
  xiiToolsProjectRequest e;
  e.m_Type = xiiToolsProjectRequest::Type::SuggestContainerWindow;
  e.m_Documents.PushBack(pDoc);
  s_Requests.Broadcast(e);

  return e.m_iContainerWindowUniqueIdentifier;
}

xiiStringBuilder xiiToolsProject::GetPathForDocumentGuid(const xiiUuid& guid)
{
  xiiToolsProjectRequest e;
  e.m_Type         = xiiToolsProjectRequest::Type::GetPathForDocumentGuid;
  e.m_documentGuid = guid;
  s_Requests.Broadcast(e, 1); // this can be sent while CanCloseProject is processed, so allow one additional recursion depth
  return e.m_sAbsDocumentPath;
}

xiiStatus xiiToolsProject::CreateOrOpenProject(xiiStringView sProjectPath, bool bCreate)
{
  if (s_bOpeningProject || (GetSingleton() && GetSingleton()->m_State != State::Ready))
    return xiiStatus("Cannot open a project during another project lifecycle transition.");
  // Reject malformed/incompatible candidates before destroying the current project.
  if (!bCreate)
  {
    xiiProjectManifest candidate;
    XII_SUCCEED_OR_RETURN(candidate.Load(sProjectPath));
  }
  CloseProject();
  s_bOpeningProject = true;
  XII_SCOPE_EXIT(s_bOpeningProject = false);
  new xiiToolsProject(sProjectPath);

  xiiStatus ret(XII_SUCCESS);

  if (bCreate)
  {
    ret = GetSingleton()->Create();
    if (ret.Succeeded())
      xiiToolsProject::SaveProjectState();
  }
  else
    ret = GetSingleton()->Open();

  if (ret.Failed())
  {
    GetSingleton()->m_State = State::Failed;
    s_bOpeningProject       = false;
    CloseProject();
    return ret;
  }

  return XII_SUCCESS;
}

xiiStatus xiiToolsProject::OpenProject(xiiStringView sProjectPath)
{
  xiiStatus status = CreateOrOpenProject(sProjectPath, false);

  return status;
}

xiiStatus xiiToolsProject::CreateProject(xiiStringView sProjectPath)
{
  return CreateOrOpenProject(sProjectPath, true);
}

void xiiToolsProject::BroadcastSaveAll()
{
  xiiToolsProjectEvent e;
  e.m_pProject = GetSingleton();
  e.m_Type     = xiiToolsProjectEvent::Type::SaveAll;

  s_Events.Broadcast(e);
}

void xiiToolsProject::BroadcastConfigChanged()
{
  xiiToolsProjectEvent e;
  e.m_pProject = GetSingleton();
  e.m_Type     = xiiToolsProjectEvent::Type::ProjectConfigChanged;

  s_Events.Broadcast(e);
}

void xiiToolsProject::AddAllowedDocumentRoot(xiiStringView sPath)
{
  xiiStringBuilder s = sPath;
  s.MakeCleanPath();
  s.Trim("", "/");

  m_AllowedDocumentRoots.PushBack(s);
}


bool xiiToolsProject::IsDocumentInAllowedRoot(xiiStringView sDocumentPath, xiiString* out_pRelativePath) const
{
  for (xiiUInt32 i = m_AllowedDocumentRoots.GetCount(); i > 0; --i)
  {
    const auto& root = m_AllowedDocumentRoots[i - 1];

    xiiStringBuilder s = sDocumentPath;
    if (!s.IsPathBelowFolder(root))
      continue;

    if (out_pRelativePath)
    {
      xiiStringBuilder sText = sDocumentPath;
      sText.MakeRelativeTo(root).IgnoreResult();

      *out_pRelativePath = sText;
    }

    return true;
  }

  return false;
}

const xiiString xiiToolsProject::GetProjectName(bool bSanitize) const
{
  xiiStringBuilder sTemp = xiiToolsProject::GetSingleton()->GetProjectFile();
  sTemp.PathParentDirectory();
  sTemp.Trim("/");

  if (!bSanitize)
    return m_Manifest.m_sName;

  const xiiStringBuilder sOrgName = sTemp.GetFileName();
  sTemp.Clear();

  bool bAnyAscii = false;

  for (xiiStringIterator it = sOrgName.GetIteratorFront(); it.IsValid(); ++it)
  {
    const xiiUInt32 c = it.GetCharacter();

    if (!xiiStringUtils::IsIdentifierDelimiter_C_Code(c))
    {
      bAnyAscii = true;

      // valid character to be used in C as an identifier
      sTemp.Append(c);
    }
    else if (c == ' ')
    {
      // skip
    }
    else
    {
      sTemp.AppendFormat("{}", xiiArgU(c, 1, false, 16));
    }
  }

  if (!bAnyAscii)
  {
    const xiiUInt32 uiHash = xiiHashingUtils::xxHash32String(sTemp);
    sTemp.SetFormat("Project{}", uiHash);
  }

  if (sTemp.IsEmpty())
  {
    sTemp = "Project";
  }

  if (sTemp.GetCharacterCount() > 20)
  {
    sTemp.Shrink(0, sTemp.GetCharacterCount() - 20);
  }

  return sTemp;
}

xiiString xiiToolsProject::GetProjectDirectory() const
{
  xiiStringBuilder s = GetProjectFile();

  s.PathParentDirectory();
  s.Trim("", "/\\");

  return s;
}

xiiString xiiToolsProject::GetProjectDataFolder() const
{
  xiiStringBuilder s = GetProjectDirectory();
  s.AppendPath("Editor");

  return s;
}

xiiString xiiToolsProject::FindProjectDirectoryForDocument(xiiStringView sDocumentPath)
{
  xiiStringBuilder sPath = sDocumentPath;
  sPath.PathParentDirectory();

  xiiStringBuilder sTemp;

  while (!sPath.IsEmpty())
  {
    sTemp = sPath;
    sTemp.AppendPath("xiiProject");

    if (xiiOSFile::ExistsFile(sTemp))
    {
      xiiProjectManifest manifest;
      if (manifest.Load(sTemp).Succeeded())
        return sPath;
      // A malformed nearest project is a boundary, never silently select a parent project.
      return "";
    }

    sPath.PathParentDirectory();
  }

  return "";
}
