#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

struct xiiTransformComponentFlags
{
  using StorageType = xiiUInt16;

  enum Enum
  {
    None              = 0,
    Running           = XII_BIT(0),
    AutoReturnStart   = XII_BIT(1),
    AutoReturnEnd     = XII_BIT(2),
    AnimationReversed = XII_BIT(5),
    Default           = Running | AutoReturnStart | AutoReturnEnd
  };

  struct Bits
  {
    StorageType Running : 1;
    StorageType AutoReturnStart : 1;
    StorageType AutoReturnEnd : 1;
    StorageType Unused1 : 1;
    StorageType Unused2 : 1;
    StorageType AnimationReversed : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiTransformComponentFlags);

class XII_GAMEENGINE_DLL xiiTransformComponent : public xiiComponent
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTransformComponent, xiiComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiTransformComponent

public:
  xiiTransformComponent();
  ~xiiTransformComponent();

  void SetDirectionForwards(bool bForwards); // [ scriptable ]
  void ToggleDirection();                    // [ scriptable ]
  bool IsDirectionForwards() const;          // [ scriptable ]

  bool IsRunning(void) const;     // [ property ]
  void SetRunning(bool bRunning); // [ property ]

  bool GetReverseAtStart(void) const; // [ property ]
  void SetReverseAtStart(bool b);     // [ property ]

  bool GetReverseAtEnd(void) const; // [ property ]
  void SetReverseAtEnd(bool b);     // [ property ]

  float m_fAnimationSpeed = 1.0f; // [ property ]

protected:
  xiiBitflags<xiiTransformComponentFlags> m_Flags;
  xiiTime                                 m_AnimationTime;
};
