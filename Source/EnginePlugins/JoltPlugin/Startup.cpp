/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/StringBuilder.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltKinematicActorComponent.h>
#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>
#include <JoltPlugin/Joints/JoltJointComponent.h>
#include <JoltPlugin/Resources/JoltCollisionMeshResource.h>
#include <JoltPlugin/Shapes/JoltPrimitiveShapeComponent.h>
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
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltFixedJointComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltPointJointComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltHingeJointComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltSliderJointComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltDistanceJointComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltCharacterControllerComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltKinematicActorComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltDynamicActorComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltStaticActorComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltBoxShapeComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltSphereShapeComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltCapsuleShapeComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltCylinderShapeComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltConvexHullShapeComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiJoltMeshShapeComponent>());
        pWorld->DeleteModule<xiiJoltPhysicsWorldModule>();
      }
    }

    xiiJoltCollisionMeshResource::CleanupDynamicPluginReferences();
    // Release cached native shapes while Jolt's allocator, material, and registered types are still available.
    xiiResourceManager::FreeAllUnusedResources();
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
