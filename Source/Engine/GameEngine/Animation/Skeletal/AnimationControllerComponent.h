#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimController.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>

using xiiSkeletonResourceHandle  = xiiTypedResourceHandle<class xiiSkeletonResource>;
using xiiAnimGraphResourceHandle = xiiTypedResourceHandle<class xiiAnimGraphResource>;

using xiiAnimationControllerComponentManager = xiiComponentManagerSimple<class xiiAnimationControllerComponent, xiiComponentUpdateType::WhenSimulating, xiiBlockStorageType::FreeList>;

class XII_GAMEENGINE_DLL xiiAnimationControllerComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiAnimationControllerComponent, xiiComponent, xiiAnimationControllerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimationControllerComponent

public:
  xiiAnimationControllerComponent();
  ~xiiAnimationControllerComponent();

  void        SetAnimGraphFile(const char* szFile); // [ property ]
  const char* GetAnimGraphFile() const;             // [ property ]

  xiiEnum<xiiAnimationInvisibleUpdateRate> m_InvisibleUpdateRate; // [ property ]

protected:
  void Update();

  xiiEnum<xiiRootMotionMode> m_RootMotionMode;

  xiiAnimGraphResourceHandle m_hAnimGraph;
  xiiAnimController          m_AnimController;
  xiiAnimPoseGenerator       m_PoseGenerator;

  xiiTime m_ElapsedTimeSinceUpdate = xiiTime::Zero();
};
