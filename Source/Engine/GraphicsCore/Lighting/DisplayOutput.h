/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// Selects whether exposure comes from the physical camera or GPU luminance metering.
struct XII_GRAPHICSCORE_DLL xiiExposureMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Manual,
    Automatic,

    ENUM_COUNT,

    Default = Automatic
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiExposureMode);

/// Filmic curve applied after scene exposure and before display encoding.
struct XII_GRAPHICSCORE_DLL xiiToneMappingOperator
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    ACES,
    Reinhard,
    Uncharted2,

    ENUM_COUNT,

    Default = ACES
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiToneMappingOperator);

/// Display transfer function expected by the presentation surface.
struct XII_GRAPHICSCORE_DLL xiiDisplayOutputMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    SDRsRGB,
    HDR10PQ,
    scRGB,

    ENUM_COUNT,

    Default = SDRsRGB
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDisplayOutputMode);

/// Per-view exposure controls expressed in EV100 and physical-camera units.
struct XII_GRAPHICSCORE_DLL xiiExposureSettings
{
  xiiEnum<xiiExposureMode> m_Mode;

  float m_fMinimumEV100          = -8.0f;
  float m_fMaximumEV100          = 16.0f;
  float m_fExposureCompensation  = 0.0f;
  float m_fLowPercentile         = 0.005f;
  float m_fHighPercentile        = 0.98f;
  float m_fAdaptationSpeedBright = 3.0f;
  float m_fAdaptationSpeedDark   = 1.0f;
  float m_fMinimumLogLuminance   = -12.0f;
  float m_fMaximumLogLuminance   = 20.0f;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiExposureSettings);

/// Optional artistic display-linear grading. Identity defaults preserve calibrated simulation output.
struct XII_GRAPHICSCORE_DLL xiiColorGradingSettings
{
  float m_fSaturation        = 1.0f;
  float m_fContrast          = 1.0f;
  float m_fVignetteStrength  = 0.0f;
  float m_fVignetteRoundness = 1.0f;
  float m_fFilmGrainStrength = 0.0f;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiColorGradingSettings);

/// Per-view tone mapping and display calibration.
///
/// Scene values are kept linear through tone mapping. The presentation pass applies
/// the transfer function required by the swapchain: sRGB, ST.2084/PQ, or scRGB.
struct XII_GRAPHICSCORE_DLL xiiDisplayOutputSettings
{
  xiiExposureSettings             m_Exposure;
  xiiColorGradingSettings         m_ColorGrading;
  xiiEnum<xiiToneMappingOperator> m_ToneMappingOperator;
  xiiEnum<xiiDisplayOutputMode>   m_OutputMode;

  float m_fBloomStrength      = 0.05f;
  float m_fBloomThreshold     = 1.0f;
  float m_fBloomKnee          = 0.5f;
  float m_fBloomRadius        = 1.0f;
  float m_fPaperWhiteNits     = 203.0f;
  float m_fMaximumDisplayNits = 1000.0f;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDisplayOutputSettings);

/// Process-wide defaults for newly-created views.
///
/// The state is allocated during GraphicsCore subsystem startup, after Foundation's
/// allocator is available, and destroyed before Foundation shutdown. Views copy the
/// defaults and remain independently configurable thereafter.
class XII_GRAPHICSCORE_DLL xiiDisplayOutputManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiDisplayOutputManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, DisplayOutputManager);

public:
  xiiDisplayOutputManager() = delete;

  [[nodiscard]] static bool IsInitialized();
  [[nodiscard]] static bool IsValid(const xiiExposureSettings& settings);
  [[nodiscard]] static bool IsValid(const xiiDisplayOutputSettings& settings);

  [[nodiscard]] static xiiDisplayOutputSettings GetDefaults();
  [[nodiscard]] static xiiUInt64                GetDefaultsRevision();
  [[nodiscard]] static xiiResult                ConfigureDefaults(const xiiDisplayOutputSettings& settings);

private:
  static void Startup();
  static void Shutdown();

  class State;
  static xiiUniquePtr<State> s_pState;
};
