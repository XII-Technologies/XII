#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>

using xiiSkeletonResourceHandle  = xiiTypedResourceHandle<class xiiSkeletonResource>;
using xiiAnimGraphResourceHandle = xiiTypedResourceHandle<class xiiAnimGraphResource>;

using xiiAnimationControllerComponentManager = xiiComponentManagerSimple<class xiiAnimationControllerComponent, xiiComponentUpdateType::WhenSimulating, xiiBlockStorageType::FreeList>;

class XII_GAMEENGINE_DLL xiiAnimationControllerComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiAnimationControllerComponent, xiiComponent, xiiAnimationControllerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimationControllerComponent

public:
  xiiAnimationControllerComponent();
  ~xiiAnimationControllerComponent();

  void        SetAnimationControllerFile(const char* szFile); // [ property ]
  const char* GetAnimationControllerFile() const;             // [ property ]

  xiiEnum<xiiAnimationInvisibleUpdateRate> m_InvisibleUpdateRate; // [ property ]

protected:
  void Update();

  xiiEnum<xiiRootMotionMode> m_RootMotionMode;

  xiiAnimGraphResourceHandle m_hAnimationController;
  xiiAnimGraph               m_AnimationGraph;
  xiiAnimPoseGenerator       m_PoseGenerator;

  xiiTime m_ElapsedTimeSinceUpdate = xiiTime::Zero();
};
