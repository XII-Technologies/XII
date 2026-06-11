/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/TextureDLL.h>

/// \brief Helper class containing methods to convert between xiiGALResourceFormat::Enum and platform-specific image formats.
class XII_TEXTURE_DLL xiiImageFormatMappings
{
public:
  /// \brief Maps a xiiGALResourceFormat::Enum to an equivalent Direct3D DXGI_FORMAT.
  static xiiUInt32 ToDxgiFormat(xiiGALResourceFormat::Enum format);

  /// \brief Maps a Direct3D DXGI_FORMAT to an equivalent xiiGALResourceFormat::Enum.
  static xiiGALResourceFormat::Enum FromDxgiFormat(xiiUInt32 uiDxgiFormat);

  /// \brief Maps a xiiGALResourceFormat::Enum to an equivalent FourCC code.
  static xiiUInt32 ToFourCc(xiiGALResourceFormat::Enum format);

  /// \brief Maps a FourCC code to an equivalent xiiGALResourceFormat::Enum.
  static xiiGALResourceFormat::Enum FromFourCc(xiiUInt32 uiFourCc);
};
