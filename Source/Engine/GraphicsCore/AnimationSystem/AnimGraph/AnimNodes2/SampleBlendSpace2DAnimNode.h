#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimController.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>

struct XII_GRAPHICSCORE_DLL xiiAnimationClip2D
{
  xiiHashedString m_sClip;
  xiiVec2         m_vPosition;

  void        SetAnimationFile(const char* szFile);
  const char* GetAnimationFile() const;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiAnimationClip2D);

class XII_GRAPHICSCORE_DLL xiiSampleBlendSpace2DAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSampleBlendSpace2DAnimNode, xiiAnimGraphNode);

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
  xiiSampleBlendSpace2DAnimNode();
  ~xiiSampleBlendSpace2DAnimNode();

  void        SetCenterClipFile(const char* szFile);
  const char* GetCenterClipFile() const;

private:
  xiiHashedString                       m_sCenterClip;                                            // [ property ]
  xiiHybridArray<xiiAnimationClip2D, 8> m_Clips;                                                  // [ property ]
  xiiTime                               m_InputResponse     = xiiTime::MakeFromMilliseconds(100); // [ property ]
  bool                                  m_bLoop             = true;                               // [ property ]
  float                                 m_fRootMotionAmount = 0.0f;                               // [ property ]
  float                                 m_fPlaybackSpeed    = 1.0f;                               // [ property ]

  xiiAnimGraphTriggerInputPin    m_InStart;       // [ property ]
  xiiAnimGraphBoolInputPin       m_InLoop;        // [ property ]
  xiiAnimGraphNumberInputPin     m_InSpeed;       // [ property ]
  xiiAnimGraphNumberInputPin     m_InCoordX;      // [ property ]
  xiiAnimGraphNumberInputPin     m_InCoordY;      // [ property ]
  xiiAnimGraphLocalPoseOutputPin m_OutPose;       // [ property ]
  xiiAnimGraphTriggerOutputPin   m_OutOnStarted;  // [ property ]
  xiiAnimGraphTriggerOutputPin   m_OutOnFinished; // [ property ]

  struct ClipToPlay
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32                              m_uiIndex;
    float                                  m_fWeight   = 1.0f;
    const xiiAnimController::AnimClipInfo* m_pClipInfo = nullptr;
  };

  struct InstanceState
  {
    xiiTime m_CenterPlaybackTime    = xiiTime::MakeFromHours(1000);
    float   m_fOtherPlaybackPosNorm = 0.0f;
    float   m_fLastValueX           = 0.0f;
    float   m_fLastValueY           = 0.0f;
  };

  void UpdateCenterClipPlaybackTime(const xiiAnimController::AnimClipInfo& centerInfo, InstanceState* pState, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, xiiAnimPoseEventTrackSampleMode& out_eventSamplingCenter) const;
  void PlayClips(xiiAnimController& ref_controller, const xiiAnimController::AnimClipInfo& centerInfo, InstanceState* pState, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, xiiArrayPtr<ClipToPlay> clips, xiiUInt32 uiMaxWeightClip) const;
  void ComputeClipsAndWeights(xiiAnimController& ref_controller, const xiiAnimController::AnimClipInfo& centerInfo, const xiiVec2& p, xiiDynamicArray<ClipToPlay>& out_Clips, xiiUInt32& out_uiMaxWeightClip) const;
};
