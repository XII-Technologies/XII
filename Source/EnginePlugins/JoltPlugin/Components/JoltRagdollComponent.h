#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Math/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct xiiMsgAnimationPoseUpdated;
struct xiiMsgPhysicsAddImpulse;
struct xiiMsgPhysicsAddForce;
struct xiiSkeletonResourceGeometry;
class xiiJoltUserData;
class xiiSkeletonJoint;
struct xiiMsgAnimationPoseProposal;
struct xiiMsgRetrieveBoneState;
class xiiJoltWorldModule;
namespace JPH
{
  class RagdollSettings;
}

namespace JPH
{
  class Ragdoll;
}

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

struct xiiJoltRagdollStart
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    BindPose,
    WaitForPose,
    Wait,
    Default = BindPose
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_JOLTPLUGIN_DLL, xiiJoltRagdollStart);

//////////////////////////////////////////////////////////////////////////

struct XII_JOLTPLUGIN_DLL xiiJoltRagdollConstraint : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltRagdollConstraint, xiiReflectedClass);

  xiiString m_sBone;
  xiiVec3   m_vRelativePosition;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

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

  void OnAnimationPoseProposal(xiiMsgAnimationPoseProposal& ref_msg); // [ msg handler ]
  void OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& ref_msg);   // [ msg handler ]
  void OnRetrieveBoneState(xiiMsgRetrieveBoneState& ref_msg) const;   // [ msg handler ]

  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]
  void  SetGravityFactor(float fFactor);                      // [ property ]

  xiiUInt8 m_uiCollisionLayer = 0;     // [ property ]
  bool     m_bSelfCollision   = false; // [ property ]

  void AddImpulseAtPos(xiiMsgPhysicsAddImpulse& ref_msg); // [ message ]
  void AddForceAtPos(xiiMsgPhysicsAddForce& ref_msg);     // [ message ]

protected:
  struct Limb
  {
    xiiHashedString m_sName;
    // physx::PxRigidBody* m_pPxBody = nullptr;
    void*     m_pBodyDesc   = nullptr;
    xiiUInt16 m_uiPartIndex = 0xFFFFu;
  };

  struct LimbConfig
  {
    xiiTransform m_GlobalTransform;
    void*        m_pBodyDesc   = nullptr;
    xiiUInt16    m_uiPartIndex = 0xFFFFu;
  };

  struct Impulse
  {
    xiiVec3 m_vPos     = xiiVec3::ZeroVector();
    xiiVec3 m_vImpulse = xiiVec3::ZeroVector();
    // physx::PxRigidBody* m_pRigidBody = nullptr;
  };

  float                                     m_fGravityFactor = 1.0f; // [ property ]
  xiiDynamicArray<xiiJoltRagdollConstraint> m_Constraints;           // [ property ]
  xiiEnum<xiiJoltRagdollStart>              m_Start;                 // [ property ]

  Impulse                   m_NextImpulse;
  xiiSkeletonResourceHandle m_hSkeleton;

  bool                     m_bLimbsSetup         = false;
  float                    m_fStiffness          = 10.0f;
  xiiUInt32                m_uiObjectFilterID    = xiiInvalidIndex;
  xiiUInt32                m_uiJoltUserDataIndex = xiiInvalidIndex;
  xiiJoltUserData*         m_pJoltUserData       = nullptr;
  xiiDynamicArray<Limb>    m_Limbs;
  JPH::Ragdoll*            m_pRagdoll         = nullptr;
  JPH::RagdollSettings*    m_pRagdollSettings = nullptr;
  xiiTransform             m_RootBodyLocalTransform;
  xiiDynamicArray<xiiMat4> m_LimbPoses;

  void         Update();
  void         CreateConstraints();
  void         SetupLimbsFromBindPose();
  bool         EnsureSkeletonIsKnown();
  virtual void ClearPhysicsObjects();
  virtual void SetupJoltBasics(xiiJoltWorldModule* pPxModule);
  virtual void FinishSetupLimbs();
  void         SetupLimbs(const xiiMsgAnimationPoseUpdated& pose);
  void         SetupLimbBodiesAndGeometry(const xiiSkeletonResource* pSkeleton, const xiiMsgAnimationPoseUpdated& pose);
  void         SetupLimbJoints(const xiiSkeletonResource* pSkeleton);
  virtual void CreateLimbBody(const LimbConfig& parentLimb, LimbConfig& thisLimb);
  void         AddLimbGeometry(xiiBasisAxis::Enum srcBoneDir, LimbConfig& limb, const xiiSkeletonResourceGeometry& geo);
  virtual void CreateLimbJoint(const xiiSkeletonJoint& thisJoint, void* pParentBodyDesc, const xiiTransform& parentFrame, void* pThisBodyDesc, const xiiTransform& thisFrame);
  void         ApplyImpulse();
  void         ComputeLimbModelSpaceTransform(xiiTransform& transform, const xiiMsgAnimationPoseUpdated& pose, xiiUInt32 uiIndex);
  void         ComputeLimbGlobalTransform(xiiTransform& transform, const xiiMsgAnimationPoseUpdated& pose, xiiUInt32 uiIndex);
  void         RetrievePhysicsPose();
  virtual void WakeUp();
};
