/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

/// The authoring menu exposes only node kinds evaluated by the current runtime.
struct xiiAnimationGraphNodeKind
{
  using StorageType = xiiUInt8;
  enum Enum
  {
    Output,
    Clip,
    Blend1D,
    Additive,
    LayeredBlend,
    StateMachine,
    Default = Clip
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_SHAREDPLUGINASSETS_DLL, xiiAnimationGraphNodeKind);

/// Tool-side settings. Pose dependencies are document connections, not duplicated
/// numeric indices. Names identify transition states; unnamed nodes use their document UUID.
class XII_SHAREDPLUGINASSETS_DLL xiiAnimationGraphEditorNode : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationGraphEditorNode, xiiReflectedClass);

public:
  xiiString                          m_sName;
  xiiEnum<xiiAnimationGraphNodeKind> m_Kind;
  xiiString                          m_sClip;
  xiiString                          m_sParameter;
  float                              m_fPlaybackSpeed = 1;
  float                              m_fWeight        = 1;
  xiiUInt16                          m_uiTargetJoint  = xiiMath::MaxValue<xiiUInt16>();
  xiiUInt16                          m_uiStateCount   = 2;
  xiiDynamicArray<float>             m_BlendThresholds;

  xiiAnimGraphNodeType::Enum GetRuntimeType() const;
  xiiUInt32                  GetInputCount() const;
  void                       GetInputName(xiiUInt32 uiInput, xiiStringBuilder& out_name) const;
};

struct XII_SHAREDPLUGINASSETS_DLL xiiAnimationGraphEditorTransition
{
  xiiString m_sFromNode;
  xiiString m_sToNode;
  xiiString m_sConditionParameter;
  float     m_fConditionThreshold = 0.5f;
  xiiTime   m_BlendDuration       = xiiTime::MakeFromMilliseconds(150);
};
XII_DECLARE_REFLECTABLE_TYPE(XII_SHAREDPLUGINASSETS_DLL, xiiAnimationGraphEditorTransition);
