#pragma once

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_GRAPHICSCORE_DLL xiiSwitchPoseAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSwitchPoseAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSelectPoseAnimNode

private:
  xiiTime                                          m_TransitionDuration = xiiTime::Milliseconds(200); // [ property ]
  xiiUInt8                                         m_uiPosesCount       = 0;                          // [ property ]
  xiiHybridArray<xiiAnimGraphLocalPoseInputPin, 4> m_InPoses;                                         // [ property ]
  xiiAnimGraphNumberInputPin                       m_InIndex;                                         // [ property ]
  xiiAnimGraphLocalPoseOutputPin                   m_OutPose;                                         // [ property ]

  struct InstanceData
  {
    xiiTime m_TransitionTime;
    xiiInt8 m_iTransitionFromIndex = -1;
    xiiInt8 m_iTransitionToIndex   = -1;
  };
};
