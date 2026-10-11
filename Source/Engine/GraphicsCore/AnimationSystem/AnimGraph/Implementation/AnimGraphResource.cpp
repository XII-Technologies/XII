/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Algorithm/HashStream.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiAnimGraphNodeType, 1)
  XII_ENUM_CONSTANTS(xiiAnimGraphNodeType::Invalid, xiiAnimGraphNodeType::Output, xiiAnimGraphNodeType::Clip, xiiAnimGraphNodeType::Blend1D)
  XII_ENUM_CONSTANTS(xiiAnimGraphNodeType::Additive, xiiAnimGraphNodeType::LayeredBlend, xiiAnimGraphNodeType::StateMachine, xiiAnimGraphNodeType::TwoBoneIK, xiiAnimGraphNodeType::AimIK)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiAnimGraphNodeFlags, 1)
  XII_BITFLAGS_CONSTANTS(xiiAnimGraphNodeFlags::Loop, xiiAnimGraphNodeFlags::Synchronize, xiiAnimGraphNodeFlags::RootMotion, xiiAnimGraphNodeFlags::Additive, xiiAnimGraphNodeFlags::WriteSkinning)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphResource, 1, xiiRTTIDefaultAllocator<xiiAnimGraphResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiAnimGraphResource);
// clang-format on

namespace
{
  static constexpr xiiUInt32 s_uiAnimGraphResourceVersion = 1U;
}

xiiResult xiiAnimGraphNode::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  inout_stream << m_Type;
  inout_stream << m_Flags;
  inout_stream << m_hClip;
  inout_stream << m_sParameter;
  inout_stream << m_fPlaybackSpeed;
  inout_stream << m_fWeight;
  inout_stream << m_fThreshold;
  inout_stream << m_uiTargetJoint;
  inout_stream.WriteArray(m_Inputs).IgnoreResult();
  inout_stream.WriteArray(m_InputThresholds).IgnoreResult();
  inout_stream.WriteArray(m_InputWeights).IgnoreResult();
  return XII_SUCCESS;
}

xiiResult xiiAnimGraphNode::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_sName;
  inout_stream >> m_Type;
  inout_stream >> m_Flags;
  inout_stream >> m_hClip;
  inout_stream >> m_sParameter;
  inout_stream >> m_fPlaybackSpeed;
  inout_stream >> m_fWeight;
  inout_stream >> m_fThreshold;
  inout_stream >> m_uiTargetJoint;
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Inputs));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_InputThresholds));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_InputWeights));
  return XII_SUCCESS;
}

xiiResult xiiAnimGraphTransition::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_uiFromNode;
  inout_stream << m_uiToNode;
  inout_stream << m_sConditionParameter;
  inout_stream << m_fConditionThreshold;
  inout_stream << m_BlendDuration;
  return XII_SUCCESS;
}

xiiResult xiiAnimGraphTransition::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_uiFromNode;
  inout_stream >> m_uiToNode;
  inout_stream >> m_sConditionParameter;
  inout_stream >> m_fConditionThreshold;
  inout_stream >> m_BlendDuration;
  return XII_SUCCESS;
}

void xiiAnimGraphResourceDescriptor::Clear()
{
  m_Nodes.Clear();
  m_Transitions.Clear();
  m_uiOutputNode  = xiiMath::MaxValue<xiiUInt16>();
  m_uiRuntimeHash = 0U;
}

xiiUInt16 xiiAnimGraphResourceDescriptor::AddNode(const xiiAnimGraphNode& node)
{
  XII_ASSERT_DEV(m_Nodes.GetCount() < xiiMath::MaxValue<xiiUInt16>(), "Animation graphs support up to 65535 nodes.");
  m_Nodes.PushBack(node);
  return static_cast<xiiUInt16>(m_Nodes.GetCount() - 1U);
}

xiiUInt16 xiiAnimGraphResourceDescriptor::AddClipNode(xiiStringView sName, const xiiAnimationClipResourceHandle& hClip, float fSpeed)
{
  xiiAnimGraphNode node;
  node.m_sName.Assign(sName);
  node.m_Type           = xiiAnimGraphNodeType::Clip;
  node.m_hClip          = hClip;
  node.m_fPlaybackSpeed = fSpeed;
  node.m_Flags.Add(xiiAnimGraphNodeFlags::Loop);
  return AddNode(node);
}

xiiUInt16 xiiAnimGraphResourceDescriptor::AddBlend1DNode(xiiStringView sName, xiiStringView sParameter, xiiArrayPtr<const xiiUInt16> inputs, xiiArrayPtr<const float> thresholds)
{
  xiiAnimGraphNode node;
  node.m_sName.Assign(sName);
  node.m_Type = xiiAnimGraphNodeType::Blend1D;
  node.m_sParameter.Assign(sParameter);
  node.m_Inputs.SetCount(inputs.GetCount());
  for (xiiUInt32 i = 0; i < inputs.GetCount(); ++i)
  {
    node.m_Inputs[i] = inputs[i];
  }
  node.m_InputThresholds.SetCount(thresholds.GetCount());
  for (xiiUInt32 i = 0; i < thresholds.GetCount(); ++i)
  {
    node.m_InputThresholds[i] = thresholds[i];
  }
  return AddNode(node);
}

xiiUInt16 xiiAnimGraphResourceDescriptor::AddLayeredBlendNode(xiiStringView sName, xiiUInt16 uiBaseNode, xiiUInt16 uiLayerNode, xiiUInt16 uiRootJoint, xiiStringView sWeightParameter, float fWeight)
{
  xiiAnimGraphNode node;
  node.m_sName.Assign(sName);
  node.m_Type          = xiiAnimGraphNodeType::LayeredBlend;
  node.m_uiTargetJoint = uiRootJoint;
  node.m_fWeight       = fWeight;
  node.m_sParameter.Assign(sWeightParameter);
  node.m_Inputs.PushBack(uiBaseNode);
  node.m_Inputs.PushBack(uiLayerNode);
  return AddNode(node);
}

xiiUInt16 xiiAnimGraphResourceDescriptor::AddStateMachineNode(xiiStringView sName, xiiArrayPtr<const xiiUInt16> states)
{
  xiiAnimGraphNode node;
  node.m_sName.Assign(sName);
  node.m_Type = xiiAnimGraphNodeType::StateMachine;
  node.m_Inputs.SetCount(states.GetCount());
  for (xiiUInt32 i = 0; i < states.GetCount(); ++i)
  {
    node.m_Inputs[i] = states[i];
  }
  return AddNode(node);
}

void xiiAnimGraphResourceDescriptor::AddTransition(xiiUInt16 uiFromNode, xiiUInt16 uiToNode, xiiStringView sConditionParameter, float fThreshold, xiiTime blendDuration)
{
  xiiAnimGraphTransition& transition = m_Transitions.ExpandAndGetRef();
  transition.m_uiFromNode            = uiFromNode;
  transition.m_uiToNode              = uiToNode;
  transition.m_sConditionParameter.Assign(sConditionParameter);
  transition.m_fConditionThreshold = fThreshold;
  transition.m_BlendDuration       = blendDuration;
}

void xiiAnimGraphResourceDescriptor::SetOutputNode(xiiUInt16 uiNodeIndex)
{
  m_uiOutputNode = uiNodeIndex;
}

void xiiAnimGraphResourceDescriptor::ComputeRuntimeHash()
{
  xiiHashStreamWriter32 hashWriter;
  hashWriter << m_uiOutputNode;
  hashWriter << m_Nodes.GetCount();
  for (const xiiAnimGraphNode& node : m_Nodes)
  {
    hashWriter << node.m_sName.GetHash();
    hashWriter << node.m_Type.GetValue();
    hashWriter << node.m_Flags.GetValue();
    hashWriter << (node.m_hClip.IsValid() ? node.m_hClip.GetResourceIDHash() : 0ULL);
    hashWriter << node.m_sParameter.GetHash();
    hashWriter << node.m_fPlaybackSpeed;
    hashWriter << node.m_fWeight;
    hashWriter << node.m_fThreshold;
    hashWriter << node.m_uiTargetJoint;
    hashWriter << node.m_Inputs.GetCount();
    for (xiiUInt16 uiInput : node.m_Inputs)
    {
      hashWriter << uiInput;
    }
    for (float fThreshold : node.m_InputThresholds)
    {
      hashWriter << fThreshold;
    }
    for (float fInputWeight : node.m_InputWeights)
    {
      hashWriter << fInputWeight;
    }
  }
  hashWriter << m_Transitions.GetCount();
  for (const xiiAnimGraphTransition& transition : m_Transitions)
  {
    hashWriter << transition.m_uiFromNode;
    hashWriter << transition.m_uiToNode;
    hashWriter << transition.m_sConditionParameter.GetHash();
    hashWriter << transition.m_fConditionThreshold;
    hashWriter << transition.m_BlendDuration.GetSeconds();
  }
  m_uiRuntimeHash = hashWriter.GetHashValue();
}

xiiResult xiiAnimGraphResourceDescriptor::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << s_uiAnimGraphResourceVersion;
  inout_stream.WriteArray(m_Nodes).IgnoreResult();
  inout_stream.WriteArray(m_Transitions).IgnoreResult();
  inout_stream << m_uiOutputNode;
  inout_stream << m_uiRuntimeHash;
  return XII_SUCCESS;
}

xiiResult xiiAnimGraphResourceDescriptor::Deserialize(xiiStreamReader& inout_stream)
{
  xiiUInt32 uiVersion = 0U;
  inout_stream >> uiVersion;
  XII_IGNORE_UNUSED(uiVersion);

  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Nodes));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Transitions));
  inout_stream >> m_uiOutputNode;
  inout_stream >> m_uiRuntimeHash;
  return XII_SUCCESS;
}

xiiAnimGraphResource::xiiAnimGraphResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiAnimGraphResource::~xiiAnimGraphResource() = default;

const xiiAnimGraphResourceDescriptor& xiiAnimGraphResource::GetDescriptor() const
{
  return m_Descriptor;
}

xiiUInt32 xiiAnimGraphResource::GetRuntimeHash() const
{
  return m_Descriptor.m_uiRuntimeHash;
}

xiiResourceLoadDescription xiiAnimGraphResource::UnloadData(Unload whatToUnload)
{
  XII_IGNORE_UNUSED(whatToUnload);

  m_Descriptor.Clear();

  xiiResourceLoadDescription res;
  res.m_State                      = xiiResourceState::Unloaded;
  res.m_uiQualityLevelsDiscardable = 0U;
  res.m_uiQualityLevelsLoadable    = 0U;
  return res;
}

xiiResourceLoadDescription xiiAnimGraphResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiResourceLoadDescription res;
  res.m_State                      = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0U;
  res.m_uiQualityLevelsLoadable    = 0U;

  if (pStream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  xiiAnimGraphResourceDescriptor descriptor;
  if (descriptor.Deserialize(*pStream).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  CreateResource(std::move(descriptor));
  return res;
}

void xiiAnimGraphResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiAnimGraphResource) + static_cast<xiiUInt32>(m_Descriptor.m_Nodes.GetHeapMemoryUsage() + m_Descriptor.m_Transitions.GetHeapMemoryUsage());
  out_NewMemoryUsage.m_uiMemoryGPU = 0U;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiAnimGraphResource, xiiAnimGraphResourceDescriptor)
{
  descriptor.ComputeRuntimeHash();
  m_Descriptor = std::move(descriptor);

  xiiResourceLoadDescription res;
  res.m_State                      = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0U;
  res.m_uiQualityLevelsLoadable    = 0U;
  return res;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_Implementation_AnimGraphResource);
