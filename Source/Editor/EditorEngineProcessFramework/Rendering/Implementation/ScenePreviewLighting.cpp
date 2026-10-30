/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFramework/Rendering/ScenePreviewLighting.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Pipeline/View.h>

namespace
{
  template <typename Component>
  Component* FindHelper(xiiWorld& world, xiiGameObjectHandle& inout_hObject)
  {
    xiiGameObject* pObject    = nullptr;
    Component*     pComponent = nullptr;
    if (world.TryGetObject(inout_hObject, pObject))
    {
      if (pObject->TryGetComponentOfBaseType(pComponent))
        return pComponent;

      // An externally removed component must not leave an orphan helper behind.
      world.DeleteObjectNow(inout_hObject, false);
    }
    inout_hObject.Invalidate();
    return nullptr;
  }

  void SetEditorTag(xiiGameObject& object, bool bSetEditorTag)
  {
    const xiiTag& tag = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
    if (bSetEditorTag)
      object.SetTag(tag);
    else
      object.RemoveTag(tag);
  }
} // namespace

xiiScenePreviewLighting::xiiScenePreviewLighting(xiiWorld& world) : m_World(world) {}

xiiScenePreviewLighting::~xiiScenePreviewLighting()
{
  Clear();
}

xiiResult xiiScenePreviewLighting::EnsureCreated(bool bSetEditorTag, bool bForce)
{
  XII_LOCK(m_World.GetWriteMarker());

  auto* pSky   = FindHelper<xiiSkyAtmosphereComponent>(m_World, m_hAtmosphere);
  auto* pLight = FindHelper<xiiDirectionalLightComponent>(m_World, m_hDirectionalLight);
  if (!bForce && m_World.GetObjectCount() == 0)
    return XII_SUCCESS;

  if (pSky == nullptr)
  {
    xiiGameObjectDescription description;
    description.m_sName.Assign("Editor Preview Sky");
    xiiGameObject* pObject = nullptr;
    m_hAtmosphere          = m_World.CreateObject(description, pObject);
    if (pObject == nullptr || xiiSkyAtmosphereComponent::CreateComponent(pObject, pSky).IsInvalidated())
    {
      Clear();
      return XII_FAILURE;
    }
    pSky->SetPriority(xiiMath::MinValue<xiiInt32>());
  }

  if (pLight == nullptr)
  {
    xiiGameObjectDescription description;
    description.m_sName.Assign("Editor Preview Light");
    description.m_bDynamic      = true;
    description.m_LocalRotation = xiiQuat::MakeFromEulerAngles(xiiAngle::MakeFromDegree(-14.510815f), xiiAngle::MakeFromDegree(43.07951f), xiiAngle::MakeFromDegree(93.223808f));
    xiiGameObject* pObject      = nullptr;
    m_hDirectionalLight         = m_World.CreateObject(description, pObject);
    if (pObject == nullptr || xiiDirectionalLightComponent::CreateComponent(pObject, pLight).IsInvalidated())
    {
      Clear();
      return XII_FAILURE;
    }
    pLight->SetIntensityUnit(xiiPhotometricUnit::Lux);
    pLight->SetIntensity(10.0f);
  }

  SetEditorTag(*pSky->GetOwner(), bSetEditorTag);
  SetEditorTag(*pLight->GetOwner(), bSetEditorTag);
  return XII_SUCCESS;
}

void xiiScenePreviewLighting::Clear()
{
  XII_LOCK(m_World.GetWriteMarker());
  m_World.DeleteObjectNow(m_hAtmosphere, false);
  m_World.DeleteObjectNow(m_hDirectionalLight, false);
  m_hAtmosphere.Invalidate();
  m_hDirectionalLight.Invalidate();
}

xiiPreviewLightingSettings xiiPreviewLightingSettings::MakePreset(Preset preset)
{
  xiiPreviewLightingSettings settings;
  switch (preset)
  {
    case Preset::NeutralStudio:
      break;
    case Preset::OutdoorDaylight:
      settings.m_fIlluminance     = 100000.0f;
      settings.m_fAmbientRadiance = 1000.0f;
      settings.m_fExposure *= 0.01f;
      settings.m_bSky = true;
      break;
    case Preset::LowLightStudio:
      settings.m_fIlluminance     = 10.0f;
      settings.m_fAmbientRadiance = 0.1f;
      settings.m_fExposure *= 100.0f;
      break;
    case Preset::Diagnostic:
      settings.m_fIlluminance     = 0.0f;
      settings.m_fAmbientRadiance = 0.0f;
      settings.m_bShadows         = false;
      break;
  }
  return settings;
}

bool xiiPreviewLightingSettings::IsValid() const
{
  return m_Rotation.IsValid() && m_Color.IsValid() && m_Color.r >= 0 && m_Color.g >= 0 && m_Color.b >= 0 &&
    xiiMath::IsFinite(m_fIlluminance) && m_fIlluminance >= 0 && xiiMath::IsFinite(m_fAmbientRadiance) && m_fAmbientRadiance >= 0 &&
    xiiMath::IsFinite(m_fExposure) && m_fExposure > 0 && xiiMath::IsFinite(m_fExposureCompensation) &&
    m_fExposureCompensation >= -20 && m_fExposureCompensation <= 20 && m_uiTemperature >= 1000 && m_uiTemperature <= 40000;
}

xiiResult xiiPreviewLightingSettings::ApplyToView(xiiView& view, xiiCamera& camera) const
{
  if (!IsValid())
    return XII_FAILURE;
  xiiVariantDictionary exposure;
  exposure.Insert("Mode", static_cast<xiiInt32>(m_bAutomaticExposure ? xiiExposureMode::Automatic : xiiExposureMode::Manual));
  exposure.Insert("ExposureCompensation", m_fExposureCompensation);
  xiiVariantDictionary display;
  display.Insert("Exposure", exposure);
  view.SetPipelineParameter("DisplayOutput", display);
  view.SetPipelineParameter("EnvironmentAmbientRadiance", xiiVec3(m_fAmbientRadiance));
  camera.SetExposure(m_fExposure);
  return XII_SUCCESS;
}

xiiResult xiiScenePreviewLighting::SetSettings(const xiiPreviewLightingSettings& settings)
{
  if (!settings.IsValid())
    return XII_FAILURE;
  XII_LOCK(m_World.GetWriteMarker());
  auto* sky   = FindHelper<xiiSkyAtmosphereComponent>(m_World, m_hAtmosphere);
  auto* light = FindHelper<xiiDirectionalLightComponent>(m_World, m_hDirectionalLight);
  if (sky == nullptr || light == nullptr)
    return XII_FAILURE;
  sky->SetActiveFlag(settings.m_bSky);
  light->SetIntensity(settings.m_fIlluminance);
  light->SetIntensityUnit(xiiPhotometricUnit::Lux);
  light->SetLightColor(xiiColorGammaUB(settings.m_Color));
  light->SetTemperature(settings.m_uiTemperature);
  light->SetCastShadows(settings.m_bShadows);
  if (!light->GetOwner()->GetLocalRotation().IsEqualRotation(settings.m_Rotation, xiiMath::DefaultEpsilon<float>()))
    light->GetOwner()->SetLocalRotation(settings.m_Rotation);
  return XII_SUCCESS;
}

xiiVariant xiiPreviewLightingSettings::ToVariant() const
{
  xiiVariantDictionary data;
  data.Insert("Rotation", m_Rotation);
  data.Insert("Color", m_Color);
  data.Insert("Illuminance", m_fIlluminance);
  data.Insert("AmbientRadiance", m_fAmbientRadiance);
  data.Insert("Exposure", m_fExposure);
  data.Insert("ExposureCompensation", m_fExposureCompensation);
  data.Insert("Temperature", m_uiTemperature);
  data.Insert("Sky", m_bSky);
  data.Insert("Shadows", m_bShadows);
  data.Insert("AutomaticExposure", m_bAutomaticExposure);
  return data;
}

xiiResult xiiPreviewLightingSettings::FromVariant(const xiiVariant& value, xiiPreviewLightingSettings& out_settings)
{
  if (!value.IsA<xiiVariantDictionary>())
    return XII_FAILURE;
  const auto&                data = value.Get<xiiVariantDictionary>();
  xiiPreviewLightingSettings settings;
  const auto*                valueRotation = data.GetValue("Rotation");
  if (valueRotation == nullptr || !valueRotation->IsA<xiiQuat>())
    return XII_FAILURE;
  settings.m_Rotation    = valueRotation->Get<xiiQuat>();
  const auto* valueColor = data.GetValue("Color");
  if (valueColor == nullptr || !valueColor->IsA<xiiColor>())
    return XII_FAILURE;
  settings.m_Color             = valueColor->Get<xiiColor>();
  const auto* valueIlluminance = data.GetValue("Illuminance");
  if (valueIlluminance == nullptr || !valueIlluminance->IsA<float>())
    return XII_FAILURE;
  settings.m_fIlluminance          = valueIlluminance->Get<float>();
  const auto* valueAmbientRadiance = data.GetValue("AmbientRadiance");
  if (valueAmbientRadiance == nullptr || !valueAmbientRadiance->IsA<float>())
    return XII_FAILURE;
  settings.m_fAmbientRadiance = valueAmbientRadiance->Get<float>();
  const auto* valueExposure   = data.GetValue("Exposure");
  if (valueExposure == nullptr || !valueExposure->IsA<float>())
    return XII_FAILURE;
  settings.m_fExposure                  = valueExposure->Get<float>();
  const auto* valueExposureCompensation = data.GetValue("ExposureCompensation");
  if (valueExposureCompensation == nullptr || !valueExposureCompensation->IsA<float>())
    return XII_FAILURE;
  settings.m_fExposureCompensation = valueExposureCompensation->Get<float>();
  const auto* valueTemperature     = data.GetValue("Temperature");
  if (valueTemperature == nullptr || !valueTemperature->IsA<xiiUInt32>())
    return XII_FAILURE;
  settings.m_uiTemperature = valueTemperature->Get<xiiUInt32>();
  const auto* valueSky     = data.GetValue("Sky");
  if (valueSky == nullptr || !valueSky->IsA<bool>())
    return XII_FAILURE;
  settings.m_bSky          = valueSky->Get<bool>();
  const auto* valueShadows = data.GetValue("Shadows");
  if (valueShadows == nullptr || !valueShadows->IsA<bool>())
    return XII_FAILURE;
  settings.m_bShadows                = valueShadows->Get<bool>();
  const auto* valueAutomaticExposure = data.GetValue("AutomaticExposure");
  if (valueAutomaticExposure == nullptr || !valueAutomaticExposure->IsA<bool>())
    return XII_FAILURE;
  settings.m_bAutomaticExposure = valueAutomaticExposure->Get<bool>();
  if (!settings.IsValid())
    return XII_FAILURE;
  out_settings = settings;
  return XII_SUCCESS;
}
