#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>

class XII_GRAPHICSCORE_DLL xiiSampleAnimClipAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSampleAnimClipAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSampleAnimClipAnimNode

  void        SetClip(const char* szClip);
  const char* GetClip() const;

public:
  xiiSampleAnimClipAnimNode();
  ~xiiSampleAnimClipAnimNode();

private:
  xiiHashedString m_sClip;                    // [ property ]
  bool            m_bLoop            = true;  // [ property ]
  bool            m_bApplyRootMotion = false; // [ property ]
  float           m_fPlaybackSpeed   = 1.0f;  // [ property ]

  xiiAnimGraphTriggerInputPin m_InStart; // [ property ]
  xiiAnimGraphBoolInputPin    m_InLoop;  // [ property ]
  xiiAnimGraphNumberInputPin  m_InSpeed; // [ property ]

  xiiAnimGraphLocalPoseOutputPin m_OutPose;       // [ property ]
  xiiAnimGraphTriggerOutputPin   m_OutOnStarted;  // [ property ]
  xiiAnimGraphTriggerOutputPin   m_OutOnFinished; // [ property ]

  struct InstanceState
  {
    bool    m_bPlaying = false;
    xiiTime m_PlaybackTime;
  };
};
