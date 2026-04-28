#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/LightComponent.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLightRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiLightComponent, 1)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("LightColor", GetLightColor, SetLightColor),
    XII_ACCESSOR_PROPERTY("Temperature", GetTemperature, SetTemperature)->AddAttributes(new xiiImageSliderUiAttribute("LightTemperature"), new xiiDefaultValueAttribute(6550), new xiiClampValueAttribute(1000, 50000)),
    XII_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("CastShadows", GetCastShadows, SetCastShadows),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Lights"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgSetColor, OnMsgSetColor),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

xiiLightComponent::xiiLightComponent()  = default;
xiiLightComponent::~xiiLightComponent() = default;

void xiiLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_LightColor;
  s << m_uiTemperature;
  s << m_bCastShadows;
}

void xiiLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_LightColor;
  s >> m_uiTemperature;
  s >> m_bCastShadows;
}

void xiiLightComponent::SetLightColor(xiiColorGammaUB lightColor)
{
  if (m_LightColor != lightColor)
  {
    m_LightColor = lightColor;

    InvalidateCachedRenderData();
  }
}

xiiColorGammaUB xiiLightComponent::GetLightColor() const
{
  return m_LightColor;
}

void xiiLightComponent::SetTemperature(xiiUInt32 uiTemperature)
{
  uiTemperature = xiiMath::Clamp(uiTemperature, 1000U, 50000U);

  if (m_uiTemperature != uiTemperature)
  {
    m_uiTemperature = uiTemperature;

    InvalidateCachedRenderData();
  }
}

xiiUInt32 xiiLightComponent::GetTemperature() const
{
  return m_uiTemperature;
}

void xiiLightComponent::SetIntensity(float fIntensity)
{
  fIntensity = xiiMath::Max(fIntensity, 0.0f);

  if (m_fIntensity != fIntensity)
  {
    m_fIntensity = fIntensity;

    TriggerLocalBoundsUpdate();
    InvalidateCachedRenderData();
  }
}

float xiiLightComponent::GetIntensity() const
{
  return m_fIntensity;
}

void xiiLightComponent::SetCastShadows(bool bCastShadows)
{
  if (m_bCastShadows != bCastShadows)
  {
    m_bCastShadows = bCastShadows;

    InvalidateCachedRenderData();
  }
}

bool xiiLightComponent::GetCastShadows() const
{
  return m_bCastShadows;
}

void xiiLightComponent::OnMsgSetColor(xiiMsgSetColor& ref_msg)
{
  xiiColorGammaUB newColor = m_LightColor;
  ref_msg.ModifyColor(newColor);

  if (m_LightColor != newColor)
  {
    m_LightColor = newColor;

    InvalidateCachedRenderData();
  }
}
