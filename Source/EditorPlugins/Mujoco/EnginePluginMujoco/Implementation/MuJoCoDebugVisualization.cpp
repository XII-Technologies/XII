/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginMujoco/EnginePluginMujocoPCH.h>

#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Lock.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <MujocoPlugin/System/MuJoCoPhysicsWorldModule.h>

namespace
{
  using DrawData                       = xiiMuJoCoDebugDrawData;
  constexpr xiiUInt32 MaxViewMeshEdges = 300000;
  constexpr xiiUInt32 CircleSegments   = 32;

  xiiCVarInt   cvar_MuJoCoDebugWorld("MuJoCo.Debug.World", -1, xiiCVarFlags::None, "World index targeted by MuJoCo.Debug.Modes. -1 disables the console override.");
  xiiCVarInt   cvar_MuJoCoDebugModes("MuJoCo.Debug.Modes", 0, xiiCVarFlags::None,
                                     "Add flags: 1 collision, 2 visual geometry, 4 body frames, 8 joints, 16 joint limits, 32 centers of mass, 64 inertia, "
                                     "128 actuators, 256 sensors, 512 contacts, 1024 normals, 2048 forces, 4096 statistics, 8192 hierarchy, 16384 contact frames, 32768 bounds, 65536 overlay.");
  xiiCVarFloat cvar_MuJoCoForceScale("MuJoCo.Debug.ForceScale", 0.01f, xiiCVarFlags::None, "Contact force arrow scale in meters per Newton for the selected world.");
  xiiCVarFloat cvar_MuJoCoAxisLength("MuJoCo.Debug.AxisLength", 0.25f, xiiCVarFlags::None, "Diagnostic frame axis length in meters for the selected world.");
  xiiCVarFloat cvar_MuJoCoPlaneExtent("MuJoCo.Debug.PlaneExtent", 10.0f, xiiCVarFlags::None, "Inspection half extent in meters for infinite planes in the selected world.");

  const xiiStaticArray<xiiVec3, CircleSegments>& UnitCircle()
  {
    static const auto circle = [] {
      xiiStaticArray<xiiVec3, CircleSegments> points;
      for (xiiUInt32 i = 0; i < CircleSegments; ++i)
      {
        const float angle = 2.0f * xiiMath::Pi<float>() * static_cast<float>(i) / CircleSegments;
        points.PushBack(xiiVec3(xiiMath::Cos(xiiAngle::MakeFromRadian(angle)), xiiMath::Sin(xiiAngle::MakeFromRadian(angle)), 0));
      }
      return points;
    }();
    return circle;
  }

  void SubmitLines(const xiiDebugRendererContext& context, xiiArrayPtr<const DrawData::Line> source,
                   const xiiTransform& transform, const xiiColor& color, xiiDebugRendererDepthMode depth)
  {
    for (xiiUInt32 i = 0; i < source.GetCount();)
    {
      xiiStaticArray<xiiDebugRendererLine, 256> lines;
      for (; i < source.GetCount() && lines.GetCount() < 256; ++i)
        lines.PushBack(xiiDebugRendererLine(source[i].m_vStart, source[i].m_vEnd, source[i].m_Color));
      xiiDebugRenderer::DrawLines(context, lines, color, transform, depth);
    }
  }

  void DrawPrimitive(const xiiDebugRendererContext& context, const DrawData::Geometry& geometry,
                     const xiiMuJoCoDebugSettings& settings, xiiDebugRendererDepthMode depth)
  {
    if (!geometry.m_Transform.IsValid() || !geometry.m_vSize.IsValid())
      return;
    using Type = DrawData::GeometryType;
    xiiStaticArray<xiiDebugRendererLine, 256> lines;
    const auto                                line = [&lines](const xiiVec3& a, const xiiVec3& b) {
      lines.PushBack(xiiDebugRendererLine(a, b));
    };
    const auto& size   = geometry.m_vSize;
    const auto& circle = UnitCircle();
    switch (geometry.m_Type)
    {
      case Type::Box:
      case Type::Bounds:
      {
        xiiVec3 corners[8];
        for (int i = 0; i < 8; ++i)
          corners[i] = xiiVec3(i & 1 ? size.x : -size.x, i & 2 ? size.y : -size.y, i & 4 ? size.z : -size.z);
        for (int i = 0; i < 8; ++i)
          for (int axis = 0; axis < 3; ++axis)
            if (!(i & (1 << axis)))
              line(corners[i], corners[i | (1 << axis)]);
        break;
      }
      case Type::Sphere:
      case Type::Ellipsoid:
      {
        const auto radius = geometry.m_Type == Type::Sphere ? xiiVec3(size.x) : size;
        for (xiiUInt32 i = 0; i < CircleSegments; ++i)
        {
          const auto& a = circle[i];
          const auto& b = circle[(i + 1) % CircleSegments];
          line(xiiVec3(a.x * radius.x, a.y * radius.y, 0), xiiVec3(b.x * radius.x, b.y * radius.y, 0));
          line(xiiVec3(a.x * radius.x, 0, a.y * radius.z), xiiVec3(b.x * radius.x, 0, b.y * radius.z));
          line(xiiVec3(0, a.x * radius.y, a.y * radius.z), xiiVec3(0, b.x * radius.y, b.y * radius.z));
        }
        break;
      }
      case Type::Capsule:
      case Type::Cylinder:
      {
        const float radius     = size.x;
        const float halfLength = size.y;
        for (xiiUInt32 i = 0; i < CircleSegments; ++i)
        {
          const auto a = circle[i] * radius;
          const auto b = circle[(i + 1) % CircleSegments] * radius;
          line(a + xiiVec3(0, 0, halfLength), b + xiiVec3(0, 0, halfLength));
          line(a - xiiVec3(0, 0, halfLength), b - xiiVec3(0, 0, halfLength));
          if (i % (CircleSegments / 4) == 0)
            line(a - xiiVec3(0, 0, halfLength), a + xiiVec3(0, 0, halfLength));
          if (geometry.m_Type == Type::Capsule)
          {
            // At the equator, choose the same hemisphere for both ends of each arc.
            const float offset = (a.y + b.y) >= 0 ? halfLength : -halfLength;
            line(xiiVec3(a.x, 0, offset + a.y), xiiVec3(b.x, 0, offset + b.y));
            line(xiiVec3(0, a.x, offset + a.y), xiiVec3(0, b.x, offset + b.y));
          }
        }
        break;
      }
      case Type::Plane:
      {
        const float x = size.x > 0 ? size.x : settings.m_fPlaneExtent;
        const float y = size.y > 0 ? size.y : settings.m_fPlaneExtent;
        for (int i = 0; i <= 10; ++i)
        {
          const float fraction = static_cast<float>(i) / 5.0f - 1.0f;
          line(xiiVec3(-x, fraction * y, 0), xiiVec3(x, fraction * y, 0));
          line(xiiVec3(fraction * x, -y, 0), xiiVec3(fraction * x, y, 0));
        }
        break;
      }
      case Type::Mesh:
        return;
    }
    xiiDebugRenderer::DrawLines(context, lines, geometry.m_Color, geometry.m_Transform, depth);
  }

  void SubmitMuJoCoDebugDraw(const xiiRenderWorldModuleExtractionEvent& event)
  {
    if (event.m_Type != xiiRenderWorldModuleExtractionEvent::Type::BeforeViewExtraction || !xiiGALDevice::HasDefaultDevice())
      return;
    auto* pWorld = event.m_pView->GetWorld();
    if (pWorld == nullptr)
      return;
    XII_LOCK(pWorld->GetWriteMarker());
    auto* pPhysics = pWorld->GetModule<xiiMuJoCoPhysicsWorldModule>();
    if (pPhysics == nullptr)
      return;
    auto settings = pPhysics->GetDebugSettings();
    if (cvar_MuJoCoDebugWorld >= 0 && static_cast<xiiUInt32>(static_cast<xiiInt32>(cvar_MuJoCoDebugWorld)) == pWorld->GetIndex())
    {
      settings.m_Modes.SetValue(static_cast<xiiUInt32>(xiiMath::Max(0, static_cast<xiiInt32>(cvar_MuJoCoDebugModes))));
      const float fAxis = cvar_MuJoCoAxisLength, fForce = cvar_MuJoCoForceScale, fPlane = cvar_MuJoCoPlaneExtent;
      if (xiiMath::IsFinite(fAxis) && fAxis > 0)
        settings.m_fAxisLength = fAxis;
      if (xiiMath::IsFinite(fForce) && fForce >= 0)
        settings.m_fForceScale = fForce;
      if (xiiMath::IsFinite(fPlane) && fPlane > 0)
        settings.m_fPlaneExtent = fPlane;
    }
    if (settings.m_Modes.IsNoFlagSet())
      return;
    const auto* pData = pPhysics->GetDebugDrawData(settings);
    if (pData == nullptr)
      return;
    const xiiDebugRendererContext context(*event.m_pView);
    const auto                    depth = settings.m_Modes.IsSet(xiiMuJoCoDebugMode::Overlay) ? xiiDebugRendererDepthMode::Overlay : xiiDebugRendererDepthMode::DepthTested;
    SubmitLines(context, pData->m_Lines, xiiTransform::MakeIdentity(), xiiColor::White, depth);
    xiiUInt32 uiMeshEdges         = 0;
    bool      bMeshBudgetExceeded = false;
    for (const auto& geometry : pData->m_Geometry)
    {
      if (geometry.m_Type != DrawData::GeometryType::Mesh)
        DrawPrimitive(context, geometry, settings, depth);
      else if (geometry.m_uiMesh < pData->m_Meshes.GetCount())
      {
        const auto& edges   = pData->m_Meshes[geometry.m_uiMesh].m_Edges;
        const auto  uiCount = xiiMath::Min(edges.GetCount(), MaxViewMeshEdges - uiMeshEdges);
        SubmitLines(context, edges.GetArrayPtr().GetSubArray(0, uiCount), geometry.m_Transform, geometry.m_Color, depth);
        uiMeshEdges += uiCount;
        bMeshBudgetExceeded |= uiCount < edges.GetCount();
      }
    }
    for (const auto& label : pData->m_Labels)
      xiiDebugRenderer::Draw3DText(context, label.m_sText.GetView(), label.m_vPosition, label.m_Color, 12);
    const auto uiNumericLabels = xiiMath::Min(pData->m_NonSpatialLabels.GetCount(), 32U);
    for (xiiUInt32 i = 0; i < uiNumericLabels; ++i)
      xiiDebugRenderer::DrawInfoText(context, xiiDebugTextPlacement::TopLeft, "MuJoCo.Values", pData->m_NonSpatialLabels[i].m_sText.GetView(), pData->m_NonSpatialLabels[i].m_Color);
    if (pData->m_bTruncated || bMeshBudgetExceeded || uiNumericLabels < pData->m_NonSpatialLabels.GetCount())
      xiiDebugRenderer::DrawInfoText(context, xiiDebugTextPlacement::TopLeft, "MuJoCo", "Visualization budget exceeded", xiiColor::Orange);
    if (pData->m_bUnsupportedGeometry)
      xiiDebugRenderer::DrawInfoText(context, xiiDebugTextPlacement::TopLeft, "MuJoCo", "Some geometry has no supported collision or visual representation", xiiColor::Orange);
    if (settings.m_Modes.IsSet(xiiMuJoCoDebugMode::Statistics))
    {
      const auto& info = pData->m_Simulation;
      const auto& step = pData->m_Step;
      xiiDebugRenderer::DrawInfoText(context, xiiDebugTextPlacement::TopLeft, "MuJoCo",
                                     xiiFmt("MuJoCo world {}  generation {}  step {}  time {} s  timestep {} ms", pWorld->GetIndex(),
                                            info.m_Stamp.m_uiModelGeneration, info.m_Stamp.m_uiStepIndex, xiiArgF(info.m_Stamp.m_fTime, 3), xiiArgF(info.m_fTimestep * 1000.0, 3)));
      xiiDebugRenderer::DrawInfoText(context, xiiDebugTextPlacement::TopLeft, "MuJoCo",
                                     xiiFmt("Bodies {}  Joints {}  nq {}  nv {}  Actuators {}  Sensors {}", info.m_uiBodyCount, info.m_uiJointCount,
                                            info.m_uiPositionCount, info.m_uiVelocityCount, info.m_uiActuatorCount, info.m_uiSensorCount));
      xiiDebugRenderer::DrawInfoText(context, xiiDebugTextPlacement::TopLeft, "MuJoCo",
                                     xiiFmt("Contacts {}  Constraint rows {}  Last step max iterations {}  Steps {}  Native stepping {} ms",
                                            info.m_uiContactCount, info.m_uiConstraintCount, step.m_uiMaxSolverIterations, step.m_uiStepsLastUpdate, xiiArgF(step.m_UpdateSteppingWallTime.GetMilliseconds(), 3)));
    }
  }
} // namespace

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EnginePluginMujoco, MuJoCoVisualization)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "MujocoPlugin",
    "RenderWorldEvents"
  END_SUBSYSTEM_DEPENDENCIES
  ON_CORESYSTEMS_STARTUP
  {
    xiiRenderWorldModule::GetRenderEvents().AddEventHandler(SubmitMuJoCoDebugDraw);
  }
  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiRenderWorldModule::GetRenderEvents().RemoveEventHandler(SubmitMuJoCoDebugDraw);
  }
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_STATICLINK_FILE(EnginePluginMujoco, EnginePluginMujoco_Implementation_MuJoCoDebugVisualization);
