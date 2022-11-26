#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiFogRenderData, 1, xiiRTTIDefaultAllocator<xiiFogRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiFogComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new xiiDefaultValueAttribute(xiiColorGammaUB(xiiColor(0.2f, 0.2f, 0.3f)))),
    XII_ACCESSOR_PROPERTY("Density", GetDensity, SetDensity)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("HeightFalloff", GetHeightFalloff, SetHeightFalloff)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(10.0f)),
    XII_ACCESSOR_PROPERTY("ModulateWithSkyColor", GetModulateWithSkyColor, SetModulateWithSkyColor),
    XII_ACCESSOR_PROPERTY("SkyDistance", GetSkyDistance, SetSkyDistance)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1000.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiFogComponent::xiiFogComponent()  = default;
xiiFogComponent::~xiiFogComponent() = default;

void xiiFogComponent::Deinitialize()
{
  xiiRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  SUPER::Deinitialize();
}

void xiiFogComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

void xiiFogComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();
}

void xiiFogComponent::SetColor(xiiColor color)
{
  m_Color = color;
  SetModified(XII_BIT(1));
}

xiiColor xiiFogComponent::GetColor() const
{
  return m_Color;
}

void xiiFogComponent::SetDensity(float fDensity)
{
  m_fDensity = xiiMath::Max(fDensity, 0.0f);
  SetModified(XII_BIT(2));
}

float xiiFogComponent::GetDensity() const
{
  return m_fDensity;
}

void xiiFogComponent::SetHeightFalloff(float fHeightFalloff)
{
  m_fHeightFalloff = xiiMath::Max(fHeightFalloff, 0.0f);
  SetModified(XII_BIT(3));
}

float xiiFogComponent::GetHeightFalloff() const
{
  return m_fHeightFalloff;
}

void xiiFogComponent::SetModulateWithSkyColor(bool bModulate)
{
  m_bModulateWithSkyColor = bModulate;
  SetModified(XII_BIT(4));
}

bool xiiFogComponent::GetModulateWithSkyColor() const
{
  return m_bModulateWithSkyColor;
}

void xiiFogComponent::SetSkyDistance(float fDistance)
{
  m_fSkyDistance = fDistance;
  SetModified(XII_BIT(5));
}

float xiiFogComponent::GetSkyDistance() const
{
  return m_fSkyDistance;
}

void xiiFogComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? xiiDefaultSpatialDataCategories::RenderDynamic : xiiDefaultSpatialDataCategories::RenderStatic);
}

void xiiFogComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != xiiInvalidRenderDataCategory)
    return;

  auto pRenderData = xiiCreateRenderDataForThisFrame<xiiFogRenderData>(GetOwner());

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_Color           = m_Color;
  pRenderData->m_fDensity        = m_fDensity / 100.0f;
  pRenderData->m_fHeightFalloff  = m_fHeightFalloff;
  pRenderData->m_fInvSkyDistance = m_bModulateWithSkyColor ? 1.0f / m_fSkyDistance : 0.0f;

  msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::Light, xiiRenderData::Caching::IfStatic);
}

void xiiFogComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  xiiStreamWriter& s = stream.GetStream();

  s << m_Color;
  s << m_fDensity;
  s << m_fHeightFalloff;
  s << m_fSkyDistance;
  s << m_bModulateWithSkyColor;
}

void xiiFogComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32  uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = stream.GetStream();

  s >> m_Color;
  s >> m_fDensity;
  s >> m_fHeightFalloff;

  if (uiVersion >= 2)
  {
    s >> m_fSkyDistance;
    s >> m_bModulateWithSkyColor;
  }
}

XII_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_FogComponent);
