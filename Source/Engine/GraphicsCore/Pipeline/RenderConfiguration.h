/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Enum.h>

class xiiStreamReader;
class xiiStreamWriter;

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

/// Sparse rendering policy. Each field can independently inherit the preceding scope.
/// Pipeline and view defaults apply when no scope authors a value.
struct XII_GRAPHICSCORE_DLL xiiRenderConfiguration
{
  static constexpr float s_fInheritBloomStrength = -1.0f;
  static constexpr float s_fInheritRenderScale   = 0.0f;

  float m_fBloomStrength = s_fInheritBloomStrength; ///< Zero disables bloom in normal views.
  float m_fRenderScale   = s_fInheritRenderScale;   ///< An explicit scale is in [0.1, 1].
  xiiEnum<xiiRenderGlobalIllumination> m_GlobalIllumination;

  bool operator==(const xiiRenderConfiguration&) const = default;

  bool IsValid() const;

  /// Applies the authored fields of a validated, higher-priority scope.
  void ApplyOverrides(const xiiRenderConfiguration& overrides);

  /// The enclosing asset or component owns the format version.
  xiiResult Serialize(xiiStreamWriter& stream) const;
  xiiResult Deserialize(xiiStreamReader& stream, bool bHasGlobalIllumination);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderConfiguration);
