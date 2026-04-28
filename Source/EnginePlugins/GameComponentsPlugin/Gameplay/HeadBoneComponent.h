/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameComponentsPlugin/GameComponentsDLL.h>

using xiiHeadBoneComponentManager = xiiComponentManagerSimple<class xiiHeadBoneComponent, xiiComponentUpdateType::WhenSimulating>;

/// \brief Applies a vertical rotation in local space (local Y axis) to the owner game object.
///
/// This component is meant to be used to apply a vertical rotation to a camera.
/// For first-person camera movement, typically the horizontal rotation is already taken care of
/// through the rotation of a character controller.
/// To additionally allow a limited vertical rotation, this component is introduced.
/// It is assumed that a local rotation of zero represents the "forward" camera direction and the camera is allowed
/// to rotate both up and down by a certain number of degrees, for example 80 degrees.
/// This component takes care to apply that amount of rotation and not more.
///
/// Call SetVerticalRotation() or ChangeVerticalRotation() to set or add some rotation.
class XII_GAMECOMPONENTS_DLL xiiHeadBoneComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiHeadBoneComponent, xiiComponent, xiiHeadBoneComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiHeadBoneComponent

public:
  xiiHeadBoneComponent();
  ~xiiHeadBoneComponent();

  /// \brief Sets the vertical rotation to a fixed value.
  ///
  /// The final rotation will be clamped to the maximum allowed value.
  void SetVerticalRotation(float fRadians); // [ scriptable ]

  /// \brief Adds or subtracts from the current rotation.
  ///
  /// The final rotation will be clamped to the maximum allowed value.
  void ChangeVerticalRotation(float fRadians); // [ scriptable ]

  xiiAngle m_MaxVerticalRotation = xiiAngle::MakeFromDegree(80); // [ property ]

protected:
  void Update();

  xiiAngle m_NewVerticalRotation;
  xiiAngle m_CurVerticalRotation;
};
