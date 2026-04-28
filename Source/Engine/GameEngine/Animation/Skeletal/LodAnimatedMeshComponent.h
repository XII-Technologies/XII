/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/World.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>

class XII_GAMEENGINE_DLL xiiLodAnimatedMeshComponentManager : public xiiComponentManager<class xiiLodAnimatedMeshComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiLodAnimatedMeshComponentManager(xiiWorld* pWorld);
  ~xiiLodAnimatedMeshComponentManager();

  virtual void Initialize() override;

  void Update(const xiiWorldModule::UpdateContext& context);
  void AddToUpdateList(xiiLodAnimatedMeshComponent* pComponent);

private:
  void ResourceEventHandler(const xiiResourceEvent& e);

  xiiDeque<xiiComponentHandle> m_ComponentsToUpdate;
};

struct xiiLodAnimatedMeshLod
{
  xiiMeshResourceHandle m_hMesh;
  float                 m_fThreshold;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiLodAnimatedMeshLod);

class XII_GAMEENGINE_DLL xiiLodAnimatedMeshComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiLodAnimatedMeshComponent, xiiRenderComponent, xiiLodAnimatedMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiMeshRenderData* CreateRenderData() const;
  virtual xiiResult          GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLodAnimatedMeshComponent

public:
  xiiLodAnimatedMeshComponent();
  ~xiiLodAnimatedMeshComponent();

  /// \brief An additional tint color passed to the renderer to modify the mesh.
  void            SetColor(const xiiColor& color); // [ property ]
  const xiiColor& GetColor() const;                // [ property ]

  /// \brief An additional vec4 passed to the renderer that can be used by custom material shaders for effects.
  void           SetCustomData(const xiiVec4& vData); // [ property ]
  const xiiVec4& GetCustomData() const;               // [ property ]

  /// \brief The sorting depth offset allows to tweak the order in which this mesh is rendered relative to other meshes.
  ///
  /// This is mainly useful for transparent objects to render them before or after other meshes.
  void  SetSortingDepthOffset(float fOffset); // [ property ]
  float GetSortingDepthOffset() const;        // [ property ]

  /// \brief Enables text output to show the current coverage value and selected LOD.
  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  /// \brief Disabling the LOD range overlap functionality can make it easier to determine the desired coverage thresholds.
  void SetOverlapRanges(bool bOverlap); // [ property ]
  bool GetOverlapRanges() const;        // [ property ]

  void OnMsgSetColor(xiiMsgSetColor& ref_msg); // [ msg handler ]

  void RetrievePose(xiiDynamicArray<xiiMat4>& out_modelTransforms, xiiTransform& out_rootTransform, const xiiSkeleton& skeleton);

protected:
  void UpdateSelectedLod(const xiiView& view) const;
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  mutable xiiInt32                       m_iCurLod = 0;
  xiiDynamicArray<xiiLodAnimatedMeshLod> m_Meshes;
  xiiColor                               m_Color               = xiiColor::White;
  xiiVec4                                m_vCustomData         = xiiVec4(0, 1, 0, 1);
  float                                  m_fSortingDepthOffset = 0.0f;
  xiiVec3                                m_vBoundsOffset       = xiiVec3::MakeZero();
  float                                  m_fBoundsRadius       = 1.0f;

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
