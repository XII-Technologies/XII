#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <GraphicsCore/AnimationSystem/AnimPoseGenerator.h>

class xiiGameObject;
class xiiAnimGraph;

using xiiAnimGraphResourceHandle = xiiTypedResourceHandle<class xiiAnimGraphResource>;
using xiiSkeletonResourceHandle  = xiiTypedResourceHandle<class xiiSkeletonResource>;

XII_DEFINE_AS_POD_TYPE(ozz::math::SimdFloat4);

struct xiiAnimGraphPinDataBoneWeights
{
  xiiUInt16                            m_uiOwnIndex         = 0xFFFF;
  float                                m_fOverallWeight     = 1.0f;
  const xiiAnimGraphSharedBoneWeights* m_pSharedBoneWeights = nullptr;
};

struct xiiAnimGraphPinDataLocalTransforms
{
  xiiUInt16                             m_uiOwnIndex = 0xFFFF;
  xiiAnimPoseGeneratorCommandID         m_CommandID;
  const xiiAnimGraphPinDataBoneWeights* m_pWeights       = nullptr;
  float                                 m_fOverallWeight = 1.0f;
  xiiVec3                               m_vRootMotion    = xiiVec3::MakeZero();
  bool                                  m_bUseRootMotion = false;
};

struct xiiAnimGraphPinDataModelTransforms
{
  xiiUInt16                     m_uiOwnIndex = 0xFFFF;
  xiiAnimPoseGeneratorCommandID m_CommandID;
  xiiVec3                       m_vRootMotion = xiiVec3::MakeZero();
  xiiAngle                      m_RootRotationX;
  xiiAngle                      m_RootRotationY;
  xiiAngle                      m_RootRotationZ;
  bool                          m_bUseRootMotion = false;
};

class XII_GRAPHICSCORE_DLL xiiAnimController
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiAnimController);

public:
  xiiAnimController();
  ~xiiAnimController();

  void Initialize(const xiiSkeletonResourceHandle& hSkeleton, xiiAnimPoseGenerator& ref_poseGenerator, const xiiSharedPtr<xiiBlackboard>& pBlackboard = nullptr);

  void Update(xiiTime diff, xiiGameObject* pTarget, bool bEnableIK);

  void GetRootMotion(xiiVec3& ref_vTranslation, xiiAngle& ref_rotationX, xiiAngle& ref_rotationY, xiiAngle& ref_rotationZ) const;

  const xiiSharedPtr<xiiBlackboard>& GetBlackboard() { return m_pBlackboard; }

  xiiAnimPoseGenerator& GetPoseGenerator() { return *m_pPoseGenerator; }

  static xiiSharedPtr<xiiAnimGraphSharedBoneWeights> CreateBoneWeights(const char* szUniqueName, const xiiSkeletonResource& skeleton, xiiDelegate<void(xiiAnimGraphSharedBoneWeights&)> fill);

  void SetOutputModelTransform(xiiAnimGraphPinDataModelTransforms* pModelTransform);
  void SetRootMotion(const xiiVec3& vTranslation, xiiAngle rotationX, xiiAngle rotationY, xiiAngle rotationZ);

  void AddOutputLocalTransforms(xiiAnimGraphPinDataLocalTransforms* pLocalTransforms);

  xiiAnimGraphPinDataBoneWeights*     AddPinDataBoneWeights();
  xiiAnimGraphPinDataLocalTransforms* AddPinDataLocalTransforms();
  xiiAnimGraphPinDataModelTransforms* AddPinDataModelTransforms();

  void AddAnimGraph(const xiiAnimGraphResourceHandle& hGraph);
  // TODO void RemoveAnimGraph(const xiiAnimGraphResource& hGraph);

  struct AnimClipInfo
  {
    xiiAnimationClipResourceHandle m_hClip;
  };

  const AnimClipInfo& GetAnimationClipInfo(xiiTempHashedString sClipName) const;

private:
  void GenerateLocalResultProcessors(const xiiSkeletonResource* pSkeleton);

  xiiSkeletonResourceHandle           m_hSkeleton;
  xiiAnimGraphPinDataModelTransforms* m_pCurrentModelTransforms = nullptr;

  xiiVec3  m_vRootMotion = xiiVec3::MakeZero();
  xiiAngle m_RootRotationX;
  xiiAngle m_RootRotationY;
  xiiAngle m_RootRotationZ;

  xiiDynamicArray<ozz::math::SimdFloat4, xiiAlignedAllocatorWrapper> m_BlendMask;

  xiiAnimPoseGenerator*       m_pPoseGenerator = nullptr;
  xiiSharedPtr<xiiBlackboard> m_pBlackboard    = nullptr;

  xiiHybridArray<xiiUInt32, 8> m_CurrentLocalTransformOutputs;

  static xiiMutex                                                             s_SharedDataMutex;
  static xiiHashTable<xiiString, xiiSharedPtr<xiiAnimGraphSharedBoneWeights>> s_SharedBoneWeights;

  struct GraphInstance
  {
    xiiAnimGraphResourceHandle         m_hAnimGraph;
    xiiUniquePtr<xiiAnimGraphInstance> m_pInstance;
  };

  xiiHybridArray<GraphInstance, 2> m_Instances;

  AnimClipInfo                                m_InvalidClipInfo;
  xiiHashTable<xiiHashedString, AnimClipInfo> m_AnimationClipMapping;

private:
  friend class xiiAnimGraphTriggerOutputPin;
  friend class xiiAnimGraphTriggerInputPin;
  friend class xiiAnimGraphBoneWeightsInputPin;
  friend class xiiAnimGraphBoneWeightsOutputPin;
  friend class xiiAnimGraphLocalPoseInputPin;
  friend class xiiAnimGraphLocalPoseOutputPin;
  friend class xiiAnimGraphModelPoseInputPin;
  friend class xiiAnimGraphModelPoseOutputPin;
  friend class xiiAnimGraphLocalPoseMultiInputPin;
  friend class xiiAnimGraphNumberInputPin;
  friend class xiiAnimGraphNumberOutputPin;
  friend class xiiAnimGraphBoolInputPin;
  friend class xiiAnimGraphBoolOutputPin;

  xiiHybridArray<xiiAnimGraphPinDataBoneWeights, 4>     m_PinDataBoneWeights;
  xiiHybridArray<xiiAnimGraphPinDataLocalTransforms, 4> m_PinDataLocalTransforms;
  xiiHybridArray<xiiAnimGraphPinDataModelTransforms, 2> m_PinDataModelTransforms;
};
