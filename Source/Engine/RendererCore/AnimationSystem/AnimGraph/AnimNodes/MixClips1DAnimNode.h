#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

struct XII_RENDERERCORE_DLL xiiAnimClip1D
{
  xiiAnimationClipResourceHandle m_hAnimation;
  float                          m_fPosition;

  void        SetAnimationFile(const char* szSz);
  const char* GetAnimationFile() const;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERCORE_DLL, xiiAnimClip1D);

class XII_RENDERERCORE_DLL xiiMixClips1DAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMixClips1DAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiMixClips1DAnimNode

public:
  xiiHybridArray<xiiAnimClip1D, 3> m_Clips; // [ property ]

private:
  xiiAnimGraphTriggerInputPin     m_ActivePin;    // [ property ]
  xiiAnimGraphBoneWeightsInputPin m_WeightsPin;   // [ property ]
  xiiAnimGraphNumberInputPin      m_SpeedPin;     // [ property ]
  xiiAnimGraphNumberInputPin      m_LerpPin;      // [ property ]
  xiiAnimGraphLocalPoseOutputPin  m_LocalPosePin; // [ property ]
  xiiAnimGraphTriggerOutputPin    m_OnFadeOutPin; // [ property ]

  xiiAnimState m_State; // [ property ]
};
