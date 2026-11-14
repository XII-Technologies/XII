/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Core/World/World.h>
#include <MujocoPlugin/Components/MuJoCoBodyComponent.h>
#include <MujocoPlugin/Components/MuJoCoModelComponent.h>
#include <MujocoPlugin/System/Implementation/MuJoCoSimulation.h>

namespace
{
  constexpr mjtObj InspectionTypes[] = {mjOBJ_BODY, mjOBJ_JOINT, mjOBJ_ACTUATOR, mjOBJ_SENSOR, mjOBJ_GEOM, mjOBJ_SITE};
  xiiVec3          ModelInspectionVector(const mjtNum* values)
  {
    return xiiVec3(static_cast<float>(values[0]), static_cast<float>(values[1]), static_cast<float>(values[2]));
  }
  xiiTransform InspectionPose(const mjtNum* position, const mjtNum* matrix)
  {
    mjtNum q[4];
    mju_mat2Quat(q, matrix);
    return xiiTransform(ModelInspectionVector(position), xiiQuat(static_cast<float>(q[1]), static_cast<float>(q[2]), static_cast<float>(q[3]), static_cast<float>(q[0])));
  }
} // namespace

void xiiMuJoCoPhysicsWorldModule::SetScenePreview(bool enabled, bool authoring, bool defer)
{
  if (m_bExecuting)
    return;
  const bool startPreview = enabled && !m_bScenePreview;
  if (m_bScenePreview && !enabled && m_bAuthoringPreview)
  {
    m_ModelInstances.Clear();
    m_BodyBindings.Clear();
    ClearSimulation();
  }
  m_bScenePreview     = enabled;
  m_bAuthoringPreview = enabled && authoring;
  m_bDeferComposition = enabled && defer;
  if (startPreview)
    RefreshPreviewModels();
}

void xiiMuJoCoPhysicsWorldModule::RefreshPreviewModels()
{
  if (!m_bScenePreview)
    return;
  if (auto* manager = GetWorld()->GetModule<xiiMuJoCoModelComponentManager>())
    for (auto it = manager->GetComponents(); it.IsValid(); ++it)
      if (it->IsActiveAndInitialized() && it->GetModel().IsValid())
        RequestModel(it->GetHandle(), it->GetModel());
  if (m_bAuthoringPreview)
    if (auto* pManager = GetWorld()->GetModule<xiiMuJoCoBodyComponentManager>())
      for (auto it = pManager->GetComponents(); it.IsValid(); ++it)
        if (it->IsActiveAndInitialized())
          RegisterBody(it->GetHandle(), it->GetBodyName());
  for (xiiUInt32 i = m_ModelInstances.GetCount(); i > 0; --i)
  {
    const auto&                    instance  = m_ModelInstances[i - 1];
    const xiiMuJoCoModelComponent* component = nullptr;
    if (!GetWorld()->TryGetComponent(instance.m_hComponent, component) || !component->IsActiveAndInitialized() || !component->GetModel().IsValid())
    {
      m_ModelInstances.RemoveAtAndCopy(i - 1);
      m_bCompositionDirty = true;
    }
  }
}

xiiResult xiiMuJoCoPhysicsWorldModule::RequestSingleStep()
{
  if (!IsStateAccessible() || m_bAuthoringPreview || m_bDeferComposition || m_bCompositionDirty ||
      !GetWorld()->GetWorldSimulationEnabled() || !GetWorld()->GetClock().GetPaused())
    return XII_FAILURE;
  m_bSingleStepRequested = true;
  return XII_SUCCESS;
}

bool xiiMuJoCoPhysicsWorldModule::GetModelInstanceInfo(xiiUInt32 index, xiiMuJoCoModelInstanceInfo& result) const
{
  if (m_bExecuting || index >= m_ModelInstances.GetCount())
    return false;
  const auto& instance = m_ModelInstances[index];
  result               = {};
  result.m_hComponent  = instance.m_hComponent;
  result.m_hObject     = instance.m_hObject;
  result.m_sResource   = instance.m_hResource.GetResourceID();
  const auto state     = instance.m_pResource->GetLoadingState();
  result.m_bPending    = state != xiiResourceState::Loaded && state != xiiResourceState::LoadedResourceMissing;
  result.m_bMissing    = state == xiiResourceState::LoadedResourceMissing;
  if (m_pSimulation)
    for (const auto& installed : m_pSimulation->m_Instances)
      if (installed.m_hComponent == instance.m_hComponent)
      {
        result.m_bInstalled = true;
        result.m_sNamespace = installed.m_sPrefix;
        break;
      }
  return true;
}

xiiComponentHandle xiiMuJoCoPhysicsWorldModule::GetElementOwner(xiiMuJoCoElementId id) const
{
  const auto type = static_cast<xiiUInt32>(id.m_Type);
  if (type >= XII_ARRAY_SIZE(InspectionTypes) || !IsValidElement(id, id.m_Type))
    return {};
  const auto& owners = m_pSimulation->m_ElementOwners[type];
  return static_cast<xiiUInt32>(id.m_iIndex) < owners.GetCount() ? owners[id.m_iIndex] : xiiComponentHandle{};
}

bool xiiMuJoCoPhysicsWorldModule::GetElementInfo(xiiMuJoCoElementId id, xiiMuJoCoElementInfo& result) const
{
  if (static_cast<xiiUInt32>(id.m_Type) >= XII_ARRAY_SIZE(InspectionTypes) || !IsValidElement(id, id.m_Type))
    return false;
  result             = {};
  result.m_Id        = id;
  const auto& model  = *m_pSimulation->m_pModel->GetModel();
  const auto  index  = id.m_iIndex;
  result.m_hModel    = GetElementOwner(id);
  const char* name   = mj_id2name(&model, InspectionTypes[static_cast<xiiUInt32>(id.m_Type)], index);
  result.m_sName     = name ? name : "<unnamed>";
  result.m_bBindable = name != nullptr;
  for (const auto& instance : m_pSimulation->m_Instances)
    if (instance.m_hComponent == result.m_hModel && !instance.m_sPrefix.IsEmpty() && result.m_sName.StartsWith(instance.m_sPrefix))
    {
      result.m_sName = name + instance.m_sPrefix.GetElementCount();
      if (result.m_sName.StartsWith("__xii_unnamed_"))
      {
        result.m_sName     = "<unnamed>";
        result.m_bBindable = false;
      }
      break;
    }
  switch (id.m_Type)
  {
    case xiiMuJoCoElementType::Body:
      result.m_iBody       = index;
      result.m_iParentBody = model.body_parentid[index];
      result.m_hBinding    = m_pSimulation->m_BodySceneBindings[index].m_hComponent;
      result.m_hObject     = m_pSimulation->m_BodySceneBindings[index].m_hObject;
      break;
    case xiiMuJoCoElementType::Joint:
    {
      result.m_iBody       = model.jnt_bodyid[index];
      result.m_iNativeType = model.jnt_type[index];
      switch (result.m_iNativeType)
      {
        case mjJNT_FREE:
          result.m_sType = "Free";
          break;
        case mjJNT_BALL:
          result.m_sType = "Ball";
          break;
        case mjJNT_SLIDE:
          result.m_sType = "Slide (m)";
          break;
        case mjJNT_HINGE:
          result.m_sType = "Hinge (rad)";
          break;
      }
      result.m_vAxis = ModelInspectionVector(model.jnt_axis + 3 * index);
      xiiMuJoCoJointInfo info;
      if (!GetJointInfo(id, info))
        return false;
      result.m_uiReferenceCount = info.m_uiPositionCount;
      for (xiiUInt32 i = 0; i < info.m_uiPositionCount; ++i)
        result.m_Reference[i] = model.qpos0[info.m_uiPositionAddress + i];
      result.m_sDescription = "Direct state uses nq coordinates independently of nv velocities. Quaternion order is W,X,Y,Z.";
      break;
    }
    case xiiMuJoCoElementType::Geometry:
      result.m_iBody       = model.geom_bodyid[index];
      result.m_iNativeType = model.geom_type[index];
      break;
    case xiiMuJoCoElementType::Site:
      result.m_iBody       = model.site_bodyid[index];
      result.m_iNativeType = model.site_type[index];
      break;
    case xiiMuJoCoElementType::Actuator:
    {
      result.m_iNativeType = model.actuator_trntype[index];
      result.m_iTarget     = model.actuator_trnid[2 * index];
      mjtObj targetType    = mjOBJ_UNKNOWN;
      if (result.m_iNativeType == mjTRN_JOINT || result.m_iNativeType == mjTRN_JOINTINPARENT)
      {
        result.m_iTargetType = static_cast<xiiInt32>(xiiMuJoCoElementType::Joint);
        targetType           = mjOBJ_JOINT;
        result.m_sType       = result.m_iNativeType == mjTRN_JOINT ? "Joint transmission" : "Joint transmission in parent frame";
        if (result.m_iTarget >= 0 && result.m_iTarget < model.njnt)
          result.m_iBody = model.jnt_bodyid[result.m_iTarget];
      }
      else if (result.m_iNativeType == mjTRN_SITE || result.m_iNativeType == mjTRN_SLIDERCRANK)
      {
        result.m_iTargetType = static_cast<xiiInt32>(xiiMuJoCoElementType::Site);
        targetType           = mjOBJ_SITE;
        result.m_sType       = result.m_iNativeType == mjTRN_SITE ? "Site transmission" : "Slider-crank transmission";
        if (result.m_iTarget >= 0 && result.m_iTarget < model.nsite)
          result.m_iBody = model.site_bodyid[result.m_iTarget];
      }
      else if (result.m_iNativeType == mjTRN_BODY)
      {
        result.m_iTargetType = static_cast<xiiInt32>(xiiMuJoCoElementType::Body);
        result.m_iBody       = result.m_iTarget;
        targetType           = mjOBJ_BODY;
        result.m_sType       = "Body transmission";
      }
      else if (result.m_iNativeType == mjTRN_TENDON)
      {
        targetType     = mjOBJ_TENDON;
        result.m_sType = "Tendon transmission";
      }
      if (targetType != mjOBJ_UNKNOWN && result.m_iTarget >= 0)
        if (const auto* targetName = mj_id2name(&model, targetType, result.m_iTarget))
          result.m_sTarget = targetName;
      result.m_sDescription = "Compiled transmission/gain/bias/dynamics define control semantics; commands are committed before the next step.";
      break;
    }
    case xiiMuJoCoElementType::Sensor:
    {
      xiiMuJoCoSensorInfo info;
      if (!GetSensorInfo(id, info))
        return false;
      result.m_iNativeType  = info.m_iType;
      result.m_iBody        = info.m_BodyId.m_iIndex;
      result.m_iTarget      = info.m_iObjectId;
      result.m_sDescription = GetSensorSemantics(info.m_iType);
      if (info.m_iObjectId >= 0 && info.m_iObjectType != mjOBJ_UNKNOWN)
        if (const auto* targetName = mj_id2name(&model, info.m_iObjectType, info.m_iObjectId))
          result.m_sTarget = targetName;
      break;
    }
  }
  if (result.m_iBody >= 0 && result.m_iBody < model.nbody && id.m_Type != xiiMuJoCoElementType::Body)
    result.m_iParentBody = result.m_iBody;
  return true;
}

bool xiiMuJoCoPhysicsWorldModule::GetElementPose(xiiMuJoCoElementId id, xiiTransform& pose)
{
  xiiMuJoCoElementInfo info;
  if (!GetElementInfo(id, info))
    return false;
  EvaluateState();
  if (!IsStateAccessible())
    return false;
  const auto& data  = *m_pSimulation->m_pData;
  const auto& model = *m_pSimulation->m_pModel->GetModel();
  const auto  index = id.m_iIndex;
  if (id.m_Type == xiiMuJoCoElementType::Geometry)
    pose = InspectionPose(data.geom_xpos + 3 * index, data.geom_xmat + 9 * index);
  else if (id.m_Type == xiiMuJoCoElementType::Site)
    pose = InspectionPose(data.site_xpos + 3 * index, data.site_xmat + 9 * index);
  else
  {
    if (info.m_iBody < 0 || info.m_iBody >= model.nbody)
      return false;
    pose = InspectionPose(data.xpos + 3 * info.m_iBody, data.xmat + 9 * info.m_iBody);
    if (id.m_Type == xiiMuJoCoElementType::Joint)
      pose.m_vPosition = ModelInspectionVector(data.xanchor + 3 * index);
    if (id.m_Type == xiiMuJoCoElementType::Sensor && model.sensor_objtype[index] == mjOBJ_SITE && model.sensor_objid[index] >= 0)
      pose = InspectionPose(data.site_xpos + 3 * model.sensor_objid[index], data.site_xmat + 9 * model.sensor_objid[index]);
  }
  return pose.IsValid();
}

bool xiiMuJoCoPhysicsWorldModule::GetJointAppliedForce(xiiMuJoCoElementId id, xiiArrayPtr<double> values) const
{
  xiiMuJoCoJointInfo info;
  if (!GetJointInfo(id, info) || values.GetCount() != info.m_uiVelocityCount)
    return false;
  for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
    values[i] = m_pSimulation->m_pData->qfrc_applied[info.m_uiVelocityAddress + i];
  return true;
}

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_System_Implementation_MuJoCoModelInspection);
const char* xiiMuJoCoPhysicsWorldModule::GetSensorSemantics(xiiInt32 iType)
{
  switch (iType)
  {
    case mjSENS_TOUCH:
      return "touch (N)";
    case mjSENS_ACCELEROMETER:
      return "local acceleration (m/s^2)";
    case mjSENS_VELOCIMETER:
      return "local velocity (m/s)";
    case mjSENS_GYRO:
      return "local angular velocity (rad/s)";
    case mjSENS_FORCE:
      return "local force (N)";
    case mjSENS_TORQUE:
      return "local torque (N m)";
    case mjSENS_MAGNETOMETER:
      return "local magnetic field";
    case mjSENS_RANGEFINDER:
      return "range (m)";
    case mjSENS_CAMPROJECTION:
      return "camera projection (pixels)";
    case mjSENS_JOINTPOS:
      return "joint position";
    case mjSENS_JOINTVEL:
      return "joint velocity";
    case mjSENS_BALLQUAT:
      return "joint quaternion (W,X,Y,Z)";
    case mjSENS_BALLANGVEL:
      return "joint angular velocity (rad/s)";
    case mjSENS_FRAMEPOS:
      return "frame position (m)";
    case mjSENS_FRAMEQUAT:
      return "frame quaternion (W,X,Y,Z)";
    case mjSENS_FRAMEXAXIS:
      return "frame X axis";
    case mjSENS_FRAMEYAXIS:
      return "frame Y axis";
    case mjSENS_FRAMEZAXIS:
      return "frame Z axis";
    case mjSENS_FRAMELINVEL:
      return "frame linear velocity (m/s)";
    case mjSENS_FRAMEANGVEL:
      return "frame angular velocity (rad/s)";
    case mjSENS_FRAMELINACC:
      return "frame linear acceleration (m/s^2)";
    case mjSENS_FRAMEANGACC:
      return "frame angular acceleration (rad/s^2)";
    case mjSENS_SUBTREECOM:
      return "subtree center of mass (m)";
    case mjSENS_SUBTREELINVEL:
      return "subtree velocity (m/s)";
    case mjSENS_SUBTREEANGMOM:
      return "subtree angular momentum";
    case mjSENS_GEOMDIST:
      return "geometry distance (m)";
    case mjSENS_GEOMNORMAL:
      return "geometry normal";
    case mjSENS_GEOMFROMTO:
      return "geometry segment (m)";
    case mjSENS_CONTACT:
      return "native contact channels";
    case mjSENS_E_POTENTIAL:
      return "potential energy (J)";
    case mjSENS_E_KINETIC:
      return "kinetic energy (J)";
    case mjSENS_CLOCK:
      return "time (s)";
    default:
      return "native sensor channels";
  }
}
