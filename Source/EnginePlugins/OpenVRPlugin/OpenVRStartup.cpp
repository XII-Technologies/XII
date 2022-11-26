#include <OpenVRPlugin/OpenVRPluginPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <OpenVRPlugin/OpenVRIncludes.h>
#include <OpenVRPlugin/OpenVRSingleton.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(OpenVR, OpenVRPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_STATICLINK_FILE(OpenVRPlugin, OpenVRPlugin_OpenVRStartup);
