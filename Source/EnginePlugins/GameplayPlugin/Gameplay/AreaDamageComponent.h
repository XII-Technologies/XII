#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameplayPlugin/GameplayPluginDLL.h>

class xiiPhysicsWorldModuleInterface;
struct xiiPhysicsOverlapResult;

class XII_GAMEPLAYPLUGIN_DLL xiiAreaDamageComponentManager : public xiiComponentManager<class xiiAreaDamageComponent, xiiBlockStorageType::FreeList>
{
  using SUPER = xiiComponentManager<xiiAreaDamageComponent, xiiBlockStorageType::FreeList>;

public:
  xiiAreaDamageComponentManager(xiiWorld* pWorld);

  virtual void Initialize() override;

private:
  friend class xiiAreaDamageComponent;
  xiiPhysicsWorldModuleInterface* m_pPhysicsInterface = nullptr;
};

class XII_GAMEPLAYPLUGIN_DLL xiiAreaDamageComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiAreaDamageComponent, xiiComponent, xiiAreaDamageComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiAreaDamageComponent

public:
  xiiAreaDamageComponent();
  ~xiiAreaDamageComponent();

  void ApplyAreaDamage(); // [ scriptable ]

  bool     m_bTriggerOnCreation = true;   // [ property ]
  xiiUInt8 m_uiCollisionLayer   = 0;      // [ property ]
  float    m_fRadius            = 5.0f;   // [ property ]
  float    m_fDamage            = 10.0f;  // [ property ]
  float    m_fImpulse           = 100.0f; // [ property ]
};
