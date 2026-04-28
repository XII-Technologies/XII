/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/Image/ImageFormat.h>

/// \brief Helper class containing methods to convert between xiiImageFormat::Enum and platform-specific image formats.
class XII_TEXTURE_DLL xiiImageFormatMappings
{
public:
  /// \brief Maps a xiiImageFormat::Enum to an equivalent Direct3D DXGI_FORMAT.
  static xiiUInt32 ToDxgiFormat(xiiImageFormat::Enum format);

  /// \brief Maps a Direct3D DXGI_FORMAT to an equivalent xiiImageFormat::Enum.
  static xiiImageFormat::Enum FromDxgiFormat(xiiUInt32 uiDxgiFormat);

  /// \brief Maps a xiiImageFormat::Enum to an equivalent FourCC code.
  static xiiUInt32 ToFourCc(xiiImageFormat::Enum format);

  /// \brief Maps a FourCC code to an equivalent xiiImageFormat::Enum.
  static xiiImageFormat::Enum FromFourCc(xiiUInt32 uiFourCc);
};
