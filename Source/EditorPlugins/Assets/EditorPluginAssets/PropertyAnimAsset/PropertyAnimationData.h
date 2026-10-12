/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

/// Channels stored in property-animation source documents. This is an editor schema, not a runtime animation API.
struct xiiPropertyAnimationChannel
{
  using StorageType = xiiUInt8;
  enum Enum
  {
    Number = 0,
    VectorX,
    VectorY,
    VectorZ,
    VectorW,
    RotationX,
    RotationY,
    RotationZ,
    Color,
    Default = Number
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORPLUGINASSETS_DLL, xiiPropertyAnimationChannel);

class XII_EDITORPLUGINASSETS_DLL xiiPropertyAnimationTrack : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPropertyAnimationTrack, xiiReflectedClass);

public:
  xiiString                            m_sObjectSearchSequence; ///< Sequence of named objects to search for the target
  xiiString                            m_sComponentType;        ///< Empty to reference the game object properties (position etc.)
  xiiString                            m_sPropertyPath;
  xiiEnum<xiiPropertyAnimationChannel> m_Target;

  xiiSingleCurveData        m_FloatCurve;
  xiiColorGradientAssetData m_ColorGradient;
};

class XII_EDITORPLUGINASSETS_DLL xiiPropertyAnimationTrackGroup : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPropertyAnimationTrackGroup, xiiReflectedClass);

public:
  xiiPropertyAnimationTrackGroup()                                                     = default;
  xiiPropertyAnimationTrackGroup(const xiiPropertyAnimationTrackGroup&)                = delete;
  xiiPropertyAnimationTrackGroup& operator=(const xiiPropertyAnimationTrackGroup& rhs) = delete;
  ~xiiPropertyAnimationTrackGroup();

  xiiUInt32                                   m_uiFramesPerSecond = 60;
  xiiUInt64                                   m_uiCurveDuration   = 480;
  xiiDynamicArray<xiiPropertyAnimationTrack*> m_Tracks;
  xiiEventTrackData                           m_EventTrack;
};
