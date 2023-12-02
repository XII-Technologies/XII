//#pragma once
//
//#include <GameEngine/GameEngineDLL.h>
//#include <GraphicsCore/AnimationSystem/AnimationGraph/AnimationClipSampler.h>
//#include <GraphicsCore/AnimationSystem/AnimationPose.h>
//#include <GraphicsCore/Meshes/SkinnedMeshComponent.h>
//
//using xiiAnimationClipResourceHandle = xiiTypedResourceHandle<class xiiAnimationClipResource>;
//using xiiSkeletonResourceHandle = xiiTypedResourceHandle<class xiiSkeletonResource>;
//
//using xiiMotionMatchingComponentManager = xiiComponentManagerSimple<class xiiMotionMatchingComponent, xiiComponentUpdateType::WhenSimulating> ;
//
//class XII_GAMEENGINE_DLL xiiMotionMatchingComponent : public xiiSkinnedMeshComponent
//{
//  XII_DECLARE_COMPONENT_TYPE(xiiMotionMatchingComponent, xiiSkinnedMeshComponent, xiiMotionMatchingComponentManager);
//
//  //////////////////////////////////////////////////////////////////////////
//  // xiiComponent
//
//public:
//  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
//  virtual void DeserializeComponent(xiiWorldReader& stream) override;
//
//protected:
//  virtual void OnSimulationStarted() override;
//
//
//  //////////////////////////////////////////////////////////////////////////
//  // xiiMotionMatchingComponent
//
//public:
//  xiiMotionMatchingComponent();
//  ~xiiMotionMatchingComponent();
//
//  void SetAnimation(xiiUInt32 uiIndex, const xiiAnimationClipResourceHandle& hResource);
//  xiiAnimationClipResourceHandle GetAnimation(xiiUInt32 uiIndex) const;
//
//protected:
//  void Update();
//
//  xiiUInt32 Animations_GetCount() const;                          // [ property ]
//  const char* Animations_GetValue(xiiUInt32 uiIndex) const;       // [ property ]
//  void Animations_SetValue(xiiUInt32 uiIndex, const char* value); // [ property ]
//  void Animations_Insert(xiiUInt32 uiIndex, const char* value);   // [ property ]
//  void Animations_Remove(xiiUInt32 uiIndex);                      // [ property ]
//
//  void ConfigureInput();
//  xiiVec3 GetInputDirection() const;
//  xiiQuat GetInputRotation() const;
//
//  xiiAnimationPose m_AnimationPose;
//  xiiSkeletonResourceHandle m_hSkeleton;
//
//  xiiDynamicArray<xiiAnimationClipResourceHandle> m_Animations;
//
//  xiiVec3 m_vLeftFootPos;
//  xiiVec3 m_vRightFootPos;
//
//  struct MotionData
//  {
//    xiiUInt16 m_uiAnimClipIndex;
//    xiiUInt16 m_uiKeyframeIndex;
//    xiiVec3 m_vLeftFootPosition;
//    xiiVec3 m_vLeftFootVelocity;
//    xiiVec3 m_vRightFootPosition;
//    xiiVec3 m_vRightFootVelocity;
//    xiiVec3 m_vRootVelocity;
//  };
//
//  struct TargetKeyframe
//  {
//    xiiUInt16 m_uiAnimClip;
//    xiiUInt16 m_uiKeyframe;
//  };
//
//  TargetKeyframe m_Keyframe0;
//  TargetKeyframe m_Keyframe1;
//  float m_fKeyframeLerp = 0.0f;
//
//  TargetKeyframe FindNextKeyframe(const TargetKeyframe& current, const xiiVec3& vTargetDir) const;
//
//  xiiDynamicArray<MotionData> m_MotionData;
//
//  static void PrecomputeMotion(xiiDynamicArray<MotionData>& motionData, xiiTempHashedString jointName1, xiiTempHashedString jointName2,
//    const xiiAnimationClipResourceDescriptor& animClip, xiiUInt16 uiAnimClipIndex, const xiiSkeleton& skeleton);
//
//  xiiUInt32 FindBestKeyframe(const TargetKeyframe& current, xiiVec3 vLeftFootPosition, xiiVec3 vRightFootPosition, xiiVec3 vTargetDir) const;
//};
