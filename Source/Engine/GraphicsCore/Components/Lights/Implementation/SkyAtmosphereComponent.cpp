#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkyAtmosphereRenderData, 1, xiiRTTIDefaultAllocator<xiiSkyAtmosphereRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiSkyAtmosphereComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("PlanetRadius",     GetPlanetRadius,     SetPlanetRadius)->AddAttributes(new xiiDefaultValueAttribute(6360000.0f)),
    XII_ACCESSOR_PROPERTY("AtmosphereRadius", GetAtmosphereRadius, SetAtmosphereRadius)->AddAttributes(new xiiDefaultValueAttribute(6460000.0f)),
    XII_ACCESSOR_PROPERTY("MieAnisotropy",    GetMieAnisotropy,    SetMieAnisotropy)->AddAttributes(new xiiDefaultValueAttribute(0.8f), new xiiClampValueAttribute(-1.0f, 1.0f)),
    XII_ACCESSOR_PROPERTY("SunIlluminance",   GetSunIlluminance,   SetSunIlluminance)->AddAttributes(new xiiDefaultValueAttribute(120000.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Environment"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiSkyAtmosphereComponent::xiiSkyAtmosphereComponent()  = default;
xiiSkyAtmosphereComponent::~xiiSkyAtmosphereComponent() = default;

void xiiSkyAtmosphereComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_vRayleighScattering << m_fRayleighAltScale;
  s << m_fMieScattering << m_fMieExtinction << m_fMieAnisotropy << m_fMieAltScale;
  s << m_vOzoneAbsorption << m_fOzoneAltCentre << m_fOzoneAltWidth;
  s << m_fPlanetRadius << m_fAtmosphereRadius << m_fSunIlluminance;
}

void xiiSkyAtmosphereComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_vRayleighScattering >> m_fRayleighAltScale;
  s >> m_fMieScattering >> m_fMieExtinction >> m_fMieAnisotropy >> m_fMieAltScale;
  s >> m_vOzoneAbsorption >> m_fOzoneAltCentre >> m_fOzoneAltWidth;
  s >> m_fPlanetRadius >> m_fAtmosphereRadius >> m_fSunIlluminance;
}

xiiResult xiiSkyAtmosphereComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bounds);
  XII_IGNORE_UNUSED(ref_msg);
  ref_bAlwaysVisible = true;
  return XII_SUCCESS;
}

void xiiSkyAtmosphereComponent::SetPlanetRadius(float f)
{
  m_fPlanetRadius = xiiMath::Max(f, 1.0f);
  InvalidateCachedRenderData();
}
void xiiSkyAtmosphereComponent::SetAtmosphereRadius(float f)
{
  m_fAtmosphereRadius = xiiMath::Max(f, m_fPlanetRadius + 1.0f);
  InvalidateCachedRenderData();
}
void xiiSkyAtmosphereComponent::SetMieAnisotropy(float g)
{
  m_fMieAnisotropy = xiiMath::Clamp(g, -1.0f, 1.0f);
  InvalidateCachedRenderData();
}
void xiiSkyAtmosphereComponent::SetSunIlluminance(float f)
{
  m_fSunIlluminance = xiiMath::Max(f, 0.0f);
  InvalidateCachedRenderData();
}

void xiiSkyAtmosphereComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiSkyAtmosphereRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiSkyAtmosphereRenderData>(this);
  pRenderData->m_GlobalTransform          = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds             = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject             = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent          = GetHandle();
  pRenderData->m_vRayleighScattering      = m_vRayleighScattering;
  pRenderData->m_fRayleighAltScale        = m_fRayleighAltScale;
  pRenderData->m_fMieScattering           = m_fMieScattering;
  pRenderData->m_fMieExtinction           = m_fMieExtinction;
  pRenderData->m_fMieAnisotropy           = m_fMieAnisotropy;
  pRenderData->m_fMieAltScale             = m_fMieAltScale;
  pRenderData->m_vOzoneAbsorption         = m_vOzoneAbsorption;
  pRenderData->m_fOzoneAltCentre          = m_fOzoneAltCentre;
  pRenderData->m_fOzoneAltWidth           = m_fOzoneAltWidth;
  pRenderData->m_fPlanetRadius            = m_fPlanetRadius;
  pRenderData->m_fAtmosphereRadius        = m_fAtmosphereRadius;
  pRenderData->m_vSunDirection            = GetOwner()->GetGlobalRotation() * xiiVec3(1, 0, 0);
  pRenderData->m_fSunIlluminance          = m_fSunIlluminance;
  pRenderData->m_uiSortingKey             = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Lights_Implementation_SkyAtmosphereComponent);
