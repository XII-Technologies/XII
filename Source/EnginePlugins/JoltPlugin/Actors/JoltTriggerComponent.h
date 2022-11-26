#pragma once

#include <Core/Messages/TriggerMessage.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Utilities/JoltUserData.h>

//////////////////////////////////////////////////////////////////////////

class XII_JOLTPLUGIN_DLL xiiJoltTriggerComponentManager : public xiiComponentManager<class xiiJoltTriggerComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiJoltTriggerComponentManager(xiiWorld* pWorld);
  ~xiiJoltTriggerComponentManager();

private:
  friend class xiiJoltWorldModule;
  friend class xiiJoltTriggerComponent;

  void UpdateMovingTriggers();

  xiiSet<xiiJoltTriggerComponent*> m_MovingTriggers;
};

//////////////////////////////////////////////////////////////////////////

class XII_JOLTPLUGIN_DLL xiiJoltTriggerComponent : public xiiJoltActorComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltTriggerComponent, xiiJoltActorComponent, xiiJoltTriggerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

public:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltTriggerComponent
public:
  xiiJoltTriggerComponent();
  ~xiiJoltTriggerComponent();

  void        SetTriggerMessage(const char* sz) { m_sTriggerMessage.Assign(sz); } // [ property ]
  const char* GetTriggerMessage() const { return m_sTriggerMessage.GetData(); }   // [ property ]

protected:
  friend class xiiJoltWorldModule;
  friend class xiiJoltContactListener;

  void PostTriggerMessage(const xiiGameObjectHandle& hOtherObject, xiiTriggerState::Enum triggerState) const;

  xiiHashedString                               m_sTriggerMessage;
  xiiEventMessageSender<xiiMsgTriggerTriggered> m_TriggerEventSender; // [ event ]
};
