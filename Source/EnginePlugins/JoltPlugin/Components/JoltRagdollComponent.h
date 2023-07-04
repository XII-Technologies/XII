#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Math/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>

class xiiJoltUserData;
class xiiSkeletonJoint;
class xiiJoltWorldModule;
class xiiJoltMaterial;
struct xiiMsgRetrieveBoneState;
struct xiiMsgAnimationPoseUpdated;
struct xiiMsgPhysicsAddImpulse;
struct xiiMsgPhysicsAddForce;
struct xiiSkeletonResourceGeometry;

namespace JPH
{
  class Ragdoll;
  class RagdollSettings;
  class Shape;
} // namespace JPH

using xiiSkeletonResourceHandle = xiiTypedResourceHandle<class xiiSkeletonResource>;
using xiiSurfaceResourceHandle  = xiiTypedResourceHandle<class xiiSurfaceResource>;

class XII_JOLTPLUGIN_DLL xiiJoltRagdollComponentManager : public xiiComponentManager<class xiiJoltRagdollComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiJoltRagdollComponentManager(xiiWorld* pWorld);
  ~xiiJoltRagdollComponentManager();

  virtual void Initialize() override;

private:
  friend class xiiJoltWorldModule;
  friend class xiiJoltRagdollComponent;

  void Update(const xiiWorldModule::UpdateContext& context);
};

struct xiiJoltRagdollStartMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    WithBindPose,
    WithNextAnimPose,
    WithCurrentMeshPose,
    Default = WithBindPose
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_JOLTPLUGIN_DLL, xiiJoltRagdollStartMode);

//////////////////////////////////////////////////////////////////////////

class XII_JOLTPLUGIN_DLL xiiJoltRagdollComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltRagdollComponent, xiiComponent, xiiJoltRagdollComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJoltRagdollComponent

public:
  xiiJoltRagdollComponent();
  ~xiiJoltRagdollComponent();

  xiiUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; } // [ scriptable ]

  void OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& ref_msg); // [ msg handler ]
  void OnRetrieveBoneState(xiiMsgRetrieveBoneState& ref_msg) const; // [ msg handler ]

  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]
  void  SetGravityFactor(float fFactor);                      // [ property ]

  bool  m_bSelfCollision = false; // [ property ]
  float m_fStiffness     = 10.0f; // [ property ]
  float m_fMass          = 50.0f; // [ property ]

  void                             SetStartMode(xiiEnum<xiiJoltRagdollStartMode> mode); // [ property ]
  xiiEnum<xiiJoltRagdollStartMode> GetStartMode() const { return m_StartMode; }         // [ property ]

  void OnMsgPhysicsAddImpulse(xiiMsgPhysicsAddImpulse& ref_msg); // [ msg handler ]

  /// \brief Applies an impulse to a specific part of the ragdoll.
  ///
  /// If this is called before the ragdoll becomes active, it is added to the 'initial impulse' (see SetInitialImpulse()).
  /// Once the ragdoll is activated, this initial impulse is applied to the closest body part.
  void OnMsgPhysicsAddForce(xiiMsgPhysicsAddForce& ref_msg); // [ msg handler ]

  /// \brief Call this function BEFORE activating the ragdoll component to specify an impulse that shall be applied to the closest body part when it activates.
  ///
  /// Both position and direction are given in world space.
  ///
  /// This overrides any previously set or accumulated impulses.
  /// If AFTER this call additional impulses are recorded through OnMsgPhysicsAddImpulse(), they are 'added' to the initial impulse.
  ///
  /// Only a single initial impulse is applied after the ragdoll is created.
  /// If multiple impulses are added through OnMsgPhysicsAddImpulse(), their average start position is used to determine the closest body part to apply the impulse on.
  /// Their impulses are accumulated, so the applied impulse can become quite large.
  void SetInitialImpulse(const xiiVec3& vPosition, const xiiVec3& vDirectionAndStrength); // [ scriptable ]

  /// \brief Adds to the existing initial impulse. See SetInitialImpulse().
  void AddInitialImpulse(const xiiVec3& vPosition, const xiiVec3& vDirectionAndStrength); // [ scriptable ]

  /// \brief How much of the owner object's velocity to transfer to the new ragdoll bodies.
  float   m_fOwnerVelocityScale    = 1.0f;                  // [ property ]
  float   m_fCenterVelocity        = 0.0f;                  // [ property ]
  float   m_fCenterAngularVelocity = 0.0f;                  // [ property ]
  xiiVec3 m_vCenterPosition        = xiiVec3::ZeroVector(); // [ property ]


protected:
  struct Limb
  {
    xiiUInt16 m_uiPartIndex = xiiInvalidJointIndex;
  };

  struct LimbConstructionInfo
  {
    xiiTransform m_GlobalTransform;
    xiiUInt16    m_uiJoltPartIndex = xiiInvalidJointIndex;
  };

  void         Update(bool bForce);
  xiiResult    EnsureSkeletonIsKnown();
  void         CreateLimbsFromBindPose();
  void         CreateLimbsFromCurrentMeshPose();
  void         DestroyAllLimbs();
  void         CreateLimbsFromPose(const xiiMsgAnimationPoseUpdated& pose);
  bool         HasCreatedLimbs() const;
  xiiTransform GetRagdollRootTransform() const;
  void         UpdateOwnerPosition();
  void         RetrieveRagdollPose();
  void         SendAnimationPoseMsg();
  void         ConfigureRagdollPart(void* pRagdollSettingsPart, const xiiTransform& globalTransform, xiiUInt8 uiCollisionLayer, xiiJoltWorldModule& worldModule);
  void         CreateAllLimbs(const xiiSkeletonResource& skeletonResource, const xiiMsgAnimationPoseUpdated& pose, xiiJoltWorldModule& worldModule, float fObjectScale);
  void         ComputeLimbModelSpaceTransform(xiiTransform& transform, const xiiMsgAnimationPoseUpdated& pose, xiiUInt32 uiPoseJointIndex);
  void         ComputeLimbGlobalTransform(xiiTransform& transform, const xiiMsgAnimationPoseUpdated& pose, xiiUInt32 uiPoseJointIndex);
  void         CreateLimb(const xiiSkeletonResource& skeletonResource, xiiMap<xiiUInt16, LimbConstructionInfo>& limbConstructionInfos, xiiArrayPtr<const xiiSkeletonResourceGeometry*> geometries, const xiiMsgAnimationPoseUpdated& pose, xiiJoltWorldModule& worldModule, float fObjectScale);
  JPH::Shape*  CreateLimbGeoShape(const LimbConstructionInfo& limbConstructionInfo, const xiiSkeletonResourceGeometry& geo, const xiiJoltMaterial* pJoltMaterial, const xiiQuat& qBoneDirAdjustment, const xiiTransform& skeletonRootTransform, xiiTransform& out_shapeTransform, float fObjectScale);
  void         CreateAllLimbGeoShapes(const LimbConstructionInfo& limbConstructionInfo, xiiArrayPtr<const xiiSkeletonResourceGeometry*> geometries, const xiiSkeletonJoint& thisLimbJoint, const xiiSkeletonResource& skeletonResource, float fObjectScale);
  virtual void ApplyPartInitialVelocity();
  void         ApplyBodyMass();
  void         ApplyInitialImpulse(xiiJoltWorldModule& worldModule, float fMaxImpulse);

  xiiEnum<xiiJoltRagdollStartMode> m_StartMode;             // [ property ]
  float                            m_fGravityFactor = 1.0f; // [ property ]

  xiiSkeletonResourceHandle m_hSkeleton;
  xiiDynamicArray<xiiMat4>  m_CurrentLimbTransforms;

  xiiUInt32        m_uiObjectFilterID    = xiiInvalidIndex;
  xiiUInt32        m_uiJoltUserDataIndex = xiiInvalidIndex;
  xiiJoltUserData* m_pJoltUserData       = nullptr;

  JPH::Ragdoll*         m_pRagdoll         = nullptr;
  JPH::RagdollSettings* m_pRagdollSettings = nullptr;
  xiiDynamicArray<Limb> m_Limbs;
  xiiTransform          m_RootBodyLocalTransform;
  xiiTime               m_ElapsedTimeSinceUpdate = xiiTime::Zero();

  xiiVec3  m_vInitialImpulsePosition  = xiiVec3::ZeroVector();
  xiiVec3  m_vInitialImpulseDirection = xiiVec3::ZeroVector();
  xiiUInt8 m_uiNumInitialImpulses     = 0;

  //////////////////////////////////////////////////////////////////////////

  void SetupLimbJoints(const xiiSkeletonResource* pSkeleton);
  void CreateLimbJoint(const xiiSkeletonJoint& thisJoint, void* pParentBodyDesc, const xiiTransform& parentFrame, void* pThisBodyDesc, const xiiTransform& thisFrame);
};
