#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/FogComponent.h>
#include <GraphicsCore/Pipeline/RenderDataManager.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

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
    new xiiCategoryAttribute("Effects"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiFogComponent::xiiFogComponent()  = default;
xiiFogComponent::~xiiFogComponent() = default;

void xiiFogComponent::Deinitialize()
{
  GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

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

  if (IsActiveAndInitialized())
  {
    GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

xiiColor xiiFogComponent::GetColor() const
{
  return m_Color;
}

void xiiFogComponent::SetDensity(float fDensity)
{
  m_fDensity = xiiMath::Max(fDensity, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

float xiiFogComponent::GetDensity() const
{
  return m_fDensity;
}

void xiiFogComponent::SetHeightFalloff(float fHeightFalloff)
{
  m_fHeightFalloff = xiiMath::Max(fHeightFalloff, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

float xiiFogComponent::GetHeightFalloff() const
{
  return m_fHeightFalloff;
}

void xiiFogComponent::SetModulateWithSkyColor(bool bModulate)
{
  m_bModulateWithSkyColor = bModulate;

  if (IsActiveAndInitialized())
  {
    GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

bool xiiFogComponent::GetModulateWithSkyColor() const
{
  return m_bModulateWithSkyColor;
}

void xiiFogComponent::SetSkyDistance(float fDistance)
{
  m_fSkyDistance = fDistance;

  if (IsActiveAndInitialized())
  {
    GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
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

void xiiFogComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_Color;
  s << m_fDensity;
  s << m_fHeightFalloff;
  s << m_fSkyDistance;
  s << m_bModulateWithSkyColor;
}

void xiiFogComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32  uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = inout_stream.GetStream();

  s >> m_Color;
  s >> m_fDensity;
  s >> m_fHeightFalloff;

  if (uiVersion >= 2)
  {
    s >> m_fSkyDistance;
    s >> m_bModulateWithSkyColor;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_FogComponent);
