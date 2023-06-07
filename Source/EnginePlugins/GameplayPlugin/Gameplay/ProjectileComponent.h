#pragma once

#include <Core/Physics/SurfaceResource.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameplayPlugin/GameplayPluginDLL.h>

struct xiiMsgComponentInternalTrigger;

using xiiProjectileComponentManager = class xiiProjectileComponent;

/// \brief Defines what a projectile will do when it hits a surface
struct XII_GAMEPLAYPLUGIN_DLL xiiProjectileReaction
{
  using StorageType = xiiInt8;

  enum Enum : StorageType
  {
    Absorb,      ///< The projectile simply stops and is deleted
    Reflect,     ///< Bounces away along the reflected direction
    Attach,      ///< Stops at the hit point, does not continue further and attaches itself as a child to the hit object
    PassThrough, ///< Continues flying through the geometry (but may spawn prefabs at the intersection points)

    Default = Absorb
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEPLAYPLUGIN_DLL, xiiProjectileReaction);

/// \brief Holds the information about how a projectile interacts with a specific surface type
struct XII_GAMEPLAYPLUGIN_DLL xiiProjectileSurfaceInteraction
{
  void        SetSurface(const char* szSurface);
  const char* GetSurface() const;

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

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEPLAYPLUGIN_DLL, xiiProjectileSurfaceInteraction);

class XII_GAMEPLAYPLUGIN_DLL xiiProjectileComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiProjectileComponent, xiiComponent, xiiProjectileComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiProjectileComponent

public:
  xiiProjectileComponent();
  ~xiiProjectileComponent();

  float                                               m_fMetersPerSecond;    ///< [ property ] The speed at which the projectile flies
  float                                               m_fGravityMultiplier;  ///< [ property ] If 0, the projectile is not affected by gravity.
  xiiUInt8                                            m_uiCollisionLayer;    ///< [ property ]
  xiiTime                                             m_MaxLifetime;         ///< [ property ] After this time the projectile is killed, if it didn't die already
  xiiSurfaceResourceHandle                            m_hFallbackSurface;    ///< [ property ]
  xiiHybridArray<xiiProjectileSurfaceInteraction, 12> m_SurfaceInteractions; ///< [ property ]

  void        SetTimeoutPrefab(const char* szPrefab); // [ property ]
  const char* GetTimeoutPrefab() const;               // [ property ]

  void        SetFallbackSurfaceFile(const char* szFile); // [ property ]
  const char* GetFallbackSurfaceFile() const;             // [ property ]

private:
  void Update();
  void OnTriggered(xiiMsgComponentInternalTrigger& msg); // [ msg handler ]

  xiiPrefabResourceHandle m_hTimeoutPrefab; ///< Spawned when the projectile is killed due to m_MaxLifetime coming to an end

  /// \brief If an unknown surface type is hit, the projectile will just delete itself without further interaction
  xiiInt32 FindSurfaceInteraction(const xiiSurfaceResourceHandle& hSurface) const;

  void TriggerSurfaceInteraction(const xiiSurfaceResourceHandle& hSurface, xiiGameObjectHandle hObject, const xiiVec3& vPos, const xiiVec3& vNormal, const xiiVec3& vDirection, const char* szInteraction);

  xiiVec3 m_vVelocity;
};
