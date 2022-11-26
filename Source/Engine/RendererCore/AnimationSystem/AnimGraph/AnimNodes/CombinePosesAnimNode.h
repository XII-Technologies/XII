#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_RENDERERCORE_DLL xiiCombinePosesAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCombinePosesAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLocalToModelPoseAnimNode

public:
  xiiCombinePosesAnimNode();
  ~xiiCombinePosesAnimNode();

  xiiUInt8 m_uiMaxPoses = 8; // [ property ]

private:
  xiiAnimGraphLocalPoseMultiInputPin m_LocalPosesPin; // [ property ]
  xiiAnimGraphLocalPoseOutputPin     m_LocalPosePin;  // [ property ]

  xiiDynamicArray<ozz::math::SimdFloat4, xiiAlignedAllocatorWrapper> m_BlendMask;
};
