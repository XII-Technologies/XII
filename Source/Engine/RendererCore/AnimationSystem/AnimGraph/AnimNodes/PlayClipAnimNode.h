#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

/// \brief Plays a single animation clip, either once or looped
class XII_RENDERERCORE_DLL xiiPlayClipAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPlayClipAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiPlayClipAnimNode

public:
  xiiUInt32   Clips_GetCount() const;                               // [ property ]
  const char* Clips_GetValue(xiiUInt32 uiIndex) const;              // [ property ]
  void        Clips_SetValue(xiiUInt32 uiIndex, const char* value); // [ property ]
  void        Clips_Insert(xiiUInt32 uiIndex, const char* value);   // [ property ]
  void        Clips_Remove(xiiUInt32 uiIndex);                      // [ property ]

private:
  xiiHybridArray<xiiAnimationClipResourceHandle, 1> m_Clips; // [ property ]

  xiiAnimGraphTriggerInputPin     m_ActivePin;    // [ property ]
  xiiAnimGraphBoneWeightsInputPin m_WeightsPin;   // [ property ]
  xiiAnimGraphNumberInputPin      m_SpeedPin;     // [ property ]
  xiiAnimGraphNumberInputPin      m_ClipIndexPin; // [ property ]
  xiiAnimGraphLocalPoseOutputPin  m_LocalPosePin; // [ property ]
  xiiAnimGraphTriggerOutputPin    m_OnFadeOutPin; // [ property ]

  xiiAnimState m_State; // [ property ]
  xiiUInt8     m_uiClipToPlay     = 0xFF;
  xiiUInt8     m_uiNextClipToPlay = 0xFF;
  xiiTime      m_NextClipDuration;
};
