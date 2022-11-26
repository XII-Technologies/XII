#pragma once

#include <Core/World/ComponentManager.h>
#include <JoltPlugin/Declarations.h>

namespace JPH
{
  class Body;
}

namespace JPH
{
  class Constraint;
}

struct xiiJoltConstraintLimitMode
{
  using StorageType = xiiInt8;

  enum Enum
  {
    NoLimit,
    HardLimit,
    // SoftLimit,

    Default = NoLimit
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_JOLTPLUGIN_DLL, xiiJoltConstraintLimitMode);

struct xiiJoltConstraintDriveMode
{
  using StorageType = xiiInt8;

  enum Enum
  {
    NoDrive,
    DriveVelocity,
    DrivePosition,

    Default = NoDrive
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_JOLTPLUGIN_DLL, xiiJoltConstraintDriveMode);

//////////////////////////////////////////////////////////////////////////

class XII_JOLTPLUGIN_DLL xiiJoltConstraintComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiJoltConstraintComponent, xiiComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltConstraintComponent

public:
  xiiJoltConstraintComponent();
  ~xiiJoltConstraintComponent();

  // void SetBreakForce(float value);                      // [ property ]
  // float GetBreakForce() const { return m_fBreakForce; } // [ property ]

  // void SetBreakTorque(float value);                       // [ property ]
  // float GetBreakTorque() const { return m_fBreakTorque; } // [ property ]

  void SetPairCollision(bool value);                         // [ property ]
  bool GetPairCollision() const { return m_bPairCollision; } // [ property ]

  void SetParentActorReference(const char* szReference);      // [ property ]
  void SetChildActorReference(const char* szReference);       // [ property ]
  void SetChildActorAnchorReference(const char* szReference); // [ property ]

  void SetParentActor(xiiGameObjectHandle hActor);
  void SetChildActor(xiiGameObjectHandle hActor);
  void SetChildActorAnchor(xiiGameObjectHandle hActor);

  void SetActors(xiiGameObjectHandle hActorA, const xiiTransform& localFrameA, xiiGameObjectHandle hActorB, const xiiTransform& localFrameB);

  virtual void ApplySettings() = 0;

protected:
  xiiResult FindParentBody(xiiUInt32& out_uiJoltBodyID);
  xiiResult FindChildBody(xiiUInt32& out_uiJoltBodyID);

  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) = 0;

  xiiTransform ComputeParentBodyGlobalFrame() const;
  xiiTransform ComputeChildBodyGlobalFrame() const;

  void QueueApplySettings();

  xiiGameObjectHandle m_hActorA;
  xiiGameObjectHandle m_hActorB;
  xiiGameObjectHandle m_hActorBAnchor;

  // UserFlag0 specifies whether m_localFrameA is already set
  xiiTransform m_LocalFrameA;
  // UserFlag1 specifies whether m_localFrameB is already set
  xiiTransform m_LocalFrameB;

  JPH::Constraint* m_pConstraint = nullptr;

  // float m_fBreakForce = 0.0f;
  // float m_fBreakTorque = 0.0f;
  bool m_bPairCollision = true;

private:
  const char* DummyGetter() const { return nullptr; }
};
