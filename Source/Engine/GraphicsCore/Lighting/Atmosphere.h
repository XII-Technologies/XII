/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// Authored physical parameters consumed by atmosphere rendering passes.
/// Distances and extinction coefficients are expressed in kilometres because
/// that keeps the values numerically well-conditioned in the GPU integration.
struct XII_GRAPHICSCORE_DLL xiiAtmosphereSettings
{
  float     m_fPlanetRadiusKm                 = 6360.0f;
  float     m_fAtmosphereRadiusKm             = 6460.0f;
  float     m_fRayleighScaleHeightKm          = 8.0f;
  float     m_fMieScaleHeightKm               = 1.2f;
  xiiVec3   m_vRayleighScattering             = xiiVec3(0.005802f, 0.013558f, 0.033100f);
  xiiVec3   m_vMieScattering                  = xiiVec3(0.003996f);
  xiiVec3   m_vMieAbsorption                  = xiiVec3(0.004400f);
  xiiVec3   m_vOzoneAbsorption                = xiiVec3(0.000650f, 0.001881f, 0.000085f);
  xiiVec3   m_vPlanetUpDirection              = xiiVec3(0.0f, 0.0f, 1.0f);
  float     m_fGroundAltitudeMeters           = 0.0f;
  float     m_fMiePhaseG                      = 0.8f;
  float     m_fSunLimbDarkening               = 0.6f;
  xiiUInt32 m_uiTransmittanceIntegrationSteps = 40U;
  xiiUInt32 m_uiMultiScatterSqrtSamples       = 8U;
  xiiUInt32 m_uiSkyViewIntegrationSteps       = 16U;
  xiiUInt32 m_uiAerialPerspectiveSteps        = 8U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiAtmosphereSettings);

/// Validate physical atmosphere parameters independently of rendering initialization.
XII_GRAPHICSCORE_DLL bool xiiIsValidAtmosphereSettings(const xiiAtmosphereSettings& settings);
