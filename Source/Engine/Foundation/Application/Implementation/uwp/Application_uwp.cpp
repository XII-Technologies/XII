#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  include <Foundation/Application/Application.h>
#  include <Foundation/Application/Implementation/uwp/Application_uwp.h>
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Strings/StringConversion.h>

// Disable warning produced by CppWinRT
#  pragma warning(disable : 5205)
#  include <winrt/Windows.ApplicationModel.Activation.h>
#  include <winrt/Windows.ApplicationModel.Core.h>
#  include <winrt/Windows.Foundation.Collections.h>
#  include <winrt/Windows.Foundation.h>
#  include <winrt/Windows.UI.Core.h>

using namespace winrt::Windows::ApplicationModel::Core;

xiiUwpApplication::xiiUwpApplication(xiiApplication* application) :
  m_application(application)
{
}

xiiUwpApplication::~xiiUwpApplication() {}

winrt::Windows::ApplicationModel::Core::IFrameworkView xiiUwpApplication::CreateView()
{
  return this->get_strong().try_as<winrt::Windows::ApplicationModel::Core::IFrameworkView>();
}

void xiiUwpApplication::Initialize(winrt::Windows::ApplicationModel::Core::CoreApplicationView const& applicationView)
{
  applicationView.Activated({this, &xiiUwpApplication::OnViewActivated});
}

void xiiUwpApplication::SetWindow(winrt::Windows::UI::Core::CoreWindow const& window)
{
}

void xiiUwpApplication::Load(winrt::hstring const& entryPoint)
{
}

void xiiUwpApplication::Run()
{
  if (xiiRun_Startup(m_application).Succeeded())
  {
    auto window = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
    window.Activate();

    xiiRun_MainLoop(m_application);
  }
  xiiRun_Shutdown(m_application);
}

void xiiUwpApplication::Uninitialize()
{
}

void xiiUwpApplication::OnViewActivated(winrt::Windows::ApplicationModel::Core::CoreApplicationView const& sender, winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs const& args)
{
  sender.Activated(m_activateRegistrationToken);

  if (args.Kind() == winrt::Windows::ApplicationModel::Activation::ActivationKind::Launch)
  {
    auto           launchArgs = args.as<winrt::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs>();
    winrt::hstring argHString = launchArgs.Arguments();

    xiiDynamicArray<const char*> argv;
    xiiCommandLineUtils::SplitCommandLineString(xiiStringUtf8(argHString.c_str()).GetData(), true, m_commandLineArgs, argv);

    m_application->SetCommandLineArguments(argv.GetCount(), argv.GetData());
  }
}

XII_FOUNDATION_DLL xiiResult xiiUWPRun(xiiApplication* pApp)
{
  {
    auto application = winrt::make<xiiUwpApplication>(pApp);
    winrt::Windows::ApplicationModel::Core::CoreApplication::Run(application.as<winrt::Windows::ApplicationModel::Core::IFrameworkViewSource>());
  }

  return XII_SUCCESS;
}

#endif

XII_STATICLINK_FILE(Foundation, Foundation_Application_Implementation_uwp_Application_uwp);
