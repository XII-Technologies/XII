#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct xiiMsgPhysicsAddImpulse;
struct xiiMsgPhysicsAddForce;
namespace JPH
{
  class Constraint;
}

using xiiSurfaceResourceHandle = xiiTypedResourceHandle<class xiiSurfaceResource>;

struct xiiJoltRopeAnchorConstraintMode
{
  using StorageType = xiiInt8;

  enum Enum
  {
    None,
    Point,
    Fixed,
    Cone,

    Default = Point
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_JOLTPLUGIN_DLL, xiiJoltRopeAnchorConstraintMode);

//////////////////////////////////////////////////////////////////////////

class XII_JOLTPLUGIN_DLL xiiJoltRopeComponentManager : public xiiComponentManager<class xiiJoltRopeComponent, xiiBlockStorageType::Compact>
{
public:
  xiiJoltRopeComponentManager(xiiWorld* pWorld);
  ~xiiJoltRopeComponentManager();

  virtual void Initialize() override;

private:
  void Update(const xiiWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class XII_JOLTPLUGIN_DLL xiiJoltRopeComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltRopeComponent, xiiComponent, xiiJoltRopeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJoltRopeComponent

public:
  xiiJoltRopeComponent();
  ~xiiJoltRopeComponent();

  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]
  void  SetGravityFactor(float fGravity);                     // [ property ]

  void        SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;             // [ property ]

  xiiUInt8                                 m_uiCollisionLayer = 0;            // [ property ]
  xiiUInt16                                m_uiPieces         = 16;           // [ property ]
  float                                    m_fThickness       = 0.05f;        // [ property ]
  float                                    m_fSlack           = 0.3f;         // [ property ]
  xiiEnum<xiiJoltRopeAnchorConstraintMode> m_Anchor1ConstraintMode;           // [ property ]
  xiiEnum<xiiJoltRopeAnchorConstraintMode> m_Anchor2ConstraintMode;           // [ property ]
  bool                                     m_bCCD     = false;                // [ property ]
  xiiAngle                                 m_MaxBend  = xiiAngle::Degree(30); // [ property ]
  xiiAngle                                 m_MaxTwist = xiiAngle::Degree(15); // [ property ]

  void SetAnchor1Reference(const char* szReference); // [ property ]
  void SetAnchor2Reference(const char* szReference); // [ property ]

  void SetAnchor1(xiiGameObjectHandle hActor);
  void SetAnchor2(xiiGameObjectHandle hActor);

  void AddForceAtPos(xiiMsgPhysicsAddForce& ref_msg);
  void AddImpulseAtPos(xiiMsgPhysicsAddImpulse& ref_msg);

private:
  void                   CreateRope();
  xiiResult              CreateSegmentTransforms(xiiDynamicArray<xiiTransform>& transforms, float& out_fPieceLength, xiiGameObjectHandle hAnchor1, xiiGameObjectHandle hAnchor2);
  void                   DestroyPhysicsShapes();
  void                   Update();
  void                   SendPreviewPose();
  const xiiJoltMaterial* GetJoltMaterial();
  JPH::Constraint*       CreateConstraint(const xiiGameObjectHandle& hTarget, const xiiTransform& dstLoc, xiiUInt32 uiBodyID, xiiJoltRopeAnchorConstraintMode::Enum mode);
  void                   UpdatePreview();

  xiiSurfaceResourceHandle m_hSurface;

  xiiGameObjectHandle m_hAnchor1;
  xiiGameObjectHandle m_hAnchor2;

  float     m_fTotalMass        = 1.0f;
  float     m_fMaxForcePerFrame = 0.0f;
  float     m_fBendStiffness    = 0.0f;
  xiiUInt32 m_uiObjectFilterID  = xiiInvalidIndex;
  xiiUInt32 m_uiUserDataIndex   = xiiInvalidIndex;
  bool      m_bSelfCollision    = false;
  float     m_fGravityFactor    = 1.0f;
  xiiUInt32 m_uiPreviewHash     = 0;

  JPH::Ragdoll*    m_pRagdoll           = nullptr;
  JPH::Constraint* m_pConstraintAnchor1 = nullptr;
  JPH::Constraint* m_pConstraintAnchor2 = nullptr;


private:
  const char* DummyGetter() const { return nullptr; }
};
