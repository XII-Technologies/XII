/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Message.h>

/// Describes how a color should be applied to another color.
struct xiiSetColorMode
{
  using StorageType = xiiUInt32;

  enum Enum
  {
    SetRGBA,  ///< Overrides all four RGBA values.
    SetRGB,   ///< Overrides the RGB values but leaves Alpha untouched.
    SetAlpha, ///< Overrides Alpha, leaves RGB untouched.

    AlphaBlend, ///< Modifies the target RGBA values by interpolating from the previous color towards the incoming color using the incoming alpha value.
    Additive,   ///< Adds to the RGBA values.
    Modulate,   ///< Multiplies the RGBA values.

    Default = SetRGBA
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiSetColorMode);

/// A message to modify the main color of some thing.
///
/// Components that handle this message use it to change their main color.
/// For instance a light component may change its light color, a mesh component will change the main mesh color.
struct XII_CORE_DLL xiiMsgSetColor : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgSetColor, xiiMessage);

  /// The color to apply to the target.
  xiiColor m_Color;

  /// The mode with which to apply the color to the target.
  xiiEnum<xiiSetColorMode> m_Mode;

  /// Applies m_Color using m_Mode to the given color.
  void ModifyColor(xiiColor& ref_color) const;

  /// Applies m_Color using m_Mode to the given color.
  void ModifyColor(xiiColorGammaUB& ref_color) const;

  virtual void Serialize(xiiStreamWriter& ref_stream) const override;
  virtual void Deserialize(xiiStreamReader& ref_stream, xiiUInt8 uiTypeVersion) override;
};
