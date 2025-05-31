#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimController.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>

using xiiSkeletonResourceHandle  = xiiTypedResourceHandle<class xiiSkeletonResource>;
using xiiAnimGraphResourceHandle = xiiTypedResourceHandle<class xiiAnimGraphResource>;

class xiiAnimationControllerComponentManager : public xiiComponentManager<class xiiAnimationControllerComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiAnimationControllerComponentManager(xiiWorld* pWorld);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  void Update(const xiiWorldModule::UpdateContext& context);
  void ResourceEvent(const xiiResourceEvent& e);

  xiiDeque<xiiComponentHandle> m_ComponentsToReset;
};

/// \brief Evaluates a xiiAnimGraphResource and provides the result through the xiiMsgAnimationPoseUpdated.
///
/// xiiAnimGraph's contain logic to generate an animation pose. This component decides when it is necessary
/// to reevaluate the state, which mostly means it tracks when the object is visible.
///
/// The result is sent as a recursive message, which is usually consumed by a xiiAnimatedMeshComponent.
/// The mesh component may be on the same game object or a child object.
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

  /// \brief How often to update the animation while the animated mesh is invisible.
  xiiEnum<xiiAnimationInvisibleUpdateRate> m_InvisibleUpdateRate; // [ property ]

  /// \brief If enabled, child game objects can add IK computation commands to influence the final pose.
  bool m_bEnableIK = false; // [ property ]

protected:
  void Update();

  xiiEnum<xiiRootMotionMode> m_RootMotionMode;

  xiiAnimGraphResourceHandle m_hAnimGraph;
  xiiAnimController          m_AnimController;
  xiiAnimPoseGenerator       m_PoseGenerator;

  xiiTime m_ElapsedTimeSinceUpdate = xiiTime::MakeZero();
};
