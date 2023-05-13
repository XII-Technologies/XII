#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Message.h>

struct xiiSetColorMode
{
  using StorageType = xiiUInt32;

  enum Enum
  {
    SetRGBA,
    SetRGB,
    SetAlpha,

    AlphaBlend,
    Additive,
    Modulate,

    Default = SetRGBA
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiSetColorMode);

struct XII_CORE_DLL xiiMsgSetColor : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgSetColor, xiiMessage);

  xiiColor                 m_Color;
  xiiEnum<xiiSetColorMode> m_Mode;

  void ModifyColor(xiiColor& color) const;
  void ModifyColor(xiiColorGammaUB& color) const;

  //////////////////////////////////////////////////////////////////////////
  // xiiMessage interface
  //

  virtual void Serialize(xiiStreamWriter& stream) const override;
  virtual void Deserialize(xiiStreamReader& stream, xiiUInt8 uiTypeVersion) override;
};
