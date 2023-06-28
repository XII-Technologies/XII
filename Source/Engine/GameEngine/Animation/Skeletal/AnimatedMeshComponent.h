#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>

using xiiSkeletonResourceHandle = xiiTypedResourceHandle<class xiiSkeletonResource>;

using xiiAnimatedMeshComponentManager = xiiComponentManager<class xiiAnimatedMeshComponent, xiiBlockStorageType::FreeList>;

class XII_GAMEENGINE_DLL xiiAnimatedMeshComponent : public xiiMeshComponentBase
{
  XII_DECLARE_COMPONENT_TYPE(xiiAnimatedMeshComponent, xiiMeshComponentBase, xiiAnimatedMeshComponentManager);


  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiMeshComponentBase

protected:
  virtual xiiMeshRenderData* CreateRenderData() const override;
  virtual xiiResult          GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimatedMeshComponent

public:
  xiiAnimatedMeshComponent();
  ~xiiAnimatedMeshComponent();

  void RetrievePose(xiiDynamicArray<xiiMat4>& out_ModelTransforms, xiiTransform& out_RootTransform, const xiiSkeleton& skeleton);

protected:
  void OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& msg);     // [ msg handler ]
  void OnQueryAnimationSkeleton(xiiMsgQueryAnimationSkeleton& msg); // [ msg handler ]

  void InitializeAnimationPose();

  void MapModelSpacePoseToSkinningSpace(const xiiHashTable<xiiHashedString, xiiMeshResourceDescriptor::BoneData>& bones, const xiiSkeleton& skeleton, xiiArrayPtr<const xiiMat4> modelSpaceTransforms, xiiBoundingBox* bounds);

  xiiTransform     m_RootTransform = xiiTransform::IdentityTransform();
  xiiBoundingBox   m_MaxBounds;
  xiiSkinningState m_SkinningState;
};


struct xiiRootMotionMode
{
  using StorageType = xiiInt8;

  enum Enum
  {
    Ignore,
    ApplyToOwner,
    SendMoveCharacterMsg,

    Default = Ignore
  };

  XII_GAMEENGINE_DLL static void Apply(xiiRootMotionMode::Enum mode, xiiGameObject* pObject, const xiiVec3& vTranslation, xiiAngle rotationX, xiiAngle rotationY, xiiAngle rotationZ);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiRootMotionMode);
