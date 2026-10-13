/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimationData.h>
#include <Foundation/Serialization/GraphPatch.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiPropertyAnimationChannel, 1)
  XII_ENUM_CONSTANTS(xiiPropertyAnimationChannel::Number, xiiPropertyAnimationChannel::VectorX, xiiPropertyAnimationChannel::VectorY, xiiPropertyAnimationChannel::VectorZ, xiiPropertyAnimationChannel::VectorW)
  XII_ENUM_CONSTANTS(xiiPropertyAnimationChannel::RotationX, xiiPropertyAnimationChannel::RotationY, xiiPropertyAnimationChannel::RotationZ, xiiPropertyAnimationChannel::Color)
XII_END_STATIC_REFLECTED_ENUM;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPropertyAnimationTrack, 2, xiiRTTIDefaultAllocator<xiiPropertyAnimationTrack>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectPath", m_sObjectSearchSequence),
    XII_MEMBER_PROPERTY("ComponentType", m_sComponentType),
    XII_MEMBER_PROPERTY("Property", m_sPropertyPath),
    XII_ENUM_MEMBER_PROPERTY("Target", xiiPropertyAnimationChannel, m_Target),
    XII_MEMBER_PROPERTY("FloatCurve", m_FloatCurve),
    XII_MEMBER_PROPERTY("Gradient", m_ColorGradient),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPropertyAnimationTrackGroup, 1, xiiRTTIDefaultAllocator<xiiPropertyAnimationTrackGroup>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("FPS", m_uiFramesPerSecond)->AddAttributes(new xiiDefaultValueAttribute(60)),
    XII_MEMBER_PROPERTY("Duration", m_uiCurveDuration)->AddAttributes(new xiiDefaultValueAttribute(480)),
    XII_ARRAY_MEMBER_PROPERTY("Tracks", m_Tracks)->AddFlags(xiiPropertyFlags::PointerOwner),
    XII_MEMBER_PROPERTY("EventTrack", m_EventTrack),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

xiiPropertyAnimationTrackGroup::~xiiPropertyAnimationTrackGroup()
{
  for (xiiPropertyAnimationTrack* pTrack : m_Tracks)
  {
    XII_DEFAULT_DELETE(pTrack);
  }
}

// Preserve track UUIDs, bindings and control points while replacing the removed runtime enum.
class xiiPropertyAnimationTrackPatch_1_2 : public xiiGraphPatch
{
public:
  xiiPropertyAnimationTrackPatch_1_2() : xiiGraphPatch("xiiPropertyAnimationTrack", 2) {}

  void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    auto* pTarget = pNode->FindProperty("Target");
    if (pTarget != nullptr && pTarget->m_Value.IsA<xiiString>())
    {
      xiiStringBuilder channel = pTarget->m_Value.Get<xiiString>();
      channel.ReplaceAll("xiiPropertyAnimTarget::", "xiiPropertyAnimationChannel::");
      pTarget->m_Value = channel.GetData();
    }
  }
};

static xiiPropertyAnimationTrackPatch_1_2 s_PropertyAnimationTrackPatch;
