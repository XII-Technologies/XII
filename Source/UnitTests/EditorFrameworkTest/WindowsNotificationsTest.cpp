/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Platform/Windows/WindowsNotifications.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#  include <EditorEngineProcessFramework/LongOps/LongOps.h>
#  include <EditorFramework/Assets/AssetProcessor.h>
#  include <EditorFramework/EditorApp/EditorApp.moc.h>
#  include <EditorFramework/Platform/Windows/WindowsJumpList.h>
#  include <Foundation/Application/Application.h>
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Types/ScopeExit.h>
#  include <QApplication>
#  include <QDialog>
#  include <QDir>
#  include <QFile>
#  include <QTemporaryDir>
#  include <QThread>
#  include <QXmlStreamReader>
#  include <objbase.h>
#  include <propkey.h>
#  include <roapi.h>
#  include <shobjidl.h>
#  include <thread>
#  include <windows.data.xml.dom.h>
#  include <windows.ui.notifications.h>
#  include <wrl.h>
#  include <wrl/wrappers/corewrappers.h>

struct xiiWindowsNotificationsTest
{
  static xiiWindowsNotifications& Attach(xiiQtEditorApp& editor)
  {
    editor.m_pNotifications = XII_DEFAULT_NEW(xiiWindowsNotifications, editor, *xiiQtContainerWindow::GetContainerWindow());
    return *editor.m_pNotifications;
  }
  using Message = xiiWindowsNotifications::Message;
  static bool Eligible(bool bForeground, double fSeconds, bool bSucceeded, bool bKnown = true)
  {
    return xiiWindowsNotifications::IsEligible(bForeground, fSeconds, bSucceeded, bKnown);
  }
  static QString   Xml(const QString& sTitle, const QString& sText) { return xiiWindowsNotifications::BuildXml({sTitle, sText}); }
  static xiiResult Shortcut(const QString& sPath, const QString& sExe) { return xiiWindowsNotifications::PrepareShortcut(sPath, sExe); }
  static auto      Gate(xiiWindowsNotifications& controller) { return controller.m_CallbackState; }
  static void      Queue(const auto& state, xiiUInt64 uiGeneration, bool bActivate) { xiiWindowsNotifications::QueueCallback(state, uiGeneration, bActivate); }
  static xiiUInt64 Generation(const xiiWindowsNotifications& controller) { return controller.m_uiGeneration; }
  static void      Clear(xiiWindowsNotifications& controller) { controller.Clear(); }
  static void      Stop(xiiWindowsNotifications& controller) { controller.m_bStopped = true; }
  static void      Age(xiiWindowsNotifications& controller, const QString& sName)
  {
    controller.m_Tools[sName].m_Start -= xiiTime::MakeFromSeconds(20);
  }
  static bool Finish(xiiWindowsNotifications& controller, bool bSucceeded, const QString& sName, bool bForeground, Message& out_message)
  {
    return controller.ToolFinished(bSucceeded, sName, bForeground, out_message);
  }
  static bool Empty(const xiiWindowsNotifications& controller) { return controller.m_Tools.isEmpty(); }
  static bool NativeUnused(const xiiWindowsNotifications& controller) { return !controller.m_pNative; }
  static void RateLimit(xiiWindowsNotifications& controller)
  {
    controller.m_LastAttempt = xiiTime::Now();
    controller.Notify({QStringLiteral("Test"), QStringLiteral("Must be dropped without native delivery")});
  }
  static void ProjectClosing(xiiWindowsNotifications& controller)
  {
    xiiToolsProjectEvent event;
    event.m_Type = xiiToolsProjectEvent::Type::ProjectClosing;
    controller.ProjectEventHandler(event);
  }
};

class xiiNotificationTestLongOpProxy final : public xiiLongOpProxy
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNotificationTestLongOpProxy, xiiLongOpProxy);

public:
  xiiStringView GetDisplayName() const override { return "Notification test operation"; }
  void          GetReplicationInfo(xiiStringBuilder& out_sType, xiiStreamWriter&) override { out_sType = "UnusedTestWorker"; }
  void          Finalize(xiiResult, const xiiDataBuffer&) override { ++m_uiFinalized; }
  xiiUInt32     m_uiFinalized = 0;
};

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNotificationTestLongOpProxy, 1, xiiRTTIDefaultAllocator<xiiNotificationTestLongOpProxy>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST_GROUP(EditorNotifications);

XII_CREATE_SIMPLE_TEST(EditorNotifications, AttentionPolicy)
{
  using Test = xiiWindowsNotificationsTest;
  XII_TEST_BOOL(!Test::Eligible(true, 600, false));
  XII_TEST_BOOL(!Test::Eligible(true, 600, true));
  XII_TEST_BOOL(!Test::Eligible(false, 9.99, true));
  XII_TEST_BOOL(Test::Eligible(false, 10, true));
  XII_TEST_BOOL(Test::Eligible(false, 0, false));
  XII_TEST_BOOL(!Test::Eligible(false, 600, true, false));
  XII_TEST_BOOL(!Test::Eligible(false, 9.99, false, false));
  XII_TEST_BOOL(Test::Eligible(false, 10, false, false));
}

XII_CREATE_SIMPLE_TEST(EditorNotifications, ConcurrentTools)
{
  auto&                   editor = *xiiQtEditorApp::GetSingleton();
  QWidget                 window;
  xiiWindowsNotifications controller(editor, window);
  using Test            = xiiWindowsNotificationsTest;
  const QString sShader = QStringLiteral("Compiling shaders");
  const QString sBuild  = QStringLiteral("Building C++ project");
  Test::Message message;
  Q_EMIT editor.ToolExecutionStarted(sShader);
  Q_EMIT editor.ToolExecutionStarted(sBuild);
  Q_EMIT editor.ToolExecutionStarted(sShader);
  Test::Age(controller, sShader);
  Test::Age(controller, sBuild);
  XII_TEST_BOOL(!Test::Finish(controller, false, sShader, false, message));
  XII_TEST_BOOL(Test::Finish(controller, true, sBuild, false, message));
  XII_TEST_BOOL(message.m_sText.contains(QStringLiteral("C++ project build")));
  XII_TEST_BOOL(Test::Finish(controller, true, sShader, false, message));
  XII_TEST_BOOL(message.m_sTitle == QStringLiteral("Operation failed"));
  XII_TEST_BOOL(!Test::Finish(controller, false, sShader, false, message));
  XII_TEST_BOOL(Test::Empty(controller));
  Q_EMIT editor.ToolExecutionStarted(sShader);
  Test::Age(controller, sShader);
  XII_TEST_BOOL(!Test::Finish(controller, false, sShader, true, message));
  Q_EMIT editor.ToolExecutionStarted(sShader);
  Test::ProjectClosing(controller);
  XII_TEST_BOOL(!Test::Finish(controller, false, sShader, false, message));
  Test::RateLimit(controller);
  XII_TEST_BOOL(Test::NativeUnused(controller));
}

XII_CREATE_SIMPLE_TEST(EditorNotifications, ActivationAndShutdown)
{
  auto&   editor = *xiiQtEditorApp::GetSingleton();
  QWidget window;
  auto*   pController     = new xiiWindowsNotifications(editor, window);
  using Test              = xiiWindowsNotificationsTest;
  const auto gate         = Test::Gate(*pController);
  auto       uiGeneration = Test::Generation(*pController);
  // Exercise the same callback gateway used by WinRT, from a non-GUI thread.
  std::thread worker([gate, uiGeneration]() { Test::Queue(gate, uiGeneration, true); });
  worker.join();
  XII_TEST_BOOL(!window.isVisible());
  QApplication::processEvents();
  XII_TEST_BOOL(window.isVisible());
  window.hide();
  uiGeneration = Test::Generation(*pController);
  Test::Queue(gate, uiGeneration, true);
  Test::Clear(*pController); // A newer outcome/foreground acknowledgement supersedes that click.
  QApplication::processEvents();
  XII_TEST_BOOL(!window.isVisible());

  QWidget* pBorrowed = new QWidget;
  {
    xiiWindowsNotifications borrowed(editor, *pBorrowed);
    delete pBorrowed;
    Test::Queue(Test::Gate(borrowed), Test::Generation(borrowed), true);
    QApplication::processEvents();
    XII_TEST_BOOL(Test::NativeUnused(borrowed));
  }
  Test::Queue(gate, Test::Generation(*pController), true);
  delete pController; // Queued work is removed, and late native callbacks see a closed gate.
  std::thread late([gate]() { Test::Queue(gate, 0, true); });
  late.join();
  QApplication::processEvents();
  XII_TEST_BOOL(!window.isVisible());
}

XII_CREATE_SIMPLE_TEST(EditorNotifications, NativeContentAndIdentity)
{
  using namespace Microsoft::WRL;
  using namespace Microsoft::WRL::Wrappers;
  namespace Xml           = ABI::Windows::Data::Xml::Dom;
  namespace Notifications = ABI::Windows::UI::Notifications;
  const HRESULT hr        = RoInitialize(RO_INIT_SINGLETHREADED);
  if (!XII_TEST_BOOL(SUCCEEDED(hr)))
    return;
  XII_SCOPE_EXIT(RoUninitialize());
  const QString    sTitle = QString::fromUtf8("Build <failed> & \"retry\" \xF0\x9F\x94\xA7");
  const QString    sText  = QString::fromUtf8("D:/Projects/\xE6\xB5\x8B\xE8\xAF\x95 & Demo/xiiProject");
  const QString    sXml   = xiiWindowsNotificationsTest::Xml(sTitle, sText);
  QXmlStreamReader reader(sXml);
  QStringList      texts;
  while (!reader.atEnd())
  {
    reader.readNext();
    if (reader.isStartElement() && reader.name() == QStringLiteral("text"))
      texts.push_back(reader.readElementText());
  }
  XII_TEST_BOOL(!reader.hasError());
  XII_TEST_BOOL(texts == QStringList({sTitle, sText}));
  const QString    sBoundary = QString(319, QLatin1Char('a')) + QString::fromUtf8("\xF0\x9F\x94\xA7");
  QXmlStreamReader boundary(xiiWindowsNotificationsTest::Xml(QStringLiteral("Title"), sBoundary));
  while (!boundary.atEnd())
    boundary.readNext();
  XII_TEST_BOOL(!boundary.hasError());
  ComPtr<IInspectable> instance;
  XII_TEST_BOOL(SUCCEEDED(RoActivateInstance(HStringReference(RuntimeClass_Windows_Data_Xml_Dom_XmlDocument).Get(), &instance)));
  ComPtr<Xml::IXmlDocumentIO> io;
  ComPtr<Xml::IXmlDocument>   document;
  if (!XII_TEST_BOOL(SUCCEEDED(instance.As(&io)) && SUCCEEDED(instance.As(&document))))
    return;
  XII_TEST_BOOL(SUCCEEDED(io->LoadXml(HStringReference(reinterpret_cast<const wchar_t*>(sXml.utf16())).Get())));
  ComPtr<Notifications::IToastNotificationFactory> factory;
  if (!XII_TEST_BOOL(SUCCEEDED(RoGetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), IID_PPV_ARGS(&factory)))))
    return;
  ComPtr<Notifications::IToastNotification> toast;
  XII_TEST_BOOL(SUCCEEDED(factory->CreateToastNotification(document.Get(), &toast)));

  QTemporaryDir directory;
  const QString sLink = directory.path() + QString::fromUtf8("/XII \xE6\xB5\x8B\xE8\xAF\x95.lnk");
  const QString sExe  = xiiMakeQString(xiiOSFile::GetApplicationPath());
  XII_TEST_RESULT(xiiWindowsNotificationsTest::Shortcut(sLink, sExe));
  XII_TEST_RESULT(xiiWindowsNotificationsTest::Shortcut(sLink, sExe));
  ComPtr<IShellLinkW> link;
  XII_TEST_BOOL(SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link))));
  ComPtr<IPersistFile>   file;
  ComPtr<IPropertyStore> properties;
  if (!XII_TEST_BOOL(SUCCEEDED(link.As(&file)) && SUCCEEDED(link.As(&properties))))
    return;
  const QString sNativeLink = QDir::toNativeSeparators(sLink);
  XII_TEST_BOOL(SUCCEEDED(file->Load(reinterpret_cast<const wchar_t*>(sNativeLink.utf16()), STGM_READ)));
  PROPVARIANT appId = {};
  XII_SCOPE_EXIT(PropVariantClear(&appId));
  XII_TEST_BOOL(SUCCEEDED(properties->GetValue(PKEY_AppUserModel_ID, &appId)));
  XII_TEST_BOOL(appId.vt == VT_LPWSTR && QString::fromWCharArray(appId.pwszVal) == QString::fromWCharArray(xiiWindowsJumpList::GetAppId()));
  wchar_t arguments[16] = {};
  XII_TEST_BOOL(SUCCEEDED(link->GetArguments(arguments, XII_ARRAY_SIZE(arguments))));
  XII_TEST_BOOL(arguments[0] == L'\0'); // Clicking a live toast never launches an editor with project arguments.
  XII_TEST_BOOL(xiiWindowsNotificationsTest::Shortcut(sLink, QStringLiteral("missing.exe")).Failed());
  const QString sForeign = directory.path() + QStringLiteral("/Foreign.lnk");
  QFile         foreign(sForeign);
  XII_TEST_BOOL(foreign.open(QIODevice::WriteOnly));
  foreign.write("Keep this file");
  foreign.close();
  XII_TEST_BOOL(xiiWindowsNotificationsTest::Shortcut(sForeign, sExe).Failed());
  XII_TEST_BOOL(foreign.open(QIODevice::ReadOnly));
  XII_TEST_BOOL(foreign.readAll() == QByteArray("Keep this file"));

  const QString sUnicodeExe = directory.path() + QString::fromUtf8("/XII Editor \xE6\xB5\x8B\xE8\xAF\x95.exe");
  XII_TEST_BOOL(QFile::copy(sExe, sUnicodeExe));
  XII_TEST_RESULT(xiiWindowsNotificationsTest::Shortcut(sLink, sUnicodeExe));

  QString sLongDirectory = directory.path();
  for (int i = 0; i < 5; ++i)
    sLongDirectory += QLatin1Char('/') + QString(50, QLatin1Char('a' + i));
  XII_TEST_BOOL(QDir().mkpath(sLongDirectory));
  const QString sLongExe = sLongDirectory + QString::fromUtf8("/XII Editor \xE6\xB5\x8B\xE8\xAF\x95.exe");
  XII_TEST_BOOL(sLongExe.size() > 260);
  XII_TEST_BOOL(QFile::copy(sExe, sLongExe));
  // Shell versions can reject executable/icon paths beyond MAX_PATH. Rejection must leave
  // the previously valid shortcut intact; success must preserve the entire Unicode path.
  const bool bLongPathAccepted = xiiWindowsNotificationsTest::Shortcut(sLink, sLongExe).Succeeded();
  XII_TEST_BOOL(SUCCEEDED(file->Load(reinterpret_cast<const wchar_t*>(sNativeLink.utf16()), STGM_READ)));
  wchar_t target[32768] = {};
  XII_TEST_BOOL(SUCCEEDED(link->GetPath(target, XII_ARRAY_SIZE(target), nullptr, SLGP_RAWPATH)));
  XII_TEST_BOOL(QDir::fromNativeSeparators(QString::fromWCharArray(target)).compare(bLongPathAccepted ? sLongExe : sUnicodeExe, Qt::CaseInsensitive) == 0);
}

XII_CREATE_SIMPLE_TEST(EditorNotifications, LongOperationResults)
{
  auto& manager = *xiiLongOpControllerManager::GetSingleton();
  if (!XII_TEST_BOOL(manager.GetOperations().IsEmpty()))
    return;
  xiiProcessCommunicationChannel channel;
  manager.Startup(&channel);
  XII_SCOPE_EXIT(manager.Shutdown());
  const xiiUuid document  = xiiUuid::MakeUuid();
  const xiiUuid component = xiiUuid::MakeUuid();
  manager.RegisterLongOp(document, component, "xiiNotificationTestLongOpProxy");
  XII_SCOPE_EXIT(manager.UnregisterLongOp(document, component, "xiiNotificationTestLongOpProxy"));
  auto&                                                   operation = *manager.GetOperations()[0];
  const xiiUuid                                           guid      = operation.m_OperationGuid;
  xiiLongOpControllerEvent                                last;
  xiiUInt32                                               uiFinished = 0;
  xiiEvent<const xiiLongOpControllerEvent&>::Unsubscriber subscription;
  manager.m_Events.AddEventHandler([&](const xiiLongOpControllerEvent& e) {
    if (e.m_Type == xiiLongOpControllerEvent::Type::OpFinished)
    {
      last = e;
      ++uiFinished;
      XII_TEST_BOOL(!operation.m_bIsRunning);
      XII_TEST_BOOL(operation.m_StartOrDuration.GetSeconds() >= 10);
    }
  },
                                   subscription);

  xiiLongOpResultMsg result;
  result.m_OperationGuid = guid;
  result.m_bSuccess      = false;
  xiiProcessCommunicationChannel::Event received;
  received.m_pMessage = &result;
  manager.StartOperation(guid);
  operation.m_StartOrDuration -= xiiTime::MakeFromSeconds(20);
  manager.CancelOperation(guid);
  channel.m_Events.Broadcast(received);
  XII_TEST_INT(uiFinished, 1);
  XII_TEST_BOOL(last.m_bCanceled && !last.m_bSucceeded);
  channel.m_Events.Broadcast(received);
  XII_TEST_INT(uiFinished, 1);
  manager.StartOperation(guid);
  operation.m_StartOrDuration -= xiiTime::MakeFromSeconds(20);
  channel.m_Events.Broadcast(received);
  XII_TEST_INT(uiFinished, 2);
  XII_TEST_BOOL(!last.m_bCanceled && !last.m_bSucceeded);
  manager.StartOperation(guid);
  operation.m_StartOrDuration -= xiiTime::MakeFromSeconds(20);
  result.m_bSuccess = true;
  channel.m_Events.Broadcast(received);
  XII_TEST_INT(uiFinished, 3);
  XII_TEST_BOOL(last.m_bSucceeded && !last.m_bCanceled);
  XII_TEST_INT(static_cast<xiiNotificationTestLongOpProxy*>(operation.m_pProxyOp.Borrow())->m_uiFinalized, 3);
}

XII_CREATE_SIMPLE_TEST(EditorNotifications, EditorShutdown)
{
  auto*           pApplication  = xiiApplication::GetApplicationInstance();
  const xiiString sPreviousName = pApplication->GetApplicationName();
  pApplication->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(pApplication->SetApplicationName(sPreviousName));
  auto&            editor = *xiiQtEditorApp::GetSingleton();
  QTemporaryDir    preferences;
  const QByteArray path = preferences.path().toUtf8();
  editor.StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, path.constData());
  QApplication::processEvents(); // Settle the container's initial deferred dock layout.
  auto&      controller = xiiWindowsNotificationsTest::Attach(editor);
  const auto gate       = xiiWindowsNotificationsTest::Gate(controller);
  const auto generation = xiiWindowsNotificationsTest::Generation(controller);
  xiiWindowsNotificationsTest::Queue(gate, generation, true);
  editor.ShutdownEditor();
  std::thread late([gate, generation]() { xiiWindowsNotificationsTest::Queue(gate, generation, true); });
  late.join();
  QApplication::processEvents();
  XII_TEST_BOOL(xiiQtContainerWindow::GetContainerWindow() == nullptr);
}

#endif
