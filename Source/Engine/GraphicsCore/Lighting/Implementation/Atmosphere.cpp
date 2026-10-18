/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GraphicsCore/Lighting/Atmosphere.h>

namespace
{
  bool IsValid(const xiiAtmosphereSettings& settings)
  {
    return xiiMath::IsFinite(settings.m_fPlanetRadiusKm) && settings.m_fPlanetRadiusKm > 0.0f &&
      xiiMath::IsFinite(settings.m_fAtmosphereRadiusKm) && settings.m_fAtmosphereRadiusKm > settings.m_fPlanetRadiusKm &&
      xiiMath::IsFinite(settings.m_fRayleighScaleHeightKm) && settings.m_fRayleighScaleHeightKm > 0.0f &&
      xiiMath::IsFinite(settings.m_fMieScaleHeightKm) && settings.m_fMieScaleHeightKm > 0.0f &&
      settings.m_vRayleighScattering.IsValid() && settings.m_vMieScattering.IsValid() &&
      settings.m_vMieAbsorption.IsValid() && settings.m_vOzoneAbsorption.IsValid() &&
      settings.m_vPlanetUpDirection.IsValid() && settings.m_vPlanetUpDirection.GetLengthSquared() > 1e-6f &&
      xiiMath::IsFinite(settings.m_fGroundAltitudeMeters) &&
      settings.m_vRayleighScattering.x >= 0.0f && settings.m_vRayleighScattering.y >= 0.0f && settings.m_vRayleighScattering.z >= 0.0f &&
      settings.m_vMieScattering.x >= 0.0f && settings.m_vMieScattering.y >= 0.0f && settings.m_vMieScattering.z >= 0.0f &&
      settings.m_vMieAbsorption.x >= 0.0f && settings.m_vMieAbsorption.y >= 0.0f && settings.m_vMieAbsorption.z >= 0.0f &&
      settings.m_vOzoneAbsorption.x >= 0.0f && settings.m_vOzoneAbsorption.y >= 0.0f && settings.m_vOzoneAbsorption.z >= 0.0f &&
      xiiMath::IsFinite(settings.m_fMiePhaseG) && settings.m_fMiePhaseG > -1.0f && settings.m_fMiePhaseG < 1.0f &&
      xiiMath::IsFinite(settings.m_fSunLimbDarkening) && settings.m_fSunLimbDarkening >= 0.0f && settings.m_fSunLimbDarkening <= 1.0f &&
      settings.m_uiTransmittanceIntegrationSteps > 0U && settings.m_uiTransmittanceIntegrationSteps <= 1024U &&
      settings.m_uiMultiScatterSqrtSamples > 0U && settings.m_uiMultiScatterSqrtSamples <= 64U &&
      settings.m_uiSkyViewIntegrationSteps > 0U && settings.m_uiSkyViewIntegrationSteps <= 64U &&
      settings.m_uiAerialPerspectiveSteps > 0U && settings.m_uiAerialPerspectiveSteps <= 64U;
  }

} // namespace

bool xiiIsValidAtmosphereSettings(const xiiAtmosphereSettings& settings) { return IsValid(settings); }

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAtmosphereSettings, xiiNoBase, 4, xiiRTTIDefaultAllocator<xiiAtmosphereSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PlanetRadiusKm", m_fPlanetRadiusKm)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_fPlanetRadiusKm), new xiiClampValueAttribute(1.0f, xiiVariant()), new xiiSuffixAttribute(" km")),
    XII_MEMBER_PROPERTY("AtmosphereRadiusKm", m_fAtmosphereRadiusKm)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_fAtmosphereRadiusKm), new xiiClampValueAttribute(1.0f, xiiVariant()), new xiiSuffixAttribute(" km")),
    XII_MEMBER_PROPERTY("RayleighScaleHeightKm", m_fRayleighScaleHeightKm)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_fRayleighScaleHeightKm), new xiiClampValueAttribute(0.001f, xiiVariant()), new xiiSuffixAttribute(" km")),
    XII_MEMBER_PROPERTY("MieScaleHeightKm", m_fMieScaleHeightKm)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_fMieScaleHeightKm), new xiiClampValueAttribute(0.001f, xiiVariant()), new xiiSuffixAttribute(" km")),
    XII_MEMBER_PROPERTY("RayleighScattering", m_vRayleighScattering)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_vRayleighScattering)),
    XII_MEMBER_PROPERTY("MieScattering", m_vMieScattering)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_vMieScattering)),
    XII_MEMBER_PROPERTY("MieAbsorption", m_vMieAbsorption)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_vMieAbsorption)),
    XII_MEMBER_PROPERTY("OzoneAbsorption", m_vOzoneAbsorption)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_vOzoneAbsorption)),
    XII_MEMBER_PROPERTY("PlanetUpDirection", m_vPlanetUpDirection)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_vPlanetUpDirection)),
    XII_MEMBER_PROPERTY("GroundAltitudeMeters", m_fGroundAltitudeMeters)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_fGroundAltitudeMeters), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("MiePhaseG", m_fMiePhaseG)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_fMiePhaseG), new xiiClampValueAttribute(-0.999f, 0.999f)),
    XII_MEMBER_PROPERTY("SunLimbDarkening", m_fSunLimbDarkening)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_fSunLimbDarkening), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("TransmittanceIntegrationSteps", m_uiTransmittanceIntegrationSteps)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_uiTransmittanceIntegrationSteps), new xiiClampValueAttribute(1U, 1024U)),
    XII_MEMBER_PROPERTY("MultiScatterSqrtSamples", m_uiMultiScatterSqrtSamples)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_uiMultiScatterSqrtSamples), new xiiClampValueAttribute(1U, 64U)),
    XII_MEMBER_PROPERTY("SkyViewIntegrationSteps", m_uiSkyViewIntegrationSteps)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_uiSkyViewIntegrationSteps), new xiiClampValueAttribute(1U, 64U)),
    XII_MEMBER_PROPERTY("AerialPerspectiveSteps", m_uiAerialPerspectiveSteps)->AddAttributes(new xiiDefaultValueAttribute(xiiAtmosphereSettings().m_uiAerialPerspectiveSteps), new xiiClampValueAttribute(1U, 64U)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

// clang-format on

namespace
{
  class xiiAtmosphereSettingsPatch_3_4 : public xiiGraphPatch
  {
  public:
    xiiAtmosphereSettingsPatch_3_4() : xiiGraphPatch("xiiAtmosphereSettings", 4) {}

    void Patch(xiiGraphPatchContext&, xiiAbstractObjectGraph*, xiiAbstractObjectNode* pNode) const override
    {
      // Before reflected defaults were supplied, the editor authored this entire
      // record as zero. Repair only that exact invalid record; partial settings
      // and intentionally authored values remain the user's responsibility.
      // Keep the legacy field set fixed so future properties do not change detection.
      static constexpr xiiStringView fields[] = {"PlanetRadiusKm", "AtmosphereRadiusKm", "RayleighScaleHeightKm", "MieScaleHeightKm",
                                                 "RayleighScattering", "MieScattering", "MieAbsorption", "OzoneAbsorption", "PlanetUpDirection", "GroundAltitudeMeters",
                                                 "MiePhaseG", "SunLimbDarkening", "TransmittanceIntegrationSteps", "MultiScatterSqrtSamples", "SkyViewIntegrationSteps", "AerialPerspectiveSteps"};
      const auto*                    type     = xiiGetStaticRTTI<xiiAtmosphereSettings>();
      for (const auto name : fields)
      {
        const auto* value    = pNode->FindProperty(name);
        const auto* property = type->FindPropertyByName(name);
        if (!value || !property || value->m_Value != xiiReflectionUtils::GetDefaultVariantFromType(property->GetSpecificType()))
          return;
      }
      for (const auto name : fields)
        pNode->ChangeProperty(name, xiiReflectionUtils::GetDefaultValue(type->FindPropertyByName(name)));
    }
  };

  xiiAtmosphereSettingsPatch_3_4 g_xiiAtmosphereSettingsPatch_3_4;
} // namespace

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_Atmosphere);
