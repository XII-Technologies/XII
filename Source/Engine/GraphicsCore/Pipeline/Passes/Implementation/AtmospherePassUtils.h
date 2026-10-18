/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Lighting/Atmosphere.h>
#include <Shaders/Pipeline/Passes/Atmosphere/AtmosphereConstants.h>
namespace xiiRenderPassUtils
{
  inline xiiAtmosphereConstants MakeAtmosphereConstants(const xiiAtmosphereSettings& settings)
  {

    xiiAtmosphereConstants constants            = {};
    constants.PlanetAtmosphereRadiiScaleHeights = xiiVec4(settings.m_fPlanetRadiusKm, settings.m_fAtmosphereRadiusKm, settings.m_fRayleighScaleHeightKm, settings.m_fMieScaleHeightKm);
    constants.RayleighScattering                = xiiVec4(settings.m_vRayleighScattering, 0.0f);
    constants.MieScatteringAndPhase             = xiiVec4(settings.m_vMieScattering, settings.m_fMiePhaseG);
    constants.MieAbsorption                     = xiiVec4(settings.m_vMieAbsorption, 0.0f);
    constants.OzoneAbsorption                   = xiiVec4(settings.m_vOzoneAbsorption, 0.0f);
    constants.PlanetUpAndGroundAltitudeMeters   = xiiVec4(settings.m_vPlanetUpDirection.GetNormalized(), settings.m_fGroundAltitudeMeters);
    constants.SunDiskParameters                 = xiiVec4(settings.m_fSunLimbDarkening, 0.0f, 0.0f, 0.0f);
    constants.SampleCounts                      = xiiVec4U32(settings.m_uiTransmittanceIntegrationSteps, settings.m_uiMultiScatterSqrtSamples, settings.m_uiSkyViewIntegrationSteps, settings.m_uiAerialPerspectiveSteps);
    return constants;
  }

} // namespace xiiRenderPassUtils
