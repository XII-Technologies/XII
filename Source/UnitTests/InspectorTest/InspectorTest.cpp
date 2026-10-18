/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/Application/Application.h>
#include <TestFramework/Utilities/TestSetup.h>
#include <QApplication>

class InspectorTestApplication : public xiiApplication
{
public:
  InspectorTestApplication() : xiiApplication("InspectorTest") {}
  Execution Run() override
  {
    int argc = GetArgumentCount();
    QApplication app(argc, const_cast<char**>(GetArgumentsArray()));
    xiiTestSetup::InitTestFramework("InspectorTest", "Inspector Tests", GetArgumentCount(), GetArgumentsArray());
    while (xiiTestSetup::RunTests() == xiiTestAppRun::Continue) {}
    SetReturnCode(xiiTestSetup::GetFailedTestCount());
    xiiTestSetup::DeInitTestFramework();
    return Execution::Quit;
  }
};
XII_CONSOLEAPP_ENTRY_POINT(InspectorTestApplication);
