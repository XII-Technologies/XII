#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>

struct XII_RENDERERCORE_DLL xiiAnimationClip1D
{
  xiiHashedString m_sClip;
  float           m_fPosition = 0.0f;
  float           m_fSpeed    = 1.0f;

  void        SetAnimationFile(const char* szFile);
  const char* GetAnimationFile() const;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERCORE_DLL, xiiAnimationClip1D);

class XII_RENDERERCORE_DLL xiiSampleBlendSpace1DAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSampleBlendSpace1DAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSampleBlendSpace1DAnimNode

public:
  xiiSampleBlendSpace1DAnimNode();
  ~xiiSampleBlendSpace1DAnimNode();

private:
  xiiHybridArray<xiiAnimationClip1D, 4> m_Clips;                    // [ property ]
  bool                                  m_bLoop            = true;  // [ property ]
  bool                                  m_bApplyRootMotion = false; // [ property ]
  float                                 m_fPlaybackSpeed   = 1.0f;  // [ property ]

  xiiAnimGraphTriggerInputPin    m_InStart;       // [ property ]
  xiiAnimGraphBoolInputPin       m_InLoop;        // [ property ]
  xiiAnimGraphNumberInputPin     m_InSpeed;       // [ property ]
  xiiAnimGraphNumberInputPin     m_InLerp;        // [ property ]
  xiiAnimGraphLocalPoseOutputPin m_OutPose;       // [ property ]
  xiiAnimGraphTriggerOutputPin   m_OutOnStarted;  // [ property ]
  xiiAnimGraphTriggerOutputPin   m_OutOnFinished; // [ property ]


  struct InstanceState
  {
    xiiTime m_PlaybackTime;
    bool    m_bPlaying = false;
  };
};
