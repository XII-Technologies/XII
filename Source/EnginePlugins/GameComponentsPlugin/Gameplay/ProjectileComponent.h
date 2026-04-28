/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Physics/SurfaceResource.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

struct xiiMsgComponentInternalTrigger;

using xiiProjectileComponentManager = xiiComponentManagerSimple<class xiiProjectileComponent, xiiComponentUpdateType::WhenSimulating>;

/// \brief Defines what a projectile will do when it hits a surface
struct XII_GAMECOMPONENTS_DLL xiiProjectileReaction
{
  using StorageType = xiiInt8;

  enum Enum : StorageType
  {
    Absorb,      ///< The projectile simply stops and is deleted
    Reflect,     ///< Bounces away along the reflected direction. Maintains momentum.
    Bounce,      ///< Bounces away along the reflected direction. Loses momentum.
    Attach,      ///< Stops at the hit point, does not continue further and attaches itself as a child to the hit object
    PassThrough, ///< Continues flying through the geometry (but may spawn prefabs at the intersection points)

    Default = Absorb
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMECOMPONENTS_DLL, xiiProjectileReaction);

/// \brief Holds the information about how a projectile interacts with a specific surface type
struct XII_GAMECOMPONENTS_DLL xiiProjectileSurfaceInteraction
{
  /// \brief The surface type (and derived ones) for which this interaction is used
  xiiSurfaceResourceHandle m_hSurface;

  /// \brief How the projectile itself will react when hitting the surface type
  xiiProjectileReaction::Enum m_Reaction;

  /// \brief Which interaction should be triggered. See xiiSurfaceResource.
  xiiString m_sInteraction;

  /// \brief The force (or rather impulse) that is applied on the object
  float m_fImpulse = 0.0f;

  /// \brief How much damage to do on this type of surface. Send via xiiMsgDamage
  float m_fDamage = 0.0f;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMECOMPONENTS_DLL, xiiProjectileSurfaceInteraction);

/// \brief Shoots a game object in a straight line and uses physics raycasts to detect hits.
///
/// When a raycast detects a hit, the surface information is used to determine how the projectile should proceed
/// and which prefab it should spawn as an effect.
class XII_GAMECOMPONENTS_DLL xiiProjectileComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiProjectileComponent, xiiComponent, xiiProjectileComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiProjectileComponent

public:
  xiiProjectileComponent();
  ~xiiProjectileComponent();

  /// The speed at which the projectile flies.
  float m_fMetersPerSecond; // [ property ]

  /// If 0, the projectile is not affected by gravity.
  float m_fGravityMultiplier; // [ property ]

  // If true the death prefab will be spawned when the velocity gones under the threshold to be considered static
  bool m_bSpawnPrefabOnStatic; // [ property ]

  /// Defines which other physics objects the projectile will collide with.
  xiiUInt8 m_uiCollisionLayer; // [ property ]

  /// A broad filter to ignore certain types of colliders.
  xiiBitflags<xiiPhysicsShapeType> m_ShapeTypesToHit; // [ property ]

  /// After this time the projectile is removed, if it didn't hit anything yet.
  xiiTime m_MaxLifetime; // [ property ]

  /// If the projectile hits something that has no valid surface, this surface is used instead.
  xiiSurfaceResourceHandle m_hFallbackSurface; // [ property ]

  /// Specifies how the projectile interacts with different surface types.
  xiiHybridArray<xiiProjectileSurfaceInteraction, 12> m_SurfaceInteractions; // [ property ]

  /// \brief If the projectile reaches its maximum lifetime it can spawn this prefab.
  xiiPrefabResourceHandle m_hDeathPrefab;

  /// \brief If the projectile reaches its maximum lifetime it can spawn this prefab.
  void          SetFallbackSurfaceFile(xiiStringView sFile); // [ property ]
  xiiStringView GetFallbackSurfaceFile() const;              // [ property ]

private:
  void Update();
  void OnTriggered(xiiMsgComponentInternalTrigger& msg); // [ msg handler ]

  void SpawnDeathPrefab();

  /// \brief If an unknown surface type is hit, the projectile will just delete itself without further interaction
  xiiInt32 FindSurfaceInteraction(const xiiSurfaceResourceHandle& hSurface) const;

  void TriggerSurfaceInteraction(const xiiSurfaceResourceHandle& hSurface, xiiGameObjectHandle hObject, const xiiVec3& vPos, const xiiVec3& vNormal, const xiiVec3& vDirection, xiiStringView sInteraction);

  xiiVec3 m_vVelocity;
};
