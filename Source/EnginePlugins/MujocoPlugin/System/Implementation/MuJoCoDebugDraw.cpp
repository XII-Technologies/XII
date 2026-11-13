/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/StringBuilder.h>
#include <MujocoPlugin/System/Implementation/MuJoCoSimulation.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
  constexpr xiiUInt32 MaxGeometry            = 16384;
  constexpr xiiUInt32 MaxLines               = 65536;
  constexpr xiiUInt32 MaxLabels              = 2048;
  constexpr xiiUInt32 MaxContacts            = 2048;
  constexpr xiiUInt32 MaxCachedEdges         = 300000;
  constexpr xiiUInt32 MaxMeshRepresentations = 4096;
  constexpr mjtSize   MaxNativeDecorations   = 32768;
  using DrawData                             = xiiMuJoCoDebugDrawData;

  bool ValidSettings(const xiiMuJoCoDebugSettings& settings)
  {
    return xiiMath::IsFinite(settings.m_fAxisLength) && settings.m_fAxisLength > 0 &&
      xiiMath::IsFinite(settings.m_fForceScale) && settings.m_fForceScale >= 0 &&
      xiiMath::IsFinite(settings.m_fPlaneExtent) && settings.m_fPlaneExtent > 0;
  }

  template <typename Number>
  xiiVec3 DebugVector(const Number* pValues)
  {
    return xiiVec3(static_cast<float>(pValues[0]), static_cast<float>(pValues[1]), static_cast<float>(pValues[2]));
  }

  template <typename Number>
  xiiTransform DebugPose(const Number* pPosition, const Number* pMatrix)
  {
    mjtNum matrix[9], quaternion[4];
    for (int i = 0; i < 9; ++i)
      matrix[i] = pMatrix[i];
    mju_mat2Quat(quaternion, matrix);
    return xiiTransform(DebugVector(pPosition), xiiQuat(static_cast<float>(quaternion[1]), static_cast<float>(quaternion[2]), static_cast<float>(quaternion[3]), static_cast<float>(quaternion[0])));
  }

  void Line(DrawData& data, const xiiVec3& start, const xiiVec3& end, const xiiColor& color)
  {
    if (!start.IsValid() || !end.IsValid())
      return;
    if (data.m_Lines.GetCount() >= MaxLines)
    {
      data.m_bTruncated = true;
      return;
    }
    data.m_Lines.PushBack({start, end, color});
  }

  void Marker(DrawData& data, const xiiVec3& position, float fSize, const xiiColor& color)
  {
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      auto axis         = xiiVec3::MakeZero();
      axis.GetData()[i] = fSize;
      Line(data, position - axis, position + axis, color);
    }
  }

  void Arrow(DrawData& data, const xiiVec3& start, const xiiVec3& end, const xiiColor& color)
  {
    auto        direction = end - start;
    const float fLength   = direction.GetLength();
    if (!direction.IsValid() || !xiiMath::IsFinite(fLength) || fLength <= xiiMath::SmallEpsilon<float>())
      return;
    const float fSize = xiiMath::Min(0.06f, fLength * 0.2f);
    direction /= fLength;
    const auto side = direction.GetOrthogonalVector().GetNormalized() * (fSize * 0.4f);
    Line(data, start, end, color);
    Line(data, end, end - direction * fSize + side, color);
    Line(data, end, end - direction * fSize - side, color);
  }

  void Frame(DrawData& data, const xiiTransform& pose, float fSize)
  {
    Arrow(data, pose.m_vPosition, pose.TransformPosition(xiiVec3(fSize, 0, 0)), xiiColor::Red);
    Arrow(data, pose.m_vPosition, pose.TransformPosition(xiiVec3(0, fSize, 0)), xiiColor::Green);
    Arrow(data, pose.m_vPosition, pose.TransformPosition(xiiVec3(0, 0, fSize)), xiiColor::Blue);
  }

  void Label(DrawData& data, const xiiVec3* pPosition, xiiStringView text, const xiiColor& color)
  {
    if (pPosition && !pPosition->IsValid())
      return;
    if (data.m_Labels.GetCount() + data.m_NonSpatialLabels.GetCount() >= MaxLabels)
    {
      data.m_bTruncated = true;
      return;
    }
    auto& labels = pPosition ? data.m_Labels : data.m_NonSpatialLabels;
    labels.PushBack({pPosition ? *pPosition : xiiVec3::MakeZero(), color, text});
  }

  const char* ElementName(const mjModel& model, mjtObj type, int iId)
  {
    const char* szName = mj_id2name(&model, type, iId);
    return szName ? szName : "<unnamed>";
  }


  const char* JointCoordinates(xiiMuJoCoJointType type)
  {
    switch (type)
    {
      case xiiMuJoCoJointType::Free:
        return "XYZ(m), WXYZ";
      case xiiMuJoCoJointType::Ball:
        return "WXYZ";
      case xiiMuJoCoJointType::Slide:
        return "m";
      case xiiMuJoCoJointType::Hinge:
        return "rad";
    }
    return "native";
  }

  bool ObjectPose(const mjModel& model, const mjData& state, int iType, int iId, xiiTransform& out_pose)
  {
    if (iId < 0)
      return false;
    switch (iType)
    {
      case mjOBJ_BODY:
        if (iId >= model.nbody)
          return false;
        out_pose = DebugPose(state.xipos + 3 * iId, state.ximat + 9 * iId);
        return true;
      case mjOBJ_XBODY:
        if (iId >= model.nbody)
          return false;
        out_pose = DebugPose(state.xpos + 3 * iId, state.xmat + 9 * iId);
        return true;
      case mjOBJ_GEOM:
        if (iId >= model.ngeom)
          return false;
        out_pose = DebugPose(state.geom_xpos + 3 * iId, state.geom_xmat + 9 * iId);
        return true;
      case mjOBJ_SITE:
        if (iId >= model.nsite)
          return false;
        out_pose = DebugPose(state.site_xpos + 3 * iId, state.site_xmat + 9 * iId);
        return true;
      case mjOBJ_JOINT:
        if (iId >= model.njnt)
          return false;
        out_pose = DebugPose(state.xanchor + 3 * iId, state.xmat + 9 * model.jnt_bodyid[iId]);
        return true;
      case mjOBJ_CAMERA:
        if (iId >= model.ncam)
          return false;
        out_pose = DebugPose(state.cam_xpos + 3 * iId, state.cam_xmat + 9 * iId);
        return true;
      default:
        return false;
    }
  }

  void AppendValues(xiiStringBuilder& text, xiiArrayPtr<const double> values, xiiUInt32 uiMaximumValues = 4)
  {
    text.Append("[");
    const auto uiCount = xiiMath::Min(values.GetCount(), uiMaximumValues);
    for (xiiUInt32 i = 0; i < uiCount; ++i)
      text.AppendFormat("{}{}", i ? ", " : "", xiiArgF(values[i], 3));
    if (values.GetCount() > uiCount)
      text.AppendFormat(", ... ({} values)", values.GetCount());
    text.Append("]");
  }

  bool GeometryType(int iType, DrawData::GeometryType& out_type)
  {
    switch (iType)
    {
      case mjGEOM_BOX:
        out_type = DrawData::GeometryType::Box;
        return true;
      case mjGEOM_SPHERE:
        out_type = DrawData::GeometryType::Sphere;
        return true;
      case mjGEOM_CAPSULE:
        out_type = DrawData::GeometryType::Capsule;
        return true;
      case mjGEOM_CYLINDER:
        out_type = DrawData::GeometryType::Cylinder;
        return true;
      case mjGEOM_ELLIPSOID:
        out_type = DrawData::GeometryType::Ellipsoid;
        return true;
      case mjGEOM_PLANE:
        out_type = DrawData::GeometryType::Plane;
        return true;
      case mjGEOM_MESH:
        out_type = DrawData::GeometryType::Mesh;
        return true;
      default:
        return false;
    }
  }

  void AddNativeDecoration(DrawData& data, const mjvGeom& native)
  {
    const auto pose  = DebugPose(native.pos, native.mat);
    const auto color = xiiColor(native.rgba[0], native.rgba[1], native.rgba[2], native.rgba[3]);
    if (native.type == mjGEOM_ARROW || native.type == mjGEOM_ARROW1 || native.type == mjGEOM_ARROW2)
    {
      const auto end = pose.TransformPosition(xiiVec3(0, 0, native.size[2]));
      Arrow(data, pose.m_vPosition, end, color);
      if (native.type == mjGEOM_ARROW2)
        Arrow(data, end, pose.m_vPosition, color);
    }
    else
    {
      DrawData::Geometry geometry;
      if (!GeometryType(native.type, geometry.m_Type))
      {
        data.m_bUnsupportedGeometry = true;
        return;
      }
      if (data.m_Geometry.GetCount() >= MaxGeometry)
      {
        data.m_bTruncated = true;
        return;
      }
      geometry.m_Transform = pose;
      geometry.m_vSize     = DebugVector(native.size);
      geometry.m_Color     = color;
      data.m_Geometry.PushBack(std::move(geometry));
    }
  }
} // namespace

xiiResult xiiMuJoCoPhysicsWorldModule::SetDebugSettings(const xiiMuJoCoDebugSettings& settings)
{
  if (!ValidSettings(settings) || m_bExecuting)
    return XII_FAILURE;
  m_DebugSettings = settings;
  return XII_SUCCESS;
}

const xiiMuJoCoDebugDrawData* xiiMuJoCoPhysicsWorldModule::GetDebugDrawData(const xiiMuJoCoDebugSettings& settings)
{
  if (!ValidSettings(settings) || !IsStateAccessible() || settings.m_Modes.IsNoFlagSet())
    return nullptr;
  m_RequestedDebugSettings = settings;
  m_bDebugRequested        = true;
  if (!m_pSimulation->m_pDebug)
    return nullptr;
  const auto& data = m_pSimulation->m_pDebug->m_Data;
  return data.m_Settings == settings && IsStateCurrent(data.m_Stamp) ? &data : nullptr;
}

void xiiMuJoCoPhysicsWorldModule::PrepareDebugDraw(const UpdateContext&)
{
  const auto settings = std::exchange(m_bDebugRequested, false) ? m_RequestedDebugSettings : m_DebugSettings;
  if (settings.m_Modes.IsNoFlagSet() || !IsStateAccessible())
    return;
  EvaluateState();
  const auto observations = xiiMuJoCoDebugMode::Actuators | xiiMuJoCoDebugMode::Sensors | xiiMuJoCoDebugMode::Contacts |
    xiiMuJoCoDebugMode::ContactNormals | xiiMuJoCoDebugMode::ContactForces | xiiMuJoCoDebugMode::ContactFrames | xiiMuJoCoDebugMode::Statistics;
  if (settings.m_Modes.IsAnySet(observations) && !EnsureObservations())
    return;
  // Derived-state evaluation belongs to the simulation module, before final scene extraction.
  SynchronizeBodies();
  auto& simulation = *m_pSimulation;
  if (!simulation.m_pDebug)
    simulation.m_pDebug = XII_NEW(GetAllocator(), NativeSimulation::DebugState);
  auto&      debug  = *simulation.m_pDebug;
  auto&      output = debug.m_Data;
  const auto stamp  = GetStateStamp();
  if (!GetStepDiagnostics(output.m_Step))
    return;
  if (output.m_Settings == settings && IsStateCurrent(output.m_Stamp))
    return;

  XII_PROFILE_SCOPE("MuJoCo Debug Snapshot");
  output.m_Geometry.Clear();
  output.m_Lines.Clear();
  output.m_Labels.Clear();
  output.m_NonSpatialLabels.Clear();
  output.m_bTruncated           = false;
  output.m_bUnsupportedGeometry = false;
  output.m_Settings             = settings;
  output.m_Stamp                = stamp;
  output.m_Simulation           = {};
  if (settings.m_Modes.IsSet(xiiMuJoCoDebugMode::Statistics) && !GetSimulationInfo(output.m_Simulation))
    return;

  const auto&          model = *simulation.m_pModel->GetModel();
  const auto&          state = *simulation.m_pData;
  const auto           modes = settings.m_Modes;
  xiiMuJoCoElementInfo selected;
  const bool           hasSelection = GetElementInfo(settings.m_SelectedElement, selected);
  if (debug.m_PhysicalGeometry.IsEmpty() && modes.IsAnySet(xiiMuJoCoDebugMode::CollisionGeometry | xiiMuJoCoDebugMode::VisualGeometry | xiiMuJoCoDebugMode::Bounds))
  {
    debug.m_PhysicalGeometry.SetCount(static_cast<xiiUInt32>(model.ngeom), 0);
    for (xiiUInt32 i = 0; i < debug.m_PhysicalGeometry.GetCount(); ++i)
      debug.m_PhysicalGeometry[i] = model.geom_contype[i] != 0 || model.geom_conaffinity[i] != 0;
    for (mjtSize i = 0; i < model.npair; ++i)
      for (int id : {model.pair_geom1[i], model.pair_geom2[i]})
        if (id >= 0 && id < model.ngeom)
          debug.m_PhysicalGeometry[static_cast<xiiUInt32>(id)] = 1;
  }

  // Collision meshes use MuJoCo's convex-hull graph. Visual meshes retain authored faces.
  const auto meshEdges = [&debug, &model, &output](int iMesh, bool bHull) -> xiiUInt32 {
    if (iMesh < 0 || iMesh >= model.nmesh)
    {
      output.m_bUnsupportedGeometry = true;
      return xiiInvalidIndex;
    }
    const xiiUInt32 uiKey = static_cast<xiiUInt32>(iMesh) * 2 + (bHull ? 1U : 0U);
    if (const auto* pCached = debug.m_MeshLookup.GetValue(uiKey))
      return *pCached;
    if (iMesh < 0 || iMesh >= model.nmesh || output.m_Meshes.GetCount() >= MaxMeshRepresentations)
    {
      output.m_bTruncated = true;
      return xiiInvalidIndex;
    }
    const auto uiIndex = output.m_Meshes.GetCount();
    auto&      mesh    = output.m_Meshes.ExpandAndGetRef();
    debug.m_MeshLookup.Insert(uiKey, uiIndex);
    const int* pFaces = model.mesh_face + 3 * model.mesh_faceadr[iMesh];
    mjtSize    iFaces = model.mesh_facenum[iMesh];
    if (bHull)
    {
      const mjtSize iGraph = model.mesh_graphadr[iMesh];
      if (iGraph < 0 || iGraph + 2 > model.nmeshgraph)
      {
        output.m_bUnsupportedGeometry = true;
        mesh.m_bUnsupported           = true;
        return uiIndex;
      }
      const int*    pGraph    = model.mesh_graph + iGraph;
      const mjtSize iVertices = pGraph[0];
      iFaces                  = pGraph[1];
      if (iVertices < 0 || iFaces < 0 || iGraph + 2 + 3 * iVertices + 6 * iFaces > model.nmeshgraph)
      {
        output.m_bUnsupportedGeometry = true;
        mesh.m_bUnsupported           = true;
        return uiIndex;
      }
      pFaces = pGraph + 2 + 3 * iVertices + 3 * iFaces;
    }
    xiiHashSet<xiiUInt64> edges;
    const auto*           pVertices = model.mesh_vert + 3 * model.mesh_vertadr[iMesh];
    for (mjtSize i = 0; i < iFaces; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        const int a = pFaces[3 * i + j];
        const int b = pFaces[3 * i + (j + 1) % 3];
        if (a < 0 || b < 0 || a >= model.mesh_vertnum[iMesh] || b >= model.mesh_vertnum[iMesh])
        {
          mesh.m_bTruncated = true;
          continue;
        }
        const xiiUInt64 key = (static_cast<xiiUInt64>(xiiMath::Min(a, b)) << 32) | static_cast<xiiUInt32>(xiiMath::Max(a, b));
        if (edges.Contains(key))
          continue;
        if (debug.m_uiCachedEdges >= MaxCachedEdges)
        {
          mesh.m_bTruncated = true;
          return uiIndex;
        }
        edges.Insert(key);
        mesh.m_Edges.PushBack({DebugVector(pVertices + 3 * a), DebugVector(pVertices + 3 * b), xiiColor::White});
        ++debug.m_uiCachedEdges;
      }
    }
    return uiIndex;
  };

  if (modes.IsAnySet(xiiMuJoCoDebugMode::CollisionGeometry | xiiMuJoCoDebugMode::VisualGeometry | xiiMuJoCoDebugMode::Bounds))
    for (mjtSize i = 0; i < model.ngeom; ++i)
    {
      if (output.m_Geometry.GetCount() >= MaxGeometry)
      {
        output.m_bTruncated = true;
        break;
      }
      const bool bPhysical          = debug.m_PhysicalGeometry[static_cast<xiiUInt32>(i)] != 0;
      const bool bCollisionSelected = bPhysical && modes.IsSet(xiiMuJoCoDebugMode::CollisionGeometry);
      const auto pose               = DebugPose(state.geom_xpos + 3 * i, state.geom_xmat + 9 * i);
      for (int iRepresentation = 0; iRepresentation < 2; ++iRepresentation)
      {
        const bool bHull = iRepresentation == 0;
        if ((bHull && !bCollisionSelected) || (!bHull && !modes.IsSet(xiiMuJoCoDebugMode::VisualGeometry)) ||
            (!bHull && bCollisionSelected && model.geom_type[i] != mjGEOM_MESH))
          continue;
        if (output.m_Geometry.GetCount() >= MaxGeometry)
        {
          output.m_bTruncated = true;
          break;
        }
        DrawData::Geometry geometry;
        if (!GeometryType(model.geom_type[i], geometry.m_Type))
          output.m_bUnsupportedGeometry = true;
        else
        {
          geometry.m_Transform       = pose;
          geometry.m_vSize           = DebugVector(model.geom_size + 3 * i);
          geometry.m_Color           = hasSelection &&
              ((selected.m_Id.m_Type == xiiMuJoCoElementType::Geometry && selected.m_Id.m_iIndex == i) ||
               (selected.m_Id.m_Type != xiiMuJoCoElementType::Geometry && selected.m_iBody == model.geom_bodyid[i])) ?
            xiiColor::Yellow :
            bHull ? xiiColor::Aquamarine :
                    xiiColor::LightGray;
          geometry.m_GeometryId      = GetElementId(xiiMuJoCoElementType::Geometry, static_cast<xiiUInt32>(i));
          geometry.m_hModelComponent = simulation.m_ElementOwners[static_cast<xiiUInt32>(xiiMuJoCoElementType::Geometry)].IsEmpty() ? xiiComponentHandle{} : simulation.m_ElementOwners[static_cast<xiiUInt32>(xiiMuJoCoElementType::Geometry)][static_cast<xiiUInt32>(i)];
          if (geometry.m_Type == DrawData::GeometryType::Mesh)
          {
            geometry.m_uiMesh = meshEdges(model.geom_dataid[i], bHull);
            if (geometry.m_uiMesh != xiiInvalidIndex)
            {
              output.m_bTruncated |= output.m_Meshes[geometry.m_uiMesh].m_bTruncated;
              output.m_bUnsupportedGeometry |= output.m_Meshes[geometry.m_uiMesh].m_bUnsupported;
            }
          }
          if (geometry.m_Type != DrawData::GeometryType::Mesh || geometry.m_uiMesh != xiiInvalidIndex)
            output.m_Geometry.PushBack(std::move(geometry));
        }
      }
      if (modes.IsSet(xiiMuJoCoDebugMode::Bounds))
      {
        DrawData::Geometry bounds;
        bounds.m_Type                  = DrawData::GeometryType::Bounds;
        bounds.m_Transform             = pose;
        bounds.m_Transform.m_vPosition = pose.TransformPosition(DebugVector(model.geom_aabb + 6 * i));
        bounds.m_vSize                 = DebugVector(model.geom_aabb + 6 * i + 3);
        bounds.m_Color                 = xiiColor::Yellow;
        if (output.m_Geometry.GetCount() < MaxGeometry)
          output.m_Geometry.PushBack(std::move(bounds));
      }
    }

  if (modes.IsAnySet(xiiMuJoCoDebugMode::BodyFrames | xiiMuJoCoDebugMode::CentersOfMass | xiiMuJoCoDebugMode::Hierarchy))
    for (mjtSize i = 1; i < model.nbody; ++i)
    {
      const auto position = DebugVector(state.xpos + 3 * i);
      if (modes.IsSet(xiiMuJoCoDebugMode::BodyFrames))
        Frame(output, DebugPose(state.xpos + 3 * i, state.xmat + 9 * i), settings.m_fAxisLength);
      if (modes.IsSet(xiiMuJoCoDebugMode::CentersOfMass))
        Marker(output, DebugVector(state.xipos + 3 * i), settings.m_fAxisLength * 0.15f, xiiColor::Yellow);
      if (modes.IsSet(xiiMuJoCoDebugMode::Hierarchy))
        Line(output, position, DebugVector(state.xpos + 3 * model.body_parentid[i]), xiiColor::SlateGray);
      if (output.m_Lines.GetCount() >= MaxLines)
      {
        output.m_bTruncated = true;
        break;
      }
    }

  if (modes.IsSet(xiiMuJoCoDebugMode::Inertia))
    for (mjtSize i = 1; i < model.nbody; ++i)
    {
      if (model.body_mass[i] <= 0)
        continue;
      if (output.m_Geometry.GetCount() >= MaxGeometry)
      {
        output.m_bTruncated = true;
        break;
      }
      DrawData::Geometry geometry;
      geometry.m_Type      = DrawData::GeometryType::Box;
      geometry.m_Transform = DebugPose(state.xipos + 3 * i, state.ximat + 9 * i);
      const auto* inertia  = model.body_inertia + 3 * i;
      for (int j = 0; j < 3; ++j)
      {
        const double squared          = 1.5 / model.body_mass[i] * (inertia[(j + 1) % 3] + inertia[(j + 2) % 3] - inertia[j]);
        geometry.m_vSize.GetData()[j] = static_cast<float>(xiiMath::Sqrt(xiiMath::Max(0.0, squared)));
      }
      geometry.m_Color = xiiColor::Purple;
      output.m_Geometry.PushBack(std::move(geometry));
    }

  if (modes.IsSet(xiiMuJoCoDebugMode::Joints))
  {
    const mjtSize iDecorations = model.njnt;
    if (iDecorations > MaxNativeDecorations)
      output.m_bTruncated = true;
    else
    {
      if (!debug.m_Scene.geoms)
        mjv_makeScene(nullptr, &debug.m_Scene, static_cast<int>(xiiMath::Max<mjtSize>(1, iDecorations)));
      else if (debug.m_Scene.maxgeom < iDecorations)
        mjv_makeScene(nullptr, &debug.m_Scene, static_cast<int>(iDecorations));
      mjvOption option;
      mjv_defaultOption(&option);
      std::fill_n(option.flags, mjNVISFLAG, mjtByte{0});
      std::fill_n(option.geomgroup, mjNGROUP, mjtByte{0});
      std::fill_n(option.sitegroup, mjNGROUP, mjtByte{0});
      std::fill_n(option.tendongroup, mjNGROUP, mjtByte{0});
      option.flags[mjVIS_STATIC] = 1;
      option.flags[mjVIS_JOINT]  = modes.IsSet(xiiMuJoCoDebugMode::Joints);
      debug.m_Scene.ngeom        = 0;
      // The selected native decoration paths only read mjData. No camera, lights, skin,
      // plugin visualization callback, or OpenGL renderer is involved.
      m_bExecuting = true;
      mjv_addGeoms(&model, simulation.m_pData.get(), &option, nullptr, mjCAT_DECOR, &debug.m_Scene);
      m_bExecuting = false;
      for (int i = 0; i < debug.m_Scene.ngeom; ++i)
        AddNativeDecoration(output, debug.m_Scene.geoms[i]);
    }
  }

  if (modes.IsAnySet(xiiMuJoCoDebugMode::Joints | xiiMuJoCoDebugMode::JointLimits))
    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(model.njnt); ++i)
    {
      if (output.m_Labels.GetCount() >= MaxLabels)
      {
        output.m_bTruncated = true;
        break;
      }
      const auto         id = GetElementId(xiiMuJoCoElementType::Joint, i);
      xiiMuJoCoJointInfo info;
      if (!GetJointInfo(id, info))
        continue;
      const auto       pose = DebugPose(state.xanchor + 3 * i, state.xmat + 9 * model.jnt_bodyid[i]);
      const auto       axis = DebugVector(state.xaxis + 3 * i);
      xiiStringBuilder text;
      text.SetFormat("{}  q({})", ElementName(model, mjOBJ_JOINT, i), JointCoordinates(info.m_Type));
      double positions[7] = {};
      if (GetJointPosition(id, xiiArrayPtr<double>(positions, info.m_uiPositionCount)))
        AppendValues(text, xiiArrayPtr<const double>(positions, info.m_uiPositionCount), 7);
      if (modes.IsSet(xiiMuJoCoDebugMode::Joints))
        Frame(output, pose, settings.m_fAxisLength * 0.6f);
      if (modes.IsSet(xiiMuJoCoDebugMode::JointLimits) && info.m_bLimited)
      {
        text.AppendFormat("  limits [{}, {}] {}", xiiArgF(info.m_fLowerLimit, 3), xiiArgF(info.m_fUpperLimit, 3),
                          info.m_Type == xiiMuJoCoJointType::Slide ? "m" : "rad");
        if (info.m_Type == xiiMuJoCoJointType::Slide)
        {
          // xanchor is evaluated before this slide joint's displacement.
          const auto lower = pose.m_vPosition + axis * static_cast<float>(info.m_fLowerLimit - model.qpos0[info.m_uiPositionAddress]);
          const auto upper = pose.m_vPosition + axis * static_cast<float>(info.m_fUpperLimit - model.qpos0[info.m_uiPositionAddress]);
          Line(output, lower, upper, xiiColor::Purple);
          Marker(output, lower, 0.04f, xiiColor::Purple);
          Marker(output, upper, 0.04f, xiiColor::Purple);
        }
        else if (info.m_Type == xiiMuJoCoJointType::Hinge && model.body_jntnum[model.jnt_bodyid[i]] == 1)
        {
          const auto  radial   = pose.m_qRotation * DebugVector(model.jnt_axis + 3 * i).GetOrthogonalVector().GetNormalized();
          const float fCurrent = static_cast<float>(positions[0] - model.qpos0[info.m_uiPositionAddress]);
          auto        previous = pose.m_vPosition;
          for (int j = 0; j <= 32; ++j)
          {
            const float angle = static_cast<float>(info.m_fLowerLimit + (info.m_fUpperLimit - info.m_fLowerLimit) * j / 32.0) -
              static_cast<float>(model.qpos0[info.m_uiPositionAddress]) - fCurrent;
            const auto point = pose.m_vPosition + xiiQuat::MakeFromAxisAndAngle(axis, xiiAngle::MakeFromRadian(angle)) * radial * settings.m_fAxisLength;
            Line(output, previous, point, xiiColor::Purple);
            previous = point;
          }
          Line(output, previous, pose.m_vPosition, xiiColor::Purple);
        }
      }
      Label(output, &pose.m_vPosition, text, xiiColor::White);
    }

  if (modes.IsAnySet(xiiMuJoCoDebugMode::Contacts | xiiMuJoCoDebugMode::ContactNormals | xiiMuJoCoDebugMode::ContactForces | xiiMuJoCoDebugMode::ContactFrames))
  {
    const auto uiCount = GetContactCount();
    output.m_bTruncated |= uiCount > MaxContacts;
    for (xiiUInt32 i = 0; i < xiiMath::Min(uiCount, MaxContacts); ++i)
    {
      xiiMuJoCoContactInfo contact;
      if (!GetContact(i, contact))
        continue;
      const auto&      position = contact.m_vPosition;
      const auto       color    = contact.m_fDistance < 0 ? xiiColor::OrangeRed : xiiColor::Aquamarine;
      xiiStringBuilder text;
      if (modes.IsSet(xiiMuJoCoDebugMode::Contacts))
      {
        Marker(output, position, 0.025f, color);
        Line(output, position, position + contact.m_vNormal * static_cast<float>(contact.m_fDistance), color);
        text.SetFormat("Contact {}  geoms {}/{}  distance {} m", i, contact.m_Geometries[0].m_iIndex, contact.m_Geometries[1].m_iIndex, xiiArgF(contact.m_fDistance, 4));
      }
      if (modes.IsSet(xiiMuJoCoDebugMode::ContactNormals))
        Arrow(output, position, position + contact.m_vNormal * settings.m_fAxisLength, xiiColor::Yellow);
      if (modes.IsSet(xiiMuJoCoDebugMode::ContactFrames))
      {
        Line(output, position, position + contact.m_vNormal * settings.m_fAxisLength, xiiColor::Red);
        Line(output, position, position + contact.m_vTangent1 * settings.m_fAxisLength, xiiColor::Green);
        Line(output, position, position + contact.m_vTangent2 * settings.m_fAxisLength, xiiColor::Blue);
      }
      if (modes.IsSet(xiiMuJoCoDebugMode::ContactForces) && contact.m_bHasForce)
      {
        Arrow(output, position, position + contact.m_vWorldForce * settings.m_fForceScale, xiiColor::Magenta);
        Arrow(output, position, position + contact.m_vWorldTorque * settings.m_fForceScale, xiiColor::Cyan);
        if (text.IsEmpty())
          text.SetFormat("Contact {}", i);
        text.AppendFormat("  |F| {} N (magenta)  |T| {} N m (cyan)",
                          xiiArgF(contact.m_vWorldForce.GetLength(), 3), xiiArgF(contact.m_vWorldTorque.GetLength(), 3));
      }
      if (!text.IsEmpty())
        Label(output, &position, text, color);
    }
  }

  if (modes.IsSet(xiiMuJoCoDebugMode::Actuators))
    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(model.nactuator); ++i)
    {
      if (output.m_Labels.GetCount() + output.m_NonSpatialLabels.GetCount() >= MaxLabels)
      {
        output.m_bTruncated = true;
        break;
      }
      const auto            id = GetElementId(xiiMuJoCoElementType::Actuator, i);
      xiiMuJoCoActuatorInfo info;
      if (!GetActuatorInfo(id, info))
        continue;
      debug.m_Controls.SetCount(info.m_uiControlCount);
      debug.m_Forces.SetCount(info.m_uiOutputCount);
      debug.m_Activation.SetCount(info.m_uiActivationCount);
      debug.m_ActivationDerivative.SetCount(info.m_uiActivationCount);
      if (!GetActuatorControls(id, debug.m_Controls) || !GetActuatorState(id, debug.m_Forces, debug.m_Activation, debug.m_ActivationDerivative))
        continue;
      xiiStringBuilder text;
      text.SetFormat("{}  transmission {}  ctrl", ElementName(model, mjOBJ_ACTUATOR, i), info.m_iTransmissionType);
      AppendValues(text, debug.m_Controls);
      if (info.m_iGainType == mjGAIN_SO3)
        text.Append(info.m_iControlSignature == mjCHART_QUAT ? "  orientation target (W,X,Y,Z)" : "  orientation target (exponential map)");
      else if (info.m_iGainType == mjGAIN_FIXED || info.m_iGainType == mjGAIN_AFFINE || info.m_iGainType == mjGAIN_DCMOTOR || info.m_iGainType == mjGAIN_PID)
      {
        if (info.m_iControlSignature & mjINPUT_POS)
          text.Append("  position setpoint input");
        if (info.m_iControlSignature & mjINPUT_VEL)
          text.Append("  velocity setpoint input");
      }
      text.Append("  force");
      AppendValues(text, debug.m_Forces);
      for (xiiUInt32 channel = 0; channel < xiiMath::Min(info.m_uiControlCount, 4U); ++channel)
      {
        bool   bLimited = false;
        double lower = 0, upper = 0;
        if (GetActuatorControlRange(id, channel, bLimited, lower, upper) && bLimited)
          text.AppendFormat("  ctrl[{}] range [{}, {}]", channel, xiiArgF(lower, 3), xiiArgF(upper, 3));
      }
      xiiTransform pose;
      bool         bSpatial   = false;
      const int    iTarget    = model.actuator_trnid[2 * i];
      mjtObj       targetType = mjOBJ_UNKNOWN;
      switch (info.m_iTransmissionType)
      {
        case mjTRN_JOINT:
        case mjTRN_JOINTINPARENT:
          targetType = mjOBJ_JOINT;
          bSpatial   = ObjectPose(model, state, mjOBJ_JOINT, iTarget, pose);
          break;
        case mjTRN_SITE:
        case mjTRN_SLIDERCRANK:
          targetType = mjOBJ_SITE;
          bSpatial   = ObjectPose(model, state, mjOBJ_SITE, iTarget, pose);
          break;
        case mjTRN_BODY:
          targetType = mjOBJ_BODY;
          bSpatial   = ObjectPose(model, state, mjOBJ_XBODY, iTarget, pose);
          break;
        case mjTRN_TENDON:
          targetType = mjOBJ_TENDON;
          if (iTarget >= 0 && iTarget < model.ntendon && state.ten_wrapnum[iTarget] > 0 &&
              state.ten_wrapadr[iTarget] >= 0 && state.ten_wrapadr[iTarget] < 2 * model.nwrap)
          {
            pose             = xiiTransform::MakeIdentity();
            pose.m_vPosition = DebugVector(state.wrap_xpos + 3 * state.ten_wrapadr[iTarget]);
            bSpatial         = true;
          }
          break;
        default:
          break;
      }
      if (targetType != mjOBJ_UNKNOWN && iTarget >= 0)
        text.AppendFormat("  -> {} [{}]", ElementName(model, targetType, iTarget), iTarget);
      if (bSpatial)
        Marker(output, pose.m_vPosition, 0.045f, xiiColor::Gold);
      Label(output, bSpatial ? &pose.m_vPosition : nullptr, text, xiiColor::Gold);
    }

  if (modes.IsSet(xiiMuJoCoDebugMode::Sensors))
    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(model.nsensor); ++i)
    {
      if (output.m_Labels.GetCount() + output.m_NonSpatialLabels.GetCount() >= MaxLabels)
      {
        output.m_bTruncated = true;
        break;
      }
      const auto          id = GetElementId(xiiMuJoCoElementType::Sensor, i);
      xiiMuJoCoSensorInfo info;
      if (!GetSensorInfo(id, info))
        continue;
      xiiTransform     pose, reference;
      const bool       bSpatial = ObjectPose(model, state, info.m_iObjectType, info.m_iObjectId, pose);
      xiiStringBuilder text;
      text.SetFormat("{}  {} (type {})  dim {}  ", ElementName(model, mjOBJ_SENSOR, i), GetSensorSemantics(info.m_iType), info.m_iType, info.m_uiDimension);
      xiiMuJoCoSensorReading reading;
      if (GetSensorReading(id, reading))
        AppendValues(text, reading.m_Values);
      else
      {
        debug.m_SensorValues.SetCount(info.m_uiDimension);
        xiiMuJoCoStateStamp readingStamp;
        if (CopySensorReading(id, debug.m_SensorValues, readingStamp))
          AppendValues(text, debug.m_SensorValues);
        else
          text.Append("unavailable");
      }
      if (bSpatial)
      {
        Marker(output, pose.m_vPosition, 0.04f, xiiColor::Violet);
        Frame(output, pose, settings.m_fAxisLength * 0.6f);
        if (ObjectPose(model, state, info.m_iReferenceType, info.m_iReferenceId, reference))
        {
          Line(output, pose.m_vPosition, reference.m_vPosition, xiiColor::Violet);
          Frame(output, reference, settings.m_fAxisLength * 0.4f);
        }
      }
      Label(output, bSpatial ? &pose.m_vPosition : nullptr, text, xiiColor::Violet);
    }
  if (hasSelection)
  {
    xiiTransform pose;
    if (GetElementPose(selected.m_Id, pose))
    {
      Frame(output, pose, settings.m_fAxisLength);
      Label(output, &pose.m_vPosition, selected.m_sName, xiiColor::Yellow);
      if (selected.m_iParentBody >= 0 && selected.m_iParentBody != selected.m_iBody)
      {
        const auto   parent = GetElementId(xiiMuJoCoElementType::Body, static_cast<xiiUInt32>(selected.m_iParentBody));
        xiiTransform parentPose;
        if (GetElementPose(parent, parentPose))
          Line(output, pose.m_vPosition, parentPose.m_vPosition, xiiColor::Yellow);
      }
      if (selected.m_Id.m_Type == xiiMuJoCoElementType::Joint)
        Arrow(output, pose.m_vPosition, pose.m_vPosition + pose.m_qRotation * selected.m_vAxis * settings.m_fAxisLength, xiiColor::Yellow);
    }
  }
}

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_System_Implementation_MuJoCoDebugDraw);
