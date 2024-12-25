#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/Enum.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/TexConv/TexConvEnums.h>

class xiiStreamWriter;
class xiiStreamReader;

struct XII_TEXTURE_DLL xiiTexFormat
{
  bool                         m_bSRGB = false;
  xiiEnum<xiiImageAddressMode> m_AddressModeU;
  xiiEnum<xiiImageAddressMode> m_AddressModeV;
  xiiEnum<xiiImageAddressMode> m_AddressModeW;

  // version 2
  xiiEnum<xiiTextureFilterSetting> m_TextureFilter;

  // version 3
  xiiInt16 m_iRenderTargetResolutionX = 0;
  xiiInt16 m_iRenderTargetResolutionY = 0;

  // version 4
  float m_fResolutionScale = 1.0f;

  // version 5
  xiiInt32 m_GalRenderTargetFormat = 0;

  void WriteTextureHeader(xiiStreamWriter& inout_stream) const;
  void WriteRenderTargetHeader(xiiStreamWriter& inout_stream) const;
  void ReadHeader(xiiStreamReader& inout_stream);
};
