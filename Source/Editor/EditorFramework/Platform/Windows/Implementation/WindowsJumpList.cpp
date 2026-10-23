/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Platform/Windows/WindowsJumpList.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Types/ScopeExit.h>
#  include <GuiFoundation/UIServices/UIServices.moc.h>
#  include <QApplication>
#  include <QDir>
#  include <QFile>
#  include <QFileInfo>
#  include <QStringList>
#  include <QThread>
#  include <ToolsFoundation/Utilities/RecentFilesList.h>
#  include <objbase.h>
#  include <propkey.h>
#  include <propvarutil.h>
#  include <shellapi.h>
#  include <shobjidl.h>

namespace
{
  const wchar_t* Wide(const QString& sText)
  {
    return reinterpret_cast<const wchar_t*>(sText.utf16());
  }

  QString CleanProjectPath(const QString& sPath)
  {
    return QDir::cleanPath(QDir::fromNativeSeparators(sPath));
  }

  bool ContainsProject(const QStringList& paths, const QString& sProject)
  {
    for (const QString& sPath : paths)
    {
      if (sPath.compare(sProject, Qt::CaseInsensitive) == 0)
        return true;
    }
    return false;
  }

  HRESULT ReadRemovedProjects(IObjectArray& removed, QStringList& out_projects)
  {
    UINT    uiCount = 0;
    HRESULT hr      = removed.GetCount(&uiCount);
    if (FAILED(hr))
      return hr;
    for (UINT i = 0; i < uiCount; ++i)
    {
      IShellLinkW* pLink = nullptr;
      hr                 = removed.GetAt(i, IID_PPV_ARGS(&pLink));
      // Other applications/older versions may have published Shell items instead of links.
      if (hr == E_NOINTERFACE)
        continue;
      if (FAILED(hr))
        return hr;
      XII_SCOPE_EXIT(pLink->Release());

      IPropertyStore* pProperties = nullptr;
      hr                          = pLink->QueryInterface(IID_PPV_ARGS(&pProperties));
      if (FAILED(hr))
        return hr;
      XII_SCOPE_EXIT(pProperties->Release());
      PROPVARIANT arguments = {};
      XII_SCOPE_EXIT(PropVariantClear(&arguments));
      hr = pProperties->GetValue(PKEY_Link_Arguments, &arguments);
      if (FAILED(hr))
        return hr;
      if (arguments.vt != VT_LPWSTR || arguments.pwszVal == nullptr)
        continue;

      // A property allocates the complete argument string, unlike fixed-size GetArguments buffers.
      const QString sCommand = QStringLiteral("xiiEditor.exe ") + QString::fromWCharArray(arguments.pwszVal);
      int           iArgc    = 0;
      wchar_t**     pArgv    = CommandLineToArgvW(Wide(sCommand), &iArgc);
      if (pArgv == nullptr)
        return HRESULT_FROM_WIN32(GetLastError());
      XII_SCOPE_EXIT(LocalFree(pArgv));
      for (int a = 1; a + 1 < iArgc; ++a)
      {
        if (QString::fromWCharArray(pArgv[a]).compare(QStringLiteral("-project"), Qt::CaseInsensitive) == 0)
        {
          out_projects.push_back(CleanProjectPath(QString::fromWCharArray(pArgv[a + 1])));
          break;
        }
      }
    }
    return S_OK;
  }
} // namespace

xiiResult xiiWindowsJumpList::ConfigureAppId()
{
  return SUCCEEDED(SetCurrentProcessExplicitAppUserModelID(GetAppId())) ? XII_SUCCESS : XII_FAILURE;
}

QString xiiWindowsJumpList::BuildProjectArguments(const QString& sProject)
{
  // Windows argv quoting: double backslashes preceding a quote or the closing quote.
  QString sArguments   = QStringLiteral("-project \"");
  int     iBackslashes = 0;
  for (const QChar character : sProject)
  {
    if (character == QLatin1Char('\\'))
    {
      ++iBackslashes;
      continue;
    }
    const bool bQuote = character == QLatin1Char('"');
    sArguments += QString(iBackslashes * (bQuote ? 2 : 1) + (bQuote ? 1 : 0), QLatin1Char('\\'));
    sArguments += character;
    iBackslashes = 0;
  }
  sArguments += QString(iBackslashes * 2, QLatin1Char('\\'));
  sArguments += QLatin1Char('"');
  return sArguments;
}

HRESULT xiiWindowsJumpList::CreateLink(const QString& sExecutable, const QString& sArguments, const QString& sTitle, IShellLinkW*& out_pLink)
{
  out_pLink = nullptr;
  // CreateProcess has a 32767 UTF-16 code-unit command-line limit, including executable and NUL.
  if (sExecutable.isEmpty() || sArguments.isEmpty() || sExecutable.size() + sArguments.size() + 4 > 32767)
    return E_INVALIDARG;
  IShellLinkW* pLink = nullptr;
  HRESULT      hr    = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pLink));
  if (FAILED(hr))
    return hr;
  XII_SCOPE_EXIT(if (pLink) pLink->Release());
  if (FAILED(hr = pLink->SetPath(Wide(sExecutable))) ||
      FAILED(hr = pLink->SetArguments(Wide(sArguments))) ||
      FAILED(hr = pLink->SetWorkingDirectory(Wide(QFileInfo(sExecutable).absolutePath()))) ||
      FAILED(hr = pLink->SetIconLocation(Wide(sExecutable), 0)))
    return hr;

  IPropertyStore* pProperties = nullptr;
  hr                          = pLink->QueryInterface(IID_PPV_ARGS(&pProperties));
  if (FAILED(hr))
    return hr;
  XII_SCOPE_EXIT(pProperties->Release());
  PROPVARIANT title = {};
  PROPVARIANT appId = {};
  XII_SCOPE_EXIT(PropVariantClear(&title); PropVariantClear(&appId));
  if (FAILED(hr = InitPropVariantFromString(Wide(sTitle), &title)) ||
      FAILED(hr = InitPropVariantFromString(GetAppId(), &appId)) ||
      FAILED(hr = pProperties->SetValue(PKEY_Title, title)) ||
      FAILED(hr = pProperties->SetValue(PKEY_AppUserModel_ID, appId)) ||
      FAILED(hr = pProperties->Commit()))
    return hr;
  out_pLink = pLink;
  pLink     = nullptr;
  return S_OK;
}

HRESULT xiiWindowsJumpList::BuildList(ICustomDestinationList& destinations, const QString& sExecutable, xiiRecentFilesList& recentProjects, bool& out_bHistoryChanged)
{
  out_bHistoryChanged    = false;
  UINT          uiSlots  = 0;
  IObjectArray* pRemoved = nullptr;
  HRESULT       hr       = destinations.BeginList(&uiSlots, IID_PPV_ARGS(&pRemoved));
  if (FAILED(hr))
    return hr;
  XII_SCOPE_EXIT(pRemoved->Release());
  bool bCommitted = false;
  XII_SCOPE_EXIT(if (!bCommitted) destinations.AbortList());
  QStringList removed;
  if (FAILED(hr = ReadRemovedProjects(*pRemoved, removed)))
    return hr;

  IObjectCollection* pProjects = nullptr;
  hr                           = CoCreateInstance(CLSID_EnumerableObjectCollection, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pProjects));
  if (FAILED(hr))
    return hr;
  XII_SCOPE_EXIT(pProjects->Release());
  QStringList included;
  // Copy before Shell calls, which can dispatch messages and reenter editor code.
  const auto files = recentProjects.GetFileList();
  for (const auto& recent : files)
  {
    if (included.size() >= uiSlots)
      break;
    const QString   sProject = CleanProjectPath(xiiMakeQString(recent.m_File));
    const QFileInfo info(sProject);
    if (!info.isAbsolute() || info.fileName().compare(QStringLiteral("xiiProject"), Qt::CaseInsensitive) != 0 || !info.isFile() ||
        ContainsProject(removed, sProject) || ContainsProject(included, sProject))
      continue;
    QFile file(sProject);
    if (!file.open(QIODevice::ReadOnly))
      continue;

    IShellLinkW* pLink = nullptr;
    hr                 = CreateLink(sExecutable, BuildProjectArguments(sProject), info.dir().dirName(), pLink);
    if (FAILED(hr))
      return hr;
    XII_SCOPE_EXIT(pLink->Release());
    if (FAILED(hr = pProjects->AddObject(pLink)))
      return hr;
    included.push_back(sProject);
  }
  if (!included.isEmpty())
  {
    hr = destinations.AppendCategory(L"Recent Projects", pProjects);
    // Windows privacy policy can disable destinations while still permitting common tasks.
    if (FAILED(hr) && hr != E_ACCESSDENIED)
      return hr;
  }

  IObjectCollection* pTasks = nullptr;
  hr                        = CoCreateInstance(CLSID_EnumerableObjectCollection, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pTasks));
  if (FAILED(hr))
    return hr;
  XII_SCOPE_EXIT(pTasks->Release());
  for (bool bCreate : {true, false})
  {
    IShellLinkW* pLink = nullptr;
    hr                 = CreateLink(sExecutable, bCreate ? QStringLiteral("-createProject") : QStringLiteral("-openProject"),
                    bCreate ? QStringLiteral("Create New Project") : QStringLiteral("Open Project"), pLink);
    if (FAILED(hr))
      return hr;
    XII_SCOPE_EXIT(pLink->Release());
    if (FAILED(hr = pTasks->AddObject(pLink)))
      return hr;
  }
  if (FAILED(hr = destinations.AddUserTasks(pTasks)) || FAILED(hr = destinations.CommitList()))
    return hr;
  bCommitted = true;

  for (const auto& recent : files)
  {
    if (ContainsProject(removed, CleanProjectPath(xiiMakeQString(recent.m_File))))
      out_bHistoryChanged |= recentProjects.Remove(recent.m_File);
  }
  return S_OK;
}

xiiResult xiiWindowsJumpList::Update(xiiRecentFilesList& recentProjects, bool& out_bHistoryChanged)
{
  XII_ASSERT_DEV(qApp && QThread::currentThread() == qApp->thread(), "Jump Lists require the Qt GUI thread.");
  out_bHistoryChanged = false;
  // Do not publish Editor destinations from tests, processors, or an executable whose identity failed to initialize.
  wchar_t*      szAppId  = nullptr;
  const HRESULT identity = GetCurrentProcessExplicitAppUserModelID(&szAppId);
  XII_SCOPE_EXIT(CoTaskMemFree(szAppId));
  if (FAILED(identity) || QString::fromWCharArray(szAppId) != QString::fromWCharArray(GetAppId()))
    return XII_FAILURE;
  ICustomDestinationList* pDestinations = nullptr;
  HRESULT                 hr            = CoCreateInstance(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDestinations));
  if (SUCCEEDED(hr))
  {
    XII_SCOPE_EXIT(pDestinations->Release());
    hr = pDestinations->SetAppID(GetAppId());
    if (SUCCEEDED(hr))
      hr = BuildList(*pDestinations, xiiMakeQString(xiiOSFile::GetApplicationPath()), recentProjects, out_bHistoryChanged);
  }
  if (FAILED(hr))
  {
    xiiLog::Warning("Unable to update the Editor Jump List (HRESULT 0x{0}).", xiiArgU(static_cast<xiiUInt32>(hr), 8, true, 16));
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

#endif
