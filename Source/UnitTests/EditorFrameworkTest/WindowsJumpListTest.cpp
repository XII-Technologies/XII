/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Platform/Windows/WindowsJumpList.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <EditorFramework/Dialogs/CreateProjectDlg.moc.h>
#  include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#  include <EditorFramework/EditorApp/EditorApp.moc.h>
#  include <Foundation/Application/Application.h>
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Types/ScopeExit.h>
#  include <Foundation/Utilities/CommandLineUtils.h>
#  include <GuiFoundation/UIServices/UIServices.moc.h>
#  include <QApplication>
#  include <QDir>
#  include <QFile>
#  include <QStringList>
#  include <QTemporaryDir>
#  include <QThread>
#  include <QTimer>
#  include <ToolsFoundation/Utilities/RecentFilesList.h>
#  include <objbase.h>
#  include <propkey.h>
#  include <propvarutil.h>
#  include <shellapi.h>
#  include <shobjidl.h>

struct xiiWindowsJumpListTest
{
  static QString Arguments(const QString& sProject) { return xiiWindowsJumpList::BuildProjectArguments(sProject); }
  static HRESULT Link(const QString& sArguments, IShellLinkW*& out_pLink)
  {
    return xiiWindowsJumpList::CreateLink(xiiMakeQString(xiiOSFile::GetApplicationPath()), sArguments, QStringLiteral("Project"), out_pLink);
  }
  static HRESULT Build(ICustomDestinationList& list, xiiRecentFilesList& recent, bool& out_bChanged, const QString& sExecutable = xiiMakeQString(xiiOSFile::GetApplicationPath()))
  {
    out_bChanged     = false;
    const HRESULT hr = list.SetAppID(xiiWindowsJumpList::GetAppId());
    if (FAILED(hr))
      return hr;
    return xiiWindowsJumpList::BuildList(list, sExecutable, recent, out_bChanged);
  }
  static HRESULT BuildConfigured(ICustomDestinationList& list, xiiRecentFilesList& recent, bool& out_bChanged)
  {
    return xiiWindowsJumpList::BuildList(list, xiiMakeQString(xiiOSFile::GetApplicationPath()), recent, out_bChanged);
  }
};

namespace
{
  struct ShellApartment
  {
    ShellApartment() { XII_TEST_BOOL(SUCCEEDED(m_Result) || m_Result == RPC_E_CHANGED_MODE); }
    ~ShellApartment()
    {
      if (SUCCEEDED(m_Result))
        CoUninitialize();
    }
    HRESULT m_Result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  };

  QString LinkProperty(IShellLinkW& link, REFPROPERTYKEY key)
  {
    IPropertyStore* pProperties = nullptr;
    if (!XII_TEST_BOOL(SUCCEEDED(link.QueryInterface(IID_PPV_ARGS(&pProperties)))))
      return {};
    XII_SCOPE_EXIT(pProperties->Release());
    PROPVARIANT value = {};
    XII_SCOPE_EXIT(PropVariantClear(&value));
    if (!XII_TEST_BOOL(SUCCEEDED(pProperties->GetValue(key, &value))) || value.vt != VT_LPWSTR)
      return {};
    return QString::fromWCharArray(value.pwszVal);
  }

  QStringList ParseArguments(const QString& sArguments)
  {
    const QString sCommand = QStringLiteral("xiiEditor.exe ") + sArguments;
    int           iArgc    = 0;
    wchar_t**     pArgv    = CommandLineToArgvW(reinterpret_cast<const wchar_t*>(sCommand.utf16()), &iArgc);
    if (!XII_TEST_BOOL(pArgv != nullptr))
      return {};
    XII_SCOPE_EXIT(LocalFree(pArgv));
    QStringList result;
    for (int i = 0; i < iArgc; ++i)
      result.push_back(QString::fromWCharArray(pArgv[i]));
    return result;
  }

  QString ProjectFile(QTemporaryDir& root, const QString& sName)
  {
    const QString sDirectory = root.path() + QLatin1Char('/') + sName;
    XII_TEST_BOOL(QDir().mkpath(sDirectory));
    const QString sProject = sDirectory + QStringLiteral("/xiiProject");
    QFile         file(sProject);
    XII_TEST_BOOL(file.open(QIODevice::WriteOnly));
    return sProject;
  }

  class DestinationRecorder final : public ICustomDestinationList
  {
  public:
    DestinationRecorder()
    {
      XII_TEST_BOOL(SUCCEEDED(CoCreateInstance(CLSID_EnumerableObjectCollection, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pRemoved))));
    }
    ~DestinationRecorder() { m_pRemoved->Release(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
      *ppv = nullptr;
      if (riid != IID_IUnknown && riid != IID_ICustomDestinationList)
        return E_NOINTERFACE;
      *ppv = static_cast<ICustomDestinationList*>(this);
      AddRef();
      return S_OK;
    }
    ULONG STDMETHODCALLTYPE   AddRef() override { return ++m_uiReferences; }
    ULONG STDMETHODCALLTYPE   Release() override { return --m_uiReferences; }
    HRESULT STDMETHODCALLTYPE SetAppID(LPCWSTR szAppId) override
    {
      XII_TEST_BOOL(!m_bBegun);
      m_sAppId = QString::fromWCharArray(szAppId);
      return m_IdentityResult;
    }
    HRESULT STDMETHODCALLTYPE BeginList(UINT* pSlots, REFIID riid, void** ppv) override
    {
      *ppv = nullptr;
      if (FAILED(m_BeginResult))
        return m_BeginResult;
      m_Projects.clear();
      m_Tasks.clear();
      m_bBegun = true;
      *pSlots  = m_uiSlots;
      return m_pRemoved->QueryInterface(riid, ppv);
    }
    HRESULT STDMETHODCALLTYPE AppendCategory(LPCWSTR szCategory, IObjectArray* pItems) override
    {
      XII_TEST_BOOL(m_bBegun);
      XII_TEST_BOOL(QString::fromWCharArray(szCategory) == QStringLiteral("Recent Projects"));
      ReadLinks(*pItems, m_Projects);
      return m_CategoryResult;
    }
    HRESULT STDMETHODCALLTYPE AppendKnownCategory(KNOWNDESTCATEGORY) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE AddUserTasks(IObjectArray* pItems) override
    {
      XII_TEST_BOOL(m_bBegun);
      ReadLinks(*pItems, m_Tasks);
      return m_TasksResult;
    }
    HRESULT STDMETHODCALLTYPE CommitList() override
    {
      XII_TEST_BOOL(m_bBegun);
      ++m_uiCommits;
      if (SUCCEEDED(m_CommitResult))
      {
        m_bBegun = false;
        XII_TEST_BOOL(SUCCEEDED(m_pRemoved->Clear()));
      }
      return m_CommitResult;
    }
    HRESULT STDMETHODCALLTYPE GetRemovedDestinations(REFIID riid, void** ppv) override { return m_pRemoved->QueryInterface(riid, ppv); }
    HRESULT STDMETHODCALLTYPE DeleteList(LPCWSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE AbortList() override
    {
      XII_TEST_BOOL(m_bBegun);
      m_bBegun = false;
      ++m_uiAborts;
      return S_OK;
    }
    void RemoveProject(const QString& sPath)
    {
      IShellLinkW* pLink = nullptr;
      if (!XII_TEST_BOOL(SUCCEEDED(xiiWindowsJumpListTest::Link(xiiWindowsJumpListTest::Arguments(sPath), pLink))))
        return;
      XII_SCOPE_EXIT(pLink->Release());
      XII_TEST_BOOL(SUCCEEDED(m_pRemoved->AddObject(pLink)));
    }
    void ReadLinks(IObjectArray& items, QStringList& out_arguments)
    {
      XII_TEST_BOOL(QThread::currentThread() == qApp->thread());
      out_arguments.clear();
      UINT uiCount = 0;
      XII_TEST_BOOL(SUCCEEDED(items.GetCount(&uiCount)));
      for (UINT i = 0; i < uiCount; ++i)
      {
        IShellLinkW* pLink = nullptr;
        if (!XII_TEST_BOOL(SUCCEEDED(items.GetAt(i, IID_PPV_ARGS(&pLink)))))
          continue;
        XII_SCOPE_EXIT(pLink->Release());
        out_arguments.push_back(LinkProperty(*pLink, PKEY_Link_Arguments));
        XII_TEST_BOOL(!LinkProperty(*pLink, PKEY_Title).isEmpty());
        XII_TEST_BOOL(LinkProperty(*pLink, PKEY_AppUserModel_ID) == QString::fromWCharArray(xiiWindowsJumpList::GetAppId()));
        wchar_t szIcon[32768];
        int     iIcon = -1;
        XII_TEST_BOOL(SUCCEEDED(pLink->GetIconLocation(szIcon, XII_ARRAY_SIZE(szIcon), &iIcon)));
        XII_TEST_BOOL(QString::fromWCharArray(szIcon) == QDir::toNativeSeparators(xiiMakeQString(xiiOSFile::GetApplicationPath())));
        XII_TEST_INT(iIcon, 0);
      }
    }

    QString            m_sAppId;
    QStringList        m_Projects;
    QStringList        m_Tasks;
    HRESULT            m_IdentityResult = S_OK;
    HRESULT            m_BeginResult    = S_OK;
    HRESULT            m_CategoryResult = S_OK;
    HRESULT            m_TasksResult    = S_OK;
    HRESULT            m_CommitResult   = S_OK;
    UINT               m_uiSlots        = 10;
    UINT               m_uiCommits      = 0;
    UINT               m_uiAborts       = 0;
    bool               m_bBegun         = false;
    ULONG              m_uiReferences   = 1;
    IObjectCollection* m_pRemoved       = nullptr;
  };
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(EditorJumpList);

XII_CREATE_SIMPLE_TEST(EditorJumpList, ProjectArguments)
{
  ShellApartment apartment;
  QStringList    paths = {QStringLiteral("C:/Projects/My Project/xiiProject"), QString::fromUtf8("C:/Projets/Éclairage 日本語/xiiProject"),
                          QStringLiteral("\\\\server\\share\\Project With Spaces\\xiiProject"), QStringLiteral("C:\\trailing slash\\"),
                          QStringLiteral("C:\\escaped\\\"quote\\xiiProject")};
  paths.push_back(QStringLiteral("C:/") + QString(9000, QLatin1Char('x')) + QStringLiteral("/xiiProject"));
  for (const auto& sPath : paths)
  {
    const QString sArguments = xiiWindowsJumpListTest::Arguments(sPath);
    IShellLinkW*  pLink      = nullptr;
    if (!XII_TEST_BOOL(SUCCEEDED(xiiWindowsJumpListTest::Link(sArguments, pLink))))
      continue;
    XII_SCOPE_EXIT(pLink->Release());
    const QStringList argv = ParseArguments(LinkProperty(*pLink, PKEY_Link_Arguments));
    if (!XII_TEST_INT(argv.size(), 3))
      continue;
    XII_TEST_BOOL(argv[1] == QStringLiteral("-project"));
    XII_TEST_BOOL(argv[2] == sPath);
    xiiDynamicArray<xiiString> engineArgs;
    for (const auto& arg : argv)
      engineArgs.PushBack(arg.toUtf8().constData());
    xiiCommandLineUtils command;
    command.SetCommandLine(engineArgs);
    XII_TEST_BOOL(xiiMakeQString(command.GetStringOption("-project")) == sPath);
  }
  IShellLinkW* pLink = nullptr;
  XII_TEST_BOOL(FAILED(xiiWindowsJumpListTest::Link(QString(32767, QLatin1Char('x')), pLink)));
  XII_TEST_BOOL(pLink == nullptr);
  XII_TEST_BOOL(FAILED(xiiWindowsJumpListTest::Link({}, pLink)));
  XII_TEST_BOOL(pLink == nullptr);
}

XII_CREATE_SIMPLE_TEST(EditorJumpList, RecentProjectsAndRemoval)
{
  ShellApartment apartment;
  QTemporaryDir  directory;
  XII_TEST_BOOL(directory.isValid());
  const QString first    = ProjectFile(directory, QString::fromUtf8("Éclairage 日本語"));
  const QString second   = ProjectFile(directory, QStringLiteral("Other Project"));
  const QString longPath = ProjectFile(directory, QString(120, QLatin1Char('a')) + QLatin1Char('/') + QString(120, QLatin1Char('b')) + QStringLiteral("/Long Project"));
  XII_TEST_BOOL(longPath.size() > 260);
  xiiRecentFilesList recent(20);
  recent.Insert(longPath.toUtf8().constData(), 0);
  recent.Insert(second.toUtf8().constData(), 0);
  recent.Insert(first.toUtf8().constData(), 0);
  recent.Insert((directory.path() + QStringLiteral("/Missing/xiiProject")).toUtf8().constData(), 0);
  recent.Insert(directory.path().toUtf8().constData(), 0);
  DestinationRecorder recorder;
  recorder.m_uiSlots = 2;
  bool bChanged      = false;
  XII_TEST_BOOL(SUCCEEDED(xiiWindowsJumpListTest::Build(recorder, recent, bChanged)));
  XII_TEST_BOOL(!bChanged);
  XII_TEST_BOOL(recorder.m_sAppId == QString::fromWCharArray(xiiWindowsJumpList::GetAppId()));
  XII_TEST_BOOL(recorder.m_Projects == QStringList({xiiWindowsJumpListTest::Arguments(first), xiiWindowsJumpListTest::Arguments(second)}));
  XII_TEST_BOOL(recorder.m_Tasks == QStringList({QStringLiteral("-createProject"), QStringLiteral("-openProject")}));
  recorder.RemoveProject(second.toUpper());
  XII_TEST_BOOL(SUCCEEDED(xiiWindowsJumpListTest::Build(recorder, recent, bChanged)));
  XII_TEST_BOOL(bChanged);
  XII_TEST_BOOL(recorder.m_Projects == QStringList({xiiWindowsJumpListTest::Arguments(first), xiiWindowsJumpListTest::Arguments(longPath)}));
  XII_TEST_INT(recent.GetFileList().GetCount(), 4);
  // A successful commit clears the Shell's removal list. The authoritative history must stay pruned.
  XII_TEST_BOOL(SUCCEEDED(xiiWindowsJumpListTest::Build(recorder, recent, bChanged)));
  XII_TEST_BOOL(!bChanged);
  XII_TEST_BOOL(!recorder.m_Projects.contains(xiiWindowsJumpListTest::Arguments(second)));
  recent.Insert(second.toUtf8().constData(), 0);
  XII_TEST_BOOL(SUCCEEDED(xiiWindowsJumpListTest::Build(recorder, recent, bChanged)));
  XII_TEST_BOOL(recorder.m_Projects.front() == xiiWindowsJumpListTest::Arguments(second));
  XII_TEST_BOOL(recent.Remove(second.toUtf8().constData()));
  XII_TEST_BOOL(!recent.Remove(second.toUtf8().constData()));
  // Persist/reload using the same history format used by the Editor.
  const xiiString sRoot(directory.path().toUtf8().constData());
  XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(sRoot, "JumpListHistory", ":", xiiDataDirUsage::AllowWrites));
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("JumpListHistory"));
  const xiiString sHistory((directory.path() + QStringLiteral("/RecentProjects.txt")).toUtf8().constData());
  recent.Save(sHistory);
  xiiRecentFilesList loaded(20);
  loaded.Load(sHistory);
  XII_TEST_BOOL(SUCCEEDED(xiiWindowsJumpListTest::Build(recorder, loaded, bChanged)));
  XII_TEST_BOOL(recorder.m_Projects == QStringList({xiiWindowsJumpListTest::Arguments(first), xiiWindowsJumpListTest::Arguments(longPath)}));
  XII_TEST_INT(recorder.m_uiAborts, 0);
  XII_TEST_INT(recorder.m_uiReferences, 1);
}

XII_CREATE_SIMPLE_TEST(EditorJumpList, FailuresAndPrivacy)
{
  ShellApartment     apartment;
  QTemporaryDir      directory;
  const QString      project = ProjectFile(directory, QStringLiteral("Project"));
  xiiRecentFilesList recent(20);
  recent.Insert(project.toUtf8().constData(), 0);
  DestinationRecorder recorder;
  bool                bChanged = false;
  recorder.m_IdentityResult    = E_FAIL;
  XII_TEST_BOOL(FAILED(xiiWindowsJumpListTest::Build(recorder, recent, bChanged)));
  XII_TEST_INT(recorder.m_uiAborts, 0);
  recorder.m_IdentityResult = S_OK;
  recorder.m_BeginResult    = E_FAIL;
  XII_TEST_BOOL(FAILED(xiiWindowsJumpListTest::Build(recorder, recent, bChanged)));
  XII_TEST_INT(recorder.m_uiAborts, 0);
  recorder.m_BeginResult = S_OK;
  XII_TEST_BOOL(FAILED(xiiWindowsJumpListTest::Build(recorder, recent, bChanged, {})));
  XII_TEST_INT(recorder.m_uiAborts, 1);
  recorder.m_CategoryResult = E_FAIL;
  XII_TEST_BOOL(FAILED(xiiWindowsJumpListTest::Build(recorder, recent, bChanged)));
  XII_TEST_INT(recorder.m_uiAborts, 2);
  recorder.m_CategoryResult = E_ACCESSDENIED;
  XII_TEST_BOOL(SUCCEEDED(xiiWindowsJumpListTest::Build(recorder, recent, bChanged)));
  XII_TEST_INT(recorder.m_Tasks.size(), 2);
  XII_TEST_INT(recorder.m_uiCommits, 1);
  recorder.m_TasksResult = E_FAIL;
  XII_TEST_BOOL(FAILED(xiiWindowsJumpListTest::Build(recorder, recent, bChanged)));
  XII_TEST_INT(recorder.m_uiAborts, 3);
  recorder.m_TasksResult    = S_OK;
  recorder.m_CategoryResult = S_OK;
  recorder.RemoveProject(project);
  recorder.m_CommitResult = E_FAIL;
  XII_TEST_BOOL(FAILED(xiiWindowsJumpListTest::Build(recorder, recent, bChanged)));
  XII_TEST_BOOL(!bChanged);
  XII_TEST_INT(recent.GetFileList().GetCount(), 1);
  XII_TEST_INT(recorder.m_uiAborts, 4);
  recorder.m_CommitResult = S_OK;
  XII_TEST_BOOL(SUCCEEDED(xiiWindowsJumpListTest::Build(recorder, recent, bChanged)));
  XII_TEST_BOOL(bChanged);
  XII_TEST_BOOL(recent.GetFileList().IsEmpty());
  XII_TEST_BOOL(SUCCEEDED(xiiWindowsJumpListTest::Build(recorder, recent, bChanged)));
  XII_TEST_BOOL(!bChanged);
  XII_TEST_INT(recorder.m_Tasks.size(), 2);
}

static void TestStartupAction(const char* szAction)
{
  class DialogObserver : public QObject
  {
  public:
    bool eventFilter(QObject* pObject, QEvent* pEvent) override
    {
      if (pEvent->type() == QEvent::Show)
      {
        auto* pCreate = qobject_cast<xiiQtCreateProjectDlg*>(pObject);
        auto* pOpen   = qobject_cast<xiiQtDashboardDlg*>(pObject);
        if (pCreate || pOpen)
        {
          m_uiCreate += pCreate ? 1 : 0;
          m_uiOpen += pOpen ? 1 : 0;
          auto* pDialog = static_cast<QDialog*>(pObject);
          QTimer::singleShot(0, pDialog, &QDialog::reject);
        }
      }
      return false;
    }
    xiiUInt32 m_uiCreate = 0;
    xiiUInt32 m_uiOpen   = 0;
  } observer;

  auto*           pEditor       = xiiQtEditorApp::GetSingleton();
  auto*           pApplication  = xiiApplication::GetApplicationInstance();
  const xiiString sPreviousName = pApplication->GetApplicationName();
  pApplication->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(pApplication->SetApplicationName(sPreviousName));
  xiiCommandLineUtils* pCommand          = xiiCommandLineUtils::GetGlobalInstance();
  auto                 previousArguments = pCommand->GetCommandLineArray();
  XII_SCOPE_EXIT(pCommand->SetCommandLine(previousArguments));
  qApp->installEventFilter(&observer);
  XII_SCOPE_EXIT(qApp->removeEventFilter(&observer));
  xiiDynamicArray<xiiString> arguments;
  arguments.PushBack("xiiEditor.exe");
  arguments.PushBack(szAction);
  pCommand->SetCommandLine(arguments);
  xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/JumpListStartupPreferences");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(preferences));
  pEditor->StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(pEditor->ShutdownEditor());
  QApplication::processEvents();
  XII_TEST_BOOL(!xiiToolsProject::IsProjectOpen());
  if (xiiStringView(szAction) == "-createProject")
  {
    XII_TEST_INT(observer.m_uiCreate, 1);
    XII_TEST_INT(observer.m_uiOpen, 0);
  }
  else
  {
    XII_TEST_INT(observer.m_uiCreate, 0);
    XII_TEST_INT(observer.m_uiOpen, 1);
  }
}

XII_CREATE_SIMPLE_TEST(EditorJumpList, NativeShell)
{
  if (FindWindowW(L"Shell_TrayWnd", nullptr) == nullptr)
  {
    xiiTestFramework::Output(xiiTestOutput::Details, "Explorer is unavailable; native Jump List smoke test skipped.");
    return;
  }
  ShellApartment          apartment;
  ICustomDestinationList* pDestinations = nullptr;
  if (!XII_TEST_BOOL(SUCCEEDED(CoCreateInstance(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDestinations)))))
    return;
  XII_SCOPE_EXIT(pDestinations->Release());
  // Publish to a process-specific test identity, then delete it; never replace the Editor's real Jump List.
  const QString sTestId  = QStringLiteral("XII.Technologies.Editor.Test.%1").arg(GetCurrentProcessId());
  const auto*   szTestId = reinterpret_cast<const wchar_t*>(sTestId.utf16());
  if (!XII_TEST_BOOL(SUCCEEDED(pDestinations->SetAppID(szTestId))))
    return;
  XII_SCOPE_EXIT(pDestinations->DeleteList(szTestId));
  QTemporaryDir      directory;
  const QString      project = ProjectFile(directory, QString::fromUtf8("Native Project 日本語"));
  xiiRecentFilesList recent(20);
  recent.Insert(project.toUtf8().constData(), 0);
  bool bChanged = false;
  XII_TEST_BOOL(SUCCEEDED(xiiWindowsJumpListTest::BuildConfigured(*pDestinations, recent, bChanged)));
  XII_TEST_BOOL(!bChanged);
}

XII_CREATE_SIMPLE_TEST(EditorJumpList, StartupCreateProject)
{
  TestStartupAction("-createProject");
}

XII_CREATE_SIMPLE_TEST(EditorJumpList, StartupOpenProject)
{
  TestStartupAction("-openProject");
}

#endif
