/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/PathComponent.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

struct xiiMsgAnimationReachedEnd;

//////////////////////////////////////////////////////////////////////////

using xiiFollowPathComponentManager = xiiComponentManagerSimple<class xiiFollowPathComponent, xiiComponentUpdateType::WhenSimulating>;

struct XII_GAMEENGINE_DLL xiiFollowPathMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    OnlyPosition,
    AlignUpZ,
    FullRotation,

    Default = OnlyPosition
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiFollowPathMode)

/// \brief This component makes the xiiGameObject, that it is attached to, move along a path defined by a xiiPathComponent.
///
/// Build a path using a xiiPathComponent and xiiPathNodeComponents.
/// Then attach a xiiFollowPathComponent to a free-standing xiiGameObject and reference the object with the xiiPathComponent in it.
///
/// During simulation the xiiFollowPathComponent will now move and rotate its owner object such that it moves along the path.
///
/// The start location of the 'hook' (the object with the xiiFollowPathComponent on it) may be anywhere. It will be teleported
/// onto the path. For many objects this is not a problem, but physically simulated objects may be very sensitive about this.
///
/// One option is to align the 'hook' perfectly with the start location.
/// You can achieve this, using the "Keep Simulation Changes" feature of the editor (simulate with zero speed, press K, stop simulation).
/// Another option is to instead delay the spawning of the object below the hook, by using a xiiSpawnComponent next to the xiiFollowPathComponent,
/// and thus have the payload spawn only after the hook has been placed properly.
class XII_GAMEENGINE_DLL xiiFollowPathComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiFollowPathComponent, xiiComponent, xiiFollowPathComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiFollowPathComponent

public:
  xiiFollowPathComponent();
  ~xiiFollowPathComponent();

  /// \brief Sets the reference to the game object on which a xiiPathComponent should be attached.
  void SetPathObject(const char* szReference); // [ property ]

  xiiEnum<xiiPropertyAnimMode> m_Mode;                                          ///< [ property ] How the path should be traversed.
  xiiEnum<xiiFollowPathMode>   m_FollowMode;                                    ///< [ property ] How the transform of the follower should be affected by the path.
  float                        m_fSpeed      = 1.0f;                            ///< [ property ] How fast to move along the path.
  float                        m_fLookAhead  = 1.0f;                            ///< [ property ] How far along the path to 'look ahead' to smooth the rotation. A small distance means rotations are very abrupt.
  float                        m_fSmoothing  = 0.5f;                            ///< [ property ] How much to combine the current position with the new position. 0 to 1. At zero, the position follows the path perfectly, but therefore also has very abrupt changes. With a lot of smoothing, the path becomes very sluggish.
  float                        m_fTiltAmount = 5.0f;                            ///< [ property ] How much to tilt when turning.
  xiiAngle                     m_MaxTilt     = xiiAngle::MakeFromDegree(30.0f); ///< [ property ] The max tilt angle of the object.

  /// \brief Distance along the path at which the xiiFollowPathComponent should start off.
  void  SetDistanceAlongPath(float fDistance); // [ property ]
  float GetDistanceAlongPath() const;          // [ property ]

  /// \brief Whether the component should move along the path 'forwards' or 'backwards'
  void SetDirectionForwards(bool bForwards); // [ scriptable ]

  /// \brief Toggles the direction that it travels along the path.
  void ToggleDirection(); // [ scriptable ]

  /// \brief Whether the component currently moves 'forwards' along the path.
  ///
  /// Note that if the 'speed' property is negative, moving 'forwards' along the path still means that it effectively moves backwards.
  bool IsDirectionForwards() const; // [ scriptable ]

  /// \brief Whether the component currently moves along the path, at all.
  bool IsRunning() const; // [ property ]

  /// \brief Whether to move along the path or not.
  void SetRunning(bool bRunning); // [ property ]

protected:
  void Update(bool bForce = false);

  xiiEventMessageSender<xiiMsgAnimationReachedEnd> m_ReachedEndEvent; // [ event ]
  xiiGameObjectHandle                              m_hPathObject;     // [ property ]
  xiiPathComponent::LinearSampler                  m_PathSampler;

  float    m_fStartDistance     = 0.0f; // [ property ]
  bool     m_bLastStateValid    = false;
  bool     m_bIsRunning         = true;
  bool     m_bIsRunningForwards = true;
  xiiVec3  m_vLastPosition;
  xiiVec3  m_vLastTargetPosition;
  xiiVec3  m_vLastUpDir;
  xiiAngle m_LastTiltAngle;

  const char* DummyGetter() const { return nullptr; }
};
