/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/TextureDLL.h>

/// Helper class containing methods to convert between xiiGALResourceFormat and platform-specific image formats.
class XII_TEXTURE_DLL xiiImageFormatMappings
{
public:
  /// Maps a xiiGALResourceFormat to an equivalent Direct3D DXGI_FORMAT.
  static xiiUInt32 ToDxgiFormat(xiiEnum<xiiGALResourceFormat> format);

  /// Maps a Direct3D DXGI_FORMAT to an equivalent xiiEnum<xiiGALResourceFormat>.
  static xiiEnum<xiiGALResourceFormat> FromDxgiFormat(xiiUInt32 uiDxgiFormat);

  /// Maps a xiiGALResourceFormat to an equivalent FourCC code.
  static xiiUInt32 ToFourCc(xiiEnum<xiiGALResourceFormat> format);

  /// Maps a FourCC code to an equivalent xiiGALResourceFormat.
  static xiiEnum<xiiGALResourceFormat> FromFourCc(xiiUInt32 uiFourCc);
};
