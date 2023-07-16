#include <Core/CorePCH.h>

#include <Core/Messages/SetColorMessage.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSetColorMode, 1)
XII_ENUM_CONSTANTS(xiiSetColorMode::SetRGBA, xiiSetColorMode::SetRGB, xiiSetColorMode::SetAlpha, xiiSetColorMode::AlphaBlend, xiiSetColorMode::Additive, xiiSetColorMode::Modulate)
XII_END_STATIC_REFLECTED_ENUM;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgSetColor);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgSetColor, 1, xiiRTTIDefaultAllocator<xiiMsgSetColor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Color", m_Color),
    XII_ENUM_MEMBER_PROPERTY("Mode", xiiSetColorMode, m_Mode)
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiAutoGenVisScriptMsgSender,
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiMsgSetColor::ModifyColor(xiiColor& ref_color) const
{
  switch (m_Mode)
  {
    case xiiSetColorMode::SetRGB:
      ref_color.SetRGB(m_Color.r, m_Color.g, m_Color.b);
      break;

    case xiiSetColorMode::SetAlpha:
      ref_color.a = m_Color.a;
      break;

    case xiiSetColorMode::AlphaBlend:
      ref_color = xiiMath::Lerp(ref_color, m_Color, m_Color.a);
      break;

    case xiiSetColorMode::Additive:
      ref_color += m_Color;
      break;

    case xiiSetColorMode::Modulate:
      ref_color *= m_Color;
      break;

    case xiiSetColorMode::SetRGBA:
    default:
      ref_color = m_Color;
      break;
  }
}

void xiiMsgSetColor::ModifyColor(xiiColorGammaUB& ref_color) const
{
  xiiColor temp = ref_color;
  ModifyColor(temp);
  ref_color = temp;
}

void xiiMsgSetColor::Serialize(xiiStreamWriter& ref_stream) const
{
  ref_stream << m_Color;
  ref_stream << m_Mode;
}

void xiiMsgSetColor::Deserialize(xiiStreamReader& ref_stream, xiiUInt8 uiTypeVersion)
{
  ref_stream >> m_Color;
  ref_stream >> m_Mode;
}



XII_STATICLINK_FILE(Core, Core_Messages_Implementation_SetColorMessage);
