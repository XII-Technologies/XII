
#pragma once

#include <Core/Messages/TriggerMessage.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameplayPlugin/GameplayPluginDLL.h>

class xiiPhysicsWorldModuleInterface;

class xiiRaycastComponentManager : public xiiComponentManager<class xiiRaycastComponent, xiiBlockStorageType::Compact>
{
  using SUPER = xiiComponentManager<class xiiRaycastComponent, xiiBlockStorageType::Compact>;

public:
  xiiRaycastComponentManager(xiiWorld* pWorld);

  virtual void Initialize() override;

  void Update(const xiiWorldModule::UpdateContext& context);
};

/// \brief A component which does a ray cast and positions a target object there.
///
/// This component does a ray cast along the forward axis of the game object it is attached to.
/// If this produces a hit the target object is placed there.
/// If no hit is found the target object is either placed at the maximum distance or deactivated depending on the component configuration.
///
/// This component can also trigger messages when objects enter the ray. E.g. when a player trips a laser detection beam.
/// To enable this set the trigger collision layer to another layer than the main ray cast and set a trigger message.
///
/// Sample setup:
///   m_uiCollisionLayerEndPoint = Default
///   m_uiCollisionLayerTrigger = Player
///   m_sTriggerMessage = "APlayerEnteredTheBeam"
///
/// This will lead to trigger messages being sent when a physics actor on the 'Player' layer comes between
/// the original hit on the default layer and the ray cast origin.
///
class XII_GAMEPLAYPLUGIN_DLL xiiRaycastComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRaycastComponent, xiiComponent, xiiRaycastComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void OnSimulationStarted() override;

  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  void Deinitialize() override;

  void OnActivated() override;
  void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRaycastComponent

public:
  xiiRaycastComponent();
  ~xiiRaycastComponent();

  void        SetTriggerMessage(const char* szSz); // [ property ]
  const char* GetTriggerMessage() const;           // [ property ]

  void SetRaycastEndObject(const char* szReference); // [ property ]

  xiiGameObjectHandle              m_hRaycastEndObject;                    // [ property ]
  float                            m_fMaxDistance                = 100.0f; // [ property ]
  bool                             m_bForceTargetParentless      = false;  // [ property ]
  bool                             m_bDisableTargetObjectOnNoHit = false;  // [ property ]
  xiiUInt8                         m_uiCollisionLayerEndPoint    = 0;      // [ property ]
  xiiUInt8                         m_uiCollisionLayerTrigger     = 0;      // [ property ]
  xiiBitflags<xiiPhysicsShapeType> m_ShapeTypesToHit;                      // [ property ]

private:
  void Update();

  xiiHashedString                               m_sTriggerMessage;    // [ property ]
  xiiEventMessageSender<xiiMsgTriggerTriggered> m_TriggerEventSender; // [ event ]

private:
  void PostTriggerMessage(xiiTriggerState::Enum state, xiiGameObjectHandle hObject);

  const char* DummyGetter() const { return nullptr; }

  xiiPhysicsWorldModuleInterface* m_pPhysicsWorldModule = nullptr;
  xiiGameObjectHandle             m_hLastTriggerObjectInRay;
};
