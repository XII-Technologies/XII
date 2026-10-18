/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Application/Application.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <QApplication>
#include <QFontDatabase>
#include <TestFramework/Utilities/TestSetup.h>

int RunInspectorCaptureServer();

class InspectorTestApplication : public xiiApplication
{
public:
  InspectorTestApplication() : xiiApplication("InspectorTest") {}
  Execution Run() override
  {
    if (xiiCommandLineUtils::GetGlobalInstance()->GetBoolOption("-capture-server"))
    {
      SetReturnCode(RunInspectorCaptureServer());
      return Execution::Quit;
    }
    int          argc = GetArgumentCount();
    QApplication app(argc, const_cast<char**>(GetArgumentsArray()));
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    // Qt's offscreen plugin does not discover the Windows system font collection.
    const int  font     = QFontDatabase::addApplicationFont(qEnvironmentVariable("WINDIR") + "/Fonts/segoeui.ttf");
    const auto families = QFontDatabase::applicationFontFamilies(font);
    if (!families.isEmpty())
      app.setFont(QFont(families.front(), 9));
#endif
    xiiTestSetup::InitTestFramework("InspectorTest", "Inspector Tests", GetArgumentCount(), GetArgumentsArray());
    while (xiiTestSetup::RunTests() == xiiTestAppRun::Continue)
    {
    }
    SetReturnCode(xiiTestSetup::GetFailedTestCount());
    xiiTestSetup::DeInitTestFramework();
    return Execution::Quit;
  }
};
XII_CONSOLEAPP_ENTRY_POINT(InspectorTestApplication);
