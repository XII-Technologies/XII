/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Platform/Windows/WindowsNotifications.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#  include <EditorEngineProcessFramework/LongOps/LongOps.h>
#  include <EditorFramework/Assets/AssetProcessor.h>
#  include <EditorFramework/EditorApp/EditorApp.moc.h>
#  include <EditorFramework/Platform/Windows/WindowsJumpList.h>
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Threading/Lock.h>
#  include <Foundation/Types/ScopeExit.h>
#  include <QApplication>
#  include <QDir>
#  include <QFileInfo>
#  include <QThread>
#  include <ToolsFoundation/Project/ToolsProject.h>
#  include <objbase.h>
#  include <propkey.h>
#  include <propvarutil.h>
#  include <roapi.h>
#  include <shellapi.h>
#  include <shlobj.h>
#  include <windows.data.xml.dom.h>
#  include <windows.ui.notifications.h>
#  include <wrl.h>
#  include <wrl/wrappers/corewrappers.h>

namespace
{
  using namespace Microsoft::WRL;
  using namespace Microsoft::WRL::Wrappers;
  namespace Notifications = ABI::Windows::UI::Notifications;
  namespace Xml           = ABI::Windows::Data::Xml::Dom;

  const wchar_t* NotificationWide(const QString& sText)
  {
    return reinterpret_cast<const wchar_t*>(sText.utf16());
  }

  bool NotificationsAllowed()
  {
    QUERY_USER_NOTIFICATION_STATE state;
    return SUCCEEDED(SHQueryUserNotificationState(&state)) && state == QUNS_ACCEPTS_NOTIFICATIONS;
  }
} // namespace

struct xiiWindowsNotifications::Native
{
  Native()
  {
    m_bInitialized = SUCCEEDED(RoInitialize(RO_INIT_SINGLETHREADED));
  }

  ~Native()
  {
    Clear();
    m_pNotifier.Reset();
    if (m_bInitialized)
      RoUninitialize();
  }

  void Clear()
  {
    if (m_pToast)
    {
      auto toast = m_pToast;
      m_pToast.Reset();
      // Revocation is best effort; a callback already in flight is protected by the gate.
      toast->remove_Activated(m_Activated);
      toast->remove_Dismissed(m_Dismissed);
      toast->remove_Failed(m_Failed);
      if (m_pNotifier)
        m_pNotifier->Hide(toast.Get());
    }
  }

  HRESULT Initialize()
  {
    if (!m_bInitialized)
      return E_FAIL;
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
      return HRESULT_FROM_WIN32(GetLastError());
    XII_SCOPE_EXIT(CloseHandle(hToken));
    TOKEN_ELEVATION elevation = {};
    DWORD           uiSize    = 0;
    if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &uiSize) || elevation.TokenIsElevated)
      return E_ACCESSDENIED;

    PWSTR   pPrograms = nullptr;
    HRESULT hr        = SHGetKnownFolderPath(FOLDERID_Programs, KF_FLAG_DEFAULT, nullptr, &pPrograms);
    if (FAILED(hr))
      return hr;
    XII_SCOPE_EXIT(CoTaskMemFree(pPrograms));
    const QString sShortcut = QString::fromWCharArray(pPrograms) + QStringLiteral("/XII Editor.lnk");
    if (PrepareShortcut(sShortcut, xiiMakeQString(xiiOSFile::GetApplicationPath())).Failed())
      return E_FAIL;

    ComPtr<Notifications::IToastNotificationManagerStatics> manager;
    hr = RoGetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), IID_PPV_ARGS(&manager));
    if (FAILED(hr))
      return hr;
    return manager->CreateToastNotifierWithId(HStringReference(xiiWindowsJumpList::GetAppId()).Get(), &m_pNotifier);
  }

  HRESULT Show(const Message& message, const xiiSharedPtr<CallbackState>& state, xiiUInt64 uiGeneration)
  {
    if (!m_pNotifier)
    {
      const HRESULT hr = Initialize();
      if (FAILED(hr))
        return hr;
    }
    Notifications::NotificationSetting setting;
    HRESULT                            hr = m_pNotifier->get_Setting(&setting);
    if (FAILED(hr))
      return hr;
    if (setting != Notifications::NotificationSetting_Enabled || !NotificationsAllowed())
      return S_FALSE;

    ComPtr<IInspectable> instance;
    hr = RoActivateInstance(HStringReference(RuntimeClass_Windows_Data_Xml_Dom_XmlDocument).Get(), &instance);
    if (FAILED(hr))
      return hr;
    ComPtr<Xml::IXmlDocument>   document;
    ComPtr<Xml::IXmlDocumentIO> documentIO;
    hr = instance.As(&document);
    if (SUCCEEDED(hr))
      hr = instance.As(&documentIO);
    if (FAILED(hr))
      return hr;
    const QString sXml = BuildXml(message);
    hr                 = documentIO->LoadXml(HStringReference(NotificationWide(sXml)).Get());
    if (FAILED(hr))
      return hr;

    ComPtr<Notifications::IToastNotificationFactory> factory;
    hr = RoGetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), IID_PPV_ARGS(&factory));
    if (FAILED(hr))
      return hr;
    ComPtr<Notifications::IToastNotification> toast;
    hr = factory->CreateToastNotification(document.Get(), &toast);
    if (FAILED(hr))
      return hr;
    auto activated = Callback<ABI::Windows::Foundation::ITypedEventHandler<Notifications::ToastNotification*, IInspectable*>>(
      [state, uiGeneration](Notifications::IToastNotification*, IInspectable*) -> HRESULT {
        QueueCallback(state, uiGeneration, true);
        return S_OK;
      });
    auto dismissed = Callback<ABI::Windows::Foundation::ITypedEventHandler<Notifications::ToastNotification*, Notifications::ToastDismissedEventArgs*>>(
      [state, uiGeneration](Notifications::IToastNotification*, Notifications::IToastDismissedEventArgs*) -> HRESULT {
        QueueCallback(state, uiGeneration, false);
        return S_OK;
      });
    auto failed = Callback<ABI::Windows::Foundation::ITypedEventHandler<Notifications::ToastNotification*, Notifications::ToastFailedEventArgs*>>(
      [state, uiGeneration](Notifications::IToastNotification*, Notifications::IToastFailedEventArgs*) -> HRESULT {
        QueueCallback(state, uiGeneration, false);
        return S_OK;
      });
    m_Activated = {};
    m_Dismissed = {};
    m_Failed    = {};
    hr          = toast->add_Activated(activated.Get(), &m_Activated);
    if (SUCCEEDED(hr))
      hr = toast->add_Dismissed(dismissed.Get(), &m_Dismissed);
    if (SUCCEEDED(hr))
      hr = toast->add_Failed(failed.Get(), &m_Failed);
    m_pToast = toast;
    if (SUCCEEDED(hr))
    {
      // Keep the call arguments alive even if a native message acknowledges/clears the toast.
      const auto notifier = m_pNotifier;
      hr                  = notifier->Show(toast.Get());
      if (m_pToast.Get() != toast.Get())
        notifier->Hide(toast.Get());
    }
    if (FAILED(hr))
      Clear();
    return hr;
  }

  ComPtr<Notifications::IToastNotifier>     m_pNotifier;
  ComPtr<Notifications::IToastNotification> m_pToast;
  EventRegistrationToken                    m_Activated    = {};
  EventRegistrationToken                    m_Dismissed    = {};
  EventRegistrationToken                    m_Failed       = {};
  bool                                      m_bInitialized = false;
};

xiiResult xiiWindowsNotifications::PrepareShortcut(const QString& sShortcut, const QString& sExecutable)
{
  if (!QFileInfo(sExecutable).isFile() || !QFileInfo(sExecutable).isAbsolute())
    return XII_FAILURE;
  ComPtr<IShellLinkW> link;
  if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link))))
    return XII_FAILURE;
  ComPtr<IPersistFile>   file;
  ComPtr<IPropertyStore> properties;
  if (FAILED(link.As(&file)) || FAILED(link.As(&properties)))
    return XII_FAILURE;
  const QString sNativeShortcut = QDir::toNativeSeparators(sShortcut);
  // QFileInfo follows Windows .lnk targets; a broken/foreign shortcut is still an existing file.
  const DWORD uiAttributes = GetFileAttributesW(NotificationWide(sNativeShortcut));
  if (uiAttributes != INVALID_FILE_ATTRIBUTES)
  {
    if (FAILED(file->Load(NotificationWide(sNativeShortcut), STGM_READWRITE)))
      return XII_FAILURE;
    PROPVARIANT previous = {};
    XII_SCOPE_EXIT(PropVariantClear(&previous));
    // Do not replace a user's unrelated shortcut with the same display name.
    if (FAILED(properties->GetValue(PKEY_AppUserModel_ID, &previous)) || previous.vt != VT_LPWSTR ||
        previous.pwszVal == nullptr || wcscmp(previous.pwszVal, xiiWindowsJumpList::GetAppId()) != 0)
      return XII_FAILURE;
  }
  else if (GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND)
    return XII_FAILURE;
  const QString sNativeExe = QDir::toNativeSeparators(sExecutable);
  PROPVARIANT   appId      = {};
  XII_SCOPE_EXIT(PropVariantClear(&appId));
  if (FAILED(link->SetPath(NotificationWide(sNativeExe))) || FAILED(link->SetArguments(L"")) ||
      FAILED(link->SetIconLocation(NotificationWide(sNativeExe), 0)) ||
      FAILED(InitPropVariantFromString(xiiWindowsJumpList::GetAppId(), &appId)) ||
      FAILED(properties->SetValue(PKEY_AppUserModel_ID, appId)) || FAILED(properties->Commit()) ||
      FAILED(file->Save(NotificationWide(sNativeShortcut), TRUE)))
    return XII_FAILURE;
  return XII_SUCCESS;
}

QString xiiWindowsNotifications::BuildXml(const Message& message)
{
  // Legacy desktop templates deliberately use live-process activation. ToastGeneric requires
  // persistent COM activation and could launch another Editor after the original process exits.
  auto text = [](const QString& sText, int iLimit) {
    QString sValue = sText.simplified().left(iLimit);
    if (!sValue.isEmpty() && sValue.back().isHighSurrogate())
      sValue.chop(1);
    return sValue.toHtmlEscaped();
  };
  return QStringLiteral("<toast><visual><binding template=\"ToastText02\"><text id=\"1\">%1</text><text id=\"2\">%2</text></binding></visual><audio silent=\"true\"/></toast>")
    .arg(text(message.m_sTitle, 80), text(message.m_sText, 320));
}

bool xiiWindowsNotifications::IsEligible(bool bForeground, double fSeconds, bool bSucceeded, bool bKnownOperation)
{
  return !bForeground && (bKnownOperation ? (!bSucceeded || fSeconds >= 10.0) : (!bSucceeded && fSeconds >= 10.0));
}

xiiWindowsNotifications::xiiWindowsNotifications(xiiQtEditorApp& editor, QWidget& window) :
  QObject(&editor), m_pWindow(&window), m_pProcessor(xiiAssetProcessor::GetSingleton()), m_pLongOps(xiiLongOpControllerManager::GetSingleton())
{
  XII_ASSERT_DEV(QThread::currentThread() == qApp->thread(), "Notifications require the Qt GUI thread.");
  m_CallbackState           = XII_DEFAULT_NEW(CallbackState);
  m_CallbackState->m_pOwner = this;
  connect(&editor, &xiiQtEditorApp::ToolExecutionStarted, this, &xiiWindowsNotifications::ToolStarted);
  connect(&editor, &xiiQtEditorApp::ToolExecutionFinished, this, [this](bool bSucceeded, const QString& sOperation) {
    Message message;
    if (ToolFinished(bSucceeded, sOperation, IsForeground(), message))
      Notify(message);
  });
  connect(&editor, &xiiQtEditorApp::AssetImportFinished, this, [this](xiiUInt32 uiSucceeded, xiiUInt32 uiFailed, double fSeconds) {
    if ((uiSucceeded > 0 || uiFailed > 0) && IsEligible(IsForeground(), fSeconds, uiFailed == 0, true))
      Notify({uiFailed > 0 ? QStringLiteral("Asset import failed") : QStringLiteral("Asset import complete"),
              QStringLiteral("%1 imported; %2 failed. Return to XII Editor for details.").arg(uiSucceeded).arg(uiFailed)});
  });
  connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() { m_bStopped = true; Clear(); });
  connect(&window, &QObject::destroyed, this, [this]() { m_bStopped = true; Clear(); });
  connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
    if (state == Qt::ApplicationActive)
      Clear();
  });
  if (m_pProcessor)
    m_pProcessor->m_Events.AddEventHandler(xiiMakeDelegate(&xiiWindowsNotifications::ProcessorEventHandler, this));
  if (m_pLongOps)
  {
    XII_LOCK(m_pLongOps->m_Mutex);
    m_pLongOps->m_Events.AddEventHandler(xiiMakeDelegate(&xiiWindowsNotifications::LongOpEventHandler, this));
  }
  xiiToolsProject::s_Events.AddEventHandler(xiiMakeDelegate(&xiiWindowsNotifications::ProjectEventHandler, this));
}

xiiWindowsNotifications::~xiiWindowsNotifications()
{
  XII_ASSERT_DEV(QThread::currentThread() == qApp->thread(), "Notifications must shut down on the Qt GUI thread.");
  m_bStopped = true;
  {
    XII_LOCK(m_CallbackState->m_Mutex);
    m_CallbackState->m_pOwner = nullptr;
  }
  if (m_pProcessor)
    m_pProcessor->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiWindowsNotifications::ProcessorEventHandler, this));
  if (m_pLongOps)
  {
    XII_LOCK(m_pLongOps->m_Mutex);
    m_pLongOps->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiWindowsNotifications::LongOpEventHandler, this));
  }
  xiiToolsProject::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiWindowsNotifications::ProjectEventHandler, this));
  m_pNative.Clear();
}

bool xiiWindowsNotifications::IsForeground() const
{
  DWORD uiProcess = 0;
  GetWindowThreadProcessId(GetForegroundWindow(), &uiProcess);
  // A modal dialog or a document in another container also counts as receiving attention.
  return uiProcess == GetCurrentProcessId();
}

void xiiWindowsNotifications::ToolStarted(const QString& sOperation)
{
  auto& tool = m_Tools[sOperation];
  if (tool.m_uiCount++ == 0)
  {
    tool.m_Start   = xiiTime::Now();
    tool.m_bFailed = false;
  }
  Clear();
}

bool xiiWindowsNotifications::ToolFinished(bool bSucceeded, const QString& sOperation, bool bForeground, Message& out_message)
{
  auto it = m_Tools.find(sOperation);
  if (it == m_Tools.end())
    return false;
  it->m_bFailed |= !bSucceeded;
  if (--it->m_uiCount != 0)
    return false;
  const Tool tool = it.value();
  m_Tools.erase(it);
  const bool bKnown = sOperation == QStringLiteral("Compiling shaders") || sOperation == QStringLiteral("Building C++ project");
  if (!IsEligible(bForeground, (xiiTime::Now() - tool.m_Start).GetSeconds(), !tool.m_bFailed, bKnown))
    return false;
  QString sName = sOperation.isEmpty() ? QStringLiteral("Background operation") : sOperation;
  if (sOperation == QStringLiteral("Compiling shaders"))
    sName = QStringLiteral("Shader compilation");
  else if (sOperation == QStringLiteral("Building C++ project"))
    sName = QStringLiteral("C++ project build");
  out_message = {tool.m_bFailed ? QStringLiteral("Operation failed") : QStringLiteral("Operation complete"),
                 sName + (tool.m_bFailed ? QStringLiteral(" failed. Return to XII Editor for details.") : QStringLiteral(" finished. Return to XII Editor."))};
  return true;
}

void xiiWindowsNotifications::ProcessorEventHandler(const xiiAssetProcessorEvent& e)
{
  if (e.m_Type == xiiAssetProcessorEvent::Type::TransformsFinished)
  {
    const auto progress   = e.m_Progress;
    const auto generation = m_uiProjectGeneration.load();
    QMetaObject::invokeMethod(this, [this, progress, generation]() {
      if (generation != m_uiProjectGeneration.load() || progress.m_bCancelRequested || progress.m_uiCanceled > 0 ||
          !IsEligible(IsForeground(), progress.m_Duration.GetSeconds(), progress.m_uiFailed == 0 && progress.m_uiNeedsImport == 0, true))
        return;
      Notify({progress.m_uiFailed > 0 ? QStringLiteral("Asset transformation failed") : (progress.m_uiNeedsImport > 0 ? QStringLiteral("Assets require import") : QStringLiteral("Asset transformation complete")),
        QStringLiteral("%1 completed; %2 failed; %3 need import. Return to XII Editor for details.").arg(progress.m_uiCompleted).arg(progress.m_uiFailed).arg(progress.m_uiNeedsImport)}); }, Qt::QueuedConnection);
    return;
  }
  if (e.m_Type != xiiAssetProcessorEvent::Type::AssetProcessed || e.m_Result != xiiTransformResult::Failure || e.m_bRequested || e.m_bCanceled)
    return;
  const QString sAsset              = xiiMakeQString(e.m_sAssetPath);
  const auto    uiProjectGeneration = m_uiProjectGeneration.load();
  QMetaObject::invokeMethod(this, [this, sAsset, uiProjectGeneration]() {
    if (uiProjectGeneration == m_uiProjectGeneration.load())
      Notify({QStringLiteral("Background asset processing failed"), sAsset + QStringLiteral(" could not be processed. Return to XII Editor for details.")}); }, Qt::QueuedConnection);
}

void xiiWindowsNotifications::LongOpEventHandler(const xiiLongOpControllerEvent& e)
{
  if (e.m_Type != xiiLongOpControllerEvent::Type::OpFinished || e.m_bSucceeded || e.m_bCanceled)
    return;
  XII_LOCK(m_pLongOps->m_Mutex);
  const auto* pOp = m_pLongOps->GetOperation(e.m_OperationGuid);
  if (pOp == nullptr || pOp->m_StartOrDuration.GetSeconds() < 10.0)
    return;
  const QString sName               = xiiMakeQString(pOp->m_pProxyOp->GetDisplayName());
  const auto    uiProjectGeneration = m_uiProjectGeneration.load();
  QMetaObject::invokeMethod(this, [this, sName, uiProjectGeneration]() {
    if (uiProjectGeneration == m_uiProjectGeneration.load())
      Notify({QStringLiteral("Background operation failed"), sName + QStringLiteral(" failed. Return to XII Editor for details.")}); }, Qt::QueuedConnection);
}

void xiiWindowsNotifications::ProjectEventHandler(const xiiToolsProjectEvent& e)
{
  if (e.m_Type == xiiToolsProjectEvent::Type::ProjectClosing)
  {
    ++m_uiProjectGeneration;
    Clear();
    m_Tools.clear();
  }
}

void xiiWindowsNotifications::Notify(const Message& message)
{
  XII_ASSERT_DEV(QThread::currentThread() == qApp->thread(), "Toast presentation requires the Qt GUI thread.");
  if (m_bStopped || !m_pWindow || IsForeground() || xiiToolsProject::IsProjectClosing() || !NotificationsAllowed())
    return;
  const xiiTime now = xiiTime::Now();
  // Drop bursts, including failed delivery attempts. Never queue stale outcomes to display later.
  if (m_LastAttempt.IsPositive() && now - m_LastAttempt < xiiTime::MakeFromSeconds(30))
    return;
  m_LastAttempt = now;
  if (!m_pNative)
    m_pNative = XII_DEFAULT_NEW(Native);
  // A toast is optional presentation. Failure must not affect operations or create recursive log toasts.
  Clear();
  const auto    uiGeneration = m_uiGeneration;
  const HRESULT hr           = m_pNative->Show(message, m_CallbackState, uiGeneration);
  if (FAILED(hr))
  {
    Clear();
    // Reacquire WinRT services on the next eligible event after a shell/service failure.
    m_pNative.Clear();
  }
  else if (m_bStopped || uiGeneration != m_uiGeneration)
    Clear();
}

void xiiWindowsNotifications::Clear()
{
  ++m_uiGeneration;
  if (m_pNative)
    m_pNative->Clear();
}

void xiiWindowsNotifications::Activate()
{
  if (!m_bStopped && m_pWindow)
    xiiQtEditorApp::ShowEditorWindow(m_pWindow);
}

void xiiWindowsNotifications::QueueCallback(const xiiSharedPtr<CallbackState>& state, xiiUInt64 uiGeneration, bool bActivate)
{
  // Serialize enqueueing with destruction. The queued functor is tied to the receiver lifetime;
  // native threads never read QPointer or touch a QWidget, including during shutdown.
  XII_LOCK(state->m_Mutex);
  auto* pOwner = state->m_pOwner;
  if (pOwner == nullptr)
    return;
  QMetaObject::invokeMethod(pOwner, [pOwner, uiGeneration, bActivate]() {
    if (!pOwner->m_bStopped && pOwner->m_uiGeneration == uiGeneration)
    {
      pOwner->Clear();
      if (bActivate)
        pOwner->Activate();
    } }, Qt::QueuedConnection);
}

#endif
