/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/StringBuilder.h>
#include <JoltPlugin/System/JoltPhysicsWorldModule.h>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>

#include <cstdarg>

namespace
{
  JPH::TraceFunction s_PreviousTrace = nullptr;

  void TraceJolt(const char* szFormat, ...)
  {
    va_list arguments;
    va_start(arguments, szFormat);
    xiiStringBuilder sMessage;
    sMessage.SetPrintfArgs(szFormat, arguments);
    va_end(arguments);
    xiiLog::Warning("Jolt: {}", sMessage);
  }

#ifdef JPH_ENABLE_ASSERTS
  JPH::AssertFailedFunction s_PreviousAssert = nullptr;

  bool AssertJolt(const char* szExpression, const char* szMessage, const char* szFile, JPH::uint uiLine)
  {
    return xiiFailedCheck(szFile, uiLine, "Jolt", szExpression, szMessage != nullptr ? szMessage : "");
  }
#endif
} // namespace

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Jolt, JoltPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    JPH::RegisterDefaultAllocator();
    s_PreviousTrace = JPH::Trace;
    JPH::Trace = TraceJolt;
#ifdef JPH_ENABLE_ASSERTS
    s_PreviousAssert = JPH::AssertFailed;
    JPH::AssertFailed = AssertJolt;
#endif
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    // Plugin changes run with world updates quiesced. Detach the modules while their RTTI and code are still loaded.
    for (xiiUInt32 uiIndex = 0; uiIndex < xiiWorld::GetWorldCount(); ++uiIndex)
    {
      if (xiiWorld* pWorld = xiiWorld::GetWorld(uiIndex))
      {
        XII_LOCK(pWorld->GetWriteMarker());
        pWorld->DeleteModule<xiiJoltPhysicsWorldModule>();
      }
    }

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
    JPH::Trace = s_PreviousTrace;
#ifdef JPH_ENABLE_ASSERTS
    JPH::AssertFailed = s_PreviousAssert;
#endif
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Startup);
