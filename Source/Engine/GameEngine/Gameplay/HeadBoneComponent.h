#pragma once

#include <GameEngine/Animation/TransformComponent.h>
#include <GameEngine/GameEngineDLL.h>

typedef xiiComponentManagerSimple<class xiiHeadBoneComponent, xiiComponentUpdateType::WhenSimulating> xiiHeadBoneComponentManager;

class XII_GAMEENGINE_DLL xiiHeadBoneComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiHeadBoneComponent, xiiComponent, xiiHeadBoneComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiHeadBoneComponent

public:
  xiiHeadBoneComponent();
  ~xiiHeadBoneComponent();


  void SetVerticalRotation(float radians);    // [ scriptable ]
  void ChangeVerticalRotation(float radians); // [ scriptable ]

  xiiAngle m_NewVerticalRotation;                        // [ property ]
  xiiAngle m_MaxVerticalRotation = xiiAngle::Degree(80); // [ property ]

protected:
  void Update();

  xiiAngle m_CurVerticalRotation;
};
