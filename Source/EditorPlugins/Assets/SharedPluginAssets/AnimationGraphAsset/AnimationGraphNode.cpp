/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <Foundation/Strings/StringBuilder.h>
#include <SharedPluginAssets/AnimationGraphAsset/AnimationGraphNode.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiAnimationGraphNodeKind, 1)
  XII_ENUM_CONSTANTS(xiiAnimationGraphNodeKind::Output, xiiAnimationGraphNodeKind::Clip, xiiAnimationGraphNodeKind::Blend1D, xiiAnimationGraphNodeKind::Additive, xiiAnimationGraphNodeKind::LayeredBlend, xiiAnimationGraphNodeKind::StateMachine)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationGraphEditorNode, 1, xiiRTTIDefaultAllocator<xiiAnimationGraphEditorNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_ENUM_MEMBER_PROPERTY("Kind", xiiAnimationGraphNodeKind, m_Kind),
    XII_MEMBER_PROPERTY("Clip", m_sClip)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    XII_MEMBER_PROPERTY("Parameter", m_sParameter),
    XII_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("TargetJoint", m_uiTargetJoint)->AddAttributes(new xiiDefaultValueAttribute(xiiMath::MaxValue<xiiUInt16>())),
    XII_MEMBER_PROPERTY("StateCount", m_uiStateCount)->AddAttributes(new xiiDefaultValueAttribute(2), new xiiClampValueAttribute(1, 65535)),
    XII_ARRAY_MEMBER_PROPERTY("BlendThresholds", m_BlendThresholds),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("{Kind} {Name}"),
    new xiiCategoryAttribute("Animation"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAnimationGraphEditorTransition, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiAnimationGraphEditorTransition>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("FromNode", m_sFromNode),
    XII_MEMBER_PROPERTY("ToNode", m_sToNode),
    XII_MEMBER_PROPERTY("ConditionParameter", m_sConditionParameter),
    XII_MEMBER_PROPERTY("ConditionThreshold", m_fConditionThreshold)->AddAttributes(new xiiDefaultValueAttribute(0.5f)),
    XII_MEMBER_PROPERTY("BlendDuration", m_BlendDuration)->AddAttributes(new xiiDefaultValueAttribute(xiiTime::MakeFromMilliseconds(150)), new xiiClampValueAttribute(xiiTime::MakeZero(), xiiVariant())),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiAnimGraphNodeType::Enum xiiAnimationGraphEditorNode::GetRuntimeType() const
{
  switch (m_Kind.GetValue())
  {
    case xiiAnimationGraphNodeKind::Output:
      return xiiAnimGraphNodeType::Output;
    case xiiAnimationGraphNodeKind::Clip:
      return xiiAnimGraphNodeType::Clip;
    case xiiAnimationGraphNodeKind::Blend1D:
      return xiiAnimGraphNodeType::Blend1D;
    case xiiAnimationGraphNodeKind::Additive:
      return xiiAnimGraphNodeType::Additive;
    case xiiAnimationGraphNodeKind::LayeredBlend:
      return xiiAnimGraphNodeType::LayeredBlend;
    case xiiAnimationGraphNodeKind::StateMachine:
      return xiiAnimGraphNodeType::StateMachine;
    default:
      return xiiAnimGraphNodeType::Invalid;
  }
}

xiiUInt32 xiiAnimationGraphEditorNode::GetInputCount() const
{
  switch (m_Kind.GetValue())
  {
    case xiiAnimationGraphNodeKind::Output:
      return 1;
    case xiiAnimationGraphNodeKind::Additive:
    case xiiAnimationGraphNodeKind::LayeredBlend:
      return 2;
    case xiiAnimationGraphNodeKind::Blend1D:
      return m_BlendThresholds.GetCount();
    case xiiAnimationGraphNodeKind::StateMachine:
      return m_uiStateCount;
    default:
      return 0;
  }
}

void xiiAnimationGraphEditorNode::GetInputName(xiiUInt32 uiInput, xiiStringBuilder& out_name) const
{
  if (m_Kind == xiiAnimationGraphNodeKind::Output)
    out_name = "Pose";
  else if (m_Kind == xiiAnimationGraphNodeKind::Additive)
    out_name = uiInput == 0 ? "Base" : "Delta";
  else if (m_Kind == xiiAnimationGraphNodeKind::LayeredBlend)
    out_name = uiInput == 0 ? "Base" : "Layer";
  else
    out_name.SetFormat("{}[{}]", m_Kind == xiiAnimationGraphNodeKind::StateMachine ? "State" : "Input", uiInput);
}
