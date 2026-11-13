/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Core/World/Declarations.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/ArrayPtr.h>
#include <MujocoPlugin/System/MuJoCoState.h>

/// Identifies one evaluated state. Reset, stepping, direct state edits, and evaluation invalidate previous stamps.
struct xiiMuJoCoStateStamp
{
  xiiUInt64 m_uiModelGeneration = 0;
  xiiUInt64 m_uiStateRevision   = 0;
  xiiUInt64 m_uiStepIndex       = 0;
  double    m_fTime             = 0.0;
};

/// Type, datatype, stage, and object/reference types retain the vendored MuJoCo enum values.
/// MuJoCo does not encode a general units string; measurement semantics follow sensor type and authored configuration.
struct xiiMuJoCoSensorInfo
{
  xiiMuJoCoElementId m_Id;
  xiiInt32           m_iType          = 0;
  xiiInt32           m_iDataType      = 0;
  xiiInt32           m_iStage         = 0;
  xiiUInt32          m_uiDimension    = 0;
  xiiUInt32          m_uiDataAddress  = 0;
  xiiInt32           m_iObjectType    = 0;
  xiiInt32           m_iObjectId      = -1;
  xiiInt32           m_iReferenceType = 0;
  xiiInt32           m_iReferenceId   = -1;
  xiiMuJoCoElementId m_BodyId; ///< Associated rigid body where the sensorized object has one.
  double             m_fCutoff          = 0.0;
  double             m_fNoise           = 0.0; ///< Native configuration metadata; not a promise that the runtime adds noise.
  double             m_fDelay           = 0.0;
  double             m_fPeriod          = 0.0;
  double             m_fPhase           = 0.0;
  xiiUInt32          m_uiHistorySamples = 0;
  xiiInt32           m_iInterpolation   = 0;
};

/// Borrowed data, valid only while holding the world's write marker and until the next native evaluation or state mutation.
/// Do not retain across stepping, reset, model replacement, or module destruction. Copy data for longer retention.
/// The timestamp is evaluation time; native delay/interval settings can make a sensor measure earlier states.
/// Values preserve native scalar/vector ordering, including W,X,Y,Z quaternions.
struct xiiMuJoCoSensorReading
{
  xiiArrayPtr<const double> m_Values;
  xiiMuJoCoStateStamp       m_Stamp;
};

/// The same borrowing rules as sensor readings apply. These arrays have independent nq and nv lengths.
struct xiiMuJoCoGeneralizedStateView
{
  xiiArrayPtr<const double> m_Positions;
  xiiArrayPtr<const double> m_Velocities;
  xiiMuJoCoStateStamp       m_Stamp;
};

/// A value snapshot of one current contact. Indices only identify contacts within this evaluated state.
/// Frame axes are world-space rows of the native contact frame. Normal points from geometry 0 to geometry 1.
/// Force/torque act on participant 1 at the contact point; participant 0 receives the opposite wrench.
struct xiiMuJoCoContactInfo
{
  xiiMuJoCoStateStamp m_Stamp;
  xiiUInt32           m_uiIndex          = 0;
  xiiVec3             m_vPosition        = xiiVec3::MakeZero();
  xiiVec3             m_vNormal          = xiiVec3::MakeZero();
  xiiVec3             m_vTangent1        = xiiVec3::MakeZero();
  xiiVec3             m_vTangent2        = xiiVec3::MakeZero();
  double              m_fDistance        = 0; ///< Signed distance. Negative values indicate penetration; positive values may lie inside a margin.
  double              m_fInclusionMargin = 0;
  xiiMuJoCoElementId  m_Geometries[2];
  xiiMuJoCoElementId  m_Bodies[2];
  xiiGameObjectHandle m_hBodyObjects[2]; ///< A representative scene binding for each rigid body, if available.
  xiiComponentHandle  m_hBodyComponents[2];
  xiiComponentHandle  m_hModelComponent;
  xiiInt32            m_iFlex[2]           = {-1, -1};
  xiiInt32            m_iElement[2]        = {-1, -1};
  xiiInt32            m_iVertex[2]         = {-1, -1};
  xiiUInt32           m_uiDimension        = 0;
  xiiInt32            m_iExclusion         = 0;
  xiiInt32            m_iConstraintAddress = -1;
  bool                m_bHasForce          = false;
  double              m_ContactForce[6]    = {}; ///< Native contact-frame force XYZ followed by torque XYZ.
  xiiVec3             m_vWorldForce        = xiiVec3::MakeZero();
  xiiVec3             m_vWorldTorque       = xiiVec3::MakeZero();
};

struct xiiMuJoCoSimulationInfo
{
  xiiMuJoCoStateStamp m_Stamp;
  double              m_fTimestep         = 0;
  xiiUInt32           m_uiPositionCount   = 0;
  xiiUInt32           m_uiVelocityCount   = 0;
  xiiUInt32           m_uiActuatorCount   = 0;
  xiiUInt32           m_uiSensorCount     = 0;
  xiiUInt32           m_uiBodyCount       = 0;
  xiiUInt32           m_uiJointCount      = 0;
  xiiUInt32           m_uiContactCount    = 0; ///< Native ncon includes excluded and gap contacts; inspect each contact's exclusion/address.
  xiiUInt32           m_uiConstraintCount = 0; ///< Number of scalar solver constraint rows, nefc.
  xiiUInt32           m_uiIslandCount     = 0;
  xiiInt32            m_iSolverType       = 0;
  bool                m_bSensorsEnabled   = true;
};

/// Captured immediately after mj_step, independent of subsequent observation refreshes.
/// Update totals describe the last invocation of the module's simulation update, including zero-step updates.
struct xiiMuJoCoStepDiagnostics
{
  xiiUInt64 m_uiStepIndex            = 0;
  double    m_fStartTime             = 0;
  double    m_fEndTime               = 0;
  xiiTime   m_StepWallTime           = xiiTime::MakeZero();
  xiiTime   m_UpdateSteppingWallTime = xiiTime::MakeZero();
  xiiUInt32 m_uiStepsLastUpdate      = 0;
  xiiUInt32 m_uiContactCount         = 0;
  xiiUInt32 m_uiConstraintCount      = 0;
  xiiUInt32 m_uiMaxSolverIterations  = 0; ///< Maximum across stored native slots in the final solve, not an integrator-wide iteration sum.
};

struct xiiMuJoCoSolverInfo
{
  xiiMuJoCoStateStamp m_Stamp;
  xiiUInt32           m_uiIterations            = 0;
  xiiUInt32           m_uiStoredIterations      = 0;
  xiiUInt32           m_uiMatrixNonzeros        = 0;
  bool                m_bHasIterationStatistics = false;
};

/// Native scaled solver metrics. Availability and meaning depend on the selected solver.
struct xiiMuJoCoSolverIteration
{
  double   m_fImprovement           = 0;
  double   m_fGradient              = 0;
  double   m_fLineSlope             = 0;
  xiiInt32 m_iActiveConstraints     = 0;
  xiiInt32 m_iConstraintChanges     = 0;
  xiiInt32 m_iLineSearchEvaluations = 0;
  xiiInt32 m_iFactorUpdates         = 0;
};

struct xiiMuJoCoWarningInfo
{
  xiiUInt32 m_uiCount   = 0;
  xiiInt32  m_iLastInfo = 0;
};
