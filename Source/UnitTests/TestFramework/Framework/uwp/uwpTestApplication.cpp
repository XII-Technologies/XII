#include <TestFramework/TestFrameworkPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#  include <Foundation/Strings/StringConversion.h>
#  include <TestFramework/Framework/uwp/uwpTestApplication.h>
#  include <TestFramework/Framework/uwp/uwpTestFramework.h>
#  include <windows.ui.core.h>
#  include <wrl/event.h>

using namespace ABI::Windows::Foundation;

xiiUwpTestApplication::xiiUwpTestApplication(xiiTestFramework& testFramework) :
  m_testFramework(testFramework)
{
}

xiiUwpTestApplication::~xiiUwpTestApplication() {}

HRESULT xiiUwpTestApplication::CreateView(IFrameworkView** viewProvider)
{
  *viewProvider = this;
  return S_OK;
}

HRESULT xiiUwpTestApplication::Initialize(ICoreApplicationView* applicationView)
{
  using OnActivatedHandler = __FITypedEventHandler_2_Windows__CApplicationModel__CCore__CCoreApplicationView_Windows__CApplicationModel__CActivation__CIActivatedEventArgs;

  XII_SUCCEED_OR_RETURN(applicationView->add_Activated(Callback<OnActivatedHandler>(this, &xiiUwpTestApplication::OnActivated).Get(), &m_eventRegistrationOnActivate));

  xiiStartup::StartupBaseSystems();

  return S_OK;
}

HRESULT xiiUwpTestApplication::SetWindow(ABI::Windows::UI::Core::ICoreWindow* window)
{
  return S_OK;
}

HRESULT xiiUwpTestApplication::Load(HSTRING entryPoint)
{
  return S_OK;
}

HRESULT xiiUwpTestApplication::Run()
{
  ComPtr<ABI::Windows::UI::Core::ICoreWindowStatic> coreWindowStatics;
  XII_SUCCEED_OR_RETURN(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Core_CoreWindow).Get(), &coreWindowStatics));
  ComPtr<ABI::Windows::UI::Core::ICoreWindow> coreWindow;
  XII_SUCCEED_OR_RETURN(coreWindowStatics->GetForCurrentThread(&coreWindow));
  ComPtr<ABI::Windows::UI::Core::ICoreDispatcher> dispatcher;
  XII_SUCCEED_OR_RETURN(coreWindow->get_Dispatcher(&dispatcher));

  while (m_testFramework.RunTestExecutionLoop() == xiiTestAppRun::Continue)
  {
    dispatcher->ProcessEvents(ABI::Windows::UI::Core::CoreProcessEventsOption_ProcessAllIfPresent);
  }

  return S_OK;
}

HRESULT xiiUwpTestApplication::Uninitialize()
{
  m_testFramework.AbortTests();
  return S_OK;
}

HRESULT xiiUwpTestApplication::OnActivated(ICoreApplicationView* applicationView, IActivatedEventArgs* args)
{
  applicationView->remove_Activated(m_eventRegistrationOnActivate);

  ActivationKind activationKind;
  XII_SUCCEED_OR_RETURN(args->get_Kind(&activationKind));

  if (activationKind == ActivationKind_Launch)
  {
    ComPtr<ILaunchActivatedEventArgs> launchArgs;
    XII_SUCCEED_OR_RETURN(args->QueryInterface(launchArgs.GetAddressOf()));

    HString argHString;
    XII_SUCCEED_OR_RETURN(launchArgs->get_Arguments(argHString.GetAddressOf()));

    xiiDynamicArray<xiiString>   commandLineArgs;
    xiiDynamicArray<const char*> argv;
    xiiCommandLineUtils::SplitCommandLineString(xiiStringUtf8(argHString).GetData(), true, commandLineArgs, argv);

    xiiCommandLineUtils cmd;
    cmd.SetCommandLine(argv.GetCount(), argv.GetData(), xiiCommandLineUtils::PreferOsArgs);

    m_testFramework.GetTestSettingsFromCommandLine(cmd);

    // Setup an extended execution session to prevent app from going to sleep during testing.
    xiiUwpUtils::CreateInstance<IExtendedExecutionSession>(RuntimeClass_Windows_ApplicationModel_ExtendedExecution_ExtendedExecutionSession, m_extendedExecutionSession);
    XII_ASSERT_DEV(m_extendedExecutionSession, "Failed to create extended session. Can't prevent app from backgrounding during testing.");
    m_extendedExecutionSession->put_Reason(ExtendedExecutionReason::ExtendedExecutionReason_Unspecified);
    xiiStringHString desc("Keep Unit Tests Running");
    m_extendedExecutionSession->put_Description(desc.GetData().Get());

    using OnRevokedHandler = __FITypedEventHandler_2_IInspectable_Windows__CApplicationModel__CExtendedExecution__CExtendedExecutionRevokedEventArgs;
    XII_SUCCEED_OR_RETURN(m_extendedExecutionSession->add_Revoked(Callback<OnRevokedHandler>(this, &xiiUwpTestApplication::OnSessionRevoked).Get(), &m_eventRegistrationOnRevokedSession));

    ComPtr<__FIAsyncOperation_1_Windows__CApplicationModel__CExtendedExecution__CExtendedExecutionResult> pAsyncOp;
    if (SUCCEEDED(m_extendedExecutionSession->RequestExtensionAsync(&pAsyncOp)))
    {
      xiiUwpUtils::xiiWinRtPutCompleted<ExtendedExecutionResult, ExtendedExecutionResult>(pAsyncOp, [this](const ExtendedExecutionResult& pResult) {
        switch (pResult)
        {
          case ExtendedExecutionResult::ExtendedExecutionResult_Allowed:
            xiiLog::Info("Extended session is active.");
            break;
          case ExtendedExecutionResult::ExtendedExecutionResult_Denied:
            xiiLog::Error("Extended session is denied.");
            break;
        }
      });
    }
  }

  return S_OK;
}

HRESULT xiiUwpTestApplication::OnSessionRevoked(IInspectable* sender, IExtendedExecutionRevokedEventArgs* args)
{
  xiiLog::Error("Extended session revoked.");
  return S_OK;
}

#endif

XII_STATICLINK_FILE(TestFramework, TestFramework_Framework_uwp_uwpTestApplication);
