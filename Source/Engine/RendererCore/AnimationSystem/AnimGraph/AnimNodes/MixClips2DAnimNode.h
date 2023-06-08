#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>

struct XII_RENDERERCORE_DLL xiiAnimClip2D
{
  xiiAnimationClipResourceHandle m_hAnimation;
  xiiVec2                        m_vPosition;

  void        SetAnimationFile(const char* szSz);
  const char* GetAnimationFile() const;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERCORE_DLL, xiiAnimClip2D);

class XII_RENDERERCORE_DLL xiiMixClips2DAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMixClips2DAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiMixClips2DAnimNode

public:
  void        SetCenterClipFile(const char* szSz);
  const char* GetCenterClipFile() const;

  xiiAnimationClipResourceHandle   m_hCenterClip;   // [ property ]
  xiiHybridArray<xiiAnimClip2D, 8> m_Clips;         // [ property ]
  xiiTime                          m_InputResponse; // [ property ]

private:
  xiiAnimGraphTriggerInputPin     m_ActivePin;    // [ property ]
  xiiAnimGraphBoneWeightsInputPin m_WeightsPin;   // [ property ]
  xiiAnimGraphNumberInputPin      m_SpeedPin;     // [ property ]
  xiiAnimGraphNumberInputPin      m_XCoordPin;    // [ property ]
  xiiAnimGraphNumberInputPin      m_YCoordPin;    // [ property ]
  xiiAnimGraphLocalPoseOutputPin  m_LocalPosePin; // [ property ]
  xiiAnimGraphTriggerOutputPin    m_OnFadeOutPin; // [ property ]

  struct ClipToPlay
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiIndex;
    float     m_fWeight = 1.0f;
  };

  void UpdateCenterClipPlaybackTime(xiiAnimGraph& graph, xiiTime tDiff, xiiAnimPoseEventTrackSampleMode& out_eventSamplingCenter);
  void PlayClips(xiiAnimGraph& graph, xiiTime tDiff, xiiArrayPtr<ClipToPlay> clips, xiiUInt32 uiMaxWeightClip);
  void ComputeClipsAndWeights(const xiiVec2& p, xiiDynamicArray<ClipToPlay>& out_Clips, xiiUInt32& out_uiMaxWeightClip);

  xiiTime      m_CenterPlaybackTime;
  xiiAnimState m_State; // [ property ]

  float m_fLastValueX = 0.0f;
  float m_fLastValueY = 0.0f;
};
