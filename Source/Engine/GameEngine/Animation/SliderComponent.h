#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

using xiiSliderComponentManager = xiiComponentManagerSimple<class xiiSliderComponent, xiiComponentUpdateType::WhenSimulating>;

/// \brief Applies a sliding transform to the game object that it is attached to.
///
/// The object is moved along a local axis either once or back and forth.
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

  /// \brief How far to move the object along the axis before reaching the end point.
  float m_fDistanceToTravel = 1.0f; // [ property ]

  /// \brief The acceleration to use to reach the target speed.
  float m_fAcceleration = 0.0f; // [ property ]

  /// \brief The deceleration to use to brake to zero speed before reaching the end.
  float m_fDeceleration = 0.0; // [ property ]

  /// \brief The axis along which to move the object.
  xiiEnum<xiiBasisAxis> m_Axis = xiiBasisAxis::PositiveZ; // [ property ]

  /// \brief If non-zero, the slider starts at a random offset as if it had already been moving for up to this amount of time.
  xiiTime m_RandomStart; // [ property ]

protected:
  void Update();

  float m_fLastDistance = 0.0f;
};
