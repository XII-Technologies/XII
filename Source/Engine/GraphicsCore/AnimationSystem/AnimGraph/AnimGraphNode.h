#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Time/Time.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

class xiiSkeletonResource;
class xiiGameObject;
class xiiAnimGraphInstance;
class xiiAnimController;
class xiiStreamWriter;
class xiiStreamReader;
struct xiiAnimGraphPinDataLocalTransforms;
struct xiiAnimGraphPinDataBoneWeights;
class xiiAnimationClipResource;
struct xiiInstanceDataDesc;

using xiiAnimationClipResourceHandle = xiiTypedResourceHandle<class xiiAnimationClipResource>;

namespace ozz
{
  namespace animation
  {
    class Animation;
  }
} // namespace ozz

/// \brief Base class for all nodes in an xiiAnimGraphInstance
///
/// These nodes are used to configure which skeletal animations can be played on an object,
/// and how they would be played back exactly.
/// The nodes implement different functionality. For example logic nodes are used to figure out how to play an animation,
/// other nodes then sample and combining animation poses, and yet other nodes can inform the user about events
/// or they write state back to the animation graph's blackboard.
class XII_GRAPHICSCORE_DLL xiiAnimGraphNode : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphNode, xiiReflectedClass);

public:
  xiiAnimGraphNode();
  virtual ~xiiAnimGraphNode();

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

  const char* GetCustomNodeTitle() const { return m_sCustomNodeTitle.GetString(); }
  void        SetCustomNodeTitle(const char* szSz) { m_sCustomNodeTitle.Assign(szSz); }

protected:
  friend class xiiAnimGraphInstance;
  friend class xiiAnimGraph;
  friend class xiiAnimGraphResource;

  xiiHashedString m_sCustomNodeTitle;
  xiiUInt32       m_uiInstanceDataOffset = xiiInvalidIndex;

  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const = 0;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream)     = 0;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const = 0;
  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const { return false; }
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct XII_GRAPHICSCORE_DLL xiiAnimState
{
  enum class State
  {
    Off,
    StartedRampUp,
    RampingUp,
    Running,
    StartedRampDown,
    RampingDown,
    Finished,
  };

  // Properties:
  xiiTime m_FadeIn;                    // [ property ]
  xiiTime m_FadeOut;                   // [ property ]
  bool    m_bImmediateFadeIn  = false; // [ property ]
  bool    m_bImmediateFadeOut = false; // [ property ]
  bool    m_bLoop             = false; // [ property ]
  float   m_fPlaybackSpeed    = 1.0f;  // [ property ]
  bool    m_bApplyRootMotion  = false; // [ property ]

  // Inputs:
  bool    m_bTriggerActive       = false;
  float   m_fPlaybackSpeedFactor = 1.0f;
  xiiTime m_Duration;
  xiiTime m_DurationOfQueued;

  bool  WillStateBeOff(bool bTriggerActive) const;
  void  UpdateState(xiiTime diff);
  State GetCurrentState() const { return m_State; }
  float GetWeight() const { return m_fCurWeight; }
  float GetNormalizedPlaybackPosition() const { return m_fNormalizedPlaybackPosition; }
  bool  HasTransitioned() const { return m_bHasTransitioned; }
  bool  HasLoopedStart() const { return m_bHasLoopedStart; }
  bool  HasLoopedEnd() const { return m_bHasLoopedEnd; }
  float GetFinalSpeed() const { return m_fPlaybackSpeed * m_fPlaybackSpeedFactor; }

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

private:
  void RampWeightUpOrDown(float& inout_fWeight, float fTargetWeight, xiiTime tDiff) const;

  State m_State                       = State::Off;
  float m_fNormalizedPlaybackPosition = 0.0f;
  bool  m_bRequireLoopForRampDown     = true;
  bool  m_bHasTransitioned            = false;
  bool  m_bHasLoopedStart             = false;
  bool  m_bHasLoopedEnd               = false;
  float m_fCurWeight                  = 0.0f;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiAnimState);
