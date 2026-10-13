/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <Foundation/Application/Application.h>
#include <TestFramework/Utilities/TestSetup.h>

// Production asset plugins resolve application data during subsystem startup.
// Keep a real application alive for the entire test and shutdown lifecycle.
class EditorFrameworkTestApplication : public xiiApplication
{
public:
  EditorFrameworkTestApplication() : xiiApplication("EditorFrameworkTest"), m_pEditorApp(new xiiQtEditorApp) {}

  xiiResult BeforeCoreSystemsStartup() override
  {
    m_pEditorApp->InitQt(GetArgumentCount(), const_cast<char**>(GetArgumentsArray()));
    return xiiApplication::BeforeCoreSystemsStartup();
  }

  void AfterCoreSystemsShutdown() override
  {
    m_pEditorApp->DeInitQt();
    delete m_pEditorApp;
    m_pEditorApp = nullptr;
  }

  Execution Run() override
  {
    xiiTestSetup::InitTestFramework("EditorFrameworkTest", "Editor Framework Tests", GetArgumentCount(), GetArgumentsArray());
    while (xiiTestSetup::RunTests() == xiiTestAppRun::Continue)
    {
    }
    SetReturnCode(xiiTestSetup::GetFailedTestCount());
    xiiTestSetup::DeInitTestFramework();
    return Execution::Quit;
  }

private:
  xiiQtEditorApp* m_pEditorApp;
};

XII_CONSOLEAPP_ENTRY_POINT(EditorFrameworkTestApplication);
