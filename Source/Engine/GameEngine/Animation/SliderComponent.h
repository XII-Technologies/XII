#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

using xiiSliderComponentManager = xiiComponentManagerSimple<class xiiSliderComponent, xiiComponentUpdateType::WhenSimulating>;

class XII_GAMEENGINE_DLL xiiSliderComponent : public xiiTransformComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSliderComponent, xiiTransformComponent, xiiSliderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSliderComponent

public:
  xiiSliderComponent();
  ~xiiSliderComponent();

  float                 m_fDistanceToTravel = 1.0f;                    // [ property ]
  float                 m_fAcceleration     = 0.0f;                    // [ property ]
  float                 m_fDeceleration     = 0.0;                     // [ property ]
  xiiEnum<xiiBasisAxis> m_Axis              = xiiBasisAxis::PositiveZ; // [ property ]
  xiiTime               m_RandomStart;                                 // [ property ]

protected:
  void Update();

  float m_fLastDistance = 0.0f;
};
