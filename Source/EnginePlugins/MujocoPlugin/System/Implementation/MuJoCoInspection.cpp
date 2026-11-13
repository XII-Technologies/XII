/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Profiling/Profiling.h>
#include <MujocoPlugin/System/Implementation/MuJoCoSimulation.h>

#include <algorithm>
#include <type_traits>

namespace
{
  template <typename Number>
  bool BorrowDoubles(const Number* pValues, xiiUInt32 uiCount, xiiArrayPtr<const double>& out_values)
  {
    if constexpr (std::is_same_v<Number, double>)
    {
      out_values = xiiArrayPtr<const double>(pValues, uiCount);
      return true;
    }
    else
    {
      out_values = {};
      return false;
    }
  }

  xiiVec3 InspectionVector(const mjtNum* pValues)
  {
    return xiiVec3(static_cast<float>(pValues[0]), static_cast<float>(pValues[1]), static_cast<float>(pValues[2]));
  }

  int SensorBody(const mjModel& model, int iType, int iObject)
  {
    if (iObject < 0)
      return -1;
    switch (iType)
    {
      case mjOBJ_BODY:
      case mjOBJ_XBODY:
        return iObject < model.nbody ? iObject : -1;
      case mjOBJ_JOINT:
        return iObject < model.njnt ? model.jnt_bodyid[iObject] : -1;
      case mjOBJ_GEOM:
        return iObject < model.ngeom ? model.geom_bodyid[iObject] : -1;
      case mjOBJ_SITE:
        return iObject < model.nsite ? model.site_bodyid[iObject] : -1;
      case mjOBJ_CAMERA:
        return iObject < model.ncam ? model.cam_bodyid[iObject] : -1;
      default:
        return -1;
    }
  }
} // namespace

xiiMuJoCoStateStamp xiiMuJoCoPhysicsWorldModule::GetStateStamp() const
{
  return {m_uiModelGeneration, m_pSimulation->m_uiStateRevision, m_pSimulation->m_uiStepIndex, m_pSimulation->m_pData->time};
}

bool xiiMuJoCoPhysicsWorldModule::IsStateCurrent(const xiiMuJoCoStateStamp& stamp) const
{
  return IsStateAccessible() && stamp.m_uiModelGeneration == m_uiModelGeneration &&
    stamp.m_uiStateRevision == m_pSimulation->m_uiStateRevision;
}

bool xiiMuJoCoPhysicsWorldModule::EnsureObservations()
{
  if (!IsStateAccessible())
    return false;
  auto& simulation = *m_pSimulation;
  if (!simulation.m_bStateDirty && !simulation.m_bObservationsDirty)
    return true;

  XII_PROFILE_SCOPE("MuJoCo State Evaluation");
  if (simulation.m_bStateDirty)
    std::fill_n(simulation.m_pData->qacc_warmstart, simulation.m_pModel->GetModel()->nv, mjtNum{0});
  m_bExecuting = true;
  mj_forward(simulation.m_pModel->GetModel(), simulation.m_pData.get());
  m_bExecuting                    = false;
  simulation.m_bStateDirty        = false;
  simulation.m_bObservationsDirty = false;
  ++simulation.m_uiStateRevision;
  m_bSyncBodies = true;
  ReportNativeWarnings();
  return true;
}

bool xiiMuJoCoPhysicsWorldModule::GetSensorInfo(xiiMuJoCoElementId id, xiiMuJoCoSensorInfo& out_info) const
{
  if (!IsValidElement(id, xiiMuJoCoElementType::Sensor))
    return false;
  const auto& model           = *m_pSimulation->m_pModel->GetModel();
  const int   i               = id.m_iIndex;
  out_info.m_Id               = id;
  out_info.m_iType            = model.sensor_type[i];
  out_info.m_iDataType        = model.sensor_datatype[i];
  out_info.m_iStage           = model.sensor_needstage[i];
  out_info.m_uiDimension      = static_cast<xiiUInt32>(model.sensor_dim[i]);
  out_info.m_uiDataAddress    = static_cast<xiiUInt32>(model.sensor_adr[i]);
  out_info.m_iObjectType      = model.sensor_objtype[i];
  out_info.m_iObjectId        = model.sensor_objid[i];
  out_info.m_iReferenceType   = model.sensor_reftype[i];
  out_info.m_iReferenceId     = model.sensor_refid[i];
  const int iBody             = SensorBody(model, out_info.m_iObjectType, out_info.m_iObjectId);
  out_info.m_BodyId           = iBody >= 0 ? GetElementId(xiiMuJoCoElementType::Body, static_cast<xiiUInt32>(iBody)) : xiiMuJoCoElementId{};
  out_info.m_fCutoff          = model.sensor_cutoff[i];
  out_info.m_fNoise           = model.sensor_noise[i];
  out_info.m_fDelay           = model.sensor_delay[i];
  out_info.m_fPeriod          = model.sensor_interval[2 * i];
  out_info.m_fPhase           = model.sensor_interval[2 * i + 1];
  out_info.m_uiHistorySamples = static_cast<xiiUInt32>(model.sensor_history[2 * i]);
  out_info.m_iInterpolation   = model.sensor_history[2 * i + 1];
  return true;
}

bool xiiMuJoCoPhysicsWorldModule::GetSensorReading(xiiMuJoCoElementId id, xiiMuJoCoSensorReading& out_reading)
{
  out_reading = {};
  xiiMuJoCoSensorInfo info;
  if (!GetSensorInfo(id, info) || (m_pSimulation->m_pModel->GetModel()->opt.disableflags & mjDSBL_SENSOR) || !EnsureObservations())
    return false;
  const auto* pValues = info.m_uiDimension > 0 ? m_pSimulation->m_pData->sensordata + info.m_uiDataAddress : nullptr;
  if (!BorrowDoubles(pValues, info.m_uiDimension, out_reading.m_Values))
    return false;
  out_reading.m_Stamp = GetStateStamp();
  return true;
}

bool xiiMuJoCoPhysicsWorldModule::GetSensorReading(xiiStringView sName, xiiMuJoCoSensorReading& out_reading)
{
  return GetSensorReading(ResolveElement(xiiMuJoCoElementType::Sensor, sName), out_reading);
}

bool xiiMuJoCoPhysicsWorldModule::CopySensorReading(xiiMuJoCoElementId id, xiiArrayPtr<double> out_values, xiiMuJoCoStateStamp& out_stamp)
{
  xiiMuJoCoSensorInfo info;
  if (!GetSensorInfo(id, info) || out_values.GetCount() != info.m_uiDimension ||
      (m_pSimulation->m_pModel->GetModel()->opt.disableflags & mjDSBL_SENSOR) || !EnsureObservations())
    return false;
  for (xiiUInt32 i = 0; i < info.m_uiDimension; ++i)
    out_values[i] = m_pSimulation->m_pData->sensordata[info.m_uiDataAddress + i];
  out_stamp = GetStateStamp();
  return true;
}

bool xiiMuJoCoPhysicsWorldModule::CopySensorReading(xiiStringView sName, xiiArrayPtr<double> out_values, xiiMuJoCoStateStamp& out_stamp)
{
  return CopySensorReading(ResolveElement(xiiMuJoCoElementType::Sensor, sName), out_values, out_stamp);
}

bool xiiMuJoCoPhysicsWorldModule::GetGeneralizedStateView(xiiMuJoCoGeneralizedStateView& out_view) const
{
  out_view = {};
  if (!IsStateAccessible())
    return false;
  if (!BorrowDoubles(m_pSimulation->m_pData->qpos, GetPositionCount(), out_view.m_Positions) ||
      !BorrowDoubles(m_pSimulation->m_pData->qvel, GetVelocityCount(), out_view.m_Velocities))
    return false;
  out_view.m_Stamp = GetStateStamp();
  return true;
}

void xiiMuJoCoPhysicsWorldModule::RebuildBodyBindingMap()
{
  if (!m_pSimulation)
    return;
  auto& bindings = m_pSimulation->m_BodySceneBindings;
  for (auto& binding : bindings)
    binding = {};
  for (const auto& binding : m_BodyBindings)
  {
    if (binding.m_iBodyId < 0 || static_cast<xiiUInt32>(binding.m_iBodyId) >= bindings.GetCount())
      continue;
    auto& scene = bindings[static_cast<xiiUInt32>(binding.m_iBodyId)];
    if (scene.m_hComponent.IsInvalidated())
    {
      scene.m_hObject    = binding.m_hObject;
      scene.m_hComponent = binding.m_hComponent;
    }
  }
}

xiiUInt32 xiiMuJoCoPhysicsWorldModule::GetContactCount()
{
  return EnsureObservations() ? static_cast<xiiUInt32>(m_pSimulation->m_pData->ncon) : 0;
}

bool xiiMuJoCoPhysicsWorldModule::GetContact(xiiUInt32 uiIndex, xiiMuJoCoContactInfo& out_contact)
{
  if (!EnsureObservations() || uiIndex >= static_cast<xiiUInt32>(m_pSimulation->m_pData->ncon))
    return false;
  const auto& model              = *m_pSimulation->m_pModel->GetModel();
  const auto& data               = *m_pSimulation->m_pData;
  const auto& contact            = data.contact[uiIndex];
  out_contact                    = {};
  out_contact.m_Stamp            = GetStateStamp();
  out_contact.m_uiIndex          = uiIndex;
  out_contact.m_vPosition        = InspectionVector(contact.pos);
  out_contact.m_vNormal          = InspectionVector(contact.frame);
  out_contact.m_vTangent1        = InspectionVector(contact.frame + 3);
  out_contact.m_vTangent2        = InspectionVector(contact.frame + 6);
  out_contact.m_fDistance        = contact.dist;
  out_contact.m_fInclusionMargin = contact.includemargin;
  if (m_pSimulation->m_Instances.GetCount() == 1)
    out_contact.m_hModelComponent = m_pSimulation->m_Instances[0].m_hComponent;
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    out_contact.m_iFlex[i]    = contact.flex[i];
    out_contact.m_iElement[i] = contact.elem[i];
    out_contact.m_iVertex[i]  = contact.vert[i];
    if (contact.flex[i] >= 0 && static_cast<xiiUInt32>(contact.flex[i]) < m_pSimulation->m_FlexOwners.GetCount())
      out_contact.m_hModelComponents[i] = m_pSimulation->m_FlexOwners[contact.flex[i]];
    if (contact.geom[i] < 0 || contact.geom[i] >= model.ngeom)
      continue;
    if (static_cast<xiiUInt32>(contact.geom[i]) < m_pSimulation->m_ElementOwners[static_cast<xiiUInt32>(xiiMuJoCoElementType::Geometry)].GetCount())
      out_contact.m_hModelComponents[i] = m_pSimulation->m_ElementOwners[static_cast<xiiUInt32>(xiiMuJoCoElementType::Geometry)][contact.geom[i]];
    out_contact.m_Geometries[i] = GetElementId(xiiMuJoCoElementType::Geometry, static_cast<xiiUInt32>(contact.geom[i]));
    const int iBody             = model.geom_bodyid[contact.geom[i]];
    if (iBody < 0 || iBody >= model.nbody)
      continue;
    out_contact.m_Bodies[i]          = GetElementId(xiiMuJoCoElementType::Body, static_cast<xiiUInt32>(iBody));
    const auto& binding              = m_pSimulation->m_BodySceneBindings[static_cast<xiiUInt32>(iBody)];
    out_contact.m_hBodyObjects[i]    = binding.m_hObject;
    out_contact.m_hBodyComponents[i] = binding.m_hComponent;
  }
  if (out_contact.m_hModelComponents[0] == out_contact.m_hModelComponents[1])
    out_contact.m_hModelComponent = out_contact.m_hModelComponents[0];
  out_contact.m_uiDimension        = static_cast<xiiUInt32>(contact.dim);
  out_contact.m_iExclusion         = contact.exclude;
  out_contact.m_iConstraintAddress = contact.efc_address;
  out_contact.m_bHasForce          = contact.efc_address >= 0 && contact.efc_address < data.nefc;
  if (out_contact.m_bHasForce)
  {
    mjtNum force[6], worldForce[3], worldTorque[3];
    mj_contactForce(&model, &data, static_cast<int>(uiIndex), force);
    for (xiiUInt32 i = 0; i < 6; ++i)
      out_contact.m_ContactForce[i] = force[i];
    mju_mulMatTVec3(worldForce, contact.frame, force);
    mju_mulMatTVec3(worldTorque, contact.frame, force + 3);
    out_contact.m_vWorldForce  = InspectionVector(worldForce);
    out_contact.m_vWorldTorque = InspectionVector(worldTorque);
  }
  return true;
}

bool xiiMuJoCoPhysicsWorldModule::GetContacts(xiiUInt32 uiFirst, xiiArrayPtr<xiiMuJoCoContactInfo> out_contacts)
{
  if (!EnsureObservations())
    return false;
  const auto uiCount = static_cast<xiiUInt32>(m_pSimulation->m_pData->ncon);
  if (uiFirst > uiCount || out_contacts.GetCount() > uiCount - uiFirst)
    return false;
  for (xiiUInt32 i = 0; i < out_contacts.GetCount(); ++i)
    if (!GetContact(uiFirst + i, out_contacts[i]))
      return false;
  return true;
}

bool xiiMuJoCoPhysicsWorldModule::GetSimulationInfo(xiiMuJoCoSimulationInfo& out_info)
{
  if (!EnsureObservations())
    return false;
  const auto& model            = *m_pSimulation->m_pModel->GetModel();
  const auto& data             = *m_pSimulation->m_pData;
  out_info.m_Stamp             = GetStateStamp();
  out_info.m_fTimestep         = model.opt.timestep;
  out_info.m_uiPositionCount   = static_cast<xiiUInt32>(model.nq);
  out_info.m_uiVelocityCount   = static_cast<xiiUInt32>(model.nv);
  out_info.m_uiActuatorCount   = static_cast<xiiUInt32>(model.nactuator);
  out_info.m_uiSensorCount     = static_cast<xiiUInt32>(model.nsensor);
  out_info.m_uiBodyCount       = static_cast<xiiUInt32>(model.nbody);
  out_info.m_uiJointCount      = static_cast<xiiUInt32>(model.njnt);
  out_info.m_uiContactCount    = static_cast<xiiUInt32>(data.ncon);
  out_info.m_uiConstraintCount = static_cast<xiiUInt32>(data.nefc);
  out_info.m_uiIslandCount     = static_cast<xiiUInt32>(data.nisland);
  out_info.m_iSolverType       = model.opt.solver;
  out_info.m_bSensorsEnabled   = (model.opt.disableflags & mjDSBL_SENSOR) == 0;
  return true;
}

bool xiiMuJoCoPhysicsWorldModule::GetStepDiagnostics(xiiMuJoCoStepDiagnostics& out_diagnostics) const
{
  if (!IsStateAccessible())
    return false;
  out_diagnostics = m_pSimulation->m_StepDiagnostics;
  return true;
}

xiiUInt32 xiiMuJoCoPhysicsWorldModule::GetSolverStatisticsSlotCount() const { return mjNISLAND; }

bool xiiMuJoCoPhysicsWorldModule::GetSolverInfo(xiiUInt32 uiSlot, xiiMuJoCoSolverInfo& out_info)
{
  if (uiSlot >= mjNISLAND || !EnsureObservations())
    return false;
  const auto& data                   = *m_pSimulation->m_pData;
  out_info.m_Stamp                   = GetStateStamp();
  out_info.m_uiIterations            = static_cast<xiiUInt32>(data.solver_niter[uiSlot]);
  out_info.m_uiStoredIterations      = xiiMath::Min(out_info.m_uiIterations, static_cast<xiiUInt32>(mjNSOLVER));
  out_info.m_bHasIterationStatistics = out_info.m_uiIterations > 0;
  // MuJoCo clears iteration counts for unused slots, but their old matrix sizes can remain populated.
  out_info.m_uiMatrixNonzeros = out_info.m_bHasIterationStatistics ? static_cast<xiiUInt32>(data.solver_nnz[uiSlot]) : 0;
  return true;
}

bool xiiMuJoCoPhysicsWorldModule::GetSolverIteration(xiiUInt32 uiSlot, xiiUInt32 uiIteration, xiiMuJoCoSolverIteration& out_info)
{
  xiiMuJoCoSolverInfo info;
  if (!GetSolverInfo(uiSlot, info) || uiIteration >= info.m_uiStoredIterations)
    return false;
  const auto& stat = m_pSimulation->m_pData->solver[uiSlot * mjNSOLVER + uiIteration];
  out_info         = {stat.improvement, stat.gradient, stat.lineslope, stat.nactive, stat.nchange, stat.neval, stat.nupdate};
  return true;
}

xiiUInt32 xiiMuJoCoPhysicsWorldModule::GetWarningTypeCount() const { return mjNWARNING; }

bool xiiMuJoCoPhysicsWorldModule::GetWarning(xiiUInt32 uiType, xiiMuJoCoWarningInfo& out_info) const
{
  if (!IsStateAccessible() || uiType >= mjNWARNING)
    return false;
  const auto& warning = m_pSimulation->m_pData->warning[uiType];
  out_info            = {static_cast<xiiUInt32>(warning.number), warning.lastinfo};
  return true;
}

bool xiiMuJoCoPhysicsWorldModule::GetWarningText(xiiUInt32 uiType, xiiString& out_sText) const
{
  xiiMuJoCoWarningInfo warning;
  if (!GetWarning(uiType, warning))
    return false;
  out_sText = warning.m_uiCount > 0 ? mju_warningText(static_cast<int>(uiType), static_cast<size_t>(warning.m_iLastInfo)) : "";
  return true;
}

void xiiMuJoCoPhysicsWorldModule::ReportNativeWarnings()
{
  for (xiiUInt32 i = 0; i < mjNWARNING; ++i)
  {
    const auto& warning = m_pSimulation->m_pData->warning[i];
    if (warning.number > 0 && !m_pSimulation->m_WarningsReported[i])
    {
      m_pSimulation->m_WarningsReported[i] = true;
      xiiLog::Warning("MuJoCo world '{}': {}", GetWorld()->GetName(), mju_warningText(static_cast<int>(i), static_cast<size_t>(warning.lastinfo)));
    }
  }
}

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_System_Implementation_MuJoCoInspection);
