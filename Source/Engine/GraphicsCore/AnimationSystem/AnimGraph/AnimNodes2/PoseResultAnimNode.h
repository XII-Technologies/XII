#pragma once

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_GRAPHICSCORE_DLL xiiPoseResultAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPoseResultAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiPoseResultAnimNode

public:
  xiiPoseResultAnimNode();
  ~xiiPoseResultAnimNode();

private:
  xiiTime m_FadeDuration = xiiTime::MakeFromMilliseconds(200); // [ property ]

  xiiAnimGraphLocalPoseInputPin   m_InPose;           // [ property ]
  xiiAnimGraphNumberInputPin      m_InTargetWeight;   // [ property ]
  xiiAnimGraphNumberInputPin      m_InFadeDuration;   // [ property ]
  xiiAnimGraphBoneWeightsInputPin m_InWeights;        // [ property ]
  xiiAnimGraphTriggerOutputPin    m_OutOnFadedOut;    // [ property ]
  xiiAnimGraphTriggerOutputPin    m_OutOnFadedIn;     // [ property ]
  xiiAnimGraphNumberOutputPin     m_OutCurrentWeight; // [ property ]

  struct InstanceData
  {
    float   m_fStartWeight = 1.0f;
    float   m_fEndWeight   = 1.0f;
    xiiTime m_PlayTime;
    xiiTime m_EndTime;
  };
};
