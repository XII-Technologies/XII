#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>

class XII_GRAPHICSCORE_DLL xiiSampleAnimClipSequenceAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSampleAnimClipSequenceAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSampleAnimClipSequenceAnimNode

public:
  xiiSampleAnimClipSequenceAnimNode();
  ~xiiSampleAnimClipSequenceAnimNode();

  void        SetStartClip(const char* szClip);
  const char* GetStartClip() const;

  xiiUInt32   Clips_GetCount() const;                                 // [ property ]
  const char* Clips_GetValue(xiiUInt32 uiIndex) const;                // [ property ]
  void        Clips_SetValue(xiiUInt32 uiIndex, const char* szValue); // [ property ]
  void        Clips_Insert(xiiUInt32 uiIndex, const char* szValue);   // [ property ]
  void        Clips_Remove(xiiUInt32 uiIndex);                        // [ property ]

  void        SetEndClip(const char* szClip);
  const char* GetEndClip() const;

private:
  xiiHashedString                    m_sStartClip;               // [ property ]
  xiiHybridArray<xiiHashedString, 1> m_Clips;                    // [ property ]
  xiiHashedString                    m_sEndClip;                 // [ property ]
  bool                               m_bApplyRootMotion = false; // [ property ]
  bool                               m_bLoop            = false; // [ property ]
  float                              m_fPlaybackSpeed   = 1.0f;  // [ property ]

  xiiAnimGraphTriggerInputPin m_InStart;      // [ property ]
  xiiAnimGraphBoolInputPin    m_InLoop;       // [ property ]
  xiiAnimGraphNumberInputPin  m_InSpeed;      // [ property ]
  xiiAnimGraphNumberInputPin  m_ClipIndexPin; // [ property ]

  xiiAnimGraphLocalPoseOutputPin m_OutPose;            // [ property ]
  xiiAnimGraphTriggerOutputPin   m_OutOnMiddleStarted; // [ property ]
  xiiAnimGraphTriggerOutputPin   m_OutOnEndStarted;    // [ property ]
  xiiAnimGraphTriggerOutputPin   m_OutOnFinished;      // [ property ]

  struct InstanceState
  {
    xiiTime  m_PlaybackTime;
    xiiUInt8 m_uiState         = 0; // 0 = off, 1 = start, 2 = middle, 3 = end
    xiiUInt8 m_uiMiddleClipIdx = 0;
  };
};
