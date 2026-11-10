/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>

class xiiStreamReader;
class xiiStreamWriter;
struct xiiDDGISettings;

struct xiiRenderGlobalIllumination
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    Inherit,
    PipelineDefault,
    Disabled,
    ScreenSpace,
    DDGI,
    RayTraced,
    ENUM_COUNT,
    Default = Inherit
  };

  static constexpr xiiUInt8 BranchBit(Enum mode) { return static_cast<xiiUInt8>(1U << mode); }
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderGlobalIllumination);

struct xiiRenderFeaturePolicy
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    Inherit,
    PipelineDefault,
    Disabled,
    Default = Inherit
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderFeaturePolicy);

struct xiiOptionalRenderFeature
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    Shadows                = XII_BIT(0),
    ScreenSpaceReflections = XII_BIT(1),
    All                    = Shadows | ScreenSpaceReflections,
    Default                = 0
  };
  struct Bits
  {
    StorageType Shadows : 1;
    StorageType ScreenSpaceReflections : 1;
  };
};
XII_DECLARE_FLAGS_OPERATORS(xiiOptionalRenderFeature);

/// The structural subset of a resolved configuration. Runtime values do not enter the cache key.
struct xiiRenderPipelineVariant
{
  xiiRenderGlobalIllumination::Enum     m_GlobalIllumination = xiiRenderGlobalIllumination::PipelineDefault;
  xiiBitflags<xiiOptionalRenderFeature> m_Disabled;

  bool      UsesAuthoredTopology() const { return m_GlobalIllumination < xiiRenderGlobalIllumination::Disabled && m_Disabled.IsNoFlagSet(); }
  xiiUInt16 GetCacheKey() const
  {
    const auto mode = m_GlobalIllumination < xiiRenderGlobalIllumination::Disabled ? xiiRenderGlobalIllumination::PipelineDefault : m_GlobalIllumination;
    return static_cast<xiiUInt16>(mode | (m_Disabled.GetValue() << 8));
  }
};

/// Sparse overrides of the scheduling properties authored on a DDGI pass.
struct XII_GRAPHICSCORE_DLL xiiDDGIProbeOverrides
{
  xiiUInt32 m_uiProbeCountX              = 0;
  xiiUInt32 m_uiProbeCountY              = 0;
  xiiUInt32 m_uiProbeCountZ              = 0;
  xiiUInt32 m_uiProbeUpdateBudget        = 0;
  float     m_fProbeSpacing              = 0.0f;
  float     m_fTemporalHysteresis        = -1.0f;
  float     m_fMaximumRelocationDistance = -1.0f;

  bool operator==(const xiiDDGIProbeOverrides&) const = default;
  bool IsValid() const;
  void ApplyOverrides(const xiiDDGIProbeOverrides& overrides);
  void ApplyTo(xiiDDGISettings& settings) const;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDDGIProbeOverrides);

/// Sparse rendering policy. Each field can independently inherit the preceding scope.
/// Pipeline and view defaults apply when no scope authors a value.
struct XII_GRAPHICSCORE_DLL xiiRenderConfiguration
{
  static constexpr float s_fInheritBloomStrength = -1.0f;
  static constexpr float s_fInheritRenderScale   = 0.0f;

  float m_fBloomStrength = s_fInheritBloomStrength; ///< Zero disables bloom in normal views.
  float m_fRenderScale   = s_fInheritRenderScale;   ///< An explicit scale is in [0.1, 1].
  xiiEnum<xiiRenderGlobalIllumination> m_GlobalIllumination;
  xiiDDGIProbeOverrides                m_DDGI;
  xiiEnum<xiiRenderFeaturePolicy>      m_Shadows;
  xiiEnum<xiiRenderFeaturePolicy>      m_ScreenSpaceReflections;

  static constexpr xiiUInt32 s_uiSerializationVersion = 4;

  bool operator==(const xiiRenderConfiguration&) const = default;

  bool IsValid() const;
  xiiRenderPipelineVariant GetPipelineVariant() const;

  /// Applies the authored fields of a validated, higher-priority scope.
  void ApplyOverrides(const xiiRenderConfiguration& overrides);

  /// The enclosing asset or component owns the format version.
  xiiResult Serialize(xiiStreamWriter& stream) const;
  xiiResult Deserialize(xiiStreamReader& stream, xiiUInt32 uiVersion);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderConfiguration);
