#pragma once

#include <JoltPlugin/Actors/JoltActorComponent.h>

//////////////////////////////////////////////////////////////////////////

class XII_JOLTPLUGIN_DLL xiiJoltDynamicActorComponentManager : public xiiComponentManager<class xiiJoltDynamicActorComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiJoltDynamicActorComponentManager(xiiWorld* pWorld);
  ~xiiJoltDynamicActorComponentManager();

private:
  friend class xiiJoltWorldModule;
  friend class xiiJoltDynamicActorComponent;

  void UpdateKinematicActors(xiiTime deltaTime);
  void UpdateDynamicActors();

  xiiDynamicArray<xiiJoltDynamicActorComponent*> m_KinematicActorComponents;
};

//////////////////////////////////////////////////////////////////////////

class XII_JOLTPLUGIN_DLL xiiJoltDynamicActorComponent : public xiiJoltActorComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltDynamicActorComponent, xiiJoltActorComponent, xiiJoltDynamicActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJoltDynamicActorComponent

public:
  xiiJoltDynamicActorComponent();
  ~xiiJoltDynamicActorComponent();

  xiiUInt32 GetJoltBodyID() const { return m_uiJoltBodyID; }

  void AddImpulseAtPos(xiiMsgPhysicsAddImpulse& ref_msg); // [ message ]
  void AddForceAtPos(xiiMsgPhysicsAddForce& ref_msg);     // [ message ]

  bool GetKinematic() const { return m_bKinematic; } // [ property ]
  void SetKinematic(bool b);                         // [ property ]

  void  SetGravityFactor(float fFactor);                      // [ property ]
  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]

  void        SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;             // [ property ]

  bool                          m_bCCD            = false;                         // [ property ]
  bool                          m_bStartAsleep    = false;                         // [ property ]
  float                         m_fMass           = 0.0f;                          // [ property ]
  float                         m_fDensity        = 1.0f;                          // [ property ]
  float                         m_fLinearDamping  = 0.1f;                          // [ property ]
  float                         m_fAngularDamping = 0.05f;                         // [ property ]
  xiiSurfaceResourceHandle      m_hSurface;                                        // [ property ]
  xiiBitflags<xiiOnJoltContact> m_OnContact;                                       // [ property ]
  xiiVec3                       m_vCenterOfMass = xiiVec3::ZeroVector();           // [ property ]
  bool                          GetUseCustomCoM() const { return GetUserFlag(0); } // [ property ]
  void                          SetUseCustomCoM(bool b) { SetUserFlag(0, b); }     // [ property ]

  void AddLinearForce(const xiiVec3& vForce);      // [ scriptable ]
  void AddLinearImpulse(const xiiVec3& vImpulse);  // [ scriptable ]
  void AddAngularForce(const xiiVec3& vForce);     // [ scriptable ]
  void AddAngularImpulse(const xiiVec3& vImpulse); // [ scriptable ]

protected:
  const xiiJoltMaterial* GetJoltMaterial() const;

  bool  m_bKinematic     = false;
  float m_fGravityFactor = 1.0f; // [ property ]
};
