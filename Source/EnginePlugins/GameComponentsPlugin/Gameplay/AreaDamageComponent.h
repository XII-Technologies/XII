#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

class xiiPhysicsWorldModuleInterface;
struct xiiPhysicsOverlapResult;

class XII_GAMECOMPONENTS_DLL xiiAreaDamageComponentManager : public xiiComponentManager<class xiiAreaDamageComponent, xiiBlockStorageType::FreeList>
{
  using SUPER = xiiComponentManager<xiiAreaDamageComponent, xiiBlockStorageType::FreeList>;

public:
  xiiAreaDamageComponentManager(xiiWorld* pWorld);

  virtual void Initialize() override;

private:
  friend class xiiAreaDamageComponent;
};

/// \brief Used to apply damage to objects in the vicinity and push physical objects away.
///
/// The component queries for dynamic physics shapes within a given radius.
/// For all objects found it sends the messages xiiMsgPhysicsAddImpulse and xiiMsgDamage.
/// The former is used to apply a physical impulse, to push the objects away from the center of the explosion.
/// The second message is used to apply damage to the objects. This only has an effect, if those objects
/// handle that message type.///
///
/// This component is mainly meant as an example how to make gameplay functionality, such as explosions.
/// If its functionality is insufficient for your use-case, write your own and take its code as inspiration.
class XII_GAMECOMPONENTS_DLL xiiAreaDamageComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiAreaDamageComponent, xiiComponent, xiiAreaDamageComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

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
