#include <Texture/TexturePCH.h>

#include <Foundation/IO/Stream.h>
#include <Texture/xiiTexFormat/xiiTexFormat.h>

void xiiTexFormat::WriteTextureHeader(xiiStreamWriter& inout_stream) const
{
  xiiUInt8 uiFileFormatVersion = 2;
  inout_stream << uiFileFormatVersion;

  inout_stream << m_bSRGB;
  inout_stream << m_AddressModeU;
  inout_stream << m_AddressModeV;
  inout_stream << m_AddressModeW;
  inout_stream << m_TextureFilter;
}

void xiiTexFormat::WriteRenderTargetHeader(xiiStreamWriter& inout_stream) const
{
  xiiUInt8 uiFileFormatVersion = 5;
  inout_stream << uiFileFormatVersion;

  // version 2
  inout_stream << m_bSRGB;
  inout_stream << m_AddressModeU;
  inout_stream << m_AddressModeV;
  inout_stream << m_AddressModeW;
  inout_stream << m_TextureFilter;

  // version 3
  inout_stream << m_iRenderTargetResolutionX;
  inout_stream << m_iRenderTargetResolutionY;

  // version 4
  inout_stream << m_fResolutionScale;

  // version 5
  inout_stream << m_GalRenderTargetFormat;
}

void xiiTexFormat::ReadHeader(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiFileFormatVersion = 0;
  inout_stream >> uiFileFormatVersion;

  // version 2
  if (uiFileFormatVersion >= 2)
  {
    inout_stream >> m_bSRGB;
    inout_stream >> m_AddressModeU;
    inout_stream >> m_AddressModeV;
    inout_stream >> m_AddressModeW;
    inout_stream >> m_TextureFilter;
  }

  // version 3
  if (uiFileFormatVersion >= 3)
  {
    inout_stream >> m_iRenderTargetResolutionX;
    inout_stream >> m_iRenderTargetResolutionY;
  }

  // version 4
  if (uiFileFormatVersion >= 4)
  {
    inout_stream >> m_fResolutionScale;
  }

  // version 5
  if (uiFileFormatVersion >= 5)
  {
    inout_stream >> m_GalRenderTargetFormat;
  }
}
