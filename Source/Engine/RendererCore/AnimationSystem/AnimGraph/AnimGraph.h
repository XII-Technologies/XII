#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Types/SharedPtr.h>

class xiiGameObject;

using xiiSkeletonResourceHandle = xiiTypedResourceHandle<class xiiSkeletonResource>;

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
  xiiVec3                               m_vRootMotion    = xiiVec3::ZeroVector();
  bool                                  m_bUseRootMotion = false;
};

struct xiiAnimGraphPinDataModelTransforms
{
  xiiUInt16                     m_uiOwnIndex = 0xFFFF;
  xiiAnimPoseGeneratorCommandID m_CommandID;
  xiiVec3                       m_vRootMotion = xiiVec3::ZeroVector();
  xiiAngle                      m_RootRotationX;
  xiiAngle                      m_RootRotationY;
  xiiAngle                      m_RootRotationZ;
  bool                          m_bUseRootMotion = false;
};

class XII_RENDERERCORE_DLL xiiAnimGraph
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiAnimGraph);

public:
  xiiAnimGraph();
  ~xiiAnimGraph();

  void Configure(const xiiSkeletonResourceHandle& hSkeleton, xiiAnimPoseGenerator& poseGenerator, const xiiSharedPtr<xiiBlackboard>& pBlackboard = nullptr);

  void Update(xiiTime tDiff, xiiGameObject* pTarget);
  void GetRootMotion(xiiVec3& translation, xiiAngle& rotationX, xiiAngle& rotationY, xiiAngle& rotationZ) const;

  const xiiSharedPtr<xiiBlackboard>& GetBlackboard() { return m_pBlackboard; }

  xiiResult Serialize(xiiStreamWriter& stream) const;
  xiiResult Deserialize(xiiStreamReader& stream);

  xiiAnimPoseGenerator& GetPoseGenerator() { return *m_pPoseGenerator; }

  static xiiSharedPtr<xiiAnimGraphSharedBoneWeights> CreateBoneWeights(const char* szUniqueName, const xiiSkeletonResource& skeleton, xiiDelegate<void(xiiAnimGraphSharedBoneWeights&)> fill);

  xiiAnimGraphPinDataBoneWeights*     AddPinDataBoneWeights();
  xiiAnimGraphPinDataLocalTransforms* AddPinDataLocalTransforms();
  xiiAnimGraphPinDataModelTransforms* AddPinDataModelTransforms();

  void SetOutputModelTransform(xiiAnimGraphPinDataModelTransforms* pModelTransform);
  void SetRootMotion(const xiiVec3& translation, xiiAngle rotationX, xiiAngle rotationY, xiiAngle rotationZ);

private:
  xiiDynamicArray<xiiUniquePtr<xiiAnimGraphNode>> m_Nodes;
  xiiSkeletonResourceHandle                       m_hSkeleton;

  xiiDynamicArray<xiiDynamicArray<xiiUInt16>> m_OutputPinToInputPinMapping[xiiAnimGraphPin::ENUM_COUNT];

  // EXTEND THIS if a new type is introduced
  xiiDynamicArray<xiiInt8>                      m_TriggerInputPinStates;
  xiiDynamicArray<double>                       m_NumberInputPinStates;
  xiiDynamicArray<xiiUInt16>                    m_BoneWeightInputPinStates;
  xiiDynamicArray<xiiHybridArray<xiiUInt16, 1>> m_LocalPoseInputPinStates;
  xiiDynamicArray<xiiUInt16>                    m_ModelPoseInputPinStates;

  xiiAnimGraphPinDataModelTransforms* m_pCurrentModelTransforms = nullptr;

  xiiVec3  m_vRootMotion = xiiVec3::ZeroVector();
  xiiAngle m_RootRotationX;
  xiiAngle m_RootRotationY;
  xiiAngle m_RootRotationZ;

private:
  friend class xiiAnimationControllerAssetDocument;
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

  bool m_bInitialized = false;

  xiiAnimPoseGenerator*       m_pPoseGenerator = nullptr;
  xiiSharedPtr<xiiBlackboard> m_pBlackboard    = nullptr;

  xiiHybridArray<xiiAnimGraphPinDataBoneWeights, 4>     m_PinDataBoneWeights;
  xiiHybridArray<xiiAnimGraphPinDataLocalTransforms, 4> m_PinDataLocalTransforms;
  xiiHybridArray<xiiAnimGraphPinDataModelTransforms, 2> m_PinDataModelTransforms;

  static xiiMutex                                                             s_SharedDataMutex;
  static xiiHashTable<xiiString, xiiSharedPtr<xiiAnimGraphSharedBoneWeights>> s_SharedBoneWeights;
};
