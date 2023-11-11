#pragma once

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_RENDERERCORE_DLL xiiSwitchBoneWeightsAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSwitchBoneWeightsAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSwitchBoneWeightsAnimNode

private:
  xiiAnimGraphNumberInputPin                         m_InIndex;            // [ property ]
  xiiUInt8                                           m_uiWeightsCount = 0; // [ property ]
  xiiHybridArray<xiiAnimGraphBoneWeightsInputPin, 2> m_InWeights;          // [ property ]
  xiiAnimGraphBoneWeightsOutputPin                   m_OutWeights;         // [ property ]
};
