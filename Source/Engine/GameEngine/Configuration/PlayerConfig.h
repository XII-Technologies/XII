/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Configuration/Settings.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>

/// A named, project-owned launch description. Extra arguments are tokens, not a shell command.
struct XII_GAMEENGINE_DLL xiiPlayerLaunchConfig
{
  xiiString                  m_sName = "Default";
  xiiString                  m_sProject;                       ///< Empty selects the containing project; relative paths are relative to that project.
  xiiString                  m_sApplication;                   ///< Empty selects Player; relative paths are relative to the engine application directory.
  xiiString                  m_sScene;                         ///< Empty uses the project main scene.
  xiiString                  m_sRole           = "standalone"; ///< standalone, server, client, or server-clients (instance zero is server).
  xiiString                  m_sConnect        = "127.0.0.1";
  xiiUInt32                  m_uiInstances     = 1;
  xiiUInt32                  m_uiBasePort      = 0; ///< Zero chooses a session-specific block; game ports precede telemetry ports.
  bool                       m_bArrangeWindows = true;
  xiiDynamicArray<xiiString> m_Arguments;
  /// One command line per instance. Runtime/window overrides; managed identity and IPC arguments are reserved.
  xiiDynamicArray<xiiString> m_InstanceArguments;

  xiiResult Validate() const;
  /// Validates Player-owned numeric/window options while leaving application/plugin arguments intact.
  static xiiStatus ValidateRuntimeArguments(xiiArrayPtr<const xiiString> arguments);
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiPlayerLaunchConfig);

/// Shared startup policy, exported with RuntimeConfigs. Editor and Player read the same file.
/// This is project configuration, not an editor user preference or a global settings singleton.
class XII_GAMEENGINE_DLL xiiPlayerConfig : public xiiSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPlayerConfig, xiiSettings);

public:
  xiiString                              m_sPreloadCollection;
  xiiDynamicArray<xiiPlayerLaunchConfig> m_LaunchConfigurations;

  xiiResult                    Load(xiiStringView sPath = ":project/RuntimeConfigs/Player.ddl");
  xiiResult                    Save(xiiStringView sPath = ":project/RuntimeConfigs/Player.ddl") const;
  xiiResult                    Validate() const override;
  const xiiPlayerLaunchConfig* FindLaunch(xiiStringView sName) const;
};
