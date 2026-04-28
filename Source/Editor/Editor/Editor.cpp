/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Editor/EditorPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Utilities/CommandLineOptions.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <shellscalingapi.h>
#endif

class xiiEditorApplication : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  xiiEditorApplication() :
    xiiApplication("xiiEditor")
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
#endif
    EnableMemoryLeakReporting(true);

    m_pEditorApp = new xiiQtEditorApp;
  }

  virtual xiiResult BeforeCoreSystemsStartup() override
  {
    xiiStartup::AddApplicationTag("tool");
    xiiStartup::AddApplicationTag("editor");
    xiiStartup::AddApplicationTag("editorapp");

    xiiQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());

    return XII_SUCCESS;
  }

  virtual void AfterCoreSystemsShutdown() override
  {
    xiiQtEditorApp::GetSingleton()->DeInitQt();

    delete m_pEditorApp;
    m_pEditorApp = nullptr;
  }

  virtual Execution Run() override
  {
    {
      xiiStringBuilder sCMDHelp;
      if (xiiCommandLineOption::LogAvailableOptionsToBuffer(sCMDHelp, xiiCommandLineOption::LogAvailableModes::IfHelpRequested, "_Editor;cvar"))
      {
        xiiQtUiServices::GetSingleton()->MessageBoxInformation(sCMDHelp);
        return xiiApplication::Execution::Quit;
      }
    }

    xiiQtEditorApp::GetSingleton()->StartupEditor();
    {
      const xiiInt32 iReturnCode = xiiQtEditorApp::GetSingleton()->RunEditor();
      SetReturnCode(iReturnCode);
    }
    xiiQtEditorApp::GetSingleton()->ShutdownEditor();

    return xiiApplication::Execution::Quit;
  }

private:
  xiiQtEditorApp* m_pEditorApp = nullptr;
};

XII_APPLICATION_ENTRY_POINT(xiiEditorApplication);
