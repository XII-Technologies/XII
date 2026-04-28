/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GraphicsCore/AnimationSystem/AnimationPose.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Meshes/SkinnedMeshComponent.h>

using xiiSkeletonResourceHandle = xiiTypedResourceHandle<class xiiSkeletonResource>;

class XII_GAMEENGINE_DLL xiiAnimatedMeshComponentManager : public xiiComponentManager<class xiiAnimatedMeshComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiAnimatedMeshComponentManager(xiiWorld* pWorld);
  ~xiiAnimatedMeshComponentManager();

  virtual void Initialize() override;

  void Update(const xiiWorldModule::UpdateContext& context);
  void AddToUpdateList(xiiAnimatedMeshComponent* pComponent);

private:
  void ResourceEventHandler(const xiiResourceEvent& e);

  xiiDeque<xiiComponentHandle> m_ComponentsToUpdate;
};

/// \brief Instantiates a mesh that can be animated through skeletal animation.
///
/// The referenced mesh has to contain skinning information.
///
/// This component only creates an animated mesh for rendering. It does not animate the mesh in any way.
/// The component handles messages of type xiiMsgAnimationPoseUpdated. Using this message other systems can set a new pose
/// for the animated mesh.
///
/// For example the xiiSkeletonPoseComponent, xiiSimpleAnimationComponent and xiiAnimationControllerComponent do this
/// to change the pose of the animated mesh.
class XII_GAMEENGINE_DLL xiiAnimatedMeshComponent : public xiiMeshComponentBase
{
  XII_DECLARE_COMPONENT_TYPE(xiiAnimatedMeshComponent, xiiMeshComponentBase, xiiAnimatedMeshComponentManager);


  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

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

  void RetrievePose(xiiDynamicArray<xiiMat4>& out_modelTransforms, xiiTransform& out_rootTransform, const xiiSkeleton& skeleton);

protected:
  void OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& msg);     // [ msg handler ]
  void OnQueryAnimationSkeleton(xiiMsgQueryAnimationSkeleton& msg); // [ msg handler ]

  void InitializeAnimationPose();

  void MapModelSpacePoseToSkinningSpace(const xiiHashTable<xiiHashedString, xiiMeshResourceDescriptor::BoneData>& bones, const xiiSkeleton& skeleton, xiiArrayPtr<const xiiMat4> modelSpaceTransforms, xiiBoundingBox* bounds);

  xiiTransform              m_RootTransform = xiiTransform::MakeIdentity();
  xiiBoundingBox            m_MaxBounds;
  xiiSkinningState          m_SkinningState;
  xiiSkeletonResourceHandle m_hDefaultSkeleton;
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
