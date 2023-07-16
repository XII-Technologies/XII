#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Project/ToolsProject.h>

XII_IMPLEMENT_SINGLETON(xiiToolsProject);

xiiEvent<const xiiToolsProjectEvent&> xiiToolsProject::s_Events;
xiiEvent<xiiToolsProjectRequest&>     xiiToolsProject::s_Requests;


xiiToolsProjectRequest::xiiToolsProjectRequest()
{
  m_Type                             = Type::CanCloseProject;
  m_bCanClose                        = true;
  m_iContainerWindowUniqueIdentifier = 0;
}

xiiToolsProject::xiiToolsProject(const char* szProjectPath) :
  m_SingletonRegistrar(this)
{
  m_bIsClosing = false;

  m_sProjectPath = szProjectPath;
  XII_ASSERT_DEV(!m_sProjectPath.IsEmpty(), "Path cannot be empty.");
}

xiiToolsProject::~xiiToolsProject() = default;

xiiStatus xiiToolsProject::Create()
{
  {
    xiiOSFile ProjectFile;
    if (ProjectFile.Open(m_sProjectPath, xiiFileOpenMode::Write).Failed())
    {
      return xiiStatus(xiiFmt("Could not open/create the project file for writing: '{0}'", m_sProjectPath));
    }
    else
    {
      const char* szToken = "xiiEditor Project File";

      XII_SUCCEED_OR_RETURN(ProjectFile.Write(szToken, xiiStringUtils::GetStringElementCount(szToken) + 1));
      ProjectFile.Close();
    }
  }

  xiiToolsProjectEvent e;
  e.m_pProject = this;
  e.m_Type     = xiiToolsProjectEvent::Type::ProjectCreated;
  s_Events.Broadcast(e);

  return Open();
}

xiiStatus xiiToolsProject::Open()
{
  xiiOSFile ProjectFile;
  if (ProjectFile.Open(m_sProjectPath, xiiFileOpenMode::Read).Failed())
  {
    return xiiStatus(xiiFmt("Could not open the project file for reading: '{0}'", m_sProjectPath));
  }

  ProjectFile.Close();

  xiiToolsProjectEvent e;
  e.m_pProject = this;
  e.m_Type     = xiiToolsProjectEvent::Type::ProjectOpened;
  s_Events.Broadcast(e);

  return xiiStatus(XII_SUCCESS);
}

void xiiToolsProject::CreateSubFolder(const char* szFolder) const
{
  xiiStringBuilder sPath;

  sPath = m_sProjectPath;
  sPath.PathParentDirectory();
  sPath.AppendPath(szFolder);

  xiiOSFile::CreateDirectoryStructure(sPath).IgnoreResult();
}

void xiiToolsProject::CloseProject()
{
  if (GetSingleton())
  {
    GetSingleton()->m_bIsClosing = true;

    xiiToolsProjectEvent e;
    e.m_pProject = GetSingleton();
    e.m_Type     = xiiToolsProjectEvent::Type::ProjectClosing;
    s_Events.Broadcast(e);

    xiiDocumentManager::CloseAllDocuments();

    delete GetSingleton();

    e.m_Type = xiiToolsProjectEvent::Type::ProjectClosed;
    s_Events.Broadcast(e);
  }
}

void xiiToolsProject::SaveProjectState()
{
  if (GetSingleton())
  {
    GetSingleton()->m_bIsClosing = true;

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

xiiStatus xiiToolsProject::CreateOrOpenProject(const char* szProjectPath, bool bCreate)
{
  CloseProject();

  new xiiToolsProject(szProjectPath);

  xiiStatus ret;

  if (bCreate)
  {
    ret = GetSingleton()->Create();
    xiiToolsProject::SaveProjectState();
  }
  else
    ret = GetSingleton()->Open();

  if (ret.m_Result.Failed())
  {
    delete GetSingleton();
    return ret;
  }

  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiToolsProject::OpenProject(const char* szProjectPath)
{
  xiiStatus status = CreateOrOpenProject(szProjectPath, false);

  return status;
}

xiiStatus xiiToolsProject::CreateProject(const char* szProjectPath)
{
  return CreateOrOpenProject(szProjectPath, true);
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

void xiiToolsProject::AddAllowedDocumentRoot(const char* szPath)
{
  xiiStringBuilder s = szPath;
  s.MakeCleanPath();
  s.Trim("", "/");

  m_AllowedDocumentRoots.PushBack(s);
}


bool xiiToolsProject::IsDocumentInAllowedRoot(const char* szDocumentPath, xiiString* out_pRelativePath) const
{
  for (xiiUInt32 i = m_AllowedDocumentRoots.GetCount(); i > 0; --i)
  {
    const auto& root = m_AllowedDocumentRoots[i - 1];

    xiiStringBuilder s = szDocumentPath;
    if (!s.IsPathBelowFolder(root))
      continue;

    if (out_pRelativePath)
    {
      xiiStringBuilder sText = szDocumentPath;
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
    return sTemp.GetFileName();

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
    sTemp.Format("Project{}", uiHash);
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
  xiiStringBuilder s = GetProjectFile();
  s.Append("_data");

  return s;
}

xiiString xiiToolsProject::FindProjectDirectoryForDocument(const char* szDocumentPath)
{
  xiiStringBuilder sPath = szDocumentPath;
  sPath.PathParentDirectory();

  xiiStringBuilder sTemp;

  while (!sPath.IsEmpty())
  {
    sTemp = sPath;
    sTemp.AppendPath("xiiProject");

    if (xiiOSFile::ExistsFile(sTemp))
      return sPath;

    sPath.PathParentDirectory();
  }

  return "";
}
