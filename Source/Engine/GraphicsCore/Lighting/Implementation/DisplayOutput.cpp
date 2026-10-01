/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Lighting/DisplayOutput.h>

class xiiDisplayOutputManager::State
{
public:
  mutable xiiMutex         m_Mutex;
  xiiDisplayOutputSettings m_Defaults;
  xiiUInt64                m_uiRevision = 1U;
};

xiiUniquePtr<xiiDisplayOutputManager::State> xiiDisplayOutputManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, DisplayOutputManager)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiDisplayOutputManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiDisplayOutputManager::Shutdown();
  }
XII_END_SUBSYSTEM_DECLARATION;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiExposureMode, 1)
  XII_ENUM_CONSTANTS(xiiExposureMode::Manual, xiiExposureMode::Automatic)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiToneMappingOperator, 1)
  XII_ENUM_CONSTANTS(xiiToneMappingOperator::ACES, xiiToneMappingOperator::Reinhard, xiiToneMappingOperator::Uncharted2)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiDisplayOutputMode, 1)
  XII_ENUM_CONSTANTS(xiiDisplayOutputMode::SDRsRGB, xiiDisplayOutputMode::HDR10PQ, xiiDisplayOutputMode::scRGB)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiExposureSettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiExposureSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Mode", xiiExposureMode, m_Mode),
    XII_MEMBER_PROPERTY("MinimumEV100", m_fMinimumEV100),
    XII_MEMBER_PROPERTY("MaximumEV100", m_fMaximumEV100),
    XII_MEMBER_PROPERTY("ExposureCompensation", m_fExposureCompensation)->AddAttributes(new xiiClampValueAttribute(-16.0f, 16.0f), new xiiSuffixAttribute(" EV")),
    XII_MEMBER_PROPERTY("LowPercentile", m_fLowPercentile)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("HighPercentile", m_fHighPercentile)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("AdaptationSpeedBright", m_fAdaptationSpeedBright)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("AdaptationSpeedDark", m_fAdaptationSpeedDark)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("MinimumLogLuminance", m_fMinimumLogLuminance),
    XII_MEMBER_PROPERTY("MaximumLogLuminance", m_fMaximumLogLuminance),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiColorGradingSettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiColorGradingSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new xiiClampValueAttribute(0.0f, 4.0f)),
    XII_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new xiiClampValueAttribute(0.0f, 4.0f)),
    XII_MEMBER_PROPERTY("VignetteStrength", m_fVignetteStrength)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("VignetteRoundness", m_fVignetteRoundness)->AddAttributes(new xiiClampValueAttribute(0.25f, 4.0f)),
    XII_MEMBER_PROPERTY("FilmGrainStrength", m_fFilmGrainStrength)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiDisplayOutputSettings, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiDisplayOutputSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Exposure", m_Exposure),
    XII_MEMBER_PROPERTY("ColorGrading", m_ColorGrading),
    XII_ENUM_MEMBER_PROPERTY("ToneMappingOperator", xiiToneMappingOperator, m_ToneMappingOperator),
    XII_ENUM_MEMBER_PROPERTY("OutputMode", xiiDisplayOutputMode, m_OutputMode),
    XII_MEMBER_PROPERTY("BloomStrength", m_fBloomStrength)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("BloomThreshold", m_fBloomThreshold)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("BloomKnee", m_fBloomKnee)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("BloomRadius", m_fBloomRadius)->AddAttributes(new xiiClampValueAttribute(0.0f, 8.0f), new xiiSuffixAttribute(" px")),
    XII_MEMBER_PROPERTY("PaperWhiteNits", m_fPaperWhiteNits)->AddAttributes(new xiiClampValueAttribute(1.0f, xiiVariant()), new xiiSuffixAttribute(" nit")),
    XII_MEMBER_PROPERTY("MaximumDisplayNits", m_fMaximumDisplayNits)->AddAttributes(new xiiClampValueAttribute(1.0f, xiiVariant()), new xiiSuffixAttribute(" nit")),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

bool xiiDisplayOutputManager::IsInitialized()
{
  return s_pState != nullptr;
}

bool xiiDisplayOutputManager::IsValid(const xiiExposureSettings& settings)
{
  return settings.m_Mode.GetValue() < xiiExposureMode::ENUM_COUNT &&
    xiiMath::IsFinite(settings.m_fMinimumEV100) && xiiMath::IsFinite(settings.m_fMaximumEV100) && settings.m_fMinimumEV100 <= settings.m_fMaximumEV100 &&
    xiiMath::IsFinite(settings.m_fExposureCompensation) &&
    xiiMath::IsFinite(settings.m_fLowPercentile) && xiiMath::IsFinite(settings.m_fHighPercentile) &&
    settings.m_fLowPercentile >= 0.0f && settings.m_fLowPercentile < settings.m_fHighPercentile && settings.m_fHighPercentile <= 1.0f &&
    xiiMath::IsFinite(settings.m_fAdaptationSpeedBright) && settings.m_fAdaptationSpeedBright >= 0.0f &&
    xiiMath::IsFinite(settings.m_fAdaptationSpeedDark) && settings.m_fAdaptationSpeedDark >= 0.0f &&
    xiiMath::IsFinite(settings.m_fMinimumLogLuminance) && xiiMath::IsFinite(settings.m_fMaximumLogLuminance) &&
    settings.m_fMinimumLogLuminance < settings.m_fMaximumLogLuminance;
}

bool xiiDisplayOutputManager::IsValid(const xiiDisplayOutputSettings& settings)
{
  const xiiColorGradingSettings& grading = settings.m_ColorGrading;
  return IsValid(settings.m_Exposure) &&
    xiiMath::IsFinite(grading.m_fSaturation) && grading.m_fSaturation >= 0.0f && grading.m_fSaturation <= 4.0f &&
    xiiMath::IsFinite(grading.m_fContrast) && grading.m_fContrast >= 0.0f && grading.m_fContrast <= 4.0f &&
    xiiMath::IsFinite(grading.m_fVignetteStrength) && grading.m_fVignetteStrength >= 0.0f && grading.m_fVignetteStrength <= 1.0f &&
    xiiMath::IsFinite(grading.m_fVignetteRoundness) && grading.m_fVignetteRoundness >= 0.25f && grading.m_fVignetteRoundness <= 4.0f &&
    xiiMath::IsFinite(grading.m_fFilmGrainStrength) && grading.m_fFilmGrainStrength >= 0.0f && grading.m_fFilmGrainStrength <= 1.0f &&
    settings.m_ToneMappingOperator.GetValue() < xiiToneMappingOperator::ENUM_COUNT &&
    settings.m_OutputMode.GetValue() < xiiDisplayOutputMode::ENUM_COUNT &&
    xiiMath::IsFinite(settings.m_fBloomStrength) && settings.m_fBloomStrength >= 0.0f &&
    xiiMath::IsFinite(settings.m_fBloomThreshold) && settings.m_fBloomThreshold >= 0.0f &&
    xiiMath::IsFinite(settings.m_fBloomKnee) && settings.m_fBloomKnee >= 0.0f &&
    xiiMath::IsFinite(settings.m_fBloomRadius) && settings.m_fBloomRadius >= 0.0f && settings.m_fBloomRadius <= 8.0f &&
    xiiMath::IsFinite(settings.m_fPaperWhiteNits) && settings.m_fPaperWhiteNits > 0.0f &&
    xiiMath::IsFinite(settings.m_fMaximumDisplayNits) && settings.m_fMaximumDisplayNits >= settings.m_fPaperWhiteNits;
}

xiiDisplayOutputSettings xiiDisplayOutputManager::GetDefaults()
{
  if (s_pState == nullptr)
    return {};

  XII_LOCK(s_pState->m_Mutex);
  return s_pState->m_Defaults;
}

xiiUInt64 xiiDisplayOutputManager::GetDefaultsRevision()
{
  if (s_pState == nullptr)
    return 0U;

  XII_LOCK(s_pState->m_Mutex);
  return s_pState->m_uiRevision;
}

xiiResult xiiDisplayOutputManager::ConfigureDefaults(const xiiDisplayOutputSettings& settings)
{
  if (s_pState == nullptr || !IsValid(settings))
    return XII_FAILURE;

  XII_LOCK(s_pState->m_Mutex);
  s_pState->m_Defaults = settings;
  ++s_pState->m_uiRevision;
  return XII_SUCCESS;
}

void xiiDisplayOutputManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Display output manager started twice.");
  s_pState = XII_DEFAULT_NEW(State);
}

void xiiDisplayOutputManager::Shutdown()
{
  s_pState.Clear();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_DisplayOutput);
