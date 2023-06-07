#pragma once

#include <GameEngine/Animation/TransformComponent.h>
#include <GameplayPlugin/GameplayPluginDLL.h>

using xiiHeadBoneComponentManager = class xiiHeadBoneComponent;

class XII_GAMEPLAYPLUGIN_DLL xiiHeadBoneComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiHeadBoneComponent, xiiComponent, xiiHeadBoneComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiHeadBoneComponent

public:
  xiiHeadBoneComponent();
  ~xiiHeadBoneComponent();


  void SetVerticalRotation(float fRadians);    // [ scriptable ]
  void ChangeVerticalRotation(float fRadians); // [ scriptable ]

  xiiAngle m_NewVerticalRotation;                        // [ property ]
  xiiAngle m_MaxVerticalRotation = xiiAngle::Degree(80); // [ property ]

protected:
  void Update();

  xiiAngle m_CurVerticalRotation;
};
