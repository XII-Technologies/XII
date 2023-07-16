#include <TestFramework/TestFrameworkPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  include <TestFramework/Framework/uwp/uwpTestApplication.h>
#  include <TestFramework/Framework/uwp/uwpTestFramework.h>

#  include <Foundation/Logging/Log.h>

xiiUwpTestFramework::xiiUwpTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int argc, const char** argv) :
  xiiTestFramework(szTestName, szAbsTestDir, szRelTestDataDir, argc, argv)
{
}

xiiUwpTestFramework::~xiiUwpTestFramework()
{
  RoUninitialize();
}

void xiiUwpTestFramework::Run()
{
  ComPtr<ABI::Windows::ApplicationModel::Core::ICoreApplication> coreApplication;
  HRESULT                                                        result = ABI::Windows::Foundation::GetActivationFactory(
    HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(), &coreApplication);
  if (FAILED(result))
  {
    std::cout << "Failed to create core application." << std::endl;
    return;
  }
  else
  {
    ComPtr<xiiUwpTestApplication> application = Make<xiiUwpTestApplication>(*this);
    coreApplication->Run(application.Get());
    application.Detach(); // Was already deleted by uwp.
  }
}

#endif

XII_STATICLINK_FILE(TestFramework, TestFramework_Framework_uwp_uwpTestFramework);
