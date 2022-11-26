#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class xiiSkeletonResource;
class xiiStreamWriter;
class xiiStreamReader;

class XII_RENDERERCORE_DLL xiiBoneWeightsAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBoneWeightsAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiBoneWeightsAnimNode

public:
  xiiBoneWeightsAnimNode();
  ~xiiBoneWeightsAnimNode();

  float m_fWeight = 1.0f; // [ property ]

  xiiUInt32   RootBones_GetCount() const;                               // [ property ]
  const char* RootBones_GetValue(xiiUInt32 uiIndex) const;              // [ property ]
  void        RootBones_SetValue(xiiUInt32 uiIndex, const char* value); // [ property ]
  void        RootBones_Insert(xiiUInt32 uiIndex, const char* value);   // [ property ]
  void        RootBones_Remove(xiiUInt32 uiIndex);                      // [ property ]

private:
  xiiAnimGraphBoneWeightsOutputPin m_WeightsPin;        // [ property ]
  xiiAnimGraphBoneWeightsOutputPin m_InverseWeightsPin; // [ property ]

  xiiHybridArray<xiiHashedString, 2> m_RootBones;

  xiiSharedPtr<xiiAnimGraphSharedBoneWeights> m_pSharedBoneWeights;
  xiiSharedPtr<xiiAnimGraphSharedBoneWeights> m_pSharedInverseBoneWeights;
};
