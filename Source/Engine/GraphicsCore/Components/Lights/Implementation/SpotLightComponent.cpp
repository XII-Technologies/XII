#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/SpotLightComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSpotLightRenderData, 1, xiiRTTIDefaultAllocator<xiiSpotLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiSpotLightComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("InnerConeAngle", GetInnerConeAngle, SetInnerConeAngle)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(15.0f)), new xiiClampValueAttribute(xiiAngle::MakeFromDegree(0.0f), xiiAngle::MakeFromDegree(89.0f))),
    XII_ACCESSOR_PROPERTY("OuterConeAngle", GetOuterConeAngle, SetOuterConeAngle)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(30.0f)), new xiiClampValueAttribute(xiiAngle::MakeFromDegree(1.0f), xiiAngle::MakeFromDegree(90.0f))),
    XII_ACCESSOR_PROPERTY("Range",          GetRange,          SetRange)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.01f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("IESProfile",     GetIESProfileFile, SetIESProfileFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
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
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 1.0f, xiiColor::White, "LightColor"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiSpotLightComponent::xiiSpotLightComponent()  = default;
xiiSpotLightComponent::~xiiSpotLightComponent() = default;

void xiiSpotLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_InnerConeAngle << m_OuterConeAngle << m_fRange << m_hIESProfile;
}

void xiiSpotLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_InnerConeAngle >> m_OuterConeAngle >> m_fRange >> m_hIESProfile;
}

xiiResult xiiSpotLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bAlwaysVisible);
  XII_IGNORE_UNUSED(ref_msg);

  // Build a tight bounding sphere enclosing the spot cone
  const float fHalfAngle    = xiiMath::Min(m_OuterConeAngle.GetRadian(), xiiAngle::MakeFromDegree(90.0f).GetRadian());
  const float fBaseRadius   = m_fRange * xiiMath::Sin(fHalfAngle);
  const float fCentreDist   = m_fRange * 0.5f;
  const float fSphereRadius = xiiMath::Sqrt(fCentreDist * fCentreDist + fBaseRadius * fBaseRadius);

  ref_bounds = xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3(fCentreDist, 0, 0), fSphereRadius));
  return XII_SUCCESS;
}

void xiiSpotLightComponent::SetInnerConeAngle(xiiAngle angle)
{
  m_InnerConeAngle = xiiMath::Clamp(angle, xiiAngle::MakeZero(), m_OuterConeAngle);
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiSpotLightComponent::SetOuterConeAngle(xiiAngle angle)
{
  m_OuterConeAngle = xiiMath::Clamp(angle, xiiAngle::MakeFromDegree(1.0f), xiiAngle::MakeFromDegree(90.0f));
  m_InnerConeAngle = xiiMath::Min(m_InnerConeAngle, m_OuterConeAngle);
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiSpotLightComponent::SetRange(float fRange)
{
  m_fRange = xiiMath::Max(fRange, 0.01f);
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiSpotLightComponent::SetIESProfileFile(xiiStringView sFile)
{
  if (!sFile.IsEmpty())
    m_hIESProfile = xiiResourceManager::LoadResource<xiiTexture2DResource>(sFile);
  else
    m_hIESProfile.Invalidate();
  InvalidateCachedRenderData();
}

xiiStringView xiiSpotLightComponent::GetIESProfileFile() const
{
  if (m_hIESProfile.IsValid())
    return xiiResourceManager::GetResourceIDOrDescription(m_hIESProfile);
  return {};
}

void xiiSpotLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (m_fIntensity <= 0.0f || ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiSpotLightRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiSpotLightRenderData>(this);
  pRenderData->m_GlobalTransform      = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds         = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject         = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent      = GetHandle();
  pRenderData->m_LightColor           = m_LightColor;
  pRenderData->m_uiTemperature        = m_uiTemperature;
  pRenderData->m_fIntensity           = m_fIntensity;
  pRenderData->m_bCastShadows         = m_bCastShadows;
  pRenderData->m_fRadius              = m_fRange;
  pRenderData->m_InnerConeAngle       = m_InnerConeAngle;
  pRenderData->m_OuterConeAngle       = m_OuterConeAngle;
  pRenderData->m_fRange               = m_fRange;
  pRenderData->m_hIESProfile          = m_hIESProfile;
  pRenderData->m_uiSortingKey         = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Lights_Implementation_SpotLightComponent);
