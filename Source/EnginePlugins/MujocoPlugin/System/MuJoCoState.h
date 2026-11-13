/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>

enum class xiiMuJoCoElementType : xiiUInt8
{
  Body,
  Joint,
  Actuator
};

/// A model-scoped identifier. Replacement and world destruction invalidate it.
struct xiiMuJoCoElementId
{
  xiiUInt64            m_uiModelGeneration = 0;
  xiiInt32             m_iIndex            = -1;
  xiiMuJoCoElementType m_Type              = xiiMuJoCoElementType::Body;
};

enum class xiiMuJoCoJointType : xiiUInt8
{
  Free,
  Ball,
  Slide,
  Hinge
};

/// Positions have 7/4/1 coordinates for free/ball/scalar joints; velocities have 6/3/1 degrees of freedom.
/// Limits retain MuJoCo's native meaning: meters for slides, radians for hinges, and angular extent for balls.
struct xiiMuJoCoJointInfo
{
  xiiMuJoCoJointType m_Type              = xiiMuJoCoJointType::Hinge;
  xiiUInt32          m_uiPositionCount   = 0;
  xiiUInt32          m_uiVelocityCount   = 0;
  xiiUInt32          m_uiPositionAddress = 0;
  xiiUInt32          m_uiVelocityAddress = 0;
  bool               m_bLimited          = false;
  double             m_fLowerLimit       = 0;
  double             m_fUpperLimit       = 0;
};

/// Native type and signature values describe the compiled actuator without reinterpreting its semantics.
struct xiiMuJoCoActuatorInfo
{
  xiiInt32  m_iTransmissionType = 0;
  xiiInt32  m_iDynamicsType     = 0;
  xiiInt32  m_iGainType         = 0;
  xiiInt32  m_iBiasType         = 0;
  xiiInt32  m_iControlSignature = 0;
  xiiUInt32 m_uiControlCount    = 0;
  xiiUInt32 m_uiOutputCount     = 0;
  xiiUInt32 m_uiActivationCount = 0;
};

/// World-space pose and velocities at the regular body-frame origin. Angular velocity is radians/second.
struct xiiMuJoCoBodyState
{
  xiiVec3 m_vPosition        = xiiVec3::MakeZero();
  xiiQuat m_qRotation        = xiiQuat::MakeIdentity();
  xiiVec3 m_vLinearVelocity  = xiiVec3::MakeZero();
  xiiVec3 m_vAngularVelocity = xiiVec3::MakeZero();
};
