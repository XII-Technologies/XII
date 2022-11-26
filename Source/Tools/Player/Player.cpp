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
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>

// this injects the main function
XII_APPLICATION_ENTRY_POINT(xiiPlayerApplication);

xiiCommandLineOptionString opt_Project("_Player", "-project", "Path to the project folder.\nUsually an absolute path, though relative paths will work for projects that are located inside the XII SDK directory.", "");
xiiCommandLineOptionString opt_Scene("_Player", "-scene", "Path to a scene file.\nUsually given relative to the corresponding project data directory where it resides, but can also be given as an absolute path.", "");

xiiPlayerApplication::xiiPlayerApplication() :
  xiiGameApplication("xiiPlayer", nullptr) // we don't have a fixed project path in this app, so we need to pass that in a bit later
{
}

xiiResult xiiPlayerApplication::BeforeCoreSystemsStartup()
{
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

  m_State = State::Ok;

  if (DetermineProjectPath().Succeeded())
  {
    if (DetermineScenePath().Failed())
    {
      m_State = State::NoScene;
      m_Menu  = Menu::SceneSelection;
    }
  }

  return XII_SUCCESS;
}


void xiiPlayerApplication::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  xiiStartup::StartupHighLevelSystems();

  // create the xiiWorld into which we load all levels
  xiiWorldDesc desc("MainWorld");
  m_pWorld = XII_DEFAULT_NEW(xiiWorld, desc);

  if (m_State == State::Ok)
  {
    if (LoadScene(m_sSceneFile).Failed())
    {
      m_State = State::BadScene;
      m_Menu  = Menu::None;
      SetReturnCode(2);
    }
  }

  if (GetActiveGameState() == nullptr)
  {
    // if no scene was loaded (yet), still create a game-state, because that one creates the app's window
    // otherwise we won't see anything and can't interact with the menu
    ActivateGameState(m_pWorld.Borrow()).IgnoreResult();
  }
}

void xiiPlayerApplication::BeforeHighLevelSystemsShutdown()
{
  SUPER::BeforeHighLevelSystemsShutdown();

  m_pWorld.Clear();
}

void xiiPlayerApplication::Run_InputUpdate()
{
  SUPER::Run_InputUpdate();

  if (GetActiveGameState() && GetActiveGameState()->WasQuitRequested())
  {
    RequestQuit();
  }

  if (xiiStringUtils::IsNullOrEmpty(xiiInputManager::GetExclusiveInputSet()) ||
      xiiStringUtils::IsEqual(xiiInputManager::GetExclusiveInputSet(), "xiiPlayer"))
  {
    if (DisplayMenu())
    {
      // prevents the currently active scene from getting any input
      xiiInputManager::SetExclusiveInputSet("xiiPlayer");
    }
    else
    {
      // allows the active scene to retrieve input again
      xiiInputManager::SetExclusiveInputSet("");
    }
  }
}

xiiResult xiiPlayerApplication::DetermineProjectPath()
{
  xiiStringBuilder sProjectPath = opt_Project.GetOptionValue(xiiCommandLineOption::LogMode::FirstTime);

#if XII_DISABLED(XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // We can't specify command line arguments on many platforms so the project must be defined by xiiFileserve.
  // xiiFileserve must be started with the project special dir set. For example:
  // -specialdirs project ".../xiiEngine/Data/Samples/Testing Chambers

  if (sProjectPath.IsEmpty())
  {
    m_sAppProjectPath = ">project";
    return XII_SUCCESS;
  }
#endif

  if (sProjectPath.IsEmpty())
  {
    const xiiStringBuilder sScenePath = opt_Scene.GetOptionValue(xiiCommandLineOption::LogMode::FirstTime);

    // project path is empty, need to extract it from the scene path

    if (!sScenePath.IsAbsolutePath())
    {
      // scene path is not absolute -> can't extract project path
      m_State           = State::NoProject;
      m_sAppProjectPath = xiiFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return XII_FAILURE;
    }

    if (xiiFileSystem::FindFolderWithSubPath(sProjectPath, sScenePath, "xiiProject", "xiiSdkRoot.txt").Failed())
    {
      // couldn't find the 'xiiProject' file in any parent folder of the scene
      m_State           = State::NoProject;
      m_sAppProjectPath = xiiFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return XII_FAILURE;
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
    m_State           = State::NoProject;
    m_sAppProjectPath = xiiFileSystem::GetSdkRootDirectory();
    SetReturnCode(1);
    return XII_FAILURE;
  }

  // store it now, even if it fails, for error reporting
  m_sAppProjectPath = sProjectPath;

  if (!xiiOSFile::ExistsDirectory(sProjectPath))
  {
    m_State = State::BadProject;
    SetReturnCode(1);
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiPlayerApplication::DetermineScenePath()
{
  xiiStringBuilder sScenePath = opt_Scene.GetOptionValue(xiiCommandLineOption::LogMode::FirstTime);

#if XII_DISABLED(XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // TODO: We can't specify command line arguments on many platforms so the scene file is currently hardcoded
  if (sScenePath.IsEmpty())
  {
    m_sSceneFile = "Scenes/Empty.xiiScene";
    return XII_SUCCESS;
  }
#endif

  sScenePath.MakeCleanPath();

  if (sScenePath.IsEmpty())
    return XII_FAILURE;

  if (sScenePath.IsAbsolutePath())
  {
    // this is just to make the path shorter, when possible
    // but can fail if the scene is in another data directory
    sScenePath.MakeRelativeTo(m_sAppProjectPath).IgnoreResult();
  }

  m_sSceneFile = sScenePath;
  return XII_SUCCESS;
}

xiiResult xiiPlayerApplication::LoadScene(const char* szFile)
{
  XII_LOG_BLOCK("LoadScene", szFile);

  xiiStringBuilder sSceneFile = szFile;

  if (sSceneFile.IsEmpty())
  {
    xiiLog::Error("No scene file specified.");
    return XII_FAILURE;
  }

  xiiLog::Info("Loading scene '{}'.", szFile);

  if (sSceneFile.IsAbsolutePath())
  {
    // this can fail if the scene is in a different data directory than the project directory
    // shouldn't stop us from loading it anyway
    sSceneFile.MakeRelativeTo(m_sAppProjectPath).IgnoreResult();
  }

  if (sSceneFile.HasExtension("xiiScene") || sSceneFile.HasExtension("xiiPrefab"))
  {
    if (sSceneFile.IsRelativePath())
    {
      // if this is a path to the non-transformed source file, redirect it to the transformed file in the asset cache
      sSceneFile.Prepend("AssetCache/Common/");
      sSceneFile.ChangeFileExtension("xiiObjectGraph");
    }
  }

  if (sSceneFile != szFile)
  {
    xiiLog::Info("Redirecting scene file from '{}' to '{}'", szFile, sSceneFile);
  }

  XII_SUCCEED_OR_RETURN(LoadObjectGraph(sSceneFile));

  // (re-)create the game-state
  // this is either custom game code, or the xiiFallbackGameState
  // it is responsible for creating the main window, setting up the input devices
  // and adding high-level game logic
  {
    DeactivateGameState();
    ActivateGameState(m_pWorld.Borrow()).IgnoreResult();
  }

  xiiLog::Success("Successfully loaded scene.");
  return XII_SUCCESS;
}

xiiResult xiiPlayerApplication::LoadObjectGraph(const char* szFile)
{
  XII_LOG_BLOCK("LoadObjectGraph", szFile);

  XII_ASSERT_DEV(m_pWorld != nullptr, "xiiWorld must be created before loading anything into it.");
  XII_LOCK(m_pWorld->GetWriteMarker());

  // make sure the world is empty
  m_pWorld->Clear();

  xiiFileReader file;

  if (file.Open(szFile).Failed())
  {
    xiiLog::Error("Failed to open the file.");
    return XII_FAILURE;
  }

  // Read and skip the asset file header
  {
    xiiAssetFileHeader header;
    header.Read(file).AssertSuccess();

    char szSceneTag[16];
    file.ReadBytes(szSceneTag, sizeof(char) * 16);

    if (!xiiStringUtils::IsEqualN(szSceneTag, "[xiiBinaryScene]", 16))
    {
      xiiLog::Error("The given file isn't an object-graph file.");
      return XII_FAILURE;
    }
  }

  xiiWorldReader reader;
  if (reader.ReadWorldDescription(file).Failed())
  {
    xiiLog::Error("Error reading world description.");
    return XII_FAILURE;
  }

  reader.InstantiateWorld(*m_pWorld, nullptr);
  return XII_SUCCESS;
}

void xiiPlayerApplication::FindAvailableScenes()
{
  if (m_bCheckedForScenes)
    return;

  m_bCheckedForScenes = true;

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)
  xiiFileSystemIterator fsit;
  xiiStringBuilder      sScenePath;

  for (xiiFileSystem::StartSearch(fsit, "", xiiFileSystemIteratorFlags::ReportFilesRecursive);
       fsit.IsValid(); fsit.Next())
  {
    fsit.GetStats().GetFullPath(sScenePath);

    if (!sScenePath.HasExtension(".xiiScene"))
      continue;

    sScenePath.MakeRelativeTo(fsit.GetCurrentSearchTerm()).AssertSuccess();

    m_AvailableScenes.PushBack(sScenePath);
  }
#endif
}

bool xiiPlayerApplication::DisplayMenu()
{
  if (m_State == State::NoProject)
  {
    xiiDebugRenderer::DrawInfoText(m_pWorld.Borrow(), xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", "No project path provided.\n\nUse the command-line argument\n-project \"Path/To/xiiProject\"\nto tell xiiPlayer which project to load.\n\nWith the argument\n-scene \"Path/To/Scene.xiiScene\"\nyou can also directly load a specific scene.\n\nPress ESC to quit.", xiiColor::Red);

    return false;
  }

  if (m_State == State::BadProject)
  {
    xiiDebugRenderer::DrawInfoText(m_pWorld.Borrow(), xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", xiiFmt("Invalid project path provided.\nThe given project directory does not exist:\n\n{}\n\nPress ESC to quit.", m_sAppProjectPath), xiiColor::Red);

    return false;
  }

  if (xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeftWin) == xiiKeyState::Pressed || xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRightWin) == xiiKeyState::Pressed)
  {
    if (m_Menu == Menu::SceneSelection)
      m_Menu = Menu::None;
    else
      m_Menu = Menu::SceneSelection;
  }

  if (m_State == State::Ok && m_Menu == Menu::None)
    return false;

  xiiDebugRenderer::DrawInfoText(m_pWorld.Borrow(), xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", xiiFmt("Project: '{}'", m_sAppProjectPath), xiiColor::White);

  if (m_State == State::BadScene)
  {
    xiiDebugRenderer::DrawInfoText(m_pWorld.Borrow(), xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", xiiFmt("Failed to load scene: '{}'", m_sSceneFile), xiiColor::Red);
  }
  else
  {
    xiiDebugRenderer::DrawInfoText(m_pWorld.Borrow(), xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", xiiFmt("Scene: '{}'", m_sSceneFile), xiiColor::White);
  }

  if (m_Menu == Menu::SceneSelection)
  {
    FindAvailableScenes();

    xiiDebugRenderer::DrawInfoText(m_pWorld.Borrow(), xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", "\nSelect scene:\n", xiiColor::White);

    for (xiiUInt32 i = 0; i < m_AvailableScenes.GetCount(); ++i)
    {
      const auto& file = m_AvailableScenes[i];

      if (i == m_uiSelectedScene)
      {
        xiiDebugRenderer::DrawInfoText(m_pWorld.Borrow(), xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", xiiFmt("> {} <", file), xiiColor::Gold);
      }
      else
      {
        xiiDebugRenderer::DrawInfoText(m_pWorld.Borrow(), xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", xiiFmt("  {}  ", file), xiiColor::GhostWhite);
      }
    }

    xiiDebugRenderer::DrawInfoText(m_pWorld.Borrow(), xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", "\nPress 'Return' to load scene.\nPress the 'Windows' key to toggle this menu.", xiiColor::White);

    if (xiiInputManager::GetInputSlotState(xiiInputSlot_KeyEscape) == xiiKeyState::Pressed)
    {
      m_Menu = Menu::None;
    }
    else if (!m_AvailableScenes.IsEmpty())
    {
      if (xiiInputManager::GetInputSlotState(xiiInputSlot_KeyUp) == xiiKeyState::Pressed)
      {
        if (m_uiSelectedScene == 0)
          m_uiSelectedScene = m_AvailableScenes.GetCount() - 1;
        else
          --m_uiSelectedScene;
      }

      if (xiiInputManager::GetInputSlotState(xiiInputSlot_KeyDown) == xiiKeyState::Pressed)
      {
        if (m_uiSelectedScene == m_AvailableScenes.GetCount() - 1)
          m_uiSelectedScene = 0;
        else
          ++m_uiSelectedScene;
      }

      if (xiiInputManager::GetInputSlotState(xiiInputSlot_KeyReturn) == xiiKeyState::Pressed || xiiInputManager::GetInputSlotState(xiiInputSlot_KeyNumpadEnter) == xiiKeyState::Pressed)
      {
        m_sSceneFile = m_AvailableScenes[m_uiSelectedScene];

        if (LoadScene(m_AvailableScenes[m_uiSelectedScene]).Succeeded())
        {
          m_State = State::Ok;
          m_Menu  = Menu::None;
        }
        else
        {
          m_State = State::BadScene;
          m_Menu  = Menu::SceneSelection;
        }
      }

      return true;
    }
  }

  return false;
}
