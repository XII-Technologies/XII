#pragma once

#include <GameplayPlugin/GameplayPluginDLL.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Time/Time.h>

struct xiiMoveToComponentFlags
{
  using StorageType = xiiUInt16;

  enum Enum
  {
    None    = 0,
    Running = XII_BIT(0),
    Default = None
  };

  struct Bits
  {
    StorageType Running : 1;
  };
};

using xiiMoveToComponentManager = xiiComponentManagerSimple<class xiiMoveToComponent, xiiComponentUpdateType::WhenSimulating>;

XII_DECLARE_FLAGS_OPERATORS(xiiMoveToComponentFlags);

/// \brief A light-weight component that moves the owner object towards a single position.
///
/// The functionality of this component can only be controlled through (script) code.
/// The component is given a single point in global space. When it is set to 'running' it will move
/// the owning object towards this point. Optionally it may use acceleration or deceleration and a maximum speed
/// to reach that point.
///
/// Since the target position is given through code and can be modified at any time, this component can be used
/// for moving objects to a point that is decided dynamically. For example an elevator can be moved to a
/// specific height, depending on which floor was selected. Or an object could follow a character,
/// by updating the target position regularly.
///
/// The component sends the event 'xiiMsgAnimationReachedEnd' and resets its running state when it reaches the target position.
class XII_GAMEPLAYPLUGIN_DLL xiiMoveToComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiMoveToComponent, xiiComponent, xiiMoveToComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiTransformComponent

public:
  xiiMoveToComponent();
  ~xiiMoveToComponent();

  /// \brief If set to false, the animation stops immediately.
  void SetRunning(bool bRunning); // [ property ]
  bool IsRunning() const;         // [ property ]

  void SetTargetPosition(const xiiVec3& vPos); // [ scriptable ]

protected:
  void Update();

  xiiEventMessageSender<xiiMsgAnimationReachedEnd> m_ReachedEndMsgSender; // [ event ]

  float m_fCurTranslationSpeed     = 0;
  float m_fMaxTranslationSpeed     = 1; // [ property ]
  float m_fTranslationAcceleration = 0; // [ property ]
  float m_fTranslationDeceleration = 0; // [ property ]

  xiiVec3                              m_vTargetPosition;
  xiiBitflags<xiiMoveToComponentFlags> m_Flags;
};
