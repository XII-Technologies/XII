#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RecastPlugin/Components/RecastNavMeshComponent.h>
#include <RecastPlugin/RecastPluginDLL.h>

class xiiRecastWorldModule;
class xiiPhysicsWorldModuleInterface;

//////////////////////////////////////////////////////////////////////////

typedef xiiComponentManagerSimple<class xiiRcMarkPoiVisibleComponent, xiiComponentUpdateType::WhenSimulating> xiiRcMarkPoiVisibleComponentManager;

class XII_RECASTPLUGIN_DLL xiiRcMarkPoiVisibleComponent : public xiiRcComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRcMarkPoiVisibleComponent, xiiRcComponent, xiiRcMarkPoiVisibleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiRcMarkPoiVisibleComponent

public:
  xiiRcMarkPoiVisibleComponent();
  ~xiiRcMarkPoiVisibleComponent();

  float    m_fRadius          = 20.0f; // [ property ]
  xiiUInt8 m_uiCollisionLayer = 0;     // [ property ]

protected:
  void Update();

  xiiRecastWorldModule*           m_pWorldModule   = nullptr;
  xiiPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;

private:
  xiiUInt32 m_uiLastFirstCheckedPoint = 0;
};
