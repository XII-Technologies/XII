#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/LightComponent.h>

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiLightComponent, 6)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("LightColor", GetLightColor, SetLightColor),
    XII_ACCESSOR_PROPERTY("Temperature", GetTemperature, SetTemperature)->AddAttributes(new xiiImageSliderUiAttribute("LightTemperature"), new xiiDefaultValueAttribute(6550), new xiiClampValueAttribute(1000, 50000)),
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
}

void xiiLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_LightColor;
  s >> m_uiTemperature;
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
