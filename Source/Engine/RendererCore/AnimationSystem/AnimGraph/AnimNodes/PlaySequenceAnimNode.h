#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_RENDERERCORE_DLL xiiPlaySequenceAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPlaySequenceAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiPlaySequenceAnimNode

public:
  void        SetStartClip(const char* szFile); // [ property ]
  const char* GetStartClip() const;             // [ property ]

  xiiUInt32   MiddleClips_GetCount() const;                               // [ property ]
  const char* MiddleClips_GetValue(xiiUInt32 uiIndex) const;              // [ property ]
  void        MiddleClips_SetValue(xiiUInt32 uiIndex, const char* value); // [ property ]
  void        MiddleClips_Insert(xiiUInt32 uiIndex, const char* value);   // [ property ]
  void        MiddleClips_Remove(xiiUInt32 uiIndex);                      // [ property ]

  void        SetEndClip(const char* szFile); // [ property ]
  const char* GetEndClip() const;             // [ property ]

  xiiAnimationClipResourceHandle                    m_hStartClip;
  xiiHybridArray<xiiAnimationClipResourceHandle, 2> m_hMiddleClips;
  xiiAnimationClipResourceHandle                    m_hEndClip;

private:
  xiiAnimGraphTriggerInputPin     m_ActivePin;           // [ property ]
  xiiAnimGraphBoneWeightsInputPin m_WeightsPin;          // [ property ]
  xiiAnimGraphNumberInputPin      m_SpeedPin;            // [ property ]
  xiiAnimGraphNumberInputPin      m_ClipIndexPin;        // [ property ]
  xiiAnimGraphLocalPoseOutputPin  m_LocalPosePin;        // [ property ]
  xiiAnimGraphTriggerOutputPin    m_OnNextClipPin;       // [ property ]
  xiiAnimGraphNumberOutputPin     m_PlayingClipIndexPin; // [ property ]
  xiiAnimGraphTriggerOutputPin    m_OnFadeOutPin;        // [ property ]

  xiiAnimState m_State;

  enum class Phase : xiiUInt8
  {
    Off,
    Start,
    Middle,
    End,
  };

  Phase    m_Phase            = Phase::Off;
  xiiUInt8 m_uiClipToPlay     = 0xFF;
  xiiUInt8 m_uiNextClipToPlay = 0xFF;
};
