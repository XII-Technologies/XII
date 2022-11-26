#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class xiiPlayerApplication : public xiiGameApplication
{
public:
  typedef xiiGameApplication SUPER;

  xiiPlayerApplication();

protected:
  virtual void      Run_InputUpdate() override;
  virtual xiiResult BeforeCoreSystemsStartup() override;
  virtual void      AfterCoreSystemsStartup() override;
  virtual void      BeforeHighLevelSystemsShutdown() override;

private:
  xiiResult DetermineProjectPath();
  xiiResult DetermineScenePath();

  /// \brief Attempts to load an '.xiiScene' or '.xiiPrefab' file into the main world.
  xiiResult LoadScene(const char* szSceneFile);

  /// \brief Attempts to load an '.xiiObjectGraph' file into the main world.
  xiiResult LoadObjectGraph(const char* szFile);

  /// \brief Finds all available '.xiiScene' files in this project.
  ///
  /// Note that only files that are properly 'transformed', ie have corresponding '.xiiObjectGraph' files, will be loadable.
  void FindAvailableScenes();

  bool DisplayMenu();

  enum class State
  {
    Ok,
    NoProject,
    BadProject,
    NoScene,
    BadScene,
  };

  enum class Menu
  {
    None,
    SceneSelection,
  };

  State m_State = State::Ok;
  Menu  m_Menu  = Menu::None;

  xiiString                  m_sSceneFile;
  xiiUniquePtr<xiiWorld>     m_pWorld;
  bool                       m_bCheckedForScenes = false;
  xiiDynamicArray<xiiString> m_AvailableScenes;
  xiiUInt32                  m_uiSelectedScene = 0;
};
