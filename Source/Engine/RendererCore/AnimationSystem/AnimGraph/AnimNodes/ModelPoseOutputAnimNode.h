#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_RENDERERCORE_DLL xiiModelPoseOutputAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiModelPoseOutputAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiModelPoseOutputAnimNode

public:
  xiiModelPoseOutputAnimNode();
  ~xiiModelPoseOutputAnimNode();

private:
  xiiAnimGraphModelPoseInputPin m_ModelPosePin; // [ property ]
  xiiAnimGraphNumberInputPin    m_RotateZPin;   // [ property ]
};
