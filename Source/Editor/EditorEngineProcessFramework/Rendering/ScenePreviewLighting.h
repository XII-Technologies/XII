/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Core/World/Declarations.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Types/Variant.h>

class xiiWorld;
class xiiView;
class xiiCamera;

/// Transient inspection state. Values are linear light, lux and EV stops.
struct XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiPreviewLightingSettings
{
  enum class Preset : xiiUInt8
  {
    NeutralStudio,
    OutdoorDaylight,
    LowLightStudio,
    Diagnostic
  };

  xiiQuat   m_Rotation              = xiiQuat::MakeFromEulerAngles(xiiAngle::MakeFromDegree(-15), xiiAngle::MakeFromDegree(43), xiiAngle::MakeFromDegree(93));
  xiiColor  m_Color                 = xiiColor::White;
  float     m_fIlluminance          = 1000.0f;
  float     m_fAmbientRadiance      = 10.0f;
  float     m_fExposure             = 0.00314159265f;
  float     m_fExposureCompensation = 0.0f;
  xiiUInt32 m_uiTemperature         = 6550;
  bool      m_bSky                  = false;
  bool      m_bShadows              = true;
  bool      m_bAutomaticExposure    = false;

  static xiiPreviewLightingSettings MakePreset(Preset preset);
  bool                              IsValid() const;
  xiiVariant                        ToVariant() const;
  static xiiResult                  FromVariant(const xiiVariant& value, xiiPreviewLightingSettings& out_settings);
  xiiResult                         ApplyToView(xiiView& view, xiiCamera& camera) const;
};

/// Optional scene editing / thumbnail illumination using the native atmosphere and directional light.
/// The sky has the lowest possible priority; authored skies use the renderer's normal priority ordering.
/// Owns only its two helper objects, never scene-authored components. Must be destroyed before its world.
/// Methods acquire the world's write marker internally.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiScenePreviewLighting
{
public:
  explicit xiiScenePreviewLighting(xiiWorld& world);
  ~xiiScenePreviewLighting();

  XII_DISALLOW_COPY_AND_ASSIGN(xiiScenePreviewLighting);

  /// Creates or repairs the helpers. With bSetEditorTag they are excluded from scene export.
  /// Unless forced, creation is deferred while the world is empty. Repeated calls are idempotent.
  xiiResult EnsureCreated(bool bSetEditorTag, bool bForce = false);

  /// Applies settings only to owned helpers. Requires EnsureCreated first.
  xiiResult SetSettings(const xiiPreviewLightingSettings& settings);

  /// Immediately deletes helpers, including untagged thumbnail lighting, before scene export.
  void Clear();

private:
  xiiWorld&           m_World;
  xiiGameObjectHandle m_hAtmosphere;
  xiiGameObjectHandle m_hDirectionalLight;
};
