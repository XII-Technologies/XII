#include <Texture/TexturePCH.h>

#include <Foundation/IO/Stream.h>
#include <Texture/xiiTexFormat/xiiTexFormat.h>

void xiiTexFormat::WriteTextureHeader(xiiStreamWriter& ref_stream) const
{
  xiiUInt8 uiFileFormatVersion = 2;
  ref_stream << uiFileFormatVersion;

  ref_stream << m_bSRGB;
  ref_stream << m_AddressModeU;
  ref_stream << m_AddressModeV;
  ref_stream << m_AddressModeW;
  ref_stream << m_TextureFilter;
}

void xiiTexFormat::WriteRenderTargetHeader(xiiStreamWriter& ref_stream) const
{
  xiiUInt8 uiFileFormatVersion = 5;
  ref_stream << uiFileFormatVersion;

  // version 2
  ref_stream << m_bSRGB;
  ref_stream << m_AddressModeU;
  ref_stream << m_AddressModeV;
  ref_stream << m_AddressModeW;
  ref_stream << m_TextureFilter;

  // version 3
  ref_stream << m_iRenderTargetResolutionX;
  ref_stream << m_iRenderTargetResolutionY;

  // version 4
  ref_stream << m_fResolutionScale;

  // version 5
  ref_stream << m_GalRenderTargetFormat;
}

void xiiTexFormat::ReadHeader(xiiStreamReader& ref_stream)
{
  xiiUInt8 uiFileFormatVersion = 0;
  ref_stream >> uiFileFormatVersion;

  // version 2
  if (uiFileFormatVersion >= 2)
  {
    ref_stream >> m_bSRGB;
    ref_stream >> m_AddressModeU;
    ref_stream >> m_AddressModeV;
    ref_stream >> m_AddressModeW;
    ref_stream >> m_TextureFilter;
  }

  // version 3
  if (uiFileFormatVersion >= 3)
  {
    ref_stream >> m_iRenderTargetResolutionX;
    ref_stream >> m_iRenderTargetResolutionY;
  }

  // version 4
  if (uiFileFormatVersion >= 4)
  {
    ref_stream >> m_fResolutionScale;
  }

  // version 5
  if (uiFileFormatVersion >= 5)
  {
    ref_stream >> m_GalRenderTargetFormat;
  }
}



XII_STATICLINK_FILE(Texture, Texture_xiiTexFormat_xiiTexFormat);
