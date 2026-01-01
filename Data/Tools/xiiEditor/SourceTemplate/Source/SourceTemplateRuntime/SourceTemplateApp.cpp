#include <SourceTemplateRuntime/SourceTemplateApp.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>

// This injects the C++ main() function.
XII_APPLICATION_ENTRY_POINT(SourceTemplateApp);

SourceTemplateApp::SourceTemplateApp()
  : xiiGameApplication("SourceTemplate", nullptr)
{
}

xiiResult SourceTemplateApp::TryProjectFolder(xiiStringView sPath)
{
  xiiStringBuilder sProjDir = sPath;
  sProjDir.MakeCleanPath();

  xiiStringBuilder sProjFile;
  sProjFile.SetPath(sProjDir, "xiiProject");

  if (sProjFile.IsAbsolutePath() && xiiOSFile::ExistsFile(sProjFile))
  {
    m_sAppProjectPath = sProjDir;
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void SourceTemplateApp::DetermineProjectPath()
{
  // IMPORTANT!
  //
  // The project path has to be set for the xiiGameApplication to know where the main 'project' data directory is.
  // Without it, nothing will work (the game plugin won't be loaded etc).
  //
  // The path can be relative to the '>SDK' directory (the root folder where XII is located).
  // It may also be absolute (though this isn't portable across machines).
  // Or it can be relative to xiiOSFile::GetApplicationDirectory() (where the Game.exe is).
  //
  // If your project is inside the XII directory, use a relative path from there.
  // If it is somewhere outside, you either need to use an absolute path or some other way to locate it.
  //
  // Note that in a final exported build the project folder is always merged with the XII data folders into one package.

  // this path works for exported projects, because during export the project folder is always copied there
  xiiStringBuilder sProjDir;
  if (xiiFileSystem::ResolveSpecialDirectory(">sdk/Data/project", sProjDir).Succeeded())
  {
    if (TryProjectFolder(sProjDir).Succeeded())
      return;
  }

#ifdef RUNTIME_PROJECT_FOLDER
  // this absolute path will only work on the machine where the game is compiled,
  // but it works for projects that are located outside the xiiEngine folder
  if (TryProjectFolder(XII_PP_STRINGIFY(RUNTIME_PROJECT_FOLDER)).Succeeded())
    return;
#endif

  // in other cases, try this relative path
  m_sAppProjectPath = "Data/Samples/SourceTemplate";
}

xiiUniquePtr<xiiGameStateBase> SourceTemplateApp::CreateGameState()
{
  // usually we should only have a single non-fallback gamestate which is automatically picked
  // but if necessary, we can override this here
  return SUPER::CreateGameState();
}

xiiResult SourceTemplateApp::BeforeCoreSystemsStartup()
{
  xiiStartup::AddApplicationTag("game");

  XII_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  DetermineProjectPath();

  return XII_SUCCESS;
}

void SourceTemplateApp::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  xiiStartup::StartupHighLevelSystems();

  // we need a game state to do anything
  // if no custom game state is available, xiiFallbackGameState will be used
  // the game state is also responsible for either creating a world, or loading it
  // the xiiFallbackGameState inspects the command line to figure out which scene to load
  ActivateGameState(nullptr, {}, xiiTransform::MakeIdentity());
}

void SourceTemplateApp::Run_InputUpdate()
{
  SUPER::Run_InputUpdate();

  if (auto pGameState = GetActiveGameState())
  {
    // pass through the closing of the application
    if (pGameState->WasQuitRequested())
    {
      RequestQuit();
    }
  }
}
