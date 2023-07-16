#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageUtils.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/Image/ImageFilter.h>

template <typename TYPE>
static void SetDiff(const xiiImageView& imageA, const xiiImageView& imageB, xiiImage& out_difference, xiiUInt32 w, xiiUInt32 h, xiiUInt32 d, xiiUInt32 uiComp)
{
  const TYPE* pA = imageA.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  const TYPE* pB = imageB.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  TYPE*       pR = out_difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  for (xiiUInt32 i = 0; i < uiComp; ++i)
    pR[i] = pB[i] > pA[i] ? (pB[i] - pA[i]) : (pA[i] - pB[i]);
}

template <typename TYPE>
static xiiUInt32 GetError(const xiiImageView& difference, xiiUInt32 w, xiiUInt32 h, xiiUInt32 d, xiiUInt32 uiComp, xiiUInt32 uiPixel)
{
  const TYPE* pR = difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  xiiUInt32 uiErrorSum = 0;

  for (xiiUInt32 p = 0; p < uiPixel; ++p)
  {
    xiiUInt32 error = 0;

    for (xiiUInt32 c = 0; c < uiComp; ++c)
    {
      error += *pR;
      ++pR;
    }

    error /= uiComp;
    uiErrorSum += error * error;
  }

  return uiErrorSum;
}

void xiiImageUtils::ComputeImageDifferenceABS(const xiiImageView& imageA, const xiiImageView& imageB, xiiImage& out_difference)
{
  XII_PROFILE_SCOPE("xiiImageUtils::ComputeImageDifferenceABS");

  XII_ASSERT_DEV(imageA.GetWidth() == imageB.GetWidth(), "Dimensions do not match");
  XII_ASSERT_DEV(imageA.GetHeight() == imageB.GetHeight(), "Dimensions do not match");
  XII_ASSERT_DEV(imageA.GetDepth() == imageB.GetDepth(), "Dimensions do not match");
  XII_ASSERT_DEV(imageA.GetImageFormat() == imageB.GetImageFormat(), "Format does not match");

  xiiImageHeader differenceHeader;

  differenceHeader.SetWidth(imageA.GetWidth());
  differenceHeader.SetHeight(imageA.GetHeight());
  differenceHeader.SetDepth(imageA.GetDepth());
  differenceHeader.SetImageFormat(imageA.GetImageFormat());
  out_difference.ResetAndAlloc(differenceHeader);

  const xiiUInt32 uiSize2D = imageA.GetHeight() * imageA.GetWidth();

  for (xiiUInt32 d = 0; d < imageA.GetDepth(); ++d)
  {
    // for (xiiUInt32 h = 0; h < ImageA.GetHeight(); ++h)
    {
      // for (xiiUInt32 w = 0; w < ImageA.GetWidth(); ++w)
      {
        switch (imageA.GetImageFormat())
        {
          case xiiImageFormat::R8G8B8A8_UNORM:
          case xiiImageFormat::R8G8B8A8_UNORM_SRGB:
          case xiiImageFormat::R8G8B8A8_UINT:
          case xiiImageFormat::R8G8B8A8_SNORM:
          case xiiImageFormat::R8G8B8A8_SINT:
          case xiiImageFormat::B8G8R8A8_UNORM:
          case xiiImageFormat::B8G8R8X8_UNORM:
          case xiiImageFormat::B8G8R8A8_UNORM_SRGB:
          case xiiImageFormat::B8G8R8X8_UNORM_SRGB:
          {
            SetDiff<xiiUInt8>(imageA, imageB, out_difference, 0, 0, d, 4 * uiSize2D);
          }
          break;

          case xiiImageFormat::B8G8R8_UNORM:
          {
            SetDiff<xiiUInt8>(imageA, imageB, out_difference, 0, 0, d, 3 * uiSize2D);
          }
          break;

          default:
            XII_REPORT_FAILURE("The xiiImageFormat {0} is not implemented", (xiiUInt32)imageA.GetImageFormat());
            return;
        }
      }
    }
  }
}

xiiUInt32 xiiImageUtils::ComputeMeanSquareError(const xiiImageView& differenceImage, xiiUInt8 uiBlockSize, xiiUInt32 uiOffsetx, xiiUInt32 uiOffsety)
{
  XII_PROFILE_SCOPE("xiiImageUtils::ComputeMeanSquareError(detail)");

  XII_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  xiiUInt32 uiNumComponents = xiiImageFormat::GetNumChannels(differenceImage.GetImageFormat());

  xiiUInt32 uiWidth  = xiiMath::Min(differenceImage.GetWidth(), uiOffsetx + uiBlockSize) - uiOffsetx;
  xiiUInt32 uiHeight = xiiMath::Min(differenceImage.GetHeight(), uiOffsety + uiBlockSize) - uiOffsety;

  // Treat image as a single-component format and scale the width instead.
  uiWidth *= uiNumComponents;

  if (uiWidth == 0 || uiHeight == 0)
    return 0;

  switch (differenceImage.GetImageFormat())
  {
      // Supported formats
    case xiiImageFormat::R8G8B8A8_UNORM:
    case xiiImageFormat::R8G8B8A8_UNORM_SRGB:
    case xiiImageFormat::R8G8B8A8_UINT:
    case xiiImageFormat::R8G8B8A8_SNORM:
    case xiiImageFormat::R8G8B8A8_SINT:
    case xiiImageFormat::B8G8R8A8_UNORM:
    case xiiImageFormat::B8G8R8A8_UNORM_SRGB:
    case xiiImageFormat::B8G8R8_UNORM:
      break;

    default:
      XII_REPORT_FAILURE("The xiiImageFormat {0} is not implemented", (xiiUInt32)differenceImage.GetImageFormat());
      return 0;
  }


  xiiUInt32 error = 0;

  xiiUInt64 uiRowPitch   = differenceImage.GetRowPitch();
  xiiUInt64 uiDepthPitch = differenceImage.GetDepthPitch();

  const xiiUInt32 uiSize2D      = uiWidth * uiHeight;
  const xiiUInt8* pSlicePointer = differenceImage.GetPixelPointer<xiiUInt8>(0, 0, 0, uiOffsetx, uiOffsety);

  for (xiiUInt32 d = 0; d < differenceImage.GetDepth(); ++d)
  {
    const xiiUInt8* pRowPointer = pSlicePointer;

    for (xiiUInt32 y = 0; y < uiHeight; ++y)
    {
      const xiiUInt8* pPixelPointer = pRowPointer;
      for (xiiUInt32 x = 0; x < uiWidth; ++x)
      {
        xiiUInt32 uiDiff = *pPixelPointer;
        error += uiDiff * uiDiff;

        pPixelPointer++;
      }

      pRowPointer += uiRowPitch;
    }

    pSlicePointer += uiDepthPitch;
  }

  error /= uiSize2D;
  return error;
}

xiiUInt32 xiiImageUtils::ComputeMeanSquareError(const xiiImageView& differenceImage, xiiUInt8 uiBlockSize)
{
  XII_PROFILE_SCOPE("xiiImageUtils::ComputeMeanSquareError");

  XII_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  const xiiUInt32 uiHalfBlockSize = uiBlockSize / 2;

  const xiiUInt32 uiBlocksX = (differenceImage.GetWidth() / uiHalfBlockSize) + 1;
  const xiiUInt32 uiBlocksY = (differenceImage.GetHeight() / uiHalfBlockSize) + 1;

  xiiUInt32 uiMaxError = 0;

  for (xiiUInt32 by = 0; by < uiBlocksY; ++by)
  {
    for (xiiUInt32 bx = 0; bx < uiBlocksX; ++bx)
    {
      const xiiUInt32 uiBlockError = ComputeMeanSquareError(differenceImage, uiBlockSize, bx * uiHalfBlockSize, by * uiHalfBlockSize);

      uiMaxError = xiiMath::Max(uiMaxError, uiBlockError);
    }
  }

  return uiMaxError;
}

template <typename Func, typename ImageType>
static void ApplyFunc(ImageType& inout_image, Func func)
{
  xiiUInt32 uiWidth  = inout_image.GetWidth();
  xiiUInt32 uiHeight = inout_image.GetHeight();
  xiiUInt32 uiDepth  = inout_image.GetDepth();

  XII_ASSERT_DEV(uiWidth > 0 && uiHeight > 0 && uiDepth > 0, "The image passed to FindMinMax has illegal dimension {}x{}x{}.", uiWidth, uiHeight, uiDepth);

  xiiUInt64 uiRowPitch    = inout_image.GetRowPitch();
  xiiUInt64 uiDepthPitch  = inout_image.GetDepthPitch();
  xiiUInt32 uiNumChannels = xiiImageFormat::GetNumChannels(inout_image.GetImageFormat());

  auto pSlicePointer = inout_image.template GetPixelPointer<xiiUInt8>();

  for (xiiUInt32 z = 0; z < inout_image.GetDepth(); ++z)
  {
    auto pRowPointer = pSlicePointer;

    for (xiiUInt32 y = 0; y < uiHeight; ++y)
    {
      auto pPixelPointer = pRowPointer;
      for (xiiUInt32 x = 0; x < uiWidth; ++x)
      {
        for (xiiUInt32 c = 0; c < uiNumChannels; ++c)
        {
          func(pPixelPointer++, x, y, z, c);
        }
      }

      pRowPointer += uiRowPitch;
    }

    pSlicePointer += uiDepthPitch;
  }
}

static void FindMinMax(const xiiImageView& image, xiiUInt8& out_uiMinRgb, xiiUInt8& out_uiMaxRgb, xiiUInt8& out_uiMinAlpha, xiiUInt8& out_uiMaxAlpha)
{
  xiiImageFormat::Enum imageFormat = image.GetImageFormat();
  XII_ASSERT_DEV(xiiImageFormat::GetBitsPerChannel(imageFormat, xiiImageFormatChannel::R) == 8 && xiiImageFormat::GetDataType(imageFormat) == xiiImageFormatDataType::UNORM, "Only 8bpp unorm formats are supported in FindMinMax");

  out_uiMinRgb   = 255u;
  out_uiMinAlpha = 255u;
  out_uiMaxRgb   = 0u;
  out_uiMaxAlpha = 0u;

  auto minMax = [&](const xiiUInt8* pPixel, xiiUInt32 /*x*/, xiiUInt32 /*y*/, xiiUInt32 /*z*/, xiiUInt32 c) {
    xiiUInt8 val = *pPixel;

    if (c < 3)
    {
      out_uiMinRgb = xiiMath::Min(out_uiMinRgb, val);
      out_uiMaxRgb = xiiMath::Max(out_uiMaxRgb, val);
    }
    else
    {
      out_uiMinAlpha = xiiMath::Min(out_uiMinAlpha, val);
      out_uiMaxAlpha = xiiMath::Max(out_uiMaxAlpha, val);
    }
  };
  ApplyFunc(image, minMax);
}

void xiiImageUtils::Normalize(xiiImage& inout_image)
{
  xiiUInt8 uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha;
  Normalize(inout_image, uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha);
}

void xiiImageUtils::Normalize(xiiImage& inout_image, xiiUInt8& out_uiMinRgb, xiiUInt8& out_uiMaxRgb, xiiUInt8& out_uiMinAlpha, xiiUInt8& out_uiMaxAlpha)
{
  XII_PROFILE_SCOPE("xiiImageUtils::Normalize");

  xiiImageFormat::Enum imageFormat = inout_image.GetImageFormat();

  XII_ASSERT_DEV(xiiImageFormat::GetBitsPerChannel(imageFormat, xiiImageFormatChannel::R) == 8 && xiiImageFormat::GetDataType(imageFormat) == xiiImageFormatDataType::UNORM, "Only 8bpp unorm formats are supported in NormalizeImage");

  bool ignoreAlpha = false;
  if (imageFormat == xiiImageFormat::B8G8R8X8_UNORM || imageFormat == xiiImageFormat::B8G8R8X8_UNORM_SRGB)
  {
    ignoreAlpha = true;
  }

  FindMinMax(inout_image, out_uiMinRgb, out_uiMaxRgb, out_uiMinAlpha, out_uiMaxAlpha);
  xiiUInt8 uiRangeRgb   = out_uiMaxRgb - out_uiMinRgb;
  xiiUInt8 uiRangeAlpha = out_uiMaxAlpha - out_uiMinAlpha;

  auto normalize = [&](xiiUInt8* pPixel, xiiUInt32 /*x*/, xiiUInt32 /*y*/, xiiUInt32 /*z*/, xiiUInt32 c) {
    xiiUInt8 val = *pPixel;
    if (c < 3)
    {
      // color channels are uniform when min == max, in that case keep original value as scaling is not meaningful
      if (uiRangeRgb != 0)
      {
        *pPixel = static_cast<xiiUInt8>(255u * (static_cast<float>(val - out_uiMinRgb) / (uiRangeRgb)));
      }
    }
    else
    {
      // alpha is uniform when minAlpha == maxAlpha, in that case keep original alpha as scaling is not meaningful
      if (!ignoreAlpha && uiRangeAlpha != 0)
      {
        *pPixel = static_cast<xiiUInt8>(255u * (static_cast<float>(val - out_uiMinAlpha) / (uiRangeAlpha)));
      }
    }
  };
  ApplyFunc(inout_image, normalize);
}

void xiiImageUtils::ExtractAlphaChannel(const xiiImageView& inputImage, xiiImage& inout_outputImage)
{
  XII_PROFILE_SCOPE("xiiImageUtils::ExtractAlphaChannel");

  switch (xiiImageFormat::Enum imageFormat = inputImage.GetImageFormat())
  {
    case xiiImageFormat::R8G8B8A8_UNORM:
    case xiiImageFormat::R8G8B8A8_UNORM_SRGB:
    case xiiImageFormat::R8G8B8A8_UINT:
    case xiiImageFormat::R8G8B8A8_SNORM:
    case xiiImageFormat::R8G8B8A8_SINT:
    case xiiImageFormat::B8G8R8A8_UNORM:
    case xiiImageFormat::B8G8R8A8_UNORM_SRGB:
      break;
    default:
      XII_REPORT_FAILURE("ExtractAlpha needs an image with 8bpp and 4 channel. The xiiImageFormat {} is not supported.", (xiiUInt32)imageFormat);
      return;
  }

  xiiImageHeader outputHeader = inputImage.GetHeader();
  outputHeader.SetImageFormat(xiiImageFormat::R8_UNORM);
  inout_outputImage.ResetAndAlloc(outputHeader);

  const xiiUInt8* pInputSlice  = inputImage.GetPixelPointer<xiiUInt8>();
  xiiUInt8*       pOutputSlice = inout_outputImage.GetPixelPointer<xiiUInt8>();

  xiiUInt64 uiInputRowPitch   = inputImage.GetRowPitch();
  xiiUInt64 uiInputDepthPitch = inputImage.GetDepthPitch();

  xiiUInt64 uiOutputRowPitch   = inout_outputImage.GetRowPitch();
  xiiUInt64 uiOutputDepthPitch = inout_outputImage.GetDepthPitch();

  for (xiiUInt32 d = 0; d < inputImage.GetDepth(); ++d)
  {
    const xiiUInt8* pInputRow  = pInputSlice;
    xiiUInt8*       pOutputRow = pOutputSlice;

    for (xiiUInt32 y = 0; y < inputImage.GetHeight(); ++y)
    {
      const xiiUInt8* pInputPixel  = pInputRow;
      xiiUInt8*       pOutputPixel = pOutputRow;
      for (xiiUInt32 x = 0; x < inputImage.GetWidth(); ++x)
      {
        *pOutputPixel = pInputPixel[3];

        pInputPixel += 4;
        ++pOutputPixel;
      }

      pInputRow += uiInputRowPitch;
      pOutputRow += uiOutputRowPitch;
    }

    pInputSlice += uiInputDepthPitch;
    pOutputSlice += uiOutputDepthPitch;
  }
}

void xiiImageUtils::CropImage(const xiiImageView& input, const xiiVec2I32& vOffset, const xiiSizeU32& newsize, xiiImage& out_output)
{
  XII_PROFILE_SCOPE("xiiImageUtils::CropImage");

  XII_ASSERT_DEV(vOffset.x >= 0, "Offset is invalid");
  XII_ASSERT_DEV(vOffset.y >= 0, "Offset is invalid");
  XII_ASSERT_DEV(vOffset.x < (xiiInt32)input.GetWidth(), "Offset is invalid");
  XII_ASSERT_DEV(vOffset.y < (xiiInt32)input.GetHeight(), "Offset is invalid");

  const xiiUInt32 uiNewWidth  = xiiMath::Min(vOffset.x + newsize.width, input.GetWidth()) - vOffset.x;
  const xiiUInt32 uiNewHeight = xiiMath::Min(vOffset.y + newsize.height, input.GetHeight()) - vOffset.y;

  xiiImageHeader outputHeader;
  outputHeader.SetWidth(uiNewWidth);
  outputHeader.SetHeight(uiNewHeight);
  outputHeader.SetImageFormat(input.GetImageFormat());
  out_output.ResetAndAlloc(outputHeader);

  for (xiiUInt32 y = 0; y < uiNewHeight; ++y)
  {
    for (xiiUInt32 x = 0; x < uiNewWidth; ++x)
    {
      switch (input.GetImageFormat())
      {
        case xiiImageFormat::R8G8B8A8_UNORM:
        case xiiImageFormat::R8G8B8A8_UNORM_SRGB:
        case xiiImageFormat::R8G8B8A8_UINT:
        case xiiImageFormat::R8G8B8A8_SNORM:
        case xiiImageFormat::R8G8B8A8_SINT:
        case xiiImageFormat::B8G8R8A8_UNORM:
        case xiiImageFormat::B8G8R8X8_UNORM:
        case xiiImageFormat::B8G8R8A8_UNORM_SRGB:
        case xiiImageFormat::B8G8R8X8_UNORM_SRGB:
          out_output.GetPixelPointer<xiiUInt32>(0, 0, 0, x, y)[0] = input.GetPixelPointer<xiiUInt32>(0, 0, 0, vOffset.x + x, vOffset.y + y)[0];
          break;

        case xiiImageFormat::B8G8R8_UNORM:
          out_output.GetPixelPointer<xiiUInt8>(0, 0, 0, x, y)[0] = input.GetPixelPointer<xiiUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[0];
          out_output.GetPixelPointer<xiiUInt8>(0, 0, 0, x, y)[1] = input.GetPixelPointer<xiiUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[1];
          out_output.GetPixelPointer<xiiUInt8>(0, 0, 0, x, y)[2] = input.GetPixelPointer<xiiUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[2];
          break;

        default:
          XII_REPORT_FAILURE("The xiiImageFormat {0} is not implemented", (xiiUInt32)input.GetImageFormat());
          return;
      }
    }
  }
}

namespace
{
  template <typename T>
  void rotate180(T* pStart, T* pEnd)
  {
    pEnd = pEnd - 1;
    while (pStart < pEnd)
    {
      xiiMath::Swap(*pStart, *pEnd);
      pStart++;
      pEnd--;
    }
  }
} // namespace

void xiiImageUtils::RotateSubImage180(xiiImage& inout_image, xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/)
{
  XII_PROFILE_SCOPE("xiiImageUtils::RotateSubImage180");

  xiiUInt8* start = inout_image.GetPixelPointer<xiiUInt8>(uiMipLevel, uiFace, uiArrayIndex);
  xiiUInt8* end   = start + inout_image.GetDepthPitch(uiMipLevel);

  xiiUInt32 bytesPerPixel = xiiImageFormat::GetBitsPerPixel(inout_image.GetImageFormat()) / 8;

  switch (bytesPerPixel)
  {
    case 4:
      rotate180<xiiUInt32>(reinterpret_cast<xiiUInt32*>(start), reinterpret_cast<xiiUInt32*>(end));
      break;
    case 12:
      rotate180<xiiVec3>(reinterpret_cast<xiiVec3*>(start), reinterpret_cast<xiiVec3*>(end));
      break;
    case 16:
      rotate180<xiiVec4>(reinterpret_cast<xiiVec4*>(start), reinterpret_cast<xiiVec4*>(end));
      break;
    default:
      // fallback version
      {
        end -= bytesPerPixel;
        while (start < end)
        {
          for (xiiUInt32 i = 0; i < bytesPerPixel; i++)
          {
            xiiMath::Swap(start[i], end[i]);
          }
          start += bytesPerPixel;
          end -= bytesPerPixel;
        }
      }
  }
}

xiiResult xiiImageUtils::Copy(const xiiImageView& srcImg, const xiiRectU32& srcRect, xiiImage& inout_dstImg, const xiiVec3U32& vDstOffset, xiiUInt32 uiDstMipLevel /*= 0*/, xiiUInt32 uiDstFace /*= 0*/, xiiUInt32 uiDstArrayIndex /*= 0*/)
{
  if (inout_dstImg.GetImageFormat() != srcImg.GetImageFormat()) // Can only copy when the image formats are identical
    return XII_FAILURE;

  if (xiiImageFormat::IsCompressed(inout_dstImg.GetImageFormat())) // Compressed formats are not supported
    return XII_FAILURE;

  XII_PROFILE_SCOPE("xiiImageUtils::Copy");

  const xiiUInt64 uiDstRowPitch     = inout_dstImg.GetRowPitch(uiDstMipLevel);
  const xiiUInt64 uiSrcRowPitch     = srcImg.GetRowPitch(uiDstMipLevel);
  const xiiUInt32 uiCopyBytesPerRow = xiiImageFormat::GetBitsPerPixel(srcImg.GetImageFormat()) * srcRect.width / 8;

  xiiUInt8*       dstPtr = inout_dstImg.GetPixelPointer<xiiUInt8>(uiDstMipLevel, uiDstFace, uiDstArrayIndex, vDstOffset.x, vDstOffset.y, vDstOffset.z);
  const xiiUInt8* srcPtr = srcImg.GetPixelPointer<xiiUInt8>(0, 0, 0, srcRect.x, srcRect.y);

  for (xiiUInt32 y = 0; y < srcRect.height; y++)
  {
    xiiMemoryUtils::Copy(dstPtr, srcPtr, uiCopyBytesPerRow);

    dstPtr += uiDstRowPitch;
    srcPtr += uiSrcRowPitch;
  }

  return XII_SUCCESS;
}

xiiResult xiiImageUtils::ExtractLowerMipChain(const xiiImageView& srcImg, xiiImage& ref_dstImg, xiiUInt32 uiNumMips)
{
  const xiiImageHeader& srcImgHeader = srcImg.GetHeader();

  if (srcImgHeader.GetNumFaces() != 1 || srcImgHeader.GetNumArrayIndices() != 1)
  {
    // Lower mips aren't stored contiguously for array/cube textures and would require copying. This isn't implemented yet.
    return XII_FAILURE;
  }

  XII_PROFILE_SCOPE("xiiImageUtils::ExtractLowerMipChain");

  uiNumMips = xiiMath::Min(uiNumMips, srcImgHeader.GetNumMipLevels());

  xiiUInt32 startMipLevel = srcImgHeader.GetNumMipLevels() - uiNumMips;

  xiiImageFormat::Enum format = srcImgHeader.GetImageFormat();

  if (xiiImageFormat::RequiresFirstLevelBlockAlignment(format))
  {
    // Some block compressed image formats require resolutions that are divisible by block size,
    // therefore adjust startMipLevel accordingly
    while (srcImgHeader.GetWidth(startMipLevel) % xiiImageFormat::GetBlockWidth(format) != 0 || srcImgHeader.GetHeight(startMipLevel) % xiiImageFormat::GetBlockHeight(format) != 0)
    {
      if (uiNumMips >= srcImgHeader.GetNumMipLevels())
        return XII_FAILURE;

      if (startMipLevel == 0)
        return XII_FAILURE;

      ++uiNumMips;
      --startMipLevel;
    }
  }

  xiiImageHeader dstImgHeader = srcImgHeader;
  dstImgHeader.SetWidth(srcImgHeader.GetWidth(startMipLevel));
  dstImgHeader.SetHeight(srcImgHeader.GetHeight(startMipLevel));
  dstImgHeader.SetDepth(srcImgHeader.GetDepth(startMipLevel));
  dstImgHeader.SetNumFaces(srcImgHeader.GetNumFaces());
  dstImgHeader.SetNumArrayIndices(srcImgHeader.GetNumArrayIndices());
  dstImgHeader.SetNumMipLevels(uiNumMips);

  const xiiUInt8* pDataBegin = srcImg.GetPixelPointer<xiiUInt8>(startMipLevel);
  const xiiUInt8* pDataEnd   = srcImg.GetByteBlobPtr().GetEndPtr();
  const ptrdiff_t dataSize   = reinterpret_cast<ptrdiff_t>(pDataEnd) - reinterpret_cast<ptrdiff_t>(pDataBegin);

  const xiiConstByteBlobPtr lowResData(pDataBegin, static_cast<xiiUInt64>(dataSize));

  xiiImageView dataview;
  dataview.ResetAndViewExternalStorage(dstImgHeader, lowResData);

  ref_dstImg.ResetAndCopy(dataview);

  return XII_SUCCESS;
}

xiiUInt32 xiiImageUtils::GetSampleIndex(xiiUInt32 uiNumTexels, xiiInt32 iIndex, xiiImageAddressMode::Enum addressMode, bool& out_bUseBorderColor)
{
  out_bUseBorderColor = false;
  if (xiiUInt32(iIndex) >= uiNumTexels)
  {
    switch (addressMode)
    {
      case xiiImageAddressMode::Repeat:
        iIndex %= uiNumTexels;

        if (iIndex < 0)
        {
          iIndex += uiNumTexels;
        }
        return iIndex;

      case xiiImageAddressMode::Mirror:
      {
        if (iIndex < 0)
        {
          iIndex = -iIndex - 1;
        }
        bool flip = (iIndex / uiNumTexels) & 1;
        iIndex %= uiNumTexels;
        if (flip)
        {
          iIndex = uiNumTexels - iIndex - 1;
        }
        return iIndex;
      }

      case xiiImageAddressMode::Clamp:
        return xiiMath::Clamp<xiiInt32>(iIndex, 0, uiNumTexels - 1);

      case xiiImageAddressMode::ClampBorder:
        out_bUseBorderColor = true;
        return 0;

      default:
        XII_ASSERT_NOT_IMPLEMENTED
        return 0;
    }
  }
  return iIndex;
}

static xiiSimdVec4f LoadSample(const xiiSimdVec4f* pSource, xiiUInt32 uiNumSourceElements, xiiUInt32 uiStride, xiiInt32 iIndex, xiiImageAddressMode::Enum addressMode, const xiiSimdVec4f& vBorderColor)
{
  bool useBorderColor = false;
  // result is in the range [-(w-1), (w-1)], bring it to [0, w - 1]
  iIndex = xiiImageUtils::GetSampleIndex(uiNumSourceElements, iIndex, addressMode, useBorderColor);
  if (useBorderColor)
  {
    return vBorderColor;
  }
  return pSource[iIndex * uiStride];
}

inline static void FilterLine(
  xiiUInt32 uiNumSourceElements,
  const xiiSimdVec4f* __restrict pSourceBegin,
  xiiSimdVec4f* __restrict pTargetBegin,
  xiiUInt32                    uiStride,
  const xiiImageFilterWeights& weights,
  xiiArrayPtr<const xiiInt32>  firstSampleIndices,
  xiiImageAddressMode::Enum    addressMode,
  const xiiSimdVec4f&          vBorderColor)
{
  // Convolve the image using the precomputed weights
  const xiiUInt32 numWeights = weights.GetNumWeights();

  // When the first source index for the output is between 0 and this value,
  // we can fetch all numWeights inputs without taking addressMode into consideration,
  // which makes the inner loop a lot faster.
  const xiiInt32 trivialSourceIndicesEnd = static_cast<xiiInt32>(uiNumSourceElements) - static_cast<xiiInt32>(numWeights);
  const auto     weightsView             = weights.ViewWeights();
  const float* __restrict nextWeightPtr  = weightsView.GetPtr();
  XII_ASSERT_DEBUG((static_cast<xiiUInt32>(weightsView.GetCount()) % numWeights) == 0, "");
  for (xiiInt32 firstSourceIdx : firstSampleIndices)
  {
    xiiSimdVec4f total(0.0f, 0.0f, 0.0f, 0.0f);

    if (firstSourceIdx >= 0 && firstSourceIdx < trivialSourceIndicesEnd)
    {
      const auto* __restrict sourcePtr = pSourceBegin + firstSourceIdx * uiStride;
      for (xiiUInt32 weightIdx = 0; weightIdx < numWeights; ++weightIdx)
      {
        total = xiiSimdVec4f::MulAdd(*sourcePtr, xiiSimdVec4f(*nextWeightPtr++), total);
        sourcePtr += uiStride;
      }
    }
    else
    {
      // Very slow fallback case that respects the addressMode
      // (not a lot of pixels are taking this path, so it's probably fine)
      xiiInt32 sourceIdx = firstSourceIdx;
      for (xiiUInt32 weightIdx = 0; weightIdx < numWeights; ++weightIdx)
      {
        total = xiiSimdVec4f::MulAdd(LoadSample(pSourceBegin, uiNumSourceElements, uiStride, sourceIdx, addressMode, vBorderColor), xiiSimdVec4f(*nextWeightPtr++), total);
        sourceIdx++;
      }
    }
    // It's ok to check this once per source index, see the assert above
    // (number of weights in weightsView is divisible by numWeights)
    if (nextWeightPtr == weightsView.GetEndPtr())
    {
      nextWeightPtr = weightsView.GetPtr();
    }
    *pTargetBegin = total;
    pTargetBegin += uiStride;
  }
}

static void DownScaleFastLine(xiiUInt32 uiPixelStride, const xiiUInt8* pSrc, xiiUInt8* pDest, xiiUInt32 uiLengthIn, xiiUInt32 uiStrideIn, xiiUInt32 uiLengthOut, xiiUInt32 uiStrideOut)
{
  const xiiUInt32 downScaleFactor = uiLengthIn / uiLengthOut;
  XII_ASSERT_DEBUG(downScaleFactor >= 1, "Unable to downscale image.");

  const xiiUInt32 downScaleFactorLog2 = xiiMath::Log2i(static_cast<xiiUInt32>(downScaleFactor));
  const xiiUInt32 roundOffset         = downScaleFactor / 2;

  for (xiiUInt32 offset = 0; offset < uiLengthOut; ++offset)
  {
    for (xiiUInt32 channel = 0; channel < uiPixelStride; ++channel)
    {
      const xiiUInt32 destOffset = offset * uiStrideOut + channel;

      xiiUInt32 curChannel = roundOffset;
      for (xiiUInt32 index = 0; index < downScaleFactor; ++index)
      {
        curChannel += static_cast<xiiUInt32>(pSrc[channel + index * uiStrideIn]);
      }

      curChannel        = curChannel >> downScaleFactorLog2;
      pDest[destOffset] = static_cast<xiiUInt8>(curChannel);
    }

    pSrc += downScaleFactor * uiStrideIn;
  }
}

static void DownScaleFast(const xiiImageView& image, xiiImage& out_result, xiiUInt32 uiWidth, xiiUInt32 uiHeight)
{
  xiiImageFormat::Enum format = image.GetImageFormat();

  xiiUInt32 originalWidth    = image.GetWidth();
  xiiUInt32 originalHeight   = image.GetHeight();
  xiiUInt32 numArrayElements = image.GetNumArrayIndices();
  xiiUInt32 numFaces         = image.GetNumFaces();

  xiiUInt32 pixelStride = xiiImageFormat::GetBitsPerPixel(format) / 8;

  xiiImageHeader intermediateHeader;
  intermediateHeader.SetWidth(uiWidth);
  intermediateHeader.SetHeight(originalHeight);
  intermediateHeader.SetNumArrayIndices(numArrayElements);
  intermediateHeader.SetNumFaces(numFaces);
  intermediateHeader.SetImageFormat(format);

  xiiImage intermediate;
  intermediate.ResetAndAlloc(intermediateHeader);

  for (xiiUInt32 arrayIndex = 0; arrayIndex < numArrayElements; arrayIndex++)
  {
    for (xiiUInt32 face = 0; face < numFaces; face++)
    {
      for (xiiUInt32 row = 0; row < originalHeight; row++)
      {
        DownScaleFastLine(pixelStride, image.GetPixelPointer<xiiUInt8>(0, face, arrayIndex, 0, row), intermediate.GetPixelPointer<xiiUInt8>(0, face, arrayIndex, 0, row), originalWidth, pixelStride, uiWidth, pixelStride);
      }
    }
  }

  // input and output images may be the same, so we can't access the original image below this point

  xiiImageHeader outHeader;
  outHeader.SetWidth(uiWidth);
  outHeader.SetHeight(uiHeight);
  outHeader.SetNumArrayIndices(numArrayElements);
  outHeader.SetNumArrayIndices(numFaces);
  outHeader.SetImageFormat(format);

  out_result.ResetAndAlloc(outHeader);

  XII_ASSERT_DEBUG(intermediate.GetRowPitch() < xiiMath::MaxValue<xiiUInt32>(), "Row pitch exceeds xiiUInt32 max value.");
  XII_ASSERT_DEBUG(out_result.GetRowPitch() < xiiMath::MaxValue<xiiUInt32>(), "Row pitch exceeds xiiUInt32 max value.");

  for (xiiUInt32 arrayIndex = 0; arrayIndex < numArrayElements; arrayIndex++)
  {
    for (xiiUInt32 face = 0; face < numFaces; face++)
    {
      for (xiiUInt32 col = 0; col < uiWidth; col++)
      {
        DownScaleFastLine(pixelStride, intermediate.GetPixelPointer<xiiUInt8>(0, face, arrayIndex, col), out_result.GetPixelPointer<xiiUInt8>(0, face, arrayIndex, col), originalHeight, static_cast<xiiUInt32>(intermediate.GetRowPitch()), uiHeight, static_cast<xiiUInt32>(out_result.GetRowPitch()));
      }
    }
  }
}

static float EvaluateAverageCoverage(xiiBlobPtr<const xiiColor> colors, float fAlphaThreshold)
{
  XII_PROFILE_SCOPE("EvaluateAverageCoverage");

  xiiUInt64 totalPixels = colors.GetCount();
  xiiUInt64 count       = 0;
  for (xiiUInt32 idx = 0; idx < totalPixels; ++idx)
  {
    count += colors[idx].a >= fAlphaThreshold;
  }

  return float(count) / float(totalPixels);
}

static void NormalizeCoverage(xiiBlobPtr<xiiColor> colors, float fAlphaThreshold, float fTargetCoverage)
{
  XII_PROFILE_SCOPE("NormalizeCoverage");

  // Based on the idea in http://the-witness.net/news/2010/09/computing-alpha-mipmaps/. Note we're using a histogram
  // to find the new alpha threshold here rather than bisecting.

  // Generate histogram of alpha values
  xiiUInt64 totalPixels         = colors.GetCount();
  xiiUInt32 alphaHistogram[256] = {};
  for (xiiUInt64 idx = 0; idx < totalPixels; ++idx)
  {
    alphaHistogram[xiiMath::ColorFloatToByte(colors[idx].a)]++;
  }

  // Find range of alpha thresholds so the number of covered pixels matches by summing up the histogram
  xiiInt32 targetCount   = xiiInt32(fTargetCoverage * totalPixels);
  xiiInt32 coverageCount = 0;
  xiiInt32 maxThreshold  = 255;
  for (; maxThreshold >= 0; maxThreshold--)
  {
    coverageCount += alphaHistogram[maxThreshold];

    if (coverageCount >= targetCount)
    {
      break;
    }
  }

  coverageCount         = targetCount;
  xiiInt32 minThreshold = 0;
  for (; minThreshold < 256; minThreshold++)
  {
    coverageCount -= alphaHistogram[maxThreshold];

    if (coverageCount <= targetCount)
    {
      break;
    }
  }

  xiiInt32 currentThreshold = xiiMath::ColorFloatToByte(fAlphaThreshold);

  // Each of the alpha test thresholds in the range [minThreshold; maxThreshold] will result in the same coverage. Pick a new threshold
  // close to the old one so we scale by the smallest necessary amount.
  xiiInt32 newThreshold;
  if (currentThreshold < minThreshold)
  {
    newThreshold = minThreshold;
  }
  else if (currentThreshold > maxThreshold)
  {
    newThreshold = maxThreshold;
  }
  else
  {
    // Avoid rescaling altogether if the current threshold already preserves coverage
    return;
  }

  // Rescale alpha values
  float alphaScale = fAlphaThreshold / (newThreshold / 255.0f);
  for (xiiUInt64 idx = 0; idx < totalPixels; ++idx)
  {
    colors[idx].a *= alphaScale;
  }
}


xiiResult xiiImageUtils::Scale(const xiiImageView& source, xiiImage& ref_target, xiiUInt32 uiWidth, xiiUInt32 uiHeight, const xiiImageFilter* pFilter, xiiImageAddressMode::Enum addressModeU, xiiImageAddressMode::Enum addressModeV, const xiiColor& borderColor)
{
  return Scale3D(source, ref_target, uiWidth, uiHeight, 1, pFilter, addressModeU, addressModeV, xiiImageAddressMode::Clamp, borderColor);
}

xiiResult xiiImageUtils::Scale3D(const xiiImageView& source, xiiImage& ref_target, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiDepth, const xiiImageFilter* pFilter /*= xii_NULL*/, xiiImageAddressMode::Enum addressModeU /*= xiiImageAddressMode::Clamp*/, xiiImageAddressMode::Enum addressModeV /*= xiiImageAddressMode::Clamp*/, xiiImageAddressMode::Enum addressModeW /*= xiiImageAddressMode::Clamp*/, const xiiColor& borderColor /*= xiiColors::Black*/)
{
  XII_PROFILE_SCOPE("xiiImageUtils::Scale3D");

  if (uiWidth == 0 || uiHeight == 0 || uiDepth == 0)
  {
    xiiImageHeader header;
    header.SetImageFormat(source.GetImageFormat());
    ref_target.ResetAndAlloc(header);
    return XII_SUCCESS;
  }

  const xiiImageFormat::Enum format = source.GetImageFormat();

  const xiiUInt32 originalWidth    = source.GetWidth();
  const xiiUInt32 originalHeight   = source.GetHeight();
  const xiiUInt32 originalDepth    = source.GetDepth();
  const xiiUInt32 numFaces         = source.GetNumFaces();
  const xiiUInt32 numArrayElements = source.GetNumArrayIndices();

  if (originalWidth == uiWidth && originalHeight == uiHeight && originalDepth == uiDepth)
  {
    ref_target.ResetAndCopy(source);
    return XII_SUCCESS;
  }

  // Scaling down by an even factor?
  const xiiUInt32 downScaleFactorX = originalWidth / uiWidth;
  const xiiUInt32 downScaleFactorY = originalHeight / uiHeight;

  if (pFilter == nullptr && (format == xiiImageFormat::R8G8B8A8_UNORM || format == xiiImageFormat::B8G8R8A8_UNORM || format == xiiImageFormat::B8G8R8_UNORM) && downScaleFactorX * uiWidth == originalWidth && downScaleFactorY * uiHeight == originalHeight && uiDepth == 1 && originalDepth == 1 &&
      xiiMath::IsPowerOf2(downScaleFactorX) && xiiMath::IsPowerOf2(downScaleFactorY))
  {
    DownScaleFast(source, ref_target, uiWidth, uiHeight);
    return XII_SUCCESS;
  }

  // Fallback to default filter
  xiiImageFilterTriangle defaultFilter;
  if (!pFilter)
  {
    pFilter = &defaultFilter;
  }

  const xiiImageView* stepSource;

  // Manage scratch images for intermediate conversion or filtering
  const xiiUInt32 maxNumScratchImages = 2;
  xiiImage        scratch[maxNumScratchImages];
  bool            scratchUsed[maxNumScratchImages] = {};
  auto            allocateScratch                  = [&]() -> xiiImage& {
    for (xiiUInt32 i = 0;; ++i)
    {
      XII_ASSERT_DEV(i < maxNumScratchImages, "Failed to allocate scratch image");
      if (!scratchUsed[i])
      {
        scratchUsed[i] = true;
        return scratch[i];
      }
    }
  };
  auto releaseScratch = [&](const xiiImageView& image) {
    for (xiiUInt32 i = 0; i < maxNumScratchImages; ++i)
    {
      if (&scratch[i] == &image)
      {
        scratchUsed[i] = false;
        return;
      }
    }
  };

  if (format == xiiImageFormat::R32G32B32A32_FLOAT)
  {
    stepSource = &source;
  }
  else
  {
    xiiImage& conversionScratch = allocateScratch();
    if (xiiImageConversion::Convert(source, conversionScratch, xiiImageFormat::R32G32B32A32_FLOAT).Failed())
    {
      return XII_FAILURE;
    }

    stepSource = &conversionScratch;
  };

  xiiHybridArray<xiiInt32, 256> firstSampleIndices;
  firstSampleIndices.Reserve(xiiMath::Max(uiWidth, uiHeight, uiDepth));

  if (uiWidth != originalWidth)
  {
    xiiImageFilterWeights weights(*pFilter, originalWidth, uiWidth);
    firstSampleIndices.SetCountUninitialized(uiWidth);
    for (xiiUInt32 x = 0; x < uiWidth; ++x)
    {
      firstSampleIndices[x] = weights.GetFirstSourceSampleIndex(x);
    }

    xiiImage* stepTarget;
    if (uiHeight == originalHeight && uiDepth == originalDepth && format == xiiImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    xiiImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetWidth(uiWidth);
    stepTarget->ResetAndAlloc(stepHeader);

    for (xiiUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (xiiUInt32 face = 0; face < numFaces; ++face)
      {
        for (xiiUInt32 z = 0; z < originalDepth; ++z)
        {
          for (xiiUInt32 y = 0; y < originalHeight; ++y)
          {
            const xiiSimdVec4f* filterSource = stepSource->GetPixelPointer<xiiSimdVec4f>(0, face, arrayIndex, 0, y, z);
            xiiSimdVec4f*       filterTarget = stepTarget->GetPixelPointer<xiiSimdVec4f>(0, face, arrayIndex, 0, y, z);
            FilterLine(originalWidth, filterSource, filterTarget, 1, weights, firstSampleIndices, addressModeU, xiiSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  if (uiHeight != originalHeight)
  {
    xiiImageFilterWeights weights(*pFilter, originalHeight, uiHeight);
    firstSampleIndices.SetCount(uiHeight);
    for (xiiUInt32 y = 0; y < uiHeight; ++y)
    {
      firstSampleIndices[y] = weights.GetFirstSourceSampleIndex(y);
    }

    xiiImage* stepTarget;
    if (uiDepth == originalDepth && format == xiiImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    xiiImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetHeight(uiHeight);
    stepTarget->ResetAndAlloc(stepHeader);

    for (xiiUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (xiiUInt32 face = 0; face < numFaces; ++face)
      {
        for (xiiUInt32 z = 0; z < originalDepth; ++z)
        {
          for (xiiUInt32 x = 0; x < uiWidth; ++x)
          {
            const xiiSimdVec4f* filterSource = stepSource->GetPixelPointer<xiiSimdVec4f>(0, face, arrayIndex, x, 0, z);
            xiiSimdVec4f*       filterTarget = stepTarget->GetPixelPointer<xiiSimdVec4f>(0, face, arrayIndex, x, 0, z);
            FilterLine(originalHeight, filterSource, filterTarget, uiWidth, weights, firstSampleIndices, addressModeV, xiiSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  if (uiDepth != originalDepth)
  {
    xiiImageFilterWeights weights(*pFilter, originalDepth, uiDepth);
    firstSampleIndices.SetCount(uiDepth);
    for (xiiUInt32 z = 0; z < uiDepth; ++z)
    {
      firstSampleIndices[z] = weights.GetFirstSourceSampleIndex(z);
    }

    xiiImage* stepTarget;
    if (format == xiiImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    xiiImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetDepth(uiDepth);
    stepTarget->ResetAndAlloc(stepHeader);

    for (xiiUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (xiiUInt32 face = 0; face < numFaces; ++face)
      {
        for (xiiUInt32 y = 0; y < uiHeight; ++y)
        {
          for (xiiUInt32 x = 0; x < uiWidth; ++x)
          {
            const xiiSimdVec4f* filterSource = stepSource->GetPixelPointer<xiiSimdVec4f>(0, face, arrayIndex, x, y, 0);
            xiiSimdVec4f*       filterTarget = stepTarget->GetPixelPointer<xiiSimdVec4f>(0, face, arrayIndex, x, y, 0);
            FilterLine(originalHeight, filterSource, filterTarget, uiWidth * uiHeight, weights, firstSampleIndices, addressModeW, xiiSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  // Convert back to original format - no-op if stepSource and target are the same
  return xiiImageConversion::Convert(*stepSource, ref_target, format);
}

void xiiImageUtils::GenerateMipMaps(const xiiImageView& source, xiiImage& ref_target, const MipMapOptions& options)
{
  XII_PROFILE_SCOPE("xiiImageUtils::GenerateMipMaps");

  xiiImageHeader header = source.GetHeader();
  XII_ASSERT_DEV(header.GetImageFormat() == xiiImageFormat::R32G32B32A32_FLOAT, "The source image must be a RGBA 32-bit float format.");
  XII_ASSERT_DEV(&source != &ref_target, "Source and target must not be the same image.");

  // Make a local copy to be able to tweak some of the options
  xiiImageUtils::MipMapOptions mipMapOptions = options;

  // alpha thresholds with extreme values are not supported at the moment
  mipMapOptions.m_alphaThreshold = xiiMath::Clamp(mipMapOptions.m_alphaThreshold, 0.05f, 0.95f);

  // Enforce CLAMP addressing mode for cubemaps
  if (source.GetNumFaces() == 6)
  {
    mipMapOptions.m_addressModeU = xiiImageAddressMode::Clamp;
    mipMapOptions.m_addressModeV = xiiImageAddressMode::Clamp;
  }

  xiiUInt32 numMipMaps = header.ComputeNumberOfMipMaps();
  if (mipMapOptions.m_numMipMaps > 0 && mipMapOptions.m_numMipMaps < numMipMaps)
  {
    numMipMaps = mipMapOptions.m_numMipMaps;
  }
  header.SetNumMipLevels(numMipMaps);

  ref_target.ResetAndAlloc(header);

  for (xiiUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (xiiUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      xiiImageHeader currentMipMapHeader = header;
      currentMipMapHeader.SetNumMipLevels(1);
      currentMipMapHeader.SetNumFaces(1);
      currentMipMapHeader.SetNumArrayIndices(1);

      auto sourceView = source.GetSubImageView(0, face, arrayIndex).GetByteBlobPtr();
      auto targetView = ref_target.GetSubImageView(0, face, arrayIndex).GetByteBlobPtr();

      memcpy(targetView.GetPtr(), sourceView.GetPtr(), static_cast<size_t>(targetView.GetCount()));

      float targetCoverage = 0.0f;
      if (mipMapOptions.m_preserveCoverage)
      {
        targetCoverage = EvaluateAverageCoverage(source.GetSubImageView(0, face, arrayIndex).GetBlobPtr<xiiColor>(), mipMapOptions.m_alphaThreshold);
      }

      for (xiiUInt32 mipMapLevel = 0; mipMapLevel < numMipMaps - 1; mipMapLevel++)
      {
        xiiImageHeader nextMipMapHeader = currentMipMapHeader;
        nextMipMapHeader.SetWidth(xiiMath::Max(1u, nextMipMapHeader.GetWidth() / 2));
        nextMipMapHeader.SetHeight(xiiMath::Max(1u, nextMipMapHeader.GetHeight() / 2));
        nextMipMapHeader.SetDepth(xiiMath::Max(1u, nextMipMapHeader.GetDepth() / 2));

        auto     sourceData = ref_target.GetSubImageView(mipMapLevel, face, arrayIndex).GetByteBlobPtr();
        xiiImage currentMipMap;
        currentMipMap.ResetAndUseExternalStorage(currentMipMapHeader, sourceData);

        auto     dstData = ref_target.GetSubImageView(mipMapLevel + 1, face, arrayIndex).GetByteBlobPtr();
        xiiImage nextMipMap;
        nextMipMap.ResetAndUseExternalStorage(nextMipMapHeader, dstData);

        xiiImageUtils::Scale3D(currentMipMap, nextMipMap, nextMipMapHeader.GetWidth(), nextMipMapHeader.GetHeight(), nextMipMapHeader.GetDepth(), mipMapOptions.m_filter, mipMapOptions.m_addressModeU, mipMapOptions.m_addressModeV, mipMapOptions.m_addressModeW, mipMapOptions.m_borderColor)
          .IgnoreResult();

        if (mipMapOptions.m_preserveCoverage)
        {
          NormalizeCoverage(nextMipMap.GetBlobPtr<xiiColor>(), mipMapOptions.m_alphaThreshold, targetCoverage);
        }

        if (mipMapOptions.m_renormalizeNormals)
        {
          RenormalizeNormalMap(nextMipMap);
        }

        currentMipMapHeader = nextMipMapHeader;
      }
    }
  }
}

void xiiImageUtils::ReconstructNormalZ(xiiImage& ref_image)
{
  XII_PROFILE_SCOPE("xiiImageUtils::ReconstructNormalZ");

  XII_ASSERT_DEV(ref_image.GetImageFormat() == xiiImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  xiiSimdVec4f*       cur = ref_image.GetBlobPtr<xiiSimdVec4f>().GetPtr();
  xiiSimdVec4f* const end = ref_image.GetBlobPtr<xiiSimdVec4f>().GetEndPtr();

  xiiSimdFloat oneScalar = 1.0f;

  xiiSimdVec4f two(2.0f);

  xiiSimdVec4f minusOne(-1.0f);

  xiiSimdVec4f half(0.5f);

  for (; cur < end; cur++)
  {
    xiiSimdVec4f normal;
    // unpack from [0,1] to [-1, 1]
    normal = xiiSimdVec4f::MulAdd(*cur, two, minusOne);

    // compute Z component
    normal.SetZ((oneScalar - normal.Dot<2>(normal)).GetSqrt());

    // pack back to [0,1]
    *cur = xiiSimdVec4f::MulAdd(half, normal, half);
  }
}

void xiiImageUtils::RenormalizeNormalMap(xiiImage& ref_image)
{
  XII_PROFILE_SCOPE("xiiImageUtils::RenormalizeNormalMap");

  XII_ASSERT_DEV(ref_image.GetImageFormat() == xiiImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  xiiSimdVec4f*       start = ref_image.GetBlobPtr<xiiSimdVec4f>().GetPtr();
  xiiSimdVec4f* const end   = ref_image.GetBlobPtr<xiiSimdVec4f>().GetEndPtr();

  xiiSimdVec4f two(2.0f);

  xiiSimdVec4f minusOne(-1.0f);

  xiiSimdVec4f half(0.5f);

  for (; start < end; start++)
  {
    xiiSimdVec4f normal;
    normal = xiiSimdVec4f::MulAdd(*start, two, minusOne);
    normal.Normalize<3>();
    *start = xiiSimdVec4f::MulAdd(half, normal, half);
  }
}

void xiiImageUtils::AdjustRoughness(xiiImage& ref_roughnessMap, const xiiImageView& normalMap)
{
  XII_PROFILE_SCOPE("xiiImageUtils::AdjustRoughness");

  XII_ASSERT_DEV(ref_roughnessMap.GetImageFormat() == xiiImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");
  XII_ASSERT_DEV(normalMap.GetImageFormat() == xiiImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  XII_ASSERT_DEV(ref_roughnessMap.GetWidth() >= normalMap.GetWidth() && ref_roughnessMap.GetHeight() >= normalMap.GetHeight(), "The roughness map needs to be bigger or same size than the normal map.");

  xiiImage                     filteredNormalMap;
  xiiImageUtils::MipMapOptions options;

  // Box filter normal map without re-normalization so we have the average normal length in each mip map.
  if (ref_roughnessMap.GetWidth() != normalMap.GetWidth() || ref_roughnessMap.GetHeight() != normalMap.GetHeight())
  {
    xiiImage temp;
    xiiImageUtils::Scale(normalMap, temp, ref_roughnessMap.GetWidth(), ref_roughnessMap.GetHeight()).IgnoreResult();
    xiiImageUtils::RenormalizeNormalMap(temp);
    xiiImageUtils::GenerateMipMaps(temp, filteredNormalMap, options);
  }
  else
  {
    xiiImageUtils::GenerateMipMaps(normalMap, filteredNormalMap, options);
  }

  XII_ASSERT_DEV(ref_roughnessMap.GetNumMipLevels() == filteredNormalMap.GetNumMipLevels(), "Roughness and normal map must have the same number of mip maps");

  xiiSimdVec4f two(2.0f);
  xiiSimdVec4f minusOne(-1.0f);

  xiiUInt32 numMipLevels = ref_roughnessMap.GetNumMipLevels();
  for (xiiUInt32 mipLevel = 1; mipLevel < numMipLevels; ++mipLevel)
  {
    xiiBlobPtr<xiiSimdVec4f> roughnessData = ref_roughnessMap.GetSubImageView(mipLevel, 0, 0).GetBlobPtr<xiiSimdVec4f>();
    xiiBlobPtr<xiiSimdVec4f> normalData    = filteredNormalMap.GetSubImageView(mipLevel, 0, 0).GetBlobPtr<xiiSimdVec4f>();

    for (xiiUInt64 i = 0; i < roughnessData.GetCount(); ++i)
    {
      xiiSimdVec4f normal = xiiSimdVec4f::MulAdd(normalData[i], two, minusOne);

      float avgNormalLength = normal.GetLength<3>();
      if (avgNormalLength < 1.0f)
      {
        float avgNormalLengthSquare = avgNormalLength * avgNormalLength;
        float kappa                 = (3.0f * avgNormalLength - avgNormalLength * avgNormalLengthSquare) / (1.0f - avgNormalLengthSquare);
        float variance              = 1.0f / (2.0f * kappa);

        float oldRoughness = roughnessData[i].GetComponent<0>();
        float newRoughness = xiiMath::Sqrt(oldRoughness * oldRoughness + variance);

        roughnessData[i].Set(newRoughness);
      }
    }
  }
}

void xiiImageUtils::ChangeExposure(xiiImage& ref_image, float fBias)
{
  XII_ASSERT_DEV(ref_image.GetImageFormat() == xiiImageFormat::R32G32B32A32_FLOAT, "This function expects an RGBA 32 float image as input");

  if (fBias == 0.0f)
    return;

  XII_PROFILE_SCOPE("xiiImageUtils::ChangeExposure");

  const float multiplier = xiiMath::Pow2(fBias);

  for (xiiColor& col : ref_image.GetBlobPtr<xiiColor>())
  {
    col = multiplier * col;
  }
}

static xiiResult CopyImageRectToFace(xiiImage& ref_dstImg, const xiiImageView& srcImg, xiiUInt32 uiOffsetX, xiiUInt32 uiOffsetY, xiiUInt32 uiFaceIndex)
{
  xiiRectU32 r;
  r.x      = uiOffsetX;
  r.y      = uiOffsetY;
  r.width  = ref_dstImg.GetWidth();
  r.height = r.width;

  return xiiImageUtils::Copy(srcImg, r, ref_dstImg, xiiVec3U32(0), 0, uiFaceIndex);
}

xiiResult xiiImageUtils::CreateCubemapFromSingleFile(xiiImage& ref_dstImg, const xiiImageView& srcImg)
{
  XII_PROFILE_SCOPE("xiiImageUtils::CreateCubemapFromSingleFile");

  if (srcImg.GetNumFaces() == 6)
  {
    ref_dstImg.ResetAndCopy(srcImg);
    return XII_SUCCESS;
  }
  else if (srcImg.GetNumFaces() == 1)
  {
    if (srcImg.GetWidth() % 3 == 0 && srcImg.GetHeight() % 4 == 0 && srcImg.GetWidth() / 3 == srcImg.GetHeight() / 4)
    {
      // Vertical cube map layout
      //     +---+
      //     | Y+|
      // +---+---+---+
      // | X-| Z+| X+|
      // +---+---+---+
      //     | Y-|
      //     +---+
      //     | Z-|
      //     +---+
      const xiiUInt32 faceSize = srcImg.GetWidth() / 3;

      xiiImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      ref_dstImg.ResetAndAlloc(imgHeader);

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      XII_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 2, faceSize, 0));

      // Negative X face
      XII_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, 0, faceSize, 1));

      // Positive Y face
      XII_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, 0, 2));

      // Negative Y face
      XII_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 2, 3));

      // Positive Z face
      XII_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize, 4));

      // Negative Z face
      XII_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 3, 5));
      xiiImageUtils::RotateSubImage180(ref_dstImg, 0, 5);
    }
    else if (srcImg.GetWidth() % 4 == 0 && srcImg.GetHeight() % 3 == 0 && srcImg.GetWidth() / 4 == srcImg.GetHeight() / 3)
    {
      // Horizontal cube map layout
      //     +---+
      //     | Y+|
      // +---+---+---+---+
      // | X-| Z+| X+| Z-|
      // +---+---+---+---+
      //     | Y-|
      //     +---+
      const xiiUInt32 faceSize = srcImg.GetWidth() / 4;

      xiiImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      ref_dstImg.ResetAndAlloc(imgHeader);

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      XII_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 2, faceSize, 0));

      // Negative X face
      XII_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, 0, faceSize, 1));

      // Positive Y face
      XII_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, 0, 2));

      // Negative Y face
      XII_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 2, 3));

      // Positive Z face
      XII_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize, 4));

      // Negative Z face
      XII_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 3, faceSize, 5));
    }
    else
    {
      // Spherical mapping
      if (srcImg.GetWidth() % 4 != 0)
      {
        xiiLog::Error("Width of the input image should be a multiple of 4");
        return XII_FAILURE;
      }

      const xiiUInt32 faceSize = srcImg.GetWidth() / 4;

      xiiImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      ref_dstImg.ResetAndAlloc(imgHeader);

      // Corners of the UV space for the respective faces in model space
      const xiiVec3 faceCorners[] = {
        xiiVec3(0.5, 0.5, 0.5),   // X+
        xiiVec3(-0.5, 0.5, -0.5), // X-
        xiiVec3(-0.5, 0.5, -0.5), // Y+
        xiiVec3(-0.5, -0.5, 0.5), // Y-
        xiiVec3(-0.5, 0.5, 0.5),  // Z+
        xiiVec3(0.5, 0.5, -0.5)   // Z-
      };

      // UV Axis of the respective faces in model space
      const xiiVec3 faceAxis[] = {
        xiiVec3(0, 0, -1), xiiVec3(0, -1, 0), // X+
        xiiVec3(0, 0, 1), xiiVec3(0, -1, 0),  // X-
        xiiVec3(1, 0, 0), xiiVec3(0, 0, 1),   // Y+
        xiiVec3(1, 0, 0), xiiVec3(0, 0, -1),  // Y-
        xiiVec3(1, 0, 0), xiiVec3(0, -1, 0),  // Z+
        xiiVec3(-1, 0, 0), xiiVec3(0, -1, 0)  // Z-
      };

      const float fFaceSize  = (float)faceSize;
      const float fHalfPixel = 0.5f / fFaceSize;
      const float fPixel     = 1.0f / fFaceSize;

      const float fHalfSrcWidth = srcImg.GetWidth() / 2.0f;
      const float fSrcHeight    = (float)srcImg.GetHeight();

      const xiiUInt32 srcWidthMinus1  = srcImg.GetWidth() - 1;
      const xiiUInt32 srcHeightMinus1 = srcImg.GetHeight() - 1;

      XII_ASSERT_DEBUG(srcImg.GetRowPitch() % sizeof(xiiColor) == 0, "Row pitch should be a multiple of sizeof(xiiColor)");
      const xiiUInt64 srcRowPitch = srcImg.GetRowPitch() / sizeof(xiiColor);

      XII_ASSERT_DEBUG(ref_dstImg.GetRowPitch() % sizeof(xiiColor) == 0, "Row pitch should be a multiple of sizeof(xiiColor)");
      const xiiUInt64 faceRowPitch = ref_dstImg.GetRowPitch() / sizeof(xiiColor);

      const xiiColor* srcData = srcImg.GetPixelPointer<xiiColor>();
      const float     InvPi   = 1.0f / xiiMath::Pi<float>();

      for (xiiUInt32 faceIndex = 0; faceIndex < 6; faceIndex++)
      {
        xiiColor* faceData = ref_dstImg.GetPixelPointer<xiiColor>(0, faceIndex);
        for (xiiUInt32 y = 0; y < faceSize; y++)
        {
          const float dstV = (float)y * fPixel + fHalfPixel;

          for (xiiUInt32 x = 0; x < faceSize; x++)
          {
            const float   dstU          = (float)x * fPixel + fHalfPixel;
            const xiiVec3 modelSpacePos = faceCorners[faceIndex] + dstU * faceAxis[faceIndex * 2] + dstV * faceAxis[faceIndex * 2 + 1];
            const xiiVec3 modelSpaceDir = modelSpacePos.GetNormalized();

            const float phi   = xiiMath::ATan2(modelSpaceDir.x, modelSpaceDir.z).GetRadian() + xiiMath::Pi<float>();
            const float r     = xiiMath::Sqrt(modelSpaceDir.x * modelSpaceDir.x + modelSpaceDir.z * modelSpaceDir.z);
            const float theta = xiiMath::ATan2(modelSpaceDir.y, r).GetRadian() + xiiMath::Pi<float>() * 0.5f;

            XII_ASSERT_DEBUG(phi >= 0.0f && phi <= 2.0f * xiiMath::Pi<float>(), "");
            XII_ASSERT_DEBUG(theta >= 0.0f && theta <= xiiMath::Pi<float>(), "");

            const float srcU = phi * InvPi * fHalfSrcWidth;
            const float srcV = (1.0f - theta * InvPi) * fSrcHeight;

            xiiUInt32 x1 = (xiiUInt32)xiiMath::Floor(srcU);
            xiiUInt32 x2 = x1 + 1;
            xiiUInt32 y1 = (xiiUInt32)xiiMath::Floor(srcV);
            xiiUInt32 y2 = y1 + 1;

            const float fracX = srcU - x1;
            const float fracY = srcV - y1;

            x1 = xiiMath::Clamp(x1, 0u, srcWidthMinus1);
            x2 = xiiMath::Clamp(x2, 0u, srcWidthMinus1);
            y1 = xiiMath::Clamp(y1, 0u, srcHeightMinus1);
            y2 = xiiMath::Clamp(y2, 0u, srcHeightMinus1);

            xiiColor A = srcData[x1 + y1 * srcRowPitch];
            xiiColor B = srcData[x2 + y1 * srcRowPitch];
            xiiColor C = srcData[x1 + y2 * srcRowPitch];
            xiiColor D = srcData[x2 + y2 * srcRowPitch];

            xiiColor interpolated          = A * (1 - fracX) * (1 - fracY) + B * (fracX) * (1 - fracY) + C * (1 - fracX) * fracY + D * fracX * fracY;
            faceData[x + y * faceRowPitch] = interpolated;
          }
        }
      }
    }

    return XII_SUCCESS;
  }

  xiiLog::Error("Unexpected number of faces in cubemap input image.");
  return XII_FAILURE;
}

xiiResult xiiImageUtils::CreateCubemapFrom6Files(xiiImage& ref_dstImg, const xiiImageView* pSourceImages)
{
  XII_PROFILE_SCOPE("xiiImageUtils::CreateCubemapFrom6Files");

  xiiImageHeader header = pSourceImages[0].GetHeader();
  header.SetNumFaces(6);

  if (header.GetWidth() != header.GetHeight())
    return XII_FAILURE;

  if (!xiiMath::IsPowerOf2(header.GetWidth()))
    return XII_FAILURE;

  ref_dstImg.ResetAndAlloc(header);

  for (xiiUInt32 i = 0; i < 6; ++i)
  {
    if (pSourceImages[i].GetImageFormat() != ref_dstImg.GetImageFormat())
      return XII_FAILURE;

    if (pSourceImages[i].GetWidth() != ref_dstImg.GetWidth())
      return XII_FAILURE;

    if (pSourceImages[i].GetHeight() != ref_dstImg.GetHeight())
      return XII_FAILURE;

    XII_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, pSourceImages[i], 0, 0, i));
  }

  return XII_SUCCESS;
}

xiiResult xiiImageUtils::CreateVolumeTextureFromSingleFile(xiiImage& ref_dstImg, const xiiImageView& srcImg)
{
  XII_PROFILE_SCOPE("xiiImageUtils::CreateVolumeTextureFromSingleFile");

  const xiiUInt32 uiWidthHeight = srcImg.GetHeight();
  const xiiUInt32 uiDepth       = srcImg.GetWidth() / uiWidthHeight;

  if (!xiiMath::IsPowerOf2(uiWidthHeight))
    return XII_FAILURE;
  if (!xiiMath::IsPowerOf2(uiDepth))
    return XII_FAILURE;

  xiiImageHeader header;
  header.SetWidth(uiWidthHeight);
  header.SetHeight(uiWidthHeight);
  header.SetDepth(uiDepth);
  header.SetImageFormat(srcImg.GetImageFormat());

  ref_dstImg.ResetAndAlloc(header);

  const xiiImageView view = srcImg.GetSubImageView();

  for (xiiUInt32 d = 0; d < uiDepth; ++d)
  {
    xiiRectU32 r;
    r.x      = uiWidthHeight * d;
    r.y      = 0;
    r.width  = uiWidthHeight;
    r.height = uiWidthHeight;

    XII_SUCCEED_OR_RETURN(Copy(view, r, ref_dstImg, xiiVec3U32(0, 0, d)));
  }

  return XII_SUCCESS;
}

xiiColor xiiImageUtils::NearestSample(const xiiImageView& image, xiiImageAddressMode::Enum addressMode, xiiVec2 vUv)
{
  XII_ASSERT_DEBUG(image.GetDepth() == 1 && image.GetNumFaces() == 1 && image.GetNumArrayIndices() == 1, "Only 2d images are supported");
  XII_ASSERT_DEBUG(image.GetImageFormat() == xiiImageFormat::R32G32B32A32_FLOAT, "Unsupported format");

  return NearestSample(image.GetPixelPointer<xiiColor>(), image.GetWidth(), image.GetHeight(), addressMode, vUv);
}

xiiColor xiiImageUtils::NearestSample(const xiiColor* pPixelPointer, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiImageAddressMode::Enum addressMode, xiiVec2 vUv)
{
  const xiiInt32 w = uiWidth;
  const xiiInt32 h = uiHeight;

  vUv                 = vUv.CompMul(xiiVec2(static_cast<float>(w), static_cast<float>(h)));
  const xiiInt32 intX = (xiiInt32)xiiMath::Floor(vUv.x);
  const xiiInt32 intY = (xiiInt32)xiiMath::Floor(vUv.y);

  xiiInt32 x = intX;
  xiiInt32 y = intY;

  if (addressMode == xiiImageAddressMode::Clamp)
  {
    x = xiiMath::Clamp(x, 0, w - 1);
    y = xiiMath::Clamp(y, 0, h - 1);
  }
  else if (addressMode == xiiImageAddressMode::Repeat)
  {
    x = x % w;
    x = x < 0 ? x + w : x;
    y = y % h;
    y = y < 0 ? y + h : y;
  }
  else
  {
    XII_ASSERT_NOT_IMPLEMENTED;
  }

  return *(pPixelPointer + (y * w) + x);
}

xiiColor xiiImageUtils::BilinearSample(const xiiImageView& image, xiiImageAddressMode::Enum addressMode, xiiVec2 vUv)
{
  XII_ASSERT_DEBUG(image.GetDepth() == 1 && image.GetNumFaces() == 1 && image.GetNumArrayIndices() == 1, "Only 2d images are supported");
  XII_ASSERT_DEBUG(image.GetImageFormat() == xiiImageFormat::R32G32B32A32_FLOAT, "Unsupported format");

  return BilinearSample(image.GetPixelPointer<xiiColor>(), image.GetWidth(), image.GetHeight(), addressMode, vUv);
}

xiiColor xiiImageUtils::BilinearSample(const xiiColor* pData, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiImageAddressMode::Enum addressMode, xiiVec2 vUv)
{
  xiiInt32 w = uiWidth;
  xiiInt32 h = uiHeight;

  vUv                      = vUv.CompMul(xiiVec2(static_cast<float>(w), static_cast<float>(h))) - xiiVec2(0.5f);
  const float    floorX    = xiiMath::Floor(vUv.x);
  const float    floorY    = xiiMath::Floor(vUv.y);
  const float    fractionX = vUv.x - floorX;
  const float    fractionY = vUv.y - floorY;
  const xiiInt32 intX      = (xiiInt32)floorX;
  const xiiInt32 intY      = (xiiInt32)floorY;

  xiiColor c[4];
  for (xiiUInt32 i = 0; i < 4; ++i)
  {
    xiiInt32 x = intX + (i % 2);
    xiiInt32 y = intY + (i / 2);

    if (addressMode == xiiImageAddressMode::Clamp)
    {
      x = xiiMath::Clamp(x, 0, w - 1);
      y = xiiMath::Clamp(y, 0, h - 1);
    }
    else if (addressMode == xiiImageAddressMode::Repeat)
    {
      x = x % w;
      x = x < 0 ? x + w : x;
      y = y % h;
      y = y < 0 ? y + h : y;
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
    }

    c[i] = *(pData + (y * w) + x);
  }

  const xiiColor cr0 = xiiMath::Lerp(c[0], c[1], fractionX);
  const xiiColor cr1 = xiiMath::Lerp(c[2], c[3], fractionX);

  return xiiMath::Lerp(cr0, cr1, fractionY);
}

xiiResult xiiImageUtils::CopyChannel(xiiImage& ref_dstImg, xiiUInt8 uiDstChannelIdx, const xiiImage& srcImg, xiiUInt8 uiSrcChannelIdx)
{
  XII_PROFILE_SCOPE("xiiImageUtils::CopyChannel");

  if (uiSrcChannelIdx >= 4 || uiDstChannelIdx >= 4)
    return XII_FAILURE;

  if (ref_dstImg.GetImageFormat() != xiiImageFormat::R32G32B32A32_FLOAT)
    return XII_FAILURE;

  if (srcImg.GetImageFormat() != ref_dstImg.GetImageFormat())
    return XII_FAILURE;

  if (srcImg.GetWidth() != ref_dstImg.GetWidth())
    return XII_FAILURE;

  if (srcImg.GetHeight() != ref_dstImg.GetHeight())
    return XII_FAILURE;

  const xiiUInt32 uiNumPixels = srcImg.GetWidth() * srcImg.GetHeight();
  const float*    pSrcPixel   = srcImg.GetPixelPointer<float>();
  float*          pDstPixel   = ref_dstImg.GetPixelPointer<float>();

  pSrcPixel += uiSrcChannelIdx;
  pDstPixel += uiDstChannelIdx;

  for (xiiUInt32 i = 0; i < uiNumPixels; ++i)
  {
    *pDstPixel = *pSrcPixel;

    pSrcPixel += 4;
    pDstPixel += 4;
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageUtils);
