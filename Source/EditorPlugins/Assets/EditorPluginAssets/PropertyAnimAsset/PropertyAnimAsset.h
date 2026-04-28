/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorFramework/Document/GameObjectContextDocument.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <Foundation/Communication/Event.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

struct xiiGameObjectContextEvent;
class xiiPropertyAnimObjectAccessor;
class xiiPropertyAnimAssetDocument;
struct xiiCommandHistoryEvent;

class xiiPropertyAnimationTrack : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPropertyAnimationTrack, xiiReflectedClass);

public:
  xiiString                      m_sObjectSearchSequence; ///< Sequence of named objects to search for the target
  xiiString                      m_sComponentType;        ///< Empty to reference the game object properties (position etc.)
  xiiString                      m_sPropertyPath;
  xiiEnum<xiiPropertyAnimTarget> m_Target;

  xiiSingleCurveData        m_FloatCurve;
  xiiColorGradientAssetData m_ColorGradient;
};

class xiiPropertyAnimationTrackGroup : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPropertyAnimationTrackGroup, xiiReflectedClass);

public:
  xiiPropertyAnimationTrackGroup()                                                     = default;
  xiiPropertyAnimationTrackGroup(const xiiPropertyAnimationTrackGroup&)                = delete;
  xiiPropertyAnimationTrackGroup& operator=(const xiiPropertyAnimationTrackGroup& rhs) = delete;
  ~xiiPropertyAnimationTrackGroup();

  xiiUInt32                                   m_uiFramesPerSecond = 60;
  xiiUInt64                                   m_uiCurveDuration   = 480;
  xiiEnum<xiiPropertyAnimMode>                m_Mode;
  xiiDynamicArray<xiiPropertyAnimationTrack*> m_Tracks;
  xiiEventTrackData                           m_EventTrack;
};

struct xiiPropertyAnimAssetDocumentEvent
{
  enum class Type
  {
    AnimationLengthChanged,
    ScrubberPositionChanged,
    PlaybackChanged,
  };

  const xiiPropertyAnimAssetDocument* m_pDocument;
  Type                                m_Type;
};

class xiiPropertyAnimAssetDocument : public xiiSimpleAssetDocument<xiiPropertyAnimationTrackGroup, xiiGameObjectContextDocument>
{
  using BaseClass = xiiSimpleAssetDocument<xiiPropertyAnimationTrackGroup, xiiGameObjectContextDocument>;
  XII_ADD_DYNAMIC_REFLECTION(xiiPropertyAnimAssetDocument, BaseClass);

public:
  xiiPropertyAnimAssetDocument(xiiStringView sDocumentPath);
  ~xiiPropertyAnimAssetDocument();

  void      SetAnimationDurationTicks(xiiUInt64 uiNumTicks);
  xiiUInt64 GetAnimationDurationTicks() const;
  xiiTime   GetAnimationDurationTime() const;
  void      AdjustDuration();

  bool      SetScrubberPosition(xiiUInt64 uiTick);
  xiiUInt64 GetScrubberPosition() const { return m_uiScrubberTickPos; }

  xiiEvent<const xiiPropertyAnimAssetDocumentEvent&> m_PropertyAnimEvents;

  void SetPlayAnimation(bool bPlay);
  bool GetPlayAnimation() const { return m_bPlayAnimation; }
  void SetRepeatAnimation(bool bRepeat);
  bool GetRepeatAnimation() const { return m_bRepeatAnimation; }
  void ExecuteAnimationPlaybackStep();

  const xiiPropertyAnimationTrack* GetTrack(const xiiUuid& trackGuid) const;
  xiiPropertyAnimationTrack*       GetTrack(const xiiUuid& trackGuid);

  xiiStatus CanAnimate(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, xiiPropertyAnimTarget::Enum target) const;

  xiiUuid FindTrack(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, xiiPropertyAnimTarget::Enum target) const;
  xiiUuid CreateTrack(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, xiiPropertyAnimTarget::Enum target);

  xiiUuid FindCurveCp(const xiiUuid& trackGuid, xiiInt64 iTickX);
  xiiUuid InsertCurveCpAt(const xiiUuid& trackGuid, xiiInt64 iTickX, double fNewPosY);

  xiiUuid FindGradientColorCp(const xiiUuid& trackGuid, xiiInt64 iTickX);
  xiiUuid InsertGradientColorCpAt(const xiiUuid& trackGuid, xiiInt64 iTickX, const xiiColorGammaUB& color);

  xiiUuid FindGradientAlphaCp(const xiiUuid& trackGuid, xiiInt64 iTickX);
  xiiUuid InsertGradientAlphaCpAt(const xiiUuid& trackGuid, xiiInt64 iTickX, xiiUInt8 uiAlpha);

  xiiUuid FindGradientIntensityCp(const xiiUuid& trackGuid, xiiInt64 iTickX);
  xiiUuid InsertGradientIntensityCpAt(const xiiUuid& trackGuid, xiiInt64 iTickX, float fIntensity);

  xiiUuid InsertEventTrackCpAt(xiiInt64 iTickX, const char* szValue);

  virtual xiiManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return xiiManipulatorSearchStrategy::ChildrenOfSelectedObject;
  }

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual void               InitializeAfterLoading(bool bFirstTimeCreation) override;

private:
  void GameObjectContextEventHandler(const xiiGameObjectContextEvent& e);
  void TreeStructureEventHandler(const xiiDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);

  struct PropertyValue
  {
    xiiVariant                 m_InitialValue;
    xiiHybridArray<xiiUuid, 3> m_Tracks;
  };
  struct PropertyKeyHash
  {
    XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiPropertyReference& key)
    {
      return xiiHashingUtils::xxHash32(&key.m_Object, sizeof(xiiUuid)) + xiiHashingUtils::xxHash32(&key.m_pProperty, sizeof(const xiiAbstractProperty*)) + (xiiUInt32)key.m_Index.ComputeHash();
    }

    XII_ALWAYS_INLINE static bool Equal(const xiiPropertyReference& a, const xiiPropertyReference& b)
    {
      return a.m_Object == b.m_Object && a.m_pProperty == b.m_pProperty && a.m_Index == b.m_Index;
    }
  };

  void      RebuildMapping();
  void      RemoveTrack(const xiiUuid& track);
  void      AddTrack(const xiiUuid& track);
  xiiStatus FindTrackKeys(const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath, xiiHybridArray<xiiPropertyReference, 1>& keys) const;
  void      GenerateTrackInfo(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, xiiStringBuilder& sObjectSearchSequence, xiiStringBuilder& sComponentType, xiiStringBuilder& sPropertyPath) const;
  void      ApplyAnimation();
  void      ApplyAnimation(const xiiPropertyReference& key, const PropertyValue& value);

  xiiHashTable<xiiPropertyReference, PropertyValue, PropertyKeyHash> m_PropertyTable;
  xiiHashTable<xiiUuid, xiiHybridArray<xiiPropertyReference, 1>>     m_TrackTable;

  bool      m_bPlayAnimation   = false;
  bool      m_bRepeatAnimation = false;
  xiiTime   m_LastFrameTime;
  xiiUInt64 m_uiScrubberTickPos = 0;
};
