#pragma once

#include <GameEngine/AI/NpcComponent.h>
#include <RecastPlugin/RecastPluginDLL.h>

class xiiRecastWorldModule;
class xiiPhysicsWorldModuleInterface;
struct xiiAgentSteeringEvent;

typedef xiiComponentManagerSimple<class xiiSoldierComponent, xiiComponentUpdateType::WhenSimulating> xiiSoldierComponentManager;

class XII_RECASTPLUGIN_DLL xiiSoldierComponent : public xiiNpcComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSoldierComponent, xiiNpcComponent, xiiSoldierComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiSoldierComponent

public:
  xiiSoldierComponent();
  ~xiiSoldierComponent();

protected:
  void Update();

  void SteeringEventHandler(const xiiAgentSteeringEvent& e);

  enum class State
  {
    Idle,
    WaitingForPath,
    Walking,
    ErrorState,
  };

  State              m_State = State::Idle;
  xiiComponentHandle m_hSteeringComponent;
};
