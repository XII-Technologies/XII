#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>

class XII_GRAPHICSCORE_DLL xiiSampleFrameAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSampleFrameAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSampleFrameAnimNode

public:
  void        SetClip(const char* szClip);
  const char* GetClip() const;

  xiiHashedString m_sClip;                            // [ property ]
  float           m_fNormalizedSamplePosition = 0.0f; // [ property ]

private:
  xiiAnimGraphNumberInputPin     m_InNormalizedSamplePosition; // [ property ]
  xiiAnimGraphNumberInputPin     m_InAbsoluteSamplePosition;   // [ property ]
  xiiAnimGraphLocalPoseOutputPin m_OutPose;                    // [ property ]
};
