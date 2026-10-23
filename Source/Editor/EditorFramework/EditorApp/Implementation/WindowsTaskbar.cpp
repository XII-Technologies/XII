/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/WindowsTaskbar.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#  include <EditorFramework/Assets/AssetCurator.h>
#  include <EditorFramework/Assets/AssetProcessor.h>
#  include <EditorFramework/EditorApp/EditorApp.moc.h>
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <Foundation/Types/ScopeExit.h>
#  include <QEvent>
#  include <QThread>
#  include <ToolsFoundation/Project/ToolsProject.h>
#  include <objbase.h>
#  include <shobjidl.h>

xiiWindowsTaskbar::State xiiWindowsTaskbar::Combine(xiiArrayPtr<const State> states)
{
  State     result;
  xiiUInt32 uiValues = 0;
  for (const State& state : states)
  {
    result.m_Overlay  = xiiMath::Max(result.m_Overlay, state.m_Overlay);
    result.m_Progress = xiiMath::Max(result.m_Progress, state.m_Progress);
    if (state.m_Progress != ProgressState::Inactive && state.m_bHasValue && xiiMath::IsFinite(state.m_fCompletion))
    {
      result.m_fCompletion += xiiMath::Clamp(state.m_fCompletion, 0.0, 1.0);
      ++uiValues;
    }
  }
  if (uiValues > 0)
  {
    result.m_fCompletion /= uiValues;
    result.m_bHasValue = true;
  }
  if (result.m_Progress != ProgressState::Inactive && result.m_Overlay == Overlay::Error)
    result.m_Progress = ProgressState::Error;
  return result;
}

xiiWindowsTaskbar::xiiWindowsTaskbar(xiiQtEditorApp& editor, QWidget& window, xiiProgress& progress) :
  QObject(&editor), m_pWindow(&window), m_Progress(progress)
{
  XII_ASSERT_DEV(QThread::currentThread() == qApp->thread(), "Taskbar integration must be created on the GUI thread.");
  const HRESULT hr   = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  m_bUninitializeCom = SUCCEEDED(hr);
  // An existing apartment is usable, but its initialization belongs to the caller.
  m_bComUsable             = m_bUninitializeCom || hr == RPC_E_CHANGED_MODE;
  m_uiTaskbarButtonCreated = RegisterWindowMessageW(L"TaskbarButtonCreated");
  m_uiTaskbarCreated       = RegisterWindowMessageW(L"TaskbarCreated");

  window.installEventFilter(this);
  connect(&window, &QObject::destroyed, this, [this]() {
    m_hWindow       = nullptr;
    m_bTaskbarReady = false;
    m_bApplied      = false;
  });
  qApp->installNativeEventFilter(this);
  UpdateWindow();
  m_bTaskbarReady = window.isVisible();
  m_Progress.m_Events.AddEventHandler(xiiMakeDelegate(&xiiWindowsTaskbar::ProgressEventHandler, this));
  xiiQtUiServices::s_Events.AddEventHandler(xiiMakeDelegate(&xiiWindowsTaskbar::UiServicesEventHandler, this));

  connect(&editor, &xiiQtEditorApp::ToolExecutionStarted, this, [this]() {
    if (m_uiToolCount == 0)
      m_bToolError = false;
    ++m_uiToolCount;
    Update();
  });
  connect(&editor, &xiiQtEditorApp::ToolExecutionFinished, this, [this](bool bSucceeded) {
    XII_ASSERT_DEV(m_uiToolCount > 0, "Each tool execution must have a matching start.");
    if (m_uiToolCount > 0)
      --m_uiToolCount;
    m_bToolError |= !bSucceeded;
    Update();
  });
  connect(&m_UpdateTimer, &QTimer::timeout, this, &xiiWindowsTaskbar::Update);
  m_UpdateTimer.start(200);
  Update();
}

xiiWindowsTaskbar::~xiiWindowsTaskbar()
{
  XII_ASSERT_DEV(QThread::currentThread() == qApp->thread(), "Taskbar COM objects must be released on the GUI thread.");
  m_UpdateTimer.stop();
  m_Progress.m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiWindowsTaskbar::ProgressEventHandler, this));
  xiiQtUiServices::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiWindowsTaskbar::UiServicesEventHandler, this));
  qApp->removeNativeEventFilter(this);
  if (m_pWindow)
    m_pWindow->removeEventFilter(this);
  m_State = {};
  Apply();
  ReleaseTaskbar();
  if (m_bUninitializeCom)
    CoUninitialize();
}

void xiiWindowsTaskbar::ProgressEventHandler(const xiiProgressEvent& e)
{
  const auto   type        = e.m_Type;
  const double fCompletion = e.m_pProgressbar->GetCompletion();
  const bool   bCanceled   = e.m_pProgressbar->WasCanceled();
  // Copy the producer's values, never its progress pointer, into a GUI-thread callback.
  QMetaObject::invokeMethod(this, [this, type, fCompletion, bCanceled]() {
    switch (type)
    {
      case xiiProgressEvent::Type::ProgressStarted:
        m_Foreground = {};
        m_Foreground.m_Progress = ProgressState::Indeterminate;
        break;
      case xiiProgressEvent::Type::ProgressChanged:
        m_Foreground.m_Progress = bCanceled ? ProgressState::Paused : ProgressState::Normal;
        m_Foreground.m_fCompletion = fCompletion;
        m_Foreground.m_bHasValue = true;
        break;
      case xiiProgressEvent::Type::CancelClicked:
        m_Foreground.m_Progress = ProgressState::Paused;
        break;
      case xiiProgressEvent::Type::ProgressEnded:
        m_Foreground = {};
        break;
    }
    Update(); }, Qt::AutoConnection);
}

void xiiWindowsTaskbar::UiServicesEventHandler(const xiiQtUiServices::Event& e)
{
  if (e.m_Type != xiiQtUiServices::Event::ShowDocumentPermanentStatusBarText && e.m_Type != xiiQtUiServices::Event::ShowGlobalStatusBarText)
    return;
  const bool bGlobal = e.m_Type == xiiQtUiServices::Event::ShowGlobalStatusBarText;
  const bool bEmpty  = e.m_sText.IsEmpty();
  const auto type    = e.m_TextType;
  QMetaObject::invokeMethod(this, [this, bGlobal, bEmpty, type]() {
    if (bGlobal)
      m_bGlobalError = !bEmpty;
    else
    {
      m_StatusOverlay = bEmpty ? Overlay::None : (type == xiiQtUiServices::Event::Error ? Overlay::Error : (type == xiiQtUiServices::Event::Warning ? Overlay::Warning : Overlay::None));
      if (bEmpty)
        m_bToolError = false;
    }
    Update(); }, Qt::AutoConnection);
}

void xiiWindowsTaskbar::Update()
{
  XII_ASSERT_DEV(QThread::currentThread() == qApp->thread(), "Taskbar snapshots must be collected on the GUI thread.");
  xiiHybridArray<State, 8> states;
  states.PushBack(m_Foreground);
  State tools;
  tools.m_Progress = m_uiToolCount > 0 ? ProgressState::Indeterminate : ProgressState::Inactive;
  tools.m_Overlay  = m_bToolError || m_bGlobalError ? Overlay::Error : m_StatusOverlay;
  states.PushBack(tools);

  if (xiiToolsProject::IsProjectOpen())
  {
    if (auto* pCurator = xiiAssetCurator::GetSingleton())
    {
      xiiUInt32                                                      uiTotal = 0;
      xiiHybridArray<xiiUInt32, xiiAssetInfo::TransformState::COUNT> counts;
      pCurator->GetAssetTransformStats(uiTotal, counts);
      State           assets;
      const xiiUInt32 uiErrors  = counts[xiiAssetInfo::TransformError] + counts[xiiAssetInfo::MissingTransformDependency] + counts[xiiAssetInfo::MissingThumbnailDependency] + counts[xiiAssetInfo::MissingPackageDependency] + counts[xiiAssetInfo::CircularDependency];
      const xiiUInt32 uiPending = counts[xiiAssetInfo::NeedsTransform] + counts[xiiAssetInfo::NeedsThumbnail] + counts[xiiAssetInfo::Unknown];
      assets.m_Overlay          = uiErrors > 0 ? Overlay::Error : (counts[xiiAssetInfo::NeedsImport] > 0 ? Overlay::Warning : Overlay::None);
      if (uiPending > 0 && uiTotal > 0)
      {
        const auto* pProcessor = xiiAssetProcessor::GetSingleton();
        const bool  bRunning   = pProcessor && pProcessor->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Running;
        assets.m_Progress      = bRunning ? (counts[xiiAssetInfo::Unknown] > 0 ? ProgressState::Indeterminate : ProgressState::Normal) : ProgressState::Paused;
        assets.m_fCompletion   = static_cast<double>(counts[xiiAssetInfo::UpToDate]) / uiTotal;
        assets.m_bHasValue     = counts[xiiAssetInfo::Unknown] == 0;
      }
      states.PushBack(assets);
    }
  }

  if (auto* pLongOps = xiiLongOpControllerManager::GetSingleton())
  {
    XII_LOCK(pLongOps->m_Mutex);
    for (const auto& operation : pLongOps->GetOperations())
    {
      if (!operation->m_bIsRunning)
        continue;
      State state;
      state.m_Progress    = ProgressState::Normal;
      state.m_fCompletion = operation->m_fCompletion;
      state.m_bHasValue   = true;
      states.PushBack(state);
    }
  }
  m_State = Combine(states);
  Apply();
}

void xiiWindowsTaskbar::UpdateWindow()
{
  const auto hWindow = m_pWindow ? reinterpret_cast<xiiMinWindows::HWND>(m_pWindow->internalWinId()) : nullptr;
  if (m_hWindow == hWindow)
    return;
  m_hWindow       = hWindow;
  m_bTaskbarReady = false;
  m_bApplied      = false;
}

bool xiiWindowsTaskbar::eventFilter(QObject* pObject, QEvent* pEvent)
{
  if (pObject == m_pWindow && (pEvent->type() == QEvent::WinIdChange || pEvent->type() == QEvent::Show))
    UpdateWindow();
  if (pObject == m_pWindow && pEvent->type() == QEvent::Destroy)
  {
    m_hWindow       = nullptr;
    m_bTaskbarReady = false;
    m_bApplied      = false;
  }
  return false;
}

bool xiiWindowsTaskbar::nativeEventFilter(const QByteArray& eventType, void* pMessage, qintptr*)
{
  if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
    return false;
  const auto* pMsg = static_cast<const MSG*>(pMessage);
  if (pMsg->hwnd != xiiMinWindows::ToNative(m_hWindow))
    return false;
  if (m_uiTaskbarCreated != 0 && pMsg->message == m_uiTaskbarCreated)
  {
    m_bResetTaskbar = true;
    m_bApplied      = false;
    m_bTaskbarReady = false;
    QMetaObject::invokeMethod(this, [this]() { Apply(); }, Qt::QueuedConnection);
  }
  else if (m_uiTaskbarButtonCreated != 0 && pMsg->message == m_uiTaskbarButtonCreated)
  {
    m_bResetTaskbar = true;
    m_bApplied      = false;
    m_bTaskbarReady = true;
    QMetaObject::invokeMethod(this, [this]() { Apply(); }, Qt::QueuedConnection);
  }
  return false;
}

void xiiWindowsTaskbar::ReleaseTaskbar()
{
  if (m_pTaskbar)
  {
    m_pTaskbar->Release();
    m_pTaskbar = nullptr;
  }
  m_bApplied = false;
}

void xiiWindowsTaskbar::Apply()
{
  // COM may dispatch native messages. Defer shell resets and recursive updates until this call finishes.
  if (m_bApplying)
    return;
  m_bApplying = true;
  XII_SCOPE_EXIT(m_bApplying = false);
  if (m_bResetTaskbar)
  {
    ReleaseTaskbar();
    m_bResetTaskbar = false;
  }
  if (!m_bComUsable || !m_bTaskbarReady || !m_hWindow || (m_bApplied && m_State == m_AppliedState))
    return;
  const HWND  hWindow = xiiMinWindows::ToNative(m_hWindow);
  const State state   = m_State;
  if (!IsWindow(hWindow))
    return;
  if (!m_pTaskbar)
  {
    if (FAILED(CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pTaskbar))))
      return;
    if (FAILED(m_pTaskbar->HrInit()))
    {
      ReleaseTaskbar();
      return;
    }
  }

  TBPFLAG progress = TBPF_NOPROGRESS;
  switch (state.m_Progress)
  {
    case ProgressState::Inactive:
      break;
    case ProgressState::Normal:
      progress = TBPF_NORMAL;
      break;
    case ProgressState::Indeterminate:
      progress = TBPF_INDETERMINATE;
      break;
    case ProgressState::Paused:
      progress = TBPF_PAUSED;
      break;
    case ProgressState::Error:
      progress = TBPF_ERROR;
      break;
  }
  // SetProgressValue changes indeterminate to normal. Apply the requested color/state last.
  HRESULT hr = S_OK;
  if (state.m_bHasValue && progress != TBPF_NOPROGRESS && progress != TBPF_INDETERMINATE)
    hr = m_pTaskbar->SetProgressValue(hWindow, static_cast<ULONGLONG>(state.m_fCompletion * 1000.0), 1000);
  if (SUCCEEDED(hr))
    hr = m_pTaskbar->SetProgressState(hWindow, progress);
  if (SUCCEEDED(hr) && (!m_bApplied || state.m_Overlay != m_AppliedState.m_Overlay))
  {
    // Stock icons are shared system resources, borrowed for this call; do not destroy them.
    HICON          hIcon         = nullptr;
    const wchar_t* szDescription = L"";
    if (state.m_Overlay == Overlay::Error)
    {
      hIcon         = LoadIconW(nullptr, IDI_ERROR);
      szDescription = L"Editor errors or asset processing failures";
    }
    else if (state.m_Overlay == Overlay::Warning)
    {
      hIcon         = LoadIconW(nullptr, IDI_WARNING);
      szDescription = L"Editor warnings or assets requiring import";
    }
    hr = m_pTaskbar->SetOverlayIcon(hWindow, hIcon, szDescription);
  }
  if (FAILED(hr))
  {
    // Keep the desired state. A later tick/button recreation retries without affecting the operation.
    ReleaseTaskbar();
    return;
  }
  m_AppliedState = state;
  m_bApplied     = !m_bResetTaskbar && m_hWindow == xiiMinWindows::FromNative(hWindow);
}

#endif
