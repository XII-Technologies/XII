/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#if BUILDSYSTEM_ENABLE_BC7ENC_SUPPORT

#  include <bc7enc_rdo/rdo_bc_encoder.h>

#  include <Foundation/System/SystemInformation.h>
#  include <Texture/Image/ImageConversion.h>

xiiImageConversionEntry g_BC7EncConversions[] = {
  // Even at the lowest quality level of BC7Enc, BC1 encoding times are more than a magnitude worse than DXTextureConverter.
  // xiiImageConversionEntry(xiiImageFormat::R8G8B8A8_UNORM, xiiImageFormat::BC1_UNORM, xiiImageConversionFlags::Default),
  // xiiImageConversionEntry(xiiImageFormat::R8G8B8A8_UNORM_SRGB, xiiImageFormat::BC1_UNORM_SRGB, xiiImageConversionFlags::Default),
  xiiImageConversionEntry(xiiImageFormat::R8G8B8A8_UNORM, xiiImageFormat::BC7_UNORM, xiiImageConversionFlags::Default),
  xiiImageConversionEntry(xiiImageFormat::R8G8B8A8_UNORM_SRGB, xiiImageFormat::BC7_UNORM_SRGB, xiiImageConversionFlags::Default),
};

class xiiImageConversion_CompressBC7Enc : public xiiImageConversionStepCompressBlocks
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    return g_BC7EncConversions;
  }

  virtual xiiResult CompressBlocks(xiiConstByteBlobPtr source, xiiByteBlobPtr target, xiiUInt32 numBlocksX, xiiUInt32 numBlocksY, xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat) const override
  {
    xiiSystemInformation info      = xiiSystemInformation::Get();
    const xiiInt32       iCpuCores = info.GetCPUCoreCount();

    rdo_bc::rdo_bc_params rp;
    rp.m_rdo_max_threads   = xiiMath::Clamp<xiiInt32>(iCpuCores - 2, 2, 8);
    rp.m_status_output     = false;
    rp.m_bc1_quality_level = 18;

    switch (targetFormat)
    {
      case xiiImageFormat::BC7_UNORM:
      case xiiImageFormat::BC7_UNORM_SRGB:
        rp.m_dxgi_format = DXGI_FORMAT_BC7_UNORM;
        break;
      case xiiImageFormat::BC1_UNORM:
      case xiiImageFormat::BC1_UNORM_SRGB:
        rp.m_dxgi_format = DXGI_FORMAT_BC1_UNORM;
        break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    utils::image_u8 source_image(numBlocksX * 4, numBlocksY * 4);
    auto&           pixels = source_image.get_pixels();
    xiiMemoryUtils::Copy<xiiUInt32>(reinterpret_cast<xiiUInt32*>(pixels.data()), reinterpret_cast<const xiiUInt32*>(source.GetPtr()), numBlocksX * 4 * numBlocksY * 4);

    rdo_bc::rdo_bc_encoder encoder;
    if (!encoder.init(source_image, rp))
    {
      xiiLog::Error("rdo_bc_encoder::init() failed!");
      return XII_FAILURE;
    }

    if (!encoder.encode())
    {
      xiiLog::Error("rdo_bc_encoder::encode() failed!");
      return XII_FAILURE;
    }

    const xiiUInt32 uiTotalBytes = encoder.get_total_blocks_size_in_bytes();
    if (uiTotalBytes != target.GetCount())
    {
      xiiLog::Error("Encoder output of {} byte does not match the expected size of {} bytes", uiTotalBytes, target.GetCount());
      return XII_FAILURE;
    }
    xiiMemoryUtils::Copy<xiiUInt8>(reinterpret_cast<xiiUInt8*>(target.GetPtr()), reinterpret_cast<const xiiUInt8*>(encoder.get_blocks()), uiTotalBytes);
    return XII_SUCCESS;
  }
};

XII_STATICLINK_FORCE static xiiImageConversion_CompressBC7Enc s_conversion_compressBC7Enc;

#endif

XII_STATICLINK_FILE(Texture, Texture_Image_Conversions_BC7EncConversions);
