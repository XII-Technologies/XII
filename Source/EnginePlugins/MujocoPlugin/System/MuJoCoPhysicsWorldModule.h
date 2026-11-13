/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Core/World/WorldModule.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <MujocoPlugin/Resources/MuJoCoModelResource.h>
#include <MujocoPlugin/System/MuJoCoInspection.h>
#include <MujocoPlugin/System/MuJoCoState.h>

/// Owns the compiled MuJoCo model and simulation state for one XII world.
/// Access requires the world's write marker. Clearing the world releases the model and state.
/// State operations fail while native stepping or evaluation is executing, including from native callbacks.
class XII_MUJOCOPLUGIN_DLL xiiMuJoCoPhysicsWorldModule final : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiMuJoCoPhysicsWorldModule, xiiWorldModule);

public:
  explicit xiiMuJoCoPhysicsWorldModule(xiiWorld* pWorld);
  ~xiiMuJoCoPhysicsWorldModule() override;

  /// Loads an MJCF (.xml or .mjcf) or MJB (.mjb) from a loose file. XII paths are resolved to native paths.
  /// A failed load preserves the active simulation.
  [[nodiscard]] xiiResult LoadModel(xiiStringView sPath);

  /// Loads an MJB produced by the same MuJoCo version. The input is borrowed for the duration of the call.
  /// A failed load preserves the active simulation.
  [[nodiscard]] xiiResult LoadModelFromBuffer(xiiArrayPtr<const xiiUInt8> modelBytes);

  /// Restores the active model's initial state and evaluates it at time zero.
  void ResetSimulation();

  /// Sets the maximum number of fixed steps per world update. The limit must be positive.
  [[nodiscard]] xiiResult SetMaxCatchUpSteps(xiiUInt32 uiMaxCatchUpSteps);

  /// Identifiers are scoped to one installed model in one world. Zero indicates that no model is installed.
  xiiUInt64          GetModelGeneration() const { return m_uiModelGeneration; }
  xiiUInt32          GetElementCount(xiiMuJoCoElementType type) const;
  xiiMuJoCoElementId ResolveElement(xiiMuJoCoElementType type, xiiStringView sName) const;
  /// Resolves an authored name in the specified installed model instance.
  xiiMuJoCoElementId ResolveElement(xiiComponentHandle hModel, xiiMuJoCoElementType type, xiiStringView sName) const;
  /// Uses the nearest model component on the object or its ancestors. A lone scene model
  /// also accepts bindings elsewhere in the world.
  xiiMuJoCoElementId ResolveElementForObject(const xiiGameObject* pObject, xiiMuJoCoElementType type, xiiStringView sName) const;
  xiiMuJoCoElementId GetElementId(xiiMuJoCoElementType type, xiiUInt32 uiIndex) const;
  [[nodiscard]] bool GetElementName(xiiMuJoCoElementId id, xiiString& out_sName) const;
  [[nodiscard]] bool GetJointInfo(xiiMuJoCoElementId id, xiiMuJoCoJointInfo& out_info) const;
  [[nodiscard]] bool GetJointPosition(xiiMuJoCoElementId id, xiiArrayPtr<double> out_values) const;
  [[nodiscard]] bool GetJointVelocity(xiiMuJoCoElementId id, xiiArrayPtr<double> out_values) const;

  /// Safe direct state edits, visible immediately to joint/generalized reads. Quaternion positions use W,X,Y,Z.
  /// Output and input spans must match the metadata dimensions exactly.
  [[nodiscard]] xiiResult SetJointPosition(xiiMuJoCoElementId id, xiiArrayPtr<const double> values);
  [[nodiscard]] xiiResult SetJointVelocity(xiiMuJoCoElementId id, xiiArrayPtr<const double> values);
  [[nodiscard]] xiiResult ResetJoint(xiiMuJoCoElementId id);
  [[nodiscard]] bool      GetGeneralizedState(xiiArrayPtr<double> out_positions, xiiArrayPtr<double> out_velocities) const;
  [[nodiscard]] xiiResult SetGeneralizedState(xiiArrayPtr<const double> positions, xiiArrayPtr<const double> velocities);
  xiiUInt32               GetPositionCount() const;
  xiiUInt32               GetVelocityCount() const;

  /// Evaluates direct edits without advancing time. Body-state reads evaluate pending edits as needed.
  void               EvaluateState();
  [[nodiscard]] bool GetBodyState(xiiMuJoCoElementId id, xiiMuJoCoBodyState& out_state);
  double             GetSimulationTime() const;

  [[nodiscard]] bool GetActuatorInfo(xiiMuJoCoElementId id, xiiMuJoCoActuatorInfo& out_info) const;
  [[nodiscard]] bool GetActuatorControlRange(xiiMuJoCoElementId id, xiiUInt32 uiChannel, bool& out_bLimited, double& out_fMin, double& out_fMax) const;
  /// Reads committed native controls. Pending commands become visible here when the next step begins.
  [[nodiscard]] bool GetActuatorControls(xiiMuJoCoElementId id, xiiArrayPtr<double> out_values) const;
  /// Commands are committed before the next step, persist until changed, and honor native control clamping.
  [[nodiscard]] xiiResult SetActuatorControls(xiiMuJoCoElementId id, xiiArrayPtr<const double> values);
  /// Forces describe the most recent dynamics evaluation. Activation arrays follow native dimensions.
  [[nodiscard]] bool      GetActuatorState(xiiMuJoCoElementId id, xiiArrayPtr<double> out_forces, xiiArrayPtr<double> out_activation, xiiArrayPtr<double> out_activationDerivative) const;
  [[nodiscard]] xiiResult ResetActuatorControls(xiiMuJoCoElementId id);
  void                    ResetActuatorControls();

  /// Forces accumulate until one fixed step consumes them, including across paused/substep frames.
  [[nodiscard]] xiiResult AddJointForce(xiiMuJoCoElementId id, xiiArrayPtr<const double> values);
  /// Force and torque use world axes. Force acts at the body's center of mass.
  [[nodiscard]] xiiResult AddBodyForce(xiiMuJoCoElementId id, const xiiVec3& vForce, const xiiVec3& vTorque = xiiVec3::MakeZero());
  /// Force, torque, and application position are in world space; MuJoCo computes the generalized wrench.
  [[nodiscard]] xiiResult AddBodyForceAtPosition(xiiMuJoCoElementId id, const xiiVec3& vForce, const xiiVec3& vPosition, const xiiVec3& vTorque = xiiVec3::MakeZero());
  void                    ClearExternalForces();

  bool               IsStateAccessible() const { return m_pSimulation && !m_bExecuting; }
  [[nodiscard]] bool IsStateCurrent(const xiiMuJoCoStateStamp& stamp) const;
  [[nodiscard]] bool GetSensorInfo(xiiMuJoCoElementId id, xiiMuJoCoSensorInfo& out_info) const;
  /// Refreshes derived state once when required. Disabled native sensors return failure.
  /// Borrowed double views are available with double-precision MuJoCo. Copy access works with either precision.
  /// Copy destinations must match GetSensorInfo().m_uiDimension exactly.
  [[nodiscard]] bool GetSensorReading(xiiMuJoCoElementId id, xiiMuJoCoSensorReading& out_reading);
  [[nodiscard]] bool GetSensorReading(xiiStringView sName, xiiMuJoCoSensorReading& out_reading);
  [[nodiscard]] bool CopySensorReading(xiiMuJoCoElementId id, xiiArrayPtr<double> out_values, xiiMuJoCoStateStamp& out_stamp);
  [[nodiscard]] bool CopySensorReading(xiiStringView sName, xiiArrayPtr<double> out_values, xiiMuJoCoStateStamp& out_stamp);
  [[nodiscard]] bool GetGeneralizedStateView(xiiMuJoCoGeneralizedStateView& out_view) const;

  /// Contacts and sensors share a lazily evaluated state after the latest integrated step or direct edit.
  /// Refreshing derived state may invoke existing native callbacks; no process-global callback is installed here.
  xiiUInt32          GetContactCount();
  [[nodiscard]] bool GetContact(xiiUInt32 uiIndex, xiiMuJoCoContactInfo& out_contact);
  [[nodiscard]] bool GetContacts(xiiUInt32 uiFirst, xiiArrayPtr<xiiMuJoCoContactInfo> out_contacts);
  [[nodiscard]] bool GetSimulationInfo(xiiMuJoCoSimulationInfo& out_info);
  [[nodiscard]] bool GetStepDiagnostics(xiiMuJoCoStepDiagnostics& out_diagnostics) const;
  /// Native statistics slots can differ from detected islands when a solver runs monolithically.
  xiiUInt32          GetSolverStatisticsSlotCount() const;
  [[nodiscard]] bool GetSolverInfo(xiiUInt32 uiSlot, xiiMuJoCoSolverInfo& out_info);
  [[nodiscard]] bool GetSolverIteration(xiiUInt32 uiSlot, xiiUInt32 uiIteration, xiiMuJoCoSolverIteration& out_info);
  xiiUInt32          GetWarningTypeCount() const;
  [[nodiscard]] bool GetWarning(xiiUInt32 uiType, xiiMuJoCoWarningInfo& out_info) const;
  [[nodiscard]] bool GetWarningText(xiiUInt32 uiType, xiiString& out_sText) const;

private:
  friend class xiiMuJoCoModelComponent;
  friend class xiiMuJoCoBodyComponent;
  friend class xiiMuJoCoActuatorComponent;

  void Initialize() override;
  void Deinitialize() override;
  void OnSimulationStarted() override;
  void WorldClear() override;

  void Simulate(const UpdateContext& context);
  void RequestModel(xiiComponentHandle hOwner, const xiiMuJoCoModelResourceHandle& hModel);
  void ReleaseModel(xiiComponentHandle hOwner);
  void UpdateRequestedModels();
  void ClearSimulation();

  void RegisterBody(xiiComponentHandle hComponent, const xiiString& sBodyName);
  void UnregisterBody(xiiComponentHandle hComponent);

  struct BodyBinding
  {
    xiiComponentHandle  m_hComponent;
    xiiGameObjectHandle m_hObject;
    xiiString           m_sBodyName;
    xiiInt32            m_iBodyId = -1;
    xiiUInt32           m_uiDepth = 0;
  };

  void                ResolveBody(BodyBinding& ref_binding) const;
  void                RebuildBodyBindingMap();
  bool                EnsureObservations();
  xiiMuJoCoStateStamp GetStateStamp() const;
  void                ReportNativeWarnings();
  void                SynchronizeBodies();
  bool                IsValidElement(xiiMuJoCoElementId id, xiiMuJoCoElementType type) const;
  void                RegisterActuator(xiiComponentHandle hComponent);
  void                UnregisterActuator(xiiComponentHandle hComponent);
  void                RebindActuators();
  void                ApplyPendingControls();
  void                ApplyExternalForces();

  struct ModelInstance
  {
    xiiComponentHandle            m_hComponent;
    xiiGameObjectHandle           m_hObject;
    xiiMuJoCoModelResourceHandle  m_hResource;
    const xiiMuJoCoModelResource* m_pResource           = nullptr; ///< Kept alive by m_hResource.
    xiiTransform                  m_Placement           = xiiTransform::MakeIdentity();
    xiiTransform                  m_Observed            = xiiTransform::MakeIdentity();
    xiiUInt32                     m_uiAttemptedRevision = 0;
    bool                          m_bObserved           = false;
  };

  struct InstalledInstance
  {
    xiiComponentHandle m_hComponent;
    xiiString          m_sPrefix;
  };

  struct NativeSimulation;
  [[nodiscard]] xiiResult InstallModel(xiiSharedPtr<xiiMuJoCoCompiledModel> pModel, xiiStringView sSource, xiiArrayPtr<const InstalledInstance> instances = {});
  [[nodiscard]] xiiResult ComposeModels(xiiArrayPtr<const xiiSharedPtr<xiiMuJoCoCompiledModel>> models, xiiArrayPtr<InstalledInstance> instances);
  void                    ReadPlacement(ModelInstance& ref_instance, bool bObserveOnly);

  xiiUniquePtr<NativeSimulation>      m_pSimulation;
  xiiDynamicArray<ModelInstance>      m_ModelInstances;
  xiiDynamicArray<BodyBinding>        m_BodyBindings;
  xiiDynamicArray<xiiComponentHandle> m_ActuatorComponents;
  xiiUInt64                           m_uiModelGeneration = 0;
  xiiTime                             m_AccumulatedTime;
  xiiUInt32                           m_uiMaxCatchUpSteps = 4;
  bool                                m_bCompositionDirty = false;
  bool                                m_bSyncBodies       = false;
  bool                                m_bExecuting        = false;
};
