/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Math/Math.h>
#include <MujocoPlugin/Components/MuJoCoActuatorComponent.h>
#include <MujocoPlugin/System/Implementation/MuJoCoSimulation.h>

#include <algorithm>
#include <cmath>

namespace
{
  int NativeObjectType(xiiMuJoCoElementType type)
  {
    switch (type)
    {
      case xiiMuJoCoElementType::Body:
        return mjOBJ_BODY;
      case xiiMuJoCoElementType::Joint:
        return mjOBJ_JOINT;
      case xiiMuJoCoElementType::Actuator:
        return mjOBJ_ACTUATOR;
    }
    return mjOBJ_UNKNOWN;
  }

  bool ValuesAreFinite(xiiArrayPtr<const double> values)
  {
    for (double value : values)
    {
      if (!xiiMath::IsFinite(value) || !xiiMath::IsFinite(static_cast<mjtNum>(value)))
        return false;
    }
    return true;
  }

  bool CopyValues(const mjtNum* pSource, xiiUInt32 uiCount, xiiArrayPtr<double> out_values)
  {
    if (out_values.GetCount() != uiCount)
      return false;
    for (xiiUInt32 i = 0; i < uiCount; ++i)
      out_values[i] = pSource[i];
    return true;
  }

  void WriteValues(mjtNum* pTarget, xiiArrayPtr<const double> values)
  {
    for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
      pTarget[i] = static_cast<mjtNum>(values[i]);
  }

  int QuaternionOffset(int iType)
  {
    if (iType == mjJNT_FREE)
      return 3;
    return iType == mjJNT_BALL ? 0 : -1;
  }

  // Normalize after scaling by the largest coordinate so finite inputs cannot overflow.
  bool NormalizeQuaternion(xiiArrayPtr<const double> values, int iOffset, mjtNum* pTarget)
  {
    if (iOffset < 0)
      return true;
    double fScale = 0.0;
    for (int i = 0; i < 4; ++i)
      fScale = xiiMath::Max(fScale, xiiMath::Abs(values[iOffset + i]));
    if (fScale == 0.0)
      return false;
    double fLengthSqr = 0.0;
    for (int i = 0; i < 4; ++i)
    {
      const double fValue = values[iOffset + i] / fScale;
      fLengthSqr += fValue * fValue;
    }
    if (pTarget != nullptr)
    {
      const double fLength = std::sqrt(fLengthSqr);
      for (int i = 0; i < 4; ++i)
        pTarget[i] = static_cast<mjtNum>((values[iOffset + i] / fScale) / fLength);
    }
    return true;
  }
} // namespace

xiiUInt32 xiiMuJoCoPhysicsWorldModule::GetElementCount(xiiMuJoCoElementType type) const
{
  if (!m_pSimulation || m_bExecuting)
    return 0;
  const auto& model = *m_pSimulation->m_pModel->GetModel();
  switch (type)
  {
    case xiiMuJoCoElementType::Body:
      return static_cast<xiiUInt32>(model.nbody);
    case xiiMuJoCoElementType::Joint:
      return static_cast<xiiUInt32>(model.njnt);
    case xiiMuJoCoElementType::Actuator:
      return static_cast<xiiUInt32>(model.nactuator);
  }
  return 0;
}

xiiMuJoCoElementId xiiMuJoCoPhysicsWorldModule::GetElementId(xiiMuJoCoElementType type, xiiUInt32 uiIndex) const
{
  if (uiIndex >= GetElementCount(type))
    return {};
  return {m_uiModelGeneration, static_cast<xiiInt32>(uiIndex), type};
}

xiiMuJoCoElementId xiiMuJoCoPhysicsWorldModule::ResolveElement(xiiMuJoCoElementType type, xiiStringView sName) const
{
  if (!m_pSimulation || m_bExecuting || sName.IsEmpty() || NativeObjectType(type) == mjOBJ_UNKNOWN)
    return {};
  const xiiString sTerminatedName(sName);
  const int       iIndex = mj_name2id(m_pSimulation->m_pModel->GetModel(), NativeObjectType(type), sTerminatedName.GetData());
  return iIndex >= 0 ? GetElementId(type, static_cast<xiiUInt32>(iIndex)) : xiiMuJoCoElementId{};
}

bool xiiMuJoCoPhysicsWorldModule::IsValidElement(xiiMuJoCoElementId id, xiiMuJoCoElementType type) const
{
  return id.m_Type == type && id.m_uiModelGeneration != 0 && id.m_uiModelGeneration == m_uiModelGeneration &&
    id.m_iIndex >= 0 && static_cast<xiiUInt32>(id.m_iIndex) < GetElementCount(type);
}

bool xiiMuJoCoPhysicsWorldModule::GetElementName(xiiMuJoCoElementId id, xiiString& out_sName) const
{
  if (!IsValidElement(id, id.m_Type))
    return false;
  const char* szName = mj_id2name(m_pSimulation->m_pModel->GetModel(), NativeObjectType(id.m_Type), id.m_iIndex);
  out_sName          = szName != nullptr ? szName : "";
  return true;
}

bool xiiMuJoCoPhysicsWorldModule::GetJointInfo(xiiMuJoCoElementId id, xiiMuJoCoJointInfo& out_info) const
{
  if (!IsValidElement(id, xiiMuJoCoElementType::Joint))
    return false;
  const auto& model = *m_pSimulation->m_pModel->GetModel();
  const int   iType = model.jnt_type[id.m_iIndex];
  static_assert(static_cast<int>(xiiMuJoCoJointType::Free) == mjJNT_FREE && static_cast<int>(xiiMuJoCoJointType::Ball) == mjJNT_BALL &&
                static_cast<int>(xiiMuJoCoJointType::Slide) == mjJNT_SLIDE && static_cast<int>(xiiMuJoCoJointType::Hinge) == mjJNT_HINGE);
  out_info.m_Type = static_cast<xiiMuJoCoJointType>(iType);
  switch (iType)
  {
    case mjJNT_FREE:
      out_info.m_uiPositionCount = 7;
      out_info.m_uiVelocityCount = 6;
      break;
    case mjJNT_BALL:
      out_info.m_uiPositionCount = 4;
      out_info.m_uiVelocityCount = 3;
      break;
    default:
      out_info.m_uiPositionCount = 1;
      out_info.m_uiVelocityCount = 1;
      break;
  }
  out_info.m_uiPositionAddress = static_cast<xiiUInt32>(model.jnt_qposadr[id.m_iIndex]);
  out_info.m_uiVelocityAddress = static_cast<xiiUInt32>(model.jnt_dofadr[id.m_iIndex]);
  out_info.m_bLimited          = model.jnt_limited[id.m_iIndex] != 0;
  out_info.m_fLowerLimit       = model.jnt_range[2 * id.m_iIndex];
  out_info.m_fUpperLimit       = model.jnt_range[2 * id.m_iIndex + 1];
  return true;
}

bool xiiMuJoCoPhysicsWorldModule::GetJointPosition(xiiMuJoCoElementId id, xiiArrayPtr<double> out_values) const
{
  xiiMuJoCoJointInfo info;
  return GetJointInfo(id, info) && CopyValues(m_pSimulation->m_pData->qpos + info.m_uiPositionAddress, info.m_uiPositionCount, out_values);
}

bool xiiMuJoCoPhysicsWorldModule::GetJointVelocity(xiiMuJoCoElementId id, xiiArrayPtr<double> out_values) const
{
  xiiMuJoCoJointInfo info;
  return GetJointInfo(id, info) && CopyValues(m_pSimulation->m_pData->qvel + info.m_uiVelocityAddress, info.m_uiVelocityCount, out_values);
}

xiiResult xiiMuJoCoPhysicsWorldModule::SetJointPosition(xiiMuJoCoElementId id, xiiArrayPtr<const double> values)
{
  xiiMuJoCoJointInfo info;
  if (!GetJointInfo(id, info) || values.GetCount() != info.m_uiPositionCount || !ValuesAreFinite(values))
    return XII_FAILURE;
  const int iOffset = QuaternionOffset(static_cast<int>(info.m_Type));
  if (!NormalizeQuaternion(values, iOffset, nullptr))
    return XII_FAILURE;
  auto* pPosition = m_pSimulation->m_pData->qpos + info.m_uiPositionAddress;
  WriteValues(pPosition, values);
  NormalizeQuaternion(values, iOffset, iOffset >= 0 ? pPosition + iOffset : nullptr);
  m_pSimulation->m_bStateDirty = true;
  return XII_SUCCESS;
}

xiiResult xiiMuJoCoPhysicsWorldModule::SetJointVelocity(xiiMuJoCoElementId id, xiiArrayPtr<const double> values)
{
  xiiMuJoCoJointInfo info;
  if (!GetJointInfo(id, info) || values.GetCount() != info.m_uiVelocityCount || !ValuesAreFinite(values))
    return XII_FAILURE;
  WriteValues(m_pSimulation->m_pData->qvel + info.m_uiVelocityAddress, values);
  m_pSimulation->m_bStateDirty = true;
  return XII_SUCCESS;
}

xiiResult xiiMuJoCoPhysicsWorldModule::ResetJoint(xiiMuJoCoElementId id)
{
  xiiMuJoCoJointInfo info;
  if (!GetJointInfo(id, info))
    return XII_FAILURE;
  auto& data = *m_pSimulation->m_pData;
  mju_copy(data.qpos + info.m_uiPositionAddress, m_pSimulation->m_pModel->GetModel()->qpos0 + info.m_uiPositionAddress, info.m_uiPositionCount);
  mju_zero(data.qvel + info.m_uiVelocityAddress, info.m_uiVelocityCount);
  mju_zero(data.qfrc_applied + info.m_uiVelocityAddress, info.m_uiVelocityCount);
  m_pSimulation->m_bStateDirty = true;
  return XII_SUCCESS;
}

xiiUInt32 xiiMuJoCoPhysicsWorldModule::GetPositionCount() const
{
  return m_pSimulation && !m_bExecuting ? static_cast<xiiUInt32>(m_pSimulation->m_pModel->GetModel()->nq) : 0;
}

xiiUInt32 xiiMuJoCoPhysicsWorldModule::GetVelocityCount() const
{
  return m_pSimulation && !m_bExecuting ? static_cast<xiiUInt32>(m_pSimulation->m_pModel->GetModel()->nv) : 0;
}

bool xiiMuJoCoPhysicsWorldModule::GetGeneralizedState(xiiArrayPtr<double> out_positions, xiiArrayPtr<double> out_velocities) const
{
  if (!m_pSimulation || m_bExecuting || out_positions.GetCount() != GetPositionCount() || out_velocities.GetCount() != GetVelocityCount())
    return false;
  CopyValues(m_pSimulation->m_pData->qpos, GetPositionCount(), out_positions);
  CopyValues(m_pSimulation->m_pData->qvel, GetVelocityCount(), out_velocities);
  return true;
}

xiiResult xiiMuJoCoPhysicsWorldModule::SetGeneralizedState(xiiArrayPtr<const double> positions, xiiArrayPtr<const double> velocities)
{
  if (!m_pSimulation || m_bExecuting || positions.GetCount() != GetPositionCount() ||
      velocities.GetCount() != GetVelocityCount() || !ValuesAreFinite(positions) || !ValuesAreFinite(velocities))
    return XII_FAILURE;

  const auto& model = *m_pSimulation->m_pModel->GetModel();
  for (int i = 0; i < model.njnt; ++i)
  {
    const int iOffset = QuaternionOffset(model.jnt_type[i]);
    if (iOffset >= 0 && !NormalizeQuaternion(positions, model.jnt_qposadr[i] + iOffset, nullptr))
      return XII_FAILURE;
  }
  auto& data = *m_pSimulation->m_pData;
  WriteValues(data.qpos, positions);
  WriteValues(data.qvel, velocities);
  for (int i = 0; i < model.njnt; ++i)
  {
    const int iOffset = QuaternionOffset(model.jnt_type[i]);
    if (iOffset >= 0)
    {
      const int iAddress = model.jnt_qposadr[i] + iOffset;
      NormalizeQuaternion(positions, iAddress, data.qpos + iAddress);
    }
  }
  m_pSimulation->m_bStateDirty = true;
  return XII_SUCCESS;
}

void xiiMuJoCoPhysicsWorldModule::EvaluateState()
{
  if (!m_pSimulation || m_bExecuting || !m_pSimulation->m_bStateDirty)
    return;
  // Invalidate solver guesses once for a batch of direct edits.
  std::fill_n(m_pSimulation->m_pData->qacc_warmstart, m_pSimulation->m_pModel->GetModel()->nv, mjtNum{0});
  m_bExecuting = true;
  mj_forward(m_pSimulation->m_pModel->GetModel(), m_pSimulation->m_pData.get());
  m_bExecuting                 = false;
  m_pSimulation->m_bStateDirty = false;
  m_bSyncBodies                = true;
}

bool xiiMuJoCoPhysicsWorldModule::GetBodyState(xiiMuJoCoElementId id, xiiMuJoCoBodyState& out_state)
{
  if (!IsValidElement(id, xiiMuJoCoElementType::Body))
    return false;
  EvaluateState();
  const auto& data  = *m_pSimulation->m_pData;
  const auto  iBody = static_cast<size_t>(id.m_iIndex);
  const auto* p     = data.xpos + 3 * iBody;
  const auto* q     = data.xquat + 4 * iBody;
  mjtNum      velocity[6];
  mj_objectVelocity(m_pSimulation->m_pModel->GetModel(), &data, mjOBJ_XBODY, id.m_iIndex, velocity, 0);
  out_state.m_vPosition        = xiiVec3(static_cast<float>(p[0]), static_cast<float>(p[1]), static_cast<float>(p[2]));
  out_state.m_qRotation        = xiiQuat(static_cast<float>(q[1]), static_cast<float>(q[2]), static_cast<float>(q[3]), static_cast<float>(q[0]));
  out_state.m_vAngularVelocity = xiiVec3(static_cast<float>(velocity[0]), static_cast<float>(velocity[1]), static_cast<float>(velocity[2]));
  out_state.m_vLinearVelocity  = xiiVec3(static_cast<float>(velocity[3]), static_cast<float>(velocity[4]), static_cast<float>(velocity[5]));
  return true;
}

double xiiMuJoCoPhysicsWorldModule::GetSimulationTime() const
{
  return m_pSimulation && !m_bExecuting ? m_pSimulation->m_pData->time : 0.0;
}

bool xiiMuJoCoPhysicsWorldModule::GetActuatorInfo(xiiMuJoCoElementId id, xiiMuJoCoActuatorInfo& out_info) const
{
  if (!IsValidElement(id, xiiMuJoCoElementType::Actuator))
    return false;
  const auto& model = *m_pSimulation->m_pModel->GetModel();
  const int   i     = id.m_iIndex;
  out_info          = {model.actuator_trntype[i], model.actuator_dyntype[i], model.actuator_gaintype[i],
                       model.actuator_biastype[i], model.actuator_ctrlspec[i], static_cast<xiiUInt32>(model.actuator_ctrlnum[i]),
                       static_cast<xiiUInt32>(model.actuator_outnum[i]), static_cast<xiiUInt32>(model.actuator_actnum[i])};
  return true;
}

bool xiiMuJoCoPhysicsWorldModule::GetActuatorControlRange(xiiMuJoCoElementId id, xiiUInt32 uiChannel, bool& out_bLimited, double& out_fMin, double& out_fMax) const
{
  xiiMuJoCoActuatorInfo info;
  if (!GetActuatorInfo(id, info) || uiChannel >= info.m_uiControlCount)
    return false;
  const auto& model     = *m_pSimulation->m_pModel->GetModel();
  const auto  uiAddress = static_cast<size_t>(model.actuator_ctrladr[id.m_iIndex]) + uiChannel;
  out_bLimited          = model.actuator_ctrllimited[uiAddress] != 0;
  out_fMin              = model.actuator_ctrlrange[2 * uiAddress];
  out_fMax              = model.actuator_ctrlrange[2 * uiAddress + 1];
  return true;
}

bool xiiMuJoCoPhysicsWorldModule::GetActuatorControls(xiiMuJoCoElementId id, xiiArrayPtr<double> out_values) const
{
  xiiMuJoCoActuatorInfo info;
  if (!GetActuatorInfo(id, info))
    return false;
  if (info.m_uiControlCount == 0)
    return out_values.IsEmpty();
  return CopyValues(m_pSimulation->m_pData->ctrl + m_pSimulation->m_pModel->GetModel()->actuator_ctrladr[id.m_iIndex], info.m_uiControlCount, out_values);
}

xiiResult xiiMuJoCoPhysicsWorldModule::SetActuatorControls(xiiMuJoCoElementId id, xiiArrayPtr<const double> values)
{
  xiiMuJoCoActuatorInfo info;
  if (!GetActuatorInfo(id, info) || values.GetCount() != info.m_uiControlCount || !ValuesAreFinite(values))
    return XII_FAILURE;
  const auto& model    = *m_pSimulation->m_pModel->GetModel();
  const int   iAddress = model.actuator_ctrladr[id.m_iIndex];
  for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
  {
    const auto uiAddress = static_cast<size_t>(iAddress) + i;
    double     fValue    = values[i];
    if (!(model.opt.disableflags & mjDSBL_CLAMPCTRL) && model.actuator_ctrllimited[uiAddress])
      fValue = xiiMath::Clamp(fValue, model.actuator_ctrlrange[2 * uiAddress], model.actuator_ctrlrange[2 * uiAddress + 1]);
    m_pSimulation->m_PendingControls[static_cast<xiiUInt32>(uiAddress)] = static_cast<mjtNum>(fValue);
    m_pSimulation->m_ControlPending[static_cast<xiiUInt32>(uiAddress)]  = 1;
  }
  m_pSimulation->m_bControlsPending = true;
  return XII_SUCCESS;
}

bool xiiMuJoCoPhysicsWorldModule::GetActuatorState(xiiMuJoCoElementId id, xiiArrayPtr<double> out_forces, xiiArrayPtr<double> out_activation, xiiArrayPtr<double> out_activationDerivative) const
{
  xiiMuJoCoActuatorInfo info;
  if (!GetActuatorInfo(id, info) || out_forces.GetCount() != info.m_uiOutputCount ||
      out_activation.GetCount() != info.m_uiActivationCount || out_activationDerivative.GetCount() != info.m_uiActivationCount)
    return false;
  const auto& model = *m_pSimulation->m_pModel->GetModel();
  const auto& data  = *m_pSimulation->m_pData;
  if (info.m_uiOutputCount > 0)
    CopyValues(data.actuator_force + model.actuator_outadr[id.m_iIndex], info.m_uiOutputCount, out_forces);
  if (info.m_uiActivationCount > 0)
  {
    CopyValues(data.act + model.actuator_actadr[id.m_iIndex], info.m_uiActivationCount, out_activation);
    CopyValues(data.act_dot + model.actuator_actadr[id.m_iIndex], info.m_uiActivationCount, out_activationDerivative);
  }
  return true;
}

xiiResult xiiMuJoCoPhysicsWorldModule::ResetActuatorControls(xiiMuJoCoElementId id)
{
  xiiMuJoCoActuatorInfo info;
  if (!GetActuatorInfo(id, info))
    return XII_FAILURE;
  const int iAddress = m_pSimulation->m_pModel->GetModel()->actuator_ctrladr[id.m_iIndex];
  for (xiiUInt32 i = 0; i < info.m_uiControlCount; ++i)
  {
    const auto uiAddress                        = static_cast<xiiUInt32>(iAddress) + i;
    m_pSimulation->m_PendingControls[uiAddress] = m_pSimulation->m_NeutralControls[uiAddress];
    m_pSimulation->m_ControlPending[uiAddress]  = 1;
  }
  m_pSimulation->m_bControlsPending = true;
  return XII_SUCCESS;
}

void xiiMuJoCoPhysicsWorldModule::ResetActuatorControls()
{
  if (!m_pSimulation || m_bExecuting)
    return;
  for (xiiUInt32 i = 0; i < m_pSimulation->m_ControlPending.GetCount(); ++i)
  {
    m_pSimulation->m_PendingControls[i] = m_pSimulation->m_NeutralControls[i];
    m_pSimulation->m_ControlPending[i]  = 1;
  }
  m_pSimulation->m_bControlsPending = true;
}

void xiiMuJoCoPhysicsWorldModule::ApplyPendingControls()
{
  if (!m_pSimulation->m_bControlsPending)
    return;
  for (xiiUInt32 i = 0; i < m_pSimulation->m_ControlPending.GetCount(); ++i)
  {
    if (m_pSimulation->m_ControlPending[i])
    {
      m_pSimulation->m_pData->ctrl[i]    = m_pSimulation->m_PendingControls[i];
      m_pSimulation->m_ControlPending[i] = 0;
    }
  }
  m_pSimulation->m_bControlsPending = false;
}

xiiResult xiiMuJoCoPhysicsWorldModule::AddJointForce(xiiMuJoCoElementId id, xiiArrayPtr<const double> values)
{
  xiiMuJoCoJointInfo info;
  if (!GetJointInfo(id, info) || values.GetCount() != info.m_uiVelocityCount || !ValuesAreFinite(values))
    return XII_FAILURE;
  auto* pForce = m_pSimulation->m_pData->qfrc_applied + info.m_uiVelocityAddress;
  for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
  {
    if (!xiiMath::IsFinite(static_cast<mjtNum>(pForce[i] + values[i])))
      return XII_FAILURE;
  }
  for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
    pForce[i] += static_cast<mjtNum>(values[i]);
  m_pSimulation->m_bExternalForcesApplied = true;
  return XII_SUCCESS;
}

xiiResult xiiMuJoCoPhysicsWorldModule::AddBodyForce(xiiMuJoCoElementId id, const xiiVec3& vForce, const xiiVec3& vTorque)
{
  if (!IsValidElement(id, xiiMuJoCoElementType::Body) || id.m_iIndex == 0 || !vForce.IsValid() || !vTorque.IsValid())
    return XII_FAILURE;
  m_pSimulation->m_BodyForces.PushBack({id.m_iIndex, vForce, vTorque, xiiVec3::MakeZero(), false});
  return XII_SUCCESS;
}

xiiResult xiiMuJoCoPhysicsWorldModule::AddBodyForceAtPosition(xiiMuJoCoElementId id, const xiiVec3& vForce, const xiiVec3& vPosition, const xiiVec3& vTorque)
{
  if (!IsValidElement(id, xiiMuJoCoElementType::Body) || id.m_iIndex == 0 || !vForce.IsValid() || !vTorque.IsValid() || !vPosition.IsValid())
    return XII_FAILURE;
  m_pSimulation->m_BodyForces.PushBack({id.m_iIndex, vForce, vTorque, vPosition, true});
  return XII_SUCCESS;
}

void xiiMuJoCoPhysicsWorldModule::ApplyExternalForces()
{
  if (m_pSimulation->m_BodyForces.IsEmpty())
    return;
  m_pSimulation->m_bExternalForcesApplied = true;
  const auto* pModel                      = m_pSimulation->m_pModel->GetModel();
  auto&       data                        = *m_pSimulation->m_pData;
  for (const auto& command : m_pSimulation->m_BodyForces)
  {
    const mjtNum force[3]  = {command.m_vForce.x, command.m_vForce.y, command.m_vForce.z};
    const mjtNum torque[3] = {command.m_vTorque.x, command.m_vTorque.y, command.m_vTorque.z};
    if (command.m_bAtPosition)
    {
      const mjtNum point[3] = {command.m_vPosition.x, command.m_vPosition.y, command.m_vPosition.z};
      mj_applyFT(pModel, &data, force, torque, point, command.m_iBody, data.qfrc_applied);
    }
    else
    {
      auto* pWrench = data.xfrc_applied + 6 * static_cast<size_t>(command.m_iBody);
      for (int i = 0; i < 3; ++i)
      {
        pWrench[i] += force[i];
        pWrench[3 + i] += torque[i];
      }
    }
  }
  m_pSimulation->m_BodyForces.Clear();
}

void xiiMuJoCoPhysicsWorldModule::ClearExternalForces()
{
  if (!m_pSimulation || m_bExecuting)
    return;
  if (m_pSimulation->m_bExternalForcesApplied)
  {
    const auto* pModel = m_pSimulation->m_pModel->GetModel();
    std::fill_n(m_pSimulation->m_pData->qfrc_applied, pModel->nv, mjtNum{0});
    std::fill_n(m_pSimulation->m_pData->xfrc_applied, 6 * pModel->nbody, mjtNum{0});
    m_pSimulation->m_bExternalForcesApplied = false;
  }
  m_pSimulation->m_BodyForces.Clear();
}

void xiiMuJoCoPhysicsWorldModule::RegisterActuator(xiiComponentHandle hComponent)
{
  if (!m_ActuatorComponents.Contains(hComponent))
    m_ActuatorComponents.PushBack(hComponent);
}

void xiiMuJoCoPhysicsWorldModule::UnregisterActuator(xiiComponentHandle hComponent)
{
  m_ActuatorComponents.RemoveAndSwap(hComponent);
}

void xiiMuJoCoPhysicsWorldModule::RebindActuators()
{
  for (auto hComponent : m_ActuatorComponents)
  {
    xiiMuJoCoActuatorComponent* pComponent = nullptr;
    if (GetWorld()->TryGetComponent(hComponent, pComponent) && pComponent->IsActiveAndSimulating())
      pComponent->ApplyControl();
  }
}

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_System_Implementation_MuJoCoState);
