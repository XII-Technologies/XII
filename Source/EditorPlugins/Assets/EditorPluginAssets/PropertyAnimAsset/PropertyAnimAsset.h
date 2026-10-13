/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimationData.h>

class xiiPropertyAnimAssetDocument;

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

/// Opens existing property-animation source files for editing. The Engine no longer exports or executes these assets.
class XII_EDITORPLUGINASSETS_DLL xiiPropertyAnimAssetDocument : public xiiSimpleAssetDocument<xiiPropertyAnimationTrackGroup>
{
  using BaseClass = xiiSimpleAssetDocument<xiiPropertyAnimationTrackGroup>;
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

  xiiUuid InsertCurveCpAt(const xiiUuid& trackGuid, xiiInt64 iTickX, double fNewPosY);

  xiiUuid InsertGradientColorCpAt(const xiiUuid& trackGuid, xiiInt64 iTickX, const xiiColorGammaUB& color);

  xiiUuid InsertGradientAlphaCpAt(const xiiUuid& trackGuid, xiiInt64 iTickX, xiiUInt8 uiAlpha);

  xiiUuid InsertGradientIntensityCpAt(const xiiUuid& trackGuid, xiiInt64 iTickX, float fIntensity);

  xiiUuid InsertEventTrackCpAt(xiiInt64 iTickX, const char* szValue);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

private:
  bool      m_bPlayAnimation   = false;
  bool      m_bRepeatAnimation = false;
  xiiTime   m_LastFrameTime;
  xiiUInt64 m_uiScrubberTickPos = 0;
};
