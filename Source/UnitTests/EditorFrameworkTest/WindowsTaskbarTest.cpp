/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Platform/Windows/WindowsTaskbar.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <EditorFramework/Assets/AssetDocumentGenerator.h>
#  include <EditorFramework/EditorApp/EditorApp.moc.h>
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/Types/Delegate.h>
#  include <Foundation/Types/ScopeExit.h>
#  include <Foundation/Utilities/Progress.h>
#  include <QApplication>
#  include <QElapsedTimer>
#  include <QImage>
#  include <QStandardPaths>
#  include <QThread>
#  include <ToolsFoundation/Project/ToolsProject.h>
#  include <shobjidl.h>
#  include <thread>
#  include <utility>

namespace
{
  // Observe the real COM call boundary without changing the user's Explorer process or taskbar.
  class TaskbarRecorder final : public ITaskbarList3
  {
  public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** ppObject) override
    {
      *ppObject = nullptr;
      return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_uiReferences; }
    ULONG STDMETHODCALLTYPE Release() override
    {
      ++m_uiReleases;
      return --m_uiReferences;
    }
    HRESULT STDMETHODCALLTYPE HrInit() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE AddTab(HWND) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE DeleteTab(HWND) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE ActivateTab(HWND) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetActiveAlt(HWND) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE MarkFullscreenWindow(HWND, BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE RegisterTab(HWND, HWND) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE UnregisterTab(HWND) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetTabOrder(HWND, HWND) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetTabActive(HWND, HWND, DWORD) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE ThumbBarAddButtons(HWND, UINT, THUMBBUTTON*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE ThumbBarUpdateButtons(HWND, UINT, THUMBBUTTON*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE ThumbBarSetImageList(HWND, HIMAGELIST) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetThumbnailTooltip(HWND, LPCWSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetThumbnailClip(HWND, RECT*) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE SetProgressValue(HWND hWindow, ULONGLONG uiCompleted, ULONGLONG uiTotal) override
    {
      m_sCalls.Append("value;");
      m_uiCompleted = uiCompleted;
      m_uiTotal     = uiTotal;
      m_hWindow     = hWindow;
      XII_TEST_BOOL(QThread::currentThread() == qApp->thread());
      return m_Result;
    }

    HRESULT STDMETHODCALLTYPE SetProgressState(HWND hWindow, TBPFLAG state) override
    {
      m_sCalls.Append("state;");
      m_State   = state;
      m_hWindow = hWindow;
      XII_TEST_BOOL(QThread::currentThread() == qApp->thread());
      if (m_OnProgressState.IsValid())
      {
        auto callback     = std::move(m_OnProgressState);
        m_OnProgressState = {};
        callback();
      }
      return m_Result;
    }

    HRESULT STDMETHODCALLTYPE SetOverlayIcon(HWND hWindow, HICON hIcon, LPCWSTR szDescription) override
    {
      m_sCalls.Append("overlay;");
      m_hOverlay            = hIcon;
      m_OverlayImage        = hIcon ? QImage::fromHICON(hIcon) : QImage();
      m_sOverlayDescription = QString::fromWCharArray(szDescription);
      m_hWindow             = hWindow;
      XII_TEST_BOOL(QThread::currentThread() == qApp->thread());
      return m_OverlayResult;
    }

    xiiStringBuilder    m_sCalls;
    xiiDelegate<void()> m_OnProgressState;
    QImage              m_OverlayImage;
    QString             m_sOverlayDescription;
    TBPFLAG             m_State         = TBPF_NOPROGRESS;
    HRESULT             m_Result        = S_OK;
    HRESULT             m_OverlayResult = S_OK;
    HWND                m_hWindow       = nullptr;
    HICON               m_hOverlay      = nullptr;
    ULONGLONG           m_uiCompleted   = 0;
    ULONGLONG           m_uiTotal       = 0;
    ULONG               m_uiReferences  = 1;
    ULONG               m_uiReleases    = 0;
  };

  class TaskbarTestWindow : public QWidget
  {
  public:
    void RecreateNativeWindow()
    {
      destroy();
      winId();
    }
  };

  bool IsIconValid(HICON hIcon)
  {
    ICONINFO info = {};
    if (!GetIconInfo(hIcon, &info))
      return false;
    if (info.hbmColor)
      DeleteObject(info.hbmColor);
    if (info.hbmMask)
      DeleteObject(info.hbmMask);
    return true;
  }

  class TaskbarImportGenerator : public xiiAssetDocumentGenerator
  {
  public:
    TaskbarImportGenerator() { AddSupportedFileType("taskbarinput"); }
    void          GetImportModes(xiiStringView, xiiDynamicArray<ImportMode>&) const override {}
    xiiStringView GetDocumentExtension() const override { return "TaskbarTestAsset"; }
    xiiStringView GetGeneratorGroup() const override { return "TaskbarTest"; }
    xiiStatus     Generate(xiiStringView, xiiStringView, xiiDynamicArray<xiiDocument*>&) override
    {
      m_OnGenerate();
      return m_bFail ? xiiStatus("Expected import failure") : xiiStatus(XII_SUCCESS);
    }
    xiiDelegate<void()> m_OnGenerate;
    bool                m_bFail = false;
  };
} // namespace

struct xiiWindowsTaskbarTest
{
  using State    = xiiWindowsTaskbar::State;
  using Progress = xiiWindowsTaskbar::ProgressState;
  using Overlay  = xiiWindowsTaskbar::Overlay;

  static State     Combine(xiiArrayPtr<const State> states) { return xiiWindowsTaskbar::Combine(states); }
  static State     GetState(const xiiWindowsTaskbar& taskbar) { return taskbar.m_State; }
  static xiiUInt32 GetToolCount(const xiiWindowsTaskbar& taskbar) { return taskbar.m_uiToolCount; }
  static bool      IsReady(const xiiWindowsTaskbar& taskbar) { return taskbar.m_bTaskbarReady; }
  static bool      IsApplied(const xiiWindowsTaskbar& taskbar) { return taskbar.m_bApplied; }
  static bool      HasBackend(const xiiWindowsTaskbar& taskbar) { return taskbar.m_pTaskbar != nullptr; }
  static void      Acknowledge()
  {
    xiiQtUiServices::Event event;
    event.m_Type = xiiQtUiServices::Event::ClickedDocumentPermanentStatusBarText;
    xiiQtUiServices::s_Events.Broadcast(event);
  }
  static void CloseProject(xiiWindowsTaskbar& taskbar)
  {
    xiiToolsProjectEvent event;
    event.m_Type = xiiToolsProjectEvent::Type::ProjectClosed;
    taskbar.ProjectEventHandler(event);
  }
  static void Attach(xiiWindowsTaskbar& taskbar, TaskbarRecorder& recorder)
  {
    taskbar.ReleaseTaskbar();
    recorder.AddRef();
    taskbar.m_pTaskbar      = &recorder;
    taskbar.m_bComUsable    = true;
    taskbar.m_bResetTaskbar = false;
  }
  static void Apply(xiiWindowsTaskbar& taskbar, const State& state)
  {
    taskbar.m_State         = state;
    taskbar.m_bTaskbarReady = true;
    taskbar.Apply();
  }
  static void ShellMessage(xiiWindowsTaskbar& taskbar, bool bButton)
  {
    MSG message     = {};
    message.hwnd    = xiiMinWindows::ToNative(taskbar.m_hWindow);
    message.message = bButton ? taskbar.m_uiTaskbarButtonCreated : taskbar.m_uiTaskbarCreated;
    taskbar.nativeEventFilter(QByteArrayLiteral("windows_generic_MSG"), &message, nullptr);
  }
};

XII_CREATE_SIMPLE_TEST_GROUP(EditorTaskbar);

XII_CREATE_SIMPLE_TEST(EditorTaskbar, Arbitration)
{
  using Test               = xiiWindowsTaskbarTest;
  using Progress           = Test::Progress;
  using Overlay            = Test::Overlay;
  Test::State operations[] = {{Progress::Normal, Overlay::None, 0.25, true}, {Progress::Normal, Overlay::None, 0.75, true}};
  auto        state        = Test::Combine(operations);
  XII_TEST_BOOL(state.m_Progress == Progress::Normal);
  XII_TEST_DOUBLE(state.m_fCompletion, 0.5, 0.000001);
  operations[1].m_Progress  = Progress::Indeterminate;
  operations[1].m_bHasValue = false;
  XII_TEST_BOOL(Test::Combine(operations).m_Progress == Progress::Indeterminate);
  operations[1].m_Progress = Progress::Paused;
  XII_TEST_BOOL(Test::Combine(operations).m_Progress == Progress::Paused);
  operations[0].m_Overlay = Overlay::Error;
  operations[1].m_Overlay = Overlay::Warning;
  state                   = Test::Combine(operations);
  XII_TEST_BOOL(state.m_Progress == Progress::Error);
  XII_TEST_BOOL(state.m_Overlay == Overlay::Error);
  std::swap(operations[0], operations[1]);
  XII_TEST_BOOL(Test::Combine(operations) == state);
  operations[0]           = {};
  operations[1]           = {};
  operations[0].m_Overlay = Overlay::Warning;
  state                   = Test::Combine(operations);
  XII_TEST_BOOL(state.m_Progress == Progress::Inactive);
  XII_TEST_BOOL(state.m_Overlay == Overlay::Warning);
  operations[0] = {};
  XII_TEST_BOOL(Test::Combine(operations) == Test::State{});
}

XII_CREATE_SIMPLE_TEST(EditorTaskbar, NativePresentationAndRecovery)
{
  using Test = xiiWindowsTaskbarTest;
  TaskbarRecorder   recorder;
  TaskbarTestWindow window;
  xiiProgress       progress;
  xiiWindowsTaskbar taskbar(*xiiQtEditorApp::GetSingleton(), window, progress);
  window.winId();
  Test::Attach(taskbar, recorder);

  Test::State state{Test::Progress::Paused, Test::Overlay::Warning, 0.375, true};
  Test::Apply(taskbar, state);
  XII_TEST_STRING(recorder.m_sCalls, "value;state;overlay;");
  XII_TEST_INT(recorder.m_State, TBPF_PAUSED);
  XII_TEST_INT(recorder.m_uiCompleted, 375);
  XII_TEST_INT(recorder.m_uiTotal, 1000);
  XII_TEST_BOOL(recorder.m_hOverlay != nullptr);
  XII_TEST_BOOL(recorder.m_hWindow == reinterpret_cast<HWND>(window.winId()));
  recorder.m_sCalls.Clear();
  Test::Apply(taskbar, state);
  XII_TEST_BOOL(recorder.m_sCalls.IsEmpty());

  state.m_Progress = Test::Progress::Error;
  state.m_Overlay  = Test::Overlay::Error;
  Test::Apply(taskbar, state);
  XII_TEST_INT(recorder.m_State, TBPF_ERROR);
  recorder.m_sCalls.Clear();
  state.m_Progress = Test::Progress::Indeterminate;
  Test::Apply(taskbar, state);
  XII_TEST_STRING(recorder.m_sCalls, "state;");
  XII_TEST_INT(recorder.m_State, TBPF_INDETERMINATE);

  // Exercise the registered shell messages without restarting the user's Explorer.
  Q_EMIT xiiQtEditorApp::GetSingleton()->RequiredActionsChanged(true, false);
  xiiProgressRange range("Recovery test operation", false, &progress);
  state                     = Test::GetState(taskbar);
  const QImage warningImage = recorder.m_OverlayImage;
  const ULONG  uiReleases   = recorder.m_uiReleases;
  Test::ShellMessage(taskbar, false);
  QApplication::processEvents();
  XII_TEST_BOOL(!Test::IsReady(taskbar));
  XII_TEST_INT(recorder.m_uiReleases, uiReleases + 1);
  Test::ShellMessage(taskbar, true);
  Test::Attach(taskbar, recorder);
  recorder.m_sCalls.Clear();
  QApplication::processEvents();
  XII_TEST_STRING(recorder.m_sCalls, "state;overlay;");
  XII_TEST_BOOL(Test::GetState(taskbar) == state);

  window.RecreateNativeWindow();
  XII_TEST_BOOL(recorder.m_OverlayImage == warningImage);
  XII_TEST_BOOL(!Test::IsReady(taskbar));
  Test::ShellMessage(taskbar, true);
  Test::Attach(taskbar, recorder);
  QApplication::processEvents();
  XII_TEST_BOOL(recorder.m_hWindow == reinterpret_cast<HWND>(window.winId()));

  XII_TEST_BOOL(recorder.m_OverlayImage == warningImage);
  recorder.m_Result = E_FAIL;
  state.m_Progress  = Test::Progress::Normal;
  Test::Apply(taskbar, state);
  XII_TEST_BOOL(!Test::HasBackend(taskbar));
  XII_TEST_BOOL(!Test::IsApplied(taskbar));
  XII_TEST_BOOL(Test::GetState(taskbar) == state);
  recorder.m_Result = S_OK;
  Test::Attach(taskbar, recorder);
  Test::Apply(taskbar, state);
  XII_TEST_BOOL(Test::IsApplied(taskbar));

  Test::Apply(taskbar, {});
  XII_TEST_INT(recorder.m_State, TBPF_NOPROGRESS);
  XII_TEST_BOOL(recorder.m_hOverlay == nullptr);

  // A shell broadcast can arrive while a COM method dispatches messages. Do not release its interface mid-call.
  recorder.m_OnProgressState = [&]() {
    Test::ShellMessage(taskbar, false);
    XII_TEST_BOOL(Test::HasBackend(taskbar));
  };
  Test::Apply(taskbar, state);
  XII_TEST_BOOL(!Test::IsApplied(taskbar));
  QApplication::processEvents();
  XII_TEST_BOOL(!Test::HasBackend(taskbar));
  Q_EMIT xiiQtEditorApp::GetSingleton()->RequiredActionsChanged(false, false);
}

XII_CREATE_SIMPLE_TEST(EditorTaskbar, ProgressToolsAndGuiThread)
{
  using Test               = xiiWindowsTaskbarTest;
  auto&             editor = *xiiQtEditorApp::GetSingleton();
  TaskbarRecorder   recorder;
  QWidget           window;
  xiiProgress       progress;
  xiiWindowsTaskbar taskbar(editor, window, progress);
  window.winId();
  Test::Attach(taskbar, recorder);
  Test::Apply(taskbar, {});

  {
    xiiProgressRange range("Measured test operation", false, &progress);
    XII_TEST_BOOL(Test::GetState(taskbar).m_Progress == Test::Progress::Indeterminate);
    range.SetCompletion(0.4);
    XII_TEST_INT(recorder.m_State, TBPF_NORMAL);
    XII_TEST_INT(recorder.m_uiCompleted, 400);
    std::thread worker([&]() {
      Q_EMIT editor.ToolExecutionStarted();
      Q_EMIT editor.ToolExecutionStarted();
      Q_EMIT editor.ToolExecutionFinished(false);
    });
    worker.join();
    XII_TEST_INT(Test::GetToolCount(taskbar), 0);
    QApplication::processEvents();
    XII_TEST_INT(Test::GetToolCount(taskbar), 1);
    XII_TEST_INT(recorder.m_State, TBPF_ERROR);
    Q_EMIT editor.ToolExecutionFinished(true);
    XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::Error);
    Test::Acknowledge();
    XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::None);
    progress.UserClickedCancel();
    XII_TEST_INT(recorder.m_State, TBPF_PAUSED);
  }
  XII_TEST_INT(recorder.m_State, TBPF_NOPROGRESS);
  xiiQtUiServices::ShowAllDocumentsPermanentStatusBarMessage("Warning", xiiQtUiServices::Event::Warning);
  XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::None);
  xiiQtUiServices::ShowAllDocumentsPermanentStatusBarMessage("Error", xiiQtUiServices::Event::Error);
  XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::None);
  xiiQtUiServices::ShowAllDocumentsPermanentStatusBarMessage({}, xiiQtUiServices::Event::Info);
  XII_TEST_BOOL(recorder.m_hOverlay == nullptr);

  const QString shell = QStandardPaths::findExecutable("cmd.exe");
  if (XII_TEST_BOOL(!shell.isEmpty()))
  {
    XII_TEST_BOOL(editor.ExecuteTool(shell.toUtf8().constData(), {"/c", "exit 3"}, 10).Failed());
    XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::Error);
    XII_TEST_INT(recorder.m_State, TBPF_ERROR);
    XII_TEST_INT(Test::GetToolCount(taskbar), 0);
    XII_TEST_BOOL(editor.ExecuteTool(shell.toUtf8().constData(), {"/c", "exit 0"}, 10).Succeeded());
    XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::None);
    XII_TEST_INT(recorder.m_State, TBPF_NOPROGRESS);
    XII_TEST_INT(Test::GetToolCount(taskbar), 0);
  }
  Q_EMIT editor.ToolExecutionStarted();
  Q_EMIT editor.ToolExecutionFinished(false);
  XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::Error);
  Test::CloseProject(taskbar);
  XII_TEST_BOOL(Test::GetState(taskbar) == Test::State{});
  xiiQtUiServices::ShowGlobalStatusBarMessage("Application condition");
  Test::CloseProject(taskbar);
  XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::None);
  xiiQtUiServices::ShowGlobalStatusBarMessage({});
  XII_TEST_BOOL(Test::GetState(taskbar) == Test::State{});
}

XII_CREATE_SIMPLE_TEST(EditorTaskbar, ActionableOverlays)
{
  using Test               = xiiWindowsTaskbarTest;
  auto&             editor = *xiiQtEditorApp::GetSingleton();
  TaskbarRecorder   recorder;
  QWidget           window;
  xiiProgress       progress;
  xiiWindowsTaskbar taskbar(editor, window, progress);
  window.winId();
  Test::Attach(taskbar, recorder);
  Test::Apply(taskbar, {});

  xiiQtUiServices::ShowAllDocumentsPermanentStatusBarMessage("Old warning", xiiQtUiServices::Event::Warning);
  xiiQtUiServices::ShowAllDocumentsPermanentStatusBarMessage("Old error", xiiQtUiServices::Event::Error);
  xiiQtUiServices::ShowGlobalStatusBarMessage("New version available");
  XII_TEST_BOOL(Test::GetState(taskbar) == Test::State{});

  Q_EMIT editor.RequiredActionsChanged(false, true);
  XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::Warning);
  XII_TEST_INT(recorder.m_State, TBPF_NOPROGRESS);
  const QImage warning = recorder.m_OverlayImage;
  {
    xiiProgressRange range("Overlay progress regression", false, &progress);
    range.SetCompletion(0.625);
    Q_EMIT editor.RequiredActionsChanged(false, false);
    XII_TEST_INT(recorder.m_State, TBPF_NORMAL);
    XII_TEST_INT(recorder.m_uiCompleted, 625);
    XII_TEST_BOOL(recorder.m_hOverlay == nullptr);
    Q_EMIT editor.RequiredActionsChanged(false, true);
    XII_TEST_INT(recorder.m_State, TBPF_NORMAL);
    XII_TEST_INT(recorder.m_uiCompleted, 625);
    XII_TEST_BOOL(recorder.m_OverlayImage == warning);
  }
  Q_EMIT editor.ToolExecutionStarted();
  Q_EMIT editor.ToolExecutionFinished(false);
  XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::Error);
  XII_TEST_BOOL(recorder.m_OverlayImage != warning);
  xiiQtUiServices::ShowAllDocumentsPermanentStatusBarMessage({}, xiiQtUiServices::Event::Info);
  XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::Error);
  Test::Acknowledge();
  XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::Warning);
  XII_TEST_BOOL(recorder.m_OverlayImage == warning);
  recorder.m_sCalls.Clear();
  Q_EMIT editor.RequiredActionsChanged(true, false);
  XII_TEST_BOOL(recorder.m_sCalls.IsEmpty());
  Q_EMIT editor.RequiredActionsChanged(false, false);
  XII_TEST_BOOL(Test::GetState(taskbar) == Test::State{});
  XII_TEST_BOOL(recorder.m_hOverlay == nullptr);

  Q_EMIT editor.ToolExecutionStarted();
  Q_EMIT editor.ToolExecutionFinished(false);
  Q_EMIT editor.ToolExecutionStarted();
  XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::None);
  Q_EMIT editor.ToolExecutionFinished(true);
  xiiQtUiServices::ShowAllDocumentsPermanentStatusBarMessage("Historical error", xiiQtUiServices::Event::Error);
  XII_TEST_BOOL(Test::GetState(taskbar) == Test::State{});

  std::thread worker([&]() { Q_EMIT editor.RequiredActionsChanged(true, false); });
  worker.join();
  XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::None);
  QApplication::processEvents();
  XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::Warning);
  Test::CloseProject(taskbar);
  XII_TEST_BOOL(Test::GetState(taskbar).m_Overlay == Test::Overlay::Warning);
  Q_EMIT editor.RequiredActionsChanged(false, false);
  XII_TEST_BOOL(Test::GetState(taskbar) == Test::State{});
}

XII_CREATE_SIMPLE_TEST(EditorTaskbar, OverlayIconOwnership)
{
  using Test = xiiWindowsTaskbarTest;
  TaskbarRecorder   recorder;
  QWidget           window;
  xiiProgress       progress;
  xiiWindowsTaskbar taskbar(*xiiQtEditorApp::GetSingleton(), window, progress);
  window.winId();
  Test::Attach(taskbar, recorder);
  Test::Apply(taskbar, {Test::Progress::Inactive, Test::Overlay::Warning});
  XII_TEST_BOOL(!recorder.m_OverlayImage.isNull());
  XII_TEST_BOOL(!IsIconValid(recorder.m_hOverlay));
  XII_TEST_BOOL(recorder.m_OverlayImage.size() == QSize(16, 16) * window.devicePixelRatioF());
  XII_TEST_BOOL(!recorder.m_sOverlayDescription.isEmpty());
  bool bVisible = false;
  for (int y = 0; y < recorder.m_OverlayImage.height(); ++y)
  {
    for (int x = 0; x < recorder.m_OverlayImage.width(); ++x)
      bVisible |= recorder.m_OverlayImage.pixelColor(x, y).alpha() != 0;
  }
  XII_TEST_BOOL(bVisible);

  recorder.m_OverlayResult = E_FAIL;
  const Test::State error{Test::Progress::Inactive, Test::Overlay::Error};
  Test::Apply(taskbar, error);
  XII_TEST_BOOL(!recorder.m_OverlayImage.isNull());
  XII_TEST_BOOL(!IsIconValid(recorder.m_hOverlay));
  XII_TEST_BOOL(!Test::HasBackend(taskbar));
  XII_TEST_BOOL(Test::GetState(taskbar) == error);
  recorder.m_OverlayResult = S_OK;
  Test::Attach(taskbar, recorder);
  Test::Apply(taskbar, error);
  XII_TEST_BOOL(Test::IsApplied(taskbar));
  XII_TEST_BOOL(!IsIconValid(recorder.m_hOverlay));
  Test::Apply(taskbar, {});
  XII_TEST_BOOL(recorder.m_hOverlay == nullptr);
  XII_TEST_BOOL(recorder.m_OverlayImage.isNull());
}

XII_CREATE_SIMPLE_TEST(EditorTaskbar, AssetImportProgress)
{
  using Test = xiiWindowsTaskbarTest;
  QWidget     window;
  xiiProgress progress;
  auto*       pPreviousProgress = xiiProgress::GetGlobalProgressbar();
  xiiProgress::SetGlobalProgressbar(&progress);
  XII_SCOPE_EXIT(xiiProgress::SetGlobalProgressbar(pPreviousProgress));
  xiiWindowsTaskbar      taskbar(*xiiQtEditorApp::GetSingleton(), window, progress);
  TaskbarImportGenerator generator;
  xiiUInt32              uiImports = 0;
  generator.m_OnGenerate           = [&]() {
    ++uiImports;
    // Source-document preparation no longer starts a global/modal progress
    // range. Actual import activity comes from the existing asset processor.
    XII_TEST_BOOL(Test::GetState(taskbar).m_Progress == Test::Progress::Inactive);
  };
  XII_TEST_STATUS(generator.Import("Fixture.taskbarinput", "Test", false));
  XII_TEST_INT(uiImports, 1);
  XII_TEST_BOOL(Test::GetState(taskbar) == Test::State{});
  generator.m_bFail = true;
  XII_TEST_BOOL(generator.Import("Fixture.taskbarinput", "Test", false).Failed());
  XII_TEST_INT(uiImports, 2);
  XII_TEST_BOOL(Test::GetState(taskbar) == Test::State{});
  XII_TEST_BOOL(generator.Import("Fixture.unsupported", "Test", false).Failed());
  XII_TEST_INT(uiImports, 2);
}

XII_CREATE_SIMPLE_TEST(EditorTaskbar, ShutdownWithQueuedUpdates)
{
  using Test             = xiiWindowsTaskbarTest;
  auto&           editor = *xiiQtEditorApp::GetSingleton();
  TaskbarRecorder recorder;
  QWidget         window;
  xiiProgress     progress;
  {
    xiiWindowsTaskbar taskbar(editor, window, progress);
    window.winId();
    Test::Attach(taskbar, recorder);
    Test::Apply(taskbar, {Test::Progress::Error, Test::Overlay::Error, 0.5, true});
    std::thread worker([&]() {
      Q_EMIT editor.ToolExecutionStarted();
      Q_EMIT editor.ToolExecutionFinished(true);
    });
    worker.join();
  }
  XII_TEST_INT(recorder.m_uiReferences, 1);
  XII_TEST_INT(recorder.m_State, TBPF_NOPROGRESS);
  XII_TEST_BOOL(recorder.m_hOverlay == nullptr);
  recorder.m_sCalls.Clear();
  QApplication::processEvents();
  XII_TEST_BOOL(recorder.m_sCalls.IsEmpty());
}

XII_CREATE_SIMPLE_TEST(EditorTaskbar, NativeShell)
{
  if (FindWindowW(L"Shell_TrayWnd", nullptr) == nullptr)
  {
    xiiTestFramework::Output(xiiTestOutput::Details, "Explorer is unavailable; native shell smoke test skipped.");
    return;
  }
  using Test = xiiWindowsTaskbarTest;
  QWidget window;
  window.setWindowTitle("XII taskbar integration test");
  window.setAttribute(Qt::WA_ShowWithoutActivating);
  xiiProgress       progress;
  xiiWindowsTaskbar taskbar(*xiiQtEditorApp::GetSingleton(), window, progress);
  window.showMinimized();
  QElapsedTimer timer;
  timer.start();
  while (!Test::IsReady(taskbar) && timer.elapsed() < 3000)
    QApplication::processEvents(QEventLoop::AllEvents, 20);
  if (!XII_TEST_BOOL(Test::IsReady(taskbar)))
    return;
  {
    xiiProgressRange range("Native shell smoke test", false, &progress);
    XII_TEST_BOOL(Test::IsApplied(taskbar));
    range.SetCompletion(0.25);
    XII_TEST_BOOL(Test::IsApplied(taskbar));
    progress.UserClickedCancel();
    XII_TEST_BOOL(Test::IsApplied(taskbar));
    Q_EMIT xiiQtEditorApp::GetSingleton()->RequiredActionsChanged(true, false);
    XII_TEST_BOOL(Test::IsApplied(taskbar));
    Q_EMIT xiiQtEditorApp::GetSingleton()->ToolExecutionStarted();
    Q_EMIT xiiQtEditorApp::GetSingleton()->ToolExecutionFinished(false);
    XII_TEST_BOOL(Test::IsApplied(taskbar));
    Test::Acknowledge();
    Q_EMIT xiiQtEditorApp::GetSingleton()->RequiredActionsChanged(false, false);
  }
  XII_TEST_BOOL(Test::IsApplied(taskbar));
  XII_TEST_BOOL(Test::GetState(taskbar) == Test::State{});
}

#endif
