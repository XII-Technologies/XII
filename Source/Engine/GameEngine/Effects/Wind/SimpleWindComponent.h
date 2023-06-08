#pragma once

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

using xiiSimpleWindComponentManager = xiiComponentManagerSimple<class xiiSimpleWindComponent, xiiComponentUpdateType::WhenSimulating>;

class XII_GAMEENGINE_DLL xiiSimpleWindComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSimpleWindComponent, xiiComponent, xiiSimpleWindComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiSimpleWindComponent

public:
  xiiSimpleWindComponent();
  ~xiiSimpleWindComponent();

  xiiEnum<xiiWindStrength> m_MinWindStrength; // [ property ]
  xiiEnum<xiiWindStrength> m_MaxWindStrength; // [ property ]

  xiiAngle m_Deviation; // [ property ]

protected:
  void Update();
  void ComputeNextState();

  float   m_fLastStrength = 0;
  float   m_fNextStrength = 0;
  xiiVec3 m_vLastDirection;
  xiiVec3 m_vNextDirection;
  xiiTime m_LastChange;
  xiiTime m_NextChange;
};
