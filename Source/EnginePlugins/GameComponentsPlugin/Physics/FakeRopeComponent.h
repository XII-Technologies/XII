#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/Physics/RopeSimulator.h>

//////////////////////////////////////////////////////////////////////////

class XII_GAMECOMPONENTS_DLL xiiFakeRopeComponentManager : public xiiComponentManager<class xiiFakeRopeComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiFakeRopeComponentManager(xiiWorld* pWorld);
  ~xiiFakeRopeComponentManager();

  virtual void Initialize() override;

private:
  void Update(const xiiWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

/// \brief Simulates the behavior of a rope/cable without physical interaction with the world.
///
/// This component is mainly used for decorative purposes to add things like power lines, cables and ropes
/// that hang in a level and should swing in the wind. This component only simulates simple rope physics,
/// but doesn't take the surrounding geometry into account. Thus ropes will swing through other geometry,
/// and the only way to prevent that, is to place the rope such, that it usually doesn't come close to other objects.
///
/// The fake rope simulation is more lightweight than proper physical simulation and also stops simulating when
/// it isn't visible.
///
/// Prefer to use this over e.g. the xiiJoltRopeComponent, when the physical interaction isn't needed.
class XII_GAMECOMPONENTS_DLL xiiFakeRopeComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiFakeRopeComponent, xiiComponent, xiiFakeRopeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiFakeRopeComponent

public:
  xiiFakeRopeComponent();
  ~xiiFakeRopeComponent();

  /// Of how many pieces the rope is made up.
  xiiUInt16 m_uiPieces = 16; // [ property ]

  void SetAnchor1Reference(const char* szReference); // [ property ]
  void SetAnchor2Reference(const char* szReference); // [ property ]

  /// \brief The first game object to attach to.
  void SetAnchor1(xiiGameObjectHandle hActor);
  /// \brief The second game object to attach to.
  void SetAnchor2(xiiGameObjectHandle hActor);

  /// \brief How much the rope should sag. A value of 0 means it should be absolutely straight. In practice there is always slack, due to imprecision in the simulation.
  void  SetSlack(float fVal);
  float GetSlack() const { return m_fSlack; }

  /// \brief If true, the rope will be fixed to the first anchor, otherwise it will start there but then fall down.
  void SetAttachToAnchor1(bool bVal);
  /// \brief If true, the rope will be fixed to the second anchor, otherwise it will start there but then fall down.
  void SetAttachToAnchor2(bool bVal);
  bool GetAttachToAnchor1() const;
  bool GetAttachToAnchor2() const;

  /// How quickly a swinging rope comes to a stop.
  float m_fDamping = 0.5f; // [ property ]

private:
  xiiResult ConfigureRopeSimulator();
  void      SendCurrentPose();
  void      SendPreviewPose();
  void      RuntimeUpdate();

  xiiGameObjectHandle m_hAnchor1;
  xiiGameObjectHandle m_hAnchor2;

  float     m_fSlack        = 0.0f;
  xiiUInt32 m_uiPreviewHash = 0;

  // if the owner or the anchor object are flagged as 'dynamic', the rope must follow their movement
  // otherwise it can skip some update steps
  bool             m_bIsDynamic                = true;
  xiiUInt8         m_uiCheckEquilibriumCounter = 0;
  xiiUInt8         m_uiSleepCounter            = 0;
  xiiRopeSimulator m_RopeSim;
  float            m_fWindInfluence = 0.0f;

private:
  const char* DummyGetter() const { return nullptr; }
};
