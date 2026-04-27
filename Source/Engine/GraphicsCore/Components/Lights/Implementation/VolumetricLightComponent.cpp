#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/VolumetricLightComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVolumetricLightRenderData, 1, xiiRTTIDefaultAllocator<xiiVolumetricLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiVolumetricLightComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Scattering", GetScattering, SetScattering)->AddAttributes(new xiiDefaultValueAttribute(0.1f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Absorption", GetAbsorption, SetAbsorption)->AddAttributes(new xiiDefaultValueAttribute(0.02f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Anisotropy", GetAnisotropy, SetAnisotropy)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(-1.0f, 1.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Lights"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiVolumetricLightComponent::xiiVolumetricLightComponent()  = default;
xiiVolumetricLightComponent::~xiiVolumetricLightComponent() = default;

void xiiVolumetricLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_fScattering << m_fAbsorption << m_fAnisotropy;
}

void xiiVolumetricLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_fScattering >> m_fAbsorption >> m_fAnisotropy;
}

xiiResult xiiVolumetricLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bounds);
  XII_IGNORE_UNUSED(ref_msg);
  ref_bAlwaysVisible = true;
  return XII_SUCCESS;
}

void xiiVolumetricLightComponent::SetScattering(float f)
{
  m_fScattering = xiiMath::Max(f, 0.0f);
  InvalidateCachedRenderData();
}
void xiiVolumetricLightComponent::SetAbsorption(float f)
{
  m_fAbsorption = xiiMath::Max(f, 0.0f);
  InvalidateCachedRenderData();
}
void xiiVolumetricLightComponent::SetAnisotropy(float f)
{
  m_fAnisotropy = xiiMath::Clamp(f, -1.0f, 1.0f);
  InvalidateCachedRenderData();
}

void xiiVolumetricLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (m_fIntensity <= 0.0f || ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiVolumetricLightRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiVolumetricLightRenderData>(this);
  pRenderData->m_GlobalTransform            = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds               = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject               = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent            = GetHandle();
  pRenderData->m_LightColor                 = m_LightColor;
  pRenderData->m_uiTemperature              = m_uiTemperature;
  pRenderData->m_fIntensity                 = m_fIntensity;
  pRenderData->m_bCastShadows               = m_bCastShadows;
  pRenderData->m_fRadius                    = 0.0f;
  pRenderData->m_fScattering                = m_fScattering;
  pRenderData->m_fAbsorption                = m_fAbsorption;
  pRenderData->m_fAnisotropy                = m_fAnisotropy;
  pRenderData->m_uiSortingKey               = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Lights_Implementation_VolumetricLightComponent);
