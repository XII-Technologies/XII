/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphResource.h>

void xiiAnimGraphInstance::ResetPlayback()
{
  m_Time              = xiiTime::MakeZero();
  m_bHasGraphRevision = false;
  m_StateMachineStates.Clear();
  m_StateMachinePreviousStates.Clear();
  m_StateMachineTransitionStarts.Clear();
  m_StateMachineTransitionDurations.Clear();
}

void xiiAnimGraphInstance::Reset()
{
  ResetPlayback();
  m_Floats.Clear();
  m_Bools.Clear();
}

void xiiAnimGraphInstance::SetGraph(const xiiAnimGraphResourceHandle& hGraph)
{
  if (m_hGraph == hGraph)
    return;

  m_hGraph = hGraph;
  ResetPlayback();
}

const xiiAnimGraphResourceHandle& xiiAnimGraphInstance::GetGraph() const
{
  return m_hGraph;
}

void xiiAnimGraphInstance::SetFloat(xiiStringView sName, float fValue)
{
  xiiHashedString sHashed;
  sHashed.Assign(sName);
  m_Floats.Insert(sHashed, fValue);
}

float xiiAnimGraphInstance::GetFloat(const xiiTempHashedString& sName, float fFallback) const
{
  float fValue = fFallback;
  m_Floats.TryGetValue(sName, fValue);
  return fValue;
}

void xiiAnimGraphInstance::SetBool(xiiStringView sName, bool bValue)
{
  xiiHashedString sHashed;
  sHashed.Assign(sName);
  m_Bools.Insert(sHashed, bValue);
}

bool xiiAnimGraphInstance::GetBool(const xiiTempHashedString& sName, bool bFallback) const
{
  bool bValue = bFallback;
  m_Bools.TryGetValue(sName, bValue);
  return bValue;
}

void xiiAnimGraphInstance::Update(const xiiSkeletonResource& skeleton, xiiTime deltaTime, xiiAnimationPose& ref_pose)
{
  if (!m_hGraph.IsValid())
  {
    ref_pose.ResetToRestPose(skeleton);
    ref_pose.BuildModelSpacePose(skeleton);
    ref_pose.BuildSkinningMatrices(skeleton);
    return;
  }

  xiiResourceLock<xiiAnimGraphResource> pGraph(m_hGraph, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!pGraph || pGraph.GetAcquireResult() != xiiResourceAcquireResult::Final)
  {
    ResetPlayback();
    ref_pose.ResetToRestPose(skeleton);
    ref_pose.BuildModelSpacePose(skeleton);
    ref_pose.BuildSkinningMatrices(skeleton);
    return;
  }

  // State is indexed by descriptor node indices. Even an in-range index can
  // identify a different state after reload, so discard all playback bookkeeping.
  // Parameters belong to the instance and survive resource revisions.
  const xiiUInt32 revision = pGraph->GetCurrentResourceChangeCounter();
  if (!m_bHasGraphRevision || m_uiGraphChangeCounter != revision)
  {
    ResetPlayback();
    m_uiGraphChangeCounter = revision;
    m_bHasGraphRevision    = true;
  }
  m_Time += deltaTime;

  const xiiAnimGraphResourceDescriptor& graph = pGraph->GetDescriptor();
  EvaluateNodeIndex(graph, graph.m_uiOutputNode, skeleton, ref_pose);
  ref_pose.BuildModelSpacePose(skeleton);
  ref_pose.BuildSkinningMatrices(skeleton);
}

void xiiAnimGraphInstance::EvaluateNodeIndex(const xiiAnimGraphResourceDescriptor& graph, xiiUInt16 uiNodeIndex, const xiiSkeletonResource& skeleton, xiiAnimationPose& ref_pose)
{
  if (uiNodeIndex >= graph.m_Nodes.GetCount())
  {
    ref_pose.ResetToRestPose(skeleton);
    return;
  }

  EvaluateNode(graph, uiNodeIndex, graph.m_Nodes[uiNodeIndex], skeleton, ref_pose);
}

bool xiiAnimGraphInstance::IsTransitionConditionTrue(const xiiAnimGraphTransition& transition) const
{
  if (transition.m_sConditionParameter.IsEmpty())
    return true;

  if (GetBool(transition.m_sConditionParameter, false))
    return true;

  return GetFloat(transition.m_sConditionParameter, 0.0f) >= transition.m_fConditionThreshold;
}

xiiUInt16 xiiAnimGraphInstance::GetActiveStateNode(const xiiAnimGraphResourceDescriptor& graph, xiiUInt16 uiStateMachineNodeIndex, const xiiAnimGraphNode& stateMachineNode)
{
  if (stateMachineNode.m_Inputs.IsEmpty())
    return xiiMath::MaxValue<xiiUInt16>();

  xiiUInt16 uiActiveNode = stateMachineNode.m_Inputs[0];
  m_StateMachineStates.TryGetValue(uiStateMachineNodeIndex, uiActiveNode);

  bool bActiveIsValidInput = false;
  for (xiiUInt16 uiInput : stateMachineNode.m_Inputs)
  {
    if (uiInput == uiActiveNode)
    {
      bActiveIsValidInput = true;
      break;
    }
  }

  if (!bActiveIsValidInput)
  {
    uiActiveNode = stateMachineNode.m_Inputs[0];
  }

  for (const xiiAnimGraphTransition& transition : graph.m_Transitions)
  {
    if (transition.m_uiFromNode != uiActiveNode)
      continue;

    bool bToStateBelongsToMachine = false;
    for (xiiUInt16 uiInput : stateMachineNode.m_Inputs)
    {
      if (uiInput == transition.m_uiToNode)
      {
        bToStateBelongsToMachine = true;
        break;
      }
    }

    if (bToStateBelongsToMachine && IsTransitionConditionTrue(transition))
    {
      if (uiActiveNode != transition.m_uiToNode)
      {
        m_StateMachinePreviousStates.Insert(uiStateMachineNodeIndex, uiActiveNode);
        m_StateMachineTransitionStarts.Insert(uiStateMachineNodeIndex, m_Time);
        m_StateMachineTransitionDurations.Insert(uiStateMachineNodeIndex, transition.m_BlendDuration);
      }
      uiActiveNode = transition.m_uiToNode;
      break;
    }
  }

  m_StateMachineStates.Insert(uiStateMachineNodeIndex, uiActiveNode);
  return uiActiveNode;
}

void xiiAnimGraphInstance::EvaluateNode(const xiiAnimGraphResourceDescriptor& graph, xiiUInt16 uiNodeIndex, const xiiAnimGraphNode& node, const xiiSkeletonResource& skeleton, xiiAnimationPose& ref_pose)
{
  switch (node.m_Type.GetValue())
  {
    case xiiAnimGraphNodeType::Output:
    {
      if (!node.m_Inputs.IsEmpty())
      {
        EvaluateNodeIndex(graph, node.m_Inputs[0], skeleton, ref_pose);
      }
      else
      {
        ref_pose.ResetToRestPose(skeleton);
      }
    }
    break;

    case xiiAnimGraphNodeType::Clip:
    {
      if (node.m_hClip.IsValid())
      {
        xiiResourceLock<xiiAnimationClipResource> pClip(node.m_hClip, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
        if (pClip)
        {
          pClip->SampleLocalPose(skeleton, xiiTime::MakeFromSeconds(m_Time.GetSeconds() * node.m_fPlaybackSpeed), ref_pose);
          return;
        }
      }

      ref_pose.ResetToRestPose(skeleton);
    }
    break;

    case xiiAnimGraphNodeType::Blend1D:
    {
      if (node.m_Inputs.IsEmpty())
      {
        ref_pose.ResetToRestPose(skeleton);
        break;
      }

      const float fParameter = GetFloat(node.m_sParameter, 0.0f);
      xiiUInt32   uiUpper    = 0U;
      while (uiUpper + 1U < node.m_Inputs.GetCount() && uiUpper + 1U < node.m_InputThresholds.GetCount() && node.m_InputThresholds[uiUpper + 1U] < fParameter)
      {
        ++uiUpper;
      }

      const xiiUInt32 uiLower = uiUpper;
      uiUpper                 = xiiMath::Min(uiLower + 1U, node.m_Inputs.GetCount() - 1U);

      xiiAnimationPose lowerPose;
      xiiAnimationPose upperPose;
      EvaluateNodeIndex(graph, node.m_Inputs[uiLower], skeleton, lowerPose);

      if (uiUpper == uiLower)
      {
        ref_pose = std::move(lowerPose);
        break;
      }

      EvaluateNodeIndex(graph, node.m_Inputs[uiUpper], skeleton, upperPose);

      const float fLowerThreshold = uiLower < node.m_InputThresholds.GetCount() ? node.m_InputThresholds[uiLower] : 0.0f;
      const float fUpperThreshold = uiUpper < node.m_InputThresholds.GetCount() ? node.m_InputThresholds[uiUpper] : 1.0f;
      const float fWeight         = fUpperThreshold > fLowerThreshold ? (fParameter - fLowerThreshold) / (fUpperThreshold - fLowerThreshold) : 0.0f;
      ref_pose.Blend(lowerPose, upperPose, fWeight);
    }
    break;

    case xiiAnimGraphNodeType::Additive:
    {
      if (node.m_Inputs.GetCount() < 2U)
      {
        ref_pose.ResetToRestPose(skeleton);
        break;
      }

      xiiAnimationPose basePose;
      xiiAnimationPose additivePose;
      EvaluateNodeIndex(graph, node.m_Inputs[0], skeleton, basePose);
      EvaluateNodeIndex(graph, node.m_Inputs[1], skeleton, additivePose);
      ref_pose.AdditiveBlend(basePose, additivePose, node.m_fWeight);
    }
    break;

    case xiiAnimGraphNodeType::LayeredBlend:
    {
      if (node.m_Inputs.GetCount() < 2U)
      {
        ref_pose.ResetToRestPose(skeleton);
        break;
      }

      xiiAnimationPose basePose;
      xiiAnimationPose layerPose;
      EvaluateNodeIndex(graph, node.m_Inputs[0], skeleton, basePose);
      EvaluateNodeIndex(graph, node.m_Inputs[1], skeleton, layerPose);

      const float fWeight = node.m_sParameter.IsEmpty() ? node.m_fWeight : GetFloat(node.m_sParameter, node.m_fWeight);
      ref_pose.LayeredBlend(basePose, layerPose, skeleton, node.m_uiTargetJoint, fWeight);
    }
    break;

    case xiiAnimGraphNodeType::StateMachine:
    {
      const xiiUInt16 uiActiveStateNode = GetActiveStateNode(graph, uiNodeIndex, node);
      if (uiActiveStateNode >= graph.m_Nodes.GetCount())
      {
        ref_pose.ResetToRestPose(skeleton);
        break;
      }

      xiiUInt16  uiPreviousStateNode = xiiMath::MaxValue<xiiUInt16>();
      xiiTime    transitionStart     = xiiTime::MakeZero();
      xiiTime    transitionDuration  = xiiTime::MakeZero();
      const bool bHasBlend           = m_StateMachinePreviousStates.TryGetValue(uiNodeIndex, uiPreviousStateNode) && m_StateMachineTransitionStarts.TryGetValue(uiNodeIndex, transitionStart) && m_StateMachineTransitionDurations.TryGetValue(uiNodeIndex, transitionDuration) && uiPreviousStateNode < graph.m_Nodes.GetCount() && transitionDuration.GetSeconds() > 0.0;

      if (bHasBlend)
      {
        const double fBlend = xiiMath::Clamp((m_Time - transitionStart).GetSeconds() / transitionDuration.GetSeconds(), 0.0, 1.0);
        if (fBlend < 1.0)
        {
          xiiAnimationPose previousPose;
          xiiAnimationPose activePose;
          EvaluateNodeIndex(graph, uiPreviousStateNode, skeleton, previousPose);
          EvaluateNodeIndex(graph, uiActiveStateNode, skeleton, activePose);
          ref_pose.Blend(previousPose, activePose, static_cast<float>(fBlend));
          break;
        }

        m_StateMachinePreviousStates.Remove(uiNodeIndex);
        m_StateMachineTransitionStarts.Remove(uiNodeIndex);
        m_StateMachineTransitionDurations.Remove(uiNodeIndex);
      }

      EvaluateNodeIndex(graph, uiActiveStateNode, skeleton, ref_pose);
    }
    break;

    default:
      ref_pose.ResetToRestPose(skeleton);
      break;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_Implementation_AnimGraphInstance);
