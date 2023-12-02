#include <Player/Player.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Input/InputManager.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Animation/RotorComponent.h>
#include <GameEngine/Animation/SliderComponent.h>
#include <GameEngine/Gameplay/InputComponent.h>
#include <GameEngine/Gameplay/SpawnComponent.h>
#include <GameEngine/Gameplay/TimedDeathComponent.h>
#include <GraphicsCore/Components/CameraComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Meshes/MeshComponent.h>

// this injects the main function
XII_APPLICATION_ENTRY_POINT(xiiPlayerApplication);

// these command line options may not all be directly used in xiiPlayer, but the xiiFallbackGameState reads those options to determine which scene to load
xiiCommandLineOptionString opt_Project("_Player", "-project", "Path to the project folder.\nUsually an absolute path, though relative paths will work for projects that are located inside the XII SDK directory.", "");
xiiCommandLineOptionString opt_Scene("_Player", "-scene", "Path to a scene file.\nUsually given relative to the corresponding project data directory where it resides, but can also be given as an absolute path.", "");

xiiPlayerApplication::xiiPlayerApplication()
  : xiiGameApplication("xiiPlayer") // we don't have a fixed project path in this app, so we need to pass that in a bit later
{
}

xiiResult xiiPlayerApplication::BeforeCoreSystemsStartup()
{
  // show the command line options, if help is requested
  {
    // since this is a GUI application (not a console app), printf has no effect
    // therefore we have to show the command line options with a message box

    xiiStringBuilder cmdHelp;
    if (xiiCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, xiiCommandLineOption::LogAvailableModes::IfHelpRequested))
    {
      xiiLog::OsMessageBox(cmdHelp);
      SetReturnCode(-1);
      return XII_FAILURE;
    }
  }

  xiiStartup::AddApplicationTag("player");

  XII_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  DetermineProjectPath();

  return XII_SUCCESS;
}


void xiiPlayerApplication::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  xiiStartup::StartupHighLevelSystems();

  // we need a game state to do anything
  // if no custom game state is available, xiiFallbackGameState will be used
  // the game state is also responsible for either creating a world, or loading it
  // the xiiFallbackGameState inspects the command line to figure out which scene to load
  ActivateGameState(nullptr).AssertSuccess();
}

void xiiPlayerApplication::Run_InputUpdate()
{
  SUPER::Run_InputUpdate();

  if (GetActiveGameState() && GetActiveGameState()->WasQuitRequested())
  {
    RequestQuit();
  }
}

void xiiPlayerApplication::DetermineProjectPath()
{
  xiiStringBuilder sProjectPath = opt_Project.GetOptionValue(xiiCommandLineOption::LogMode::FirstTime);

#if XII_DISABLED(XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // We can't specify command line arguments on many platforms so the project must be defined by xiiFileserve.
  // xiiFileserve must be started with the project special dir set. For example:
  // -specialdirs project ".../XII/Data/Samples/Testing Chambers

  if (sProjectPath.IsEmpty())
  {
    m_sAppProjectPath = ">project";
    return;
  }
#endif

  if (sProjectPath.IsEmpty())
  {
    const xiiStringBuilder sScenePath = opt_Scene.GetOptionValue(xiiCommandLineOption::LogMode::FirstTime);

    // project path is empty, need to extract it from the scene path

    if (!sScenePath.IsAbsolutePath())
    {
      // scene path is not absolute -> can't extract project path
      m_sAppProjectPath = xiiFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return;
    }

    if (xiiFileSystem::FindFolderWithSubPath(sProjectPath, sScenePath, "xiiProject", "xiiSdkRoot.txt").Failed())
    {
      // couldn't find the 'xiiProject' file in any parent folder of the scene
      m_sAppProjectPath = xiiFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return;
    }
  }
  else if (!xiiPathUtils::IsAbsolutePath(sProjectPath))
  {
    // project path is not absolute, so must be relative to the SDK directory
    sProjectPath.Prepend(xiiFileSystem::GetSdkRootDirectory(), "/");
  }

  sProjectPath.MakeCleanPath();
  sProjectPath.TrimWordEnd("/xiiProject");

  if (sProjectPath.IsEmpty())
  {
    m_sAppProjectPath = xiiFileSystem::GetSdkRootDirectory();
    SetReturnCode(1);
    return;
  }

  // store it now, even if it fails, for error reporting
  m_sAppProjectPath = sProjectPath;

  if (!xiiOSFile::ExistsDirectory(sProjectPath))
  {
    SetReturnCode(1);
    return;
  }
}
