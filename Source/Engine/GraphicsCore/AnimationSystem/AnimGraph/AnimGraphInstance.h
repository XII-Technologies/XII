#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class xiiGameObject;
class xiiAnimGraph;
class xiiAnimController;

class XII_GRAPHICSCORE_DLL xiiAnimGraphInstance
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiAnimGraphInstance);

public:
  xiiAnimGraphInstance();
  ~xiiAnimGraphInstance();

  void Configure(const xiiAnimGraph& animGraph);

  void Update(xiiAnimController& ref_controller, xiiTime diff, xiiGameObject* pTarget, const xiiSkeletonResource* pSekeltonResource);

  template <typename T>
  T* GetAnimNodeInstanceData(const xiiAnimGraphNode& node)
  {
    return reinterpret_cast<T*>(xiiInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), node.m_uiInstanceDataOffset));
  }


private:
  const xiiAnimGraph* m_pAnimGraph = nullptr;

  xiiBlob m_InstanceData;

  // EXTEND THIS if a new type is introduced
  xiiInt8*                                      m_pTriggerInputPinStates    = nullptr;
  double*                                       m_pNumberInputPinStates     = nullptr;
  bool*                                         m_pBoolInputPinStates       = nullptr;
  xiiUInt16*                                    m_pBoneWeightInputPinStates = nullptr;
  xiiDynamicArray<xiiHybridArray<xiiUInt16, 1>> m_LocalPoseInputPinStates;
  xiiUInt16*                                    m_pModelPoseInputPinStates = nullptr;

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
};
