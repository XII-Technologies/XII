#pragma once

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_RENDERERCORE_DLL xiiLerpPosesAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLerpPosesAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLerpPosesAnimNode

public:
  xiiLerpPosesAnimNode();
  ~xiiLerpPosesAnimNode();

  float m_fLerp = 0.5f; // [ property ]

private:
  xiiUInt8                                         m_uiPosesCount = 0; // [ property ]
  xiiHybridArray<xiiAnimGraphLocalPoseInputPin, 2> m_InPoses;          // [ property ]
  xiiAnimGraphNumberInputPin                       m_InLerp;           // [ property ]
  xiiAnimGraphLocalPoseOutputPin                   m_OutPose;          // [ property ]
};
