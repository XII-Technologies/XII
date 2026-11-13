/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <Core/World/World.h>
#include <EnginePluginJolt/JoltAuthoring.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Lock.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <JoltPlugin/System/JoltPhysicsWorldModule.h>

namespace
{
  xiiCVarInt cvar_JoltDebugWorld("Jolt.Debug.World", -1, xiiCVarFlags::None, "World index targeted by Jolt.Debug.Modes. -1 disables the console override.");
  xiiCVarInt cvar_JoltDebugModes("Jolt.Debug.Modes", 0, xiiCVarFlags::None,
                                 "Add flags: 1 shapes, 2 body states, 4 bounds, 8 centers of mass, 16 velocities, 32 constraints, 64 limits, "
                                 "128 contacts, 256 normals, 512 characters, 1024 statistics, 2048 active bodies only, 4096 body IDs, 8192 solid shapes.");

  void SubmitJoltDebugDraw(const xiiRenderWorldModuleExtractionEvent& event)
  {
    if (event.m_Type != xiiRenderWorldModuleExtractionEvent::Type::BeforeViewExtraction || !xiiGALDevice::HasDefaultDevice())
      return;
    auto* pWorld = event.m_pView->GetWorld();
    if (pWorld == nullptr)
      return;

    XII_LOCK(pWorld->GetWriteMarker());
    if (auto* authoring = pWorld->GetModule<xiiJoltAuthoringWorldModule>())
      authoring->Draw(*event.m_pView);
    auto* pPhysics = pWorld->GetModule<xiiJoltPhysicsWorldModule>();
    if (pPhysics == nullptr)
      return;

    auto modes = pPhysics->GetDebugModes();
    if (cvar_JoltDebugWorld >= 0 && static_cast<xiiUInt32>(static_cast<xiiInt32>(cvar_JoltDebugWorld)) == pWorld->GetIndex())
      modes.SetValue(static_cast<xiiUInt32>(xiiMath::Max(0, static_cast<xiiInt32>(cvar_JoltDebugModes))));
    if (modes.IsNoFlagSet())
      return;

    const xiiDebugRendererContext context(*event.m_pView);
    auto                          geometryModes = modes;
    geometryModes.Remove(xiiJoltDebugMode::Statistics | xiiJoltDebugMode::ActiveBodiesOnly | xiiJoltDebugMode::SolidShapes);
    if (!xiiJoltPhysicsWorldModule::IsDebugDrawingSupported() && !geometryModes.IsNoFlagSet())
      xiiDebugRenderer::DrawInfoText(context, xiiDebugTextPlacement::TopLeft, "Jolt", "Jolt debug geometry requires a Debug or Dev build.", xiiColor::Orange);
    if (const auto* pData = pPhysics->GetDebugDrawData(modes))
    {
      // Batch submissions and bound the temporary conversion storage even for large meshes.
      for (xiiUInt32 uiFirst = 0; uiFirst < pData->m_Lines.GetCount();)
      {
        xiiStaticArray<xiiDebugRendererLine, 256> lines;
        for (; uiFirst < pData->m_Lines.GetCount() && lines.GetCount() < 256; ++uiFirst)
        {
          const auto& line = pData->m_Lines[uiFirst];
          lines.PushBack(xiiDebugRendererLine(line.m_vStart, line.m_vEnd, line.m_Color));
        }
        xiiDebugRenderer::DrawLines(context, lines, xiiColor::White);
      }
      for (xiiUInt32 uiFirst = 0; uiFirst < pData->m_Triangles.GetCount();)
      {
        xiiStaticArray<xiiDebugRendererTriangle, 256> triangles;
        for (; uiFirst < pData->m_Triangles.GetCount() && triangles.GetCount() < 256; ++uiFirst)
        {
          const auto&              source = pData->m_Triangles[uiFirst];
          xiiDebugRendererTriangle triangle(source.m_vPositions[0], source.m_vPositions[1], source.m_vPositions[2]);
          triangle.m_Color = source.m_Color;
          triangles.PushBack(triangle);
        }
        xiiDebugRenderer::DrawSolidTriangles(context, triangles, xiiColor::White);
      }
      for (const auto& label : pData->m_Labels)
        xiiDebugRenderer::Draw3DText(context, label.m_sText.GetView(), label.m_vPosition, label.m_Color, 12);
      if (pData->m_bTruncated)
        xiiDebugRenderer::DrawInfoText(context, xiiDebugTextPlacement::TopLeft, "Jolt", "Debug primitive budget exceeded", xiiColor::Orange);
    }

    if (modes.IsSet(xiiJoltDebugMode::Statistics))
    {
      const auto stats = pPhysics->GetSimulationDiagnostics();
      xiiDebugRenderer::DrawInfoText(context, xiiDebugTextPlacement::TopLeft, "Jolt",
                                     xiiFmt("Jolt world {} ({})", pWorld->GetIndex(), pWorld->GetName()));
      xiiDebugRenderer::DrawInfoText(context, xiiDebugTextPlacement::TopLeft, "Jolt",
                                     xiiFmt("Bodies {}  Active {}  Sleeping dynamic {}  Static {}  Dynamic {}  Kinematic {}",
                                            stats.m_uiBodies, stats.m_uiActiveBodies, stats.m_uiSleepingBodies, stats.m_uiStaticBodies, stats.m_uiDynamicBodies, stats.m_uiKinematicBodies));
      xiiDebugRenderer::DrawInfoText(context, xiiDebugTextPlacement::TopLeft, "Jolt",
                                     xiiFmt("Reported manifolds {}  Joints {}  Characters {}  Scratch reserve {} MiB",
                                            stats.m_uiReportedContactManifolds, stats.m_uiConstraints, stats.m_uiCharacters, stats.m_uiReservedScratchBytes / (1024 * 1024)));
      xiiDebugRenderer::DrawInfoText(context, xiiDebugTextPlacement::TopLeft, "Jolt",
                                     xiiFmt("Fixed step {} ms  Steps {}  Native update {} ms  Errors {}",
                                            xiiArgF(stats.m_FixedStep.GetMilliseconds(), 2), stats.m_uiSteps, xiiArgF(stats.m_LastUpdateDuration.GetMilliseconds(), 2), stats.m_uiUpdateErrors));
    }
  }
} // namespace

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EnginePluginJolt, JoltVisualization)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "JoltPlugin",
    "RenderWorldEvents"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiRenderWorldModule::GetRenderEvents().AddEventHandler(SubmitJoltDebugDraw);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiRenderWorldModule::GetRenderEvents().RemoveEventHandler(SubmitJoltDebugDraw);
    for (xiiUInt32 index = 0; index < xiiWorld::GetWorldCount(); ++index)
      if (auto* world = xiiWorld::GetWorld(index))
      {
        XII_LOCK(world->GetWriteMarker());
        world->DeleteModule<xiiJoltAuthoringWorldModule>();
      }
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_STATICLINK_FILE(EnginePluginJolt, EnginePluginJolt_Implementation_JoltDebugVisualization);
