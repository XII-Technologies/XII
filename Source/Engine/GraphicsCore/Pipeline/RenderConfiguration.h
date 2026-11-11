/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>
#include <GraphicsCore/Pipeline/RenderSettings.h>

class xiiRenderPipelinePass;
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

struct xiiRenderSettingScope
{
  enum Enum
  {
    SubsystemDefault,
    Project,
    Pipeline,
    Scene,
    Camera,
    Viewport
  };
};

struct XII_GRAPHICSCORE_DLL xiiRenderSettingSource
{
  bool                        operator==(const xiiRenderSettingSource&) const = default;
  xiiString                   m_sType;
  xiiString                   m_sPath;
  xiiUInt8                    m_uiScope = xiiRenderSettingScope::SubsystemDefault;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderSettingSource);

/// Owned collection of typed subsystem settings. Override masks express inheritance independently
/// for every property. Resolved snapshots are published at a CPU frame boundary.
struct XII_GRAPHICSCORE_DLL xiiRenderConfiguration
{
  xiiRenderConfiguration() = default;
  xiiRenderConfiguration(const xiiRenderConfiguration& other);
  xiiRenderConfiguration(xiiRenderConfiguration&& other) noexcept;
  ~xiiRenderConfiguration();
  xiiRenderConfiguration& operator=(const xiiRenderConfiguration& other);
  xiiRenderConfiguration& operator=(xiiRenderConfiguration&& other) noexcept;

  /// Owned reflected objects. RTTI serialization owns these pointers; access through the typed APIs.
  xiiDynamicArray<xiiRenderSubsystemSettings*> m_Subsystems;

  const xiiRenderSubsystemSettings*    FindSubsystem(const xiiRTTI* type) const;
  template <typename T> const T*       FindSubsystem() const { return static_cast<const T*>(FindSubsystem(xiiGetStaticRTTI<T>())); }
  xiiRenderSubsystemSettings*          EditSubsystem(const xiiRTTI* type);
  template <typename T> T*             EditSubsystem() { return static_cast<T*>(EditSubsystem(xiiGetStaticRTTI<T>())); }
  xiiResult                            SetOverride(const xiiRTTI* type, xiiStringView path, const xiiVariant& value);
  template <typename T> xiiResult      SetOverride(xiiStringView path, const xiiVariant& value) { return SetOverride(xiiGetStaticRTTI<T>(), path, value); }
  void                                 RemoveOverride(const xiiRTTI* type, xiiStringView path);
  xiiRenderSettingScope::Enum          GetSource(const xiiRTTI* type, xiiStringView path) const;
  void                                 ApplySnapshot(const xiiRenderConfiguration& snapshot);
  bool                                 HasSameSources(const xiiRenderConfiguration& other) const { return m_Sources == other.m_Sources; }
  void                                 CopySourcesFrom(const xiiRenderConfiguration& snapshot);
  xiiBitflags<xiiRenderSettingsChange> GetChanges(const xiiRenderConfiguration& previous) const;
  /// Discovers concrete subsystem types supported by these compiled pass prototypes.
  xiiResult InitializeSubsystemDefaults(xiiArrayPtr<xiiRenderPipelinePass* const> passes);
  /// Applies explicit bindings only. The caller validates a temporary pass before publishing values.
  bool      HasPassOverrides(const xiiRTTI* type) const;
  xiiResult ApplyToPass(xiiRenderPipelinePass& pass, xiiBitflags<xiiRenderSettingsChange>& out_changes) const;
  xiiResult CommitPassSettings(const xiiRenderPipelinePass& source, xiiRenderPipelinePass& target, xiiBitflags<xiiRenderSettingsChange>& out_changes) const;
  void      ApplySubsystemOverrides(const xiiRenderConfiguration& overrides, xiiRenderSettingScope::Enum scope);

  static constexpr xiiUInt32 s_uiSerializationVersion = 6;

  bool operator==(const xiiRenderConfiguration& other) const;

  bool IsValid() const;
  xiiRenderPipelineVariant GetPipelineVariant() const;

  /// Applies the authored fields of a validated, higher-priority scope.
  void ApplyOverrides(const xiiRenderConfiguration& overrides, xiiRenderSettingScope::Enum scope = xiiRenderSettingScope::Viewport);

  /// The enclosing asset or component owns the format version.
  xiiResult Serialize(xiiStreamWriter& stream) const;
  xiiResult Deserialize(xiiStreamReader& stream, xiiUInt32 uiVersion);

private:
  XII_ALLOW_PRIVATE_PROPERTIES(xiiRenderConfiguration);
  void                                    ClearSubsystems();
  xiiDynamicArray<xiiRenderSettingSource> m_Sources;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderConfiguration);
