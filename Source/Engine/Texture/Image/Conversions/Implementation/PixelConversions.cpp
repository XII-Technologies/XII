/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/Math/Float16.h>
#include <Texture/Image/Conversions/PixelConversions.h>
#include <Texture/Image/ImageConversion.h>

#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE) || (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX)
#  if XII_SSE_LEVEL >= XII_SSE_20
#    include <emmintrin.h>
#  endif

#  if XII_SSE_LEVEL >= XII_SSE_30
#    include <tmmintrin.h>
#  endif
#endif

namespace
{
  // 3D vector: 11/11/10 floating-point components.
  // The 3D vector is packed into 32 bits as follows: a 5-bit biased exponent and 6-bit mantissa for x component, a 5-bit biased exponent and
  // 6-bit mantissa for y component, a 5-bit biased exponent and a 5-bit mantissa for z. The z component is stored in the most significant bits
  // and the x component in the least significant bits. No sign bits so all partial-precision numbers are positive.
  // (Z10Y11X11): [32] ZZZZZzzz zzzYYYYY yyyyyyXX XXXxxxxx [0
  union R11G11B10
  {
    struct Parts
    {
      xiiUInt32 xm : 6; // x-mantissa
      xiiUInt32 xe : 5; // x-exponent
      xiiUInt32 ym : 6; // y-mantissa
      xiiUInt32 ye : 5; // y-exponent
      xiiUInt32 zm : 5; // z-mantissa
      xiiUInt32 ze : 5; // z-exponent
    } p;
    xiiUInt32 v;
  };
} // namespace

xiiColorBaseUB xiiDecompressA4B4G4R4(xiiUInt16 uiColor)
{
  xiiColorBaseUB result;
  result.r = ((uiColor & 0xF000u) * 17) >> 12;
  result.g = ((uiColor & 0x0F00u) * 17) >> 8;
  result.b = ((uiColor & 0x00F0u) * 17) >> 4;
  result.a = ((uiColor & 0x000Fu) * 17);
  return result;
}

xiiUInt16 xiiCompressA4B4G4R4(xiiColorBaseUB color)
{
  xiiUInt32 r = (color.r * 15 + 135) >> 8;
  xiiUInt32 g = (color.g * 15 + 135) >> 8;
  xiiUInt32 b = (color.b * 15 + 135) >> 8;
  xiiUInt32 a = (color.a * 15 + 135) >> 8;
  return static_cast<xiiUInt16>((r << 12) | (g << 8) | (b << 4) | a);
}

xiiColorBaseUB xiiDecompressB4G4R4A4(xiiUInt16 uiColor)
{
  xiiColorBaseUB result;
  result.r = ((uiColor & 0x0F00u) * 17) >> 8;
  result.g = ((uiColor & 0x00F0u) * 17) >> 4;
  result.b = ((uiColor & 0x000Fu) * 17);
  result.a = ((uiColor & 0xF000u) * 17) >> 12;
  return result;
}

xiiUInt16 xiiCompressB4G4R4A4(xiiColorBaseUB color)
{
  xiiUInt32 r = (color.r * 15 + 135) >> 8;
  xiiUInt32 g = (color.g * 15 + 135) >> 8;
  xiiUInt32 b = (color.b * 15 + 135) >> 8;
  xiiUInt32 a = (color.a * 15 + 135) >> 8;
  return static_cast<xiiUInt16>((a << 12) | (r << 8) | (g << 4) | b);
}

xiiColorBaseUB xiiDecompressB5G6R5(xiiUInt16 uiColor)
{
  xiiColorBaseUB result;
  result.r = static_cast<xiiUInt8>(((uiColor & 0xF800u) * 527 + 47104) >> 17);
  result.g = static_cast<xiiUInt8>(((uiColor & 0x07E0u) * 259 + 1056) >> 11);
  result.b = static_cast<xiiUInt8>(((uiColor & 0x001Fu) * 527 + 23) >> 6);
  result.a = 0xFF;

  return result;
}

xiiUInt16 xiiCompressB5G6R5(xiiColorBaseUB color)
{
  xiiUInt32 r = (color.r * 249 + 1024) >> 11;
  xiiUInt32 g = (color.g * 253 + 512) >> 10;
  xiiUInt32 b = (color.b * 249 + 1024) >> 11;
  return static_cast<xiiUInt16>((r << 11) | (g << 5) | b);
}

xiiColorBaseUB xiiDecompressB5G5R5X1(xiiUInt16 uiColor)
{
  xiiColorBaseUB result;
  result.r = static_cast<xiiUInt8>(((uiColor & 0x7C00u) * 527 + 23552) >> 16);
  result.g = static_cast<xiiUInt8>(((uiColor & 0x03E0u) * 527 + 736) >> 11);
  result.b = static_cast<xiiUInt8>(((uiColor & 0x001Fu) * 527 + 23) >> 6);
  result.a = 0xFF;
  return result;
}

xiiUInt16 xiiCompressB5G5R5X1(xiiColorBaseUB color)
{
  xiiUInt32 r = (color.r * 249 + 1024) >> 11;
  xiiUInt32 g = (color.g * 249 + 1024) >> 11;
  xiiUInt32 b = (color.b * 249 + 1024) >> 11;
  return static_cast<xiiUInt16>((1 << 15) | (r << 10) | (g << 5) | b);
}

xiiColorBaseUB xiiDecompressB5G5R5A1(xiiUInt16 uiColor)
{
  xiiColorBaseUB result;
  result.r = static_cast<xiiUInt8>(((uiColor & 0x7C00u) * 527 + 23552) >> 16);
  result.g = static_cast<xiiUInt8>(((uiColor & 0x03E0u) * 527 + 736) >> 11);
  result.b = static_cast<xiiUInt8>(((uiColor & 0x001Fu) * 527 + 23) >> 6);
  result.a = static_cast<xiiUInt8>(((uiColor & 0x8000u) * 255) >> 15);
  return result;
}

xiiUInt16 xiiCompressB5G5R5A1(xiiColorBaseUB color)
{
  xiiUInt32 r = (color.r * 249 + 1024) >> 11;
  xiiUInt32 g = (color.g * 249 + 1024) >> 11;
  xiiUInt32 b = (color.b * 249 + 1024) >> 11;
  xiiUInt32 a = (color.a) >> 7;
  return static_cast<xiiUInt16>((a << 15) | (r << 10) | (g << 5) | b);
}

xiiColorBaseUB xiiDecompressX1B5G5R5(xiiUInt16 uiColor)
{
  xiiColorBaseUB result;
  result.r = static_cast<xiiUInt8>(((uiColor & 0xF800u) * 527 + 23552) >> 17);
  result.g = static_cast<xiiUInt8>(((uiColor & 0x07C0u) * 527 + 736) >> 12);
  result.b = static_cast<xiiUInt8>(((uiColor & 0x003Eu) * 527 + 23) >> 7);
  result.a = 0xFF;
  return result;
}

xiiUInt16 xiiCompressX1B5G5R5(xiiColorBaseUB color)
{
  xiiUInt32 r = (color.r * 249 + 1024) >> 11;
  xiiUInt32 g = (color.g * 249 + 1024) >> 11;
  xiiUInt32 b = (color.b * 249 + 1024) >> 11;
  return static_cast<xiiUInt16>((r << 11) | (g << 6) | (b << 1) | 1);
}

xiiColorBaseUB xiiDecompressA1B5G5R5(xiiUInt16 uiColor)
{
  xiiColorBaseUB result;
  result.r = static_cast<xiiUInt8>(((uiColor & 0xF800u) * 527 + 23552) >> 17);
  result.g = static_cast<xiiUInt8>(((uiColor & 0x07C0u) * 527 + 736) >> 12);
  result.b = static_cast<xiiUInt8>(((uiColor & 0x003Eu) * 527 + 23) >> 7);
  result.a = static_cast<xiiUInt8>((uiColor & 0x0001u) * 255);
  return result;
}

xiiUInt16 xiiCompressA1B5G5R5(xiiColorBaseUB color)
{
  xiiUInt32 r = (color.r * 249 + 1024) >> 11;
  xiiUInt32 g = (color.g * 249 + 1024) >> 11;
  xiiUInt32 b = (color.b * 249 + 1024) >> 11;
  xiiUInt32 a = color.a >> 7;
  return static_cast<xiiUInt16>((r << 11) | (g << 6) | (b << 1) | a);
}

template <xiiColorBaseUB (*decompressFunc)(xiiUInt16), xiiGALResourceFormat::Enum templateSourceFormat>
class xiiImageConversionStep_Decompress16bpp : xiiImageConversionStepLinear
{
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    xiiEnum<xiiGALResourceFormat> sourceFormatSrgb = xiiGALResourceFormat::AsSrgb(templateSourceFormat);
    XII_ASSERT_DEV(sourceFormatSrgb != templateSourceFormat, "Format '{}' should have a corresponding sRGB format.", xiiArgEnum(xiiEnum<xiiGALResourceFormat>(templateSourceFormat)));

    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(templateSourceFormat, xiiGALResourceFormat::RGBA8UNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(sourceFormatSrgb, xiiGALResourceFormat::RGBA8UNormalizedSRGB, xiiImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);
    XII_IGNORE_UNUSED(targetFormat);

    xiiUInt32 uiSourceStride = 2;
    xiiUInt32 uiTargetStride = 4;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      *reinterpret_cast<xiiColorBaseUB*>(pTargetPointer) = decompressFunc(*reinterpret_cast<const xiiUInt16*>(pSourcePointer));

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);

      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

template <xiiUInt16 (*compressFunc)(xiiColorBaseUB), xiiGALResourceFormat::Enum templateTargetFormat>
class xiiImageConversionStep_Compress16bpp : xiiImageConversionStepLinear
{
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    xiiEnum<xiiGALResourceFormat> targetFormatSrgb = xiiGALResourceFormat::AsSrgb(templateTargetFormat);
    XII_ASSERT_DEV(targetFormatSrgb != templateTargetFormat, "Format '{}' should have a corresponding sRGB format.", xiiArgEnum(xiiEnum<xiiGALResourceFormat>(templateTargetFormat)));

    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UNormalized, templateTargetFormat, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UNormalizedSRGB, targetFormatSrgb, xiiImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);
    XII_IGNORE_UNUSED(targetFormat);

    xiiUInt32 uiSourceStride = 4;
    xiiUInt32 uiTargetStride = 2;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      *reinterpret_cast<xiiUInt16*>(pTargetPointer) = compressFunc(*reinterpret_cast<const xiiColorBaseUB*>(pSourcePointer));

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);

      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX

static bool IsAligned(const void* pPointer)
{
  return reinterpret_cast<size_t>(pPointer) % 16 == 0;
}

#endif

struct xiiImageSwizzleConversion32_2103 : public xiiImageConversionStepLinear
{
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::BGRA8UNormalized, xiiGALResourceFormat::RGBA8UNormalized, xiiImageConversionFlags::InPlace),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UNormalized, xiiGALResourceFormat::BGRA8UNormalized, xiiImageConversionFlags::InPlace),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UNormalized, xiiGALResourceFormat::BGRX8UNormalized, xiiImageConversionFlags::InPlace),
      xiiImageConversionEntry(xiiGALResourceFormat::BGRA8UNormalizedSRGB, xiiGALResourceFormat::RGBA8UNormalizedSRGB, xiiImageConversionFlags::InPlace),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UNormalizedSRGB, xiiGALResourceFormat::BGRA8UNormalizedSRGB, xiiImageConversionFlags::InPlace),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UNormalizedSRGB, xiiGALResourceFormat::BGRX8UNormalizedSRGB, xiiImageConversionFlags::InPlace),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);
    XII_IGNORE_UNUSED(targetFormat);

    xiiUInt32 uiSourceStride = 4;
    xiiUInt32 uiTargetStride = 4;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

#if XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX
    if (IsAligned(pSourcePointer) && IsAligned(pTargetPointer))
    {
#  if XII_SSE_LEVEL >= XII_SSE_30
      const xiiUInt32 uiElementsPerBatch = 8;

      __m128i shuffleMask = _mm_set_epi8(15, 12, 13, 14, 11, 8, 9, 10, 7, 4, 5, 6, 3, 0, 1, 2);

      // Intel optimization manual, Color Pixel Format Conversion Using SSE3
      while (uiElementCount >= uiElementsPerBatch)
      {
        __m128i in0 = reinterpret_cast<const __m128i*>(pSourcePointer)[0];
        __m128i in1 = reinterpret_cast<const __m128i*>(pSourcePointer)[1];

        reinterpret_cast<__m128i*>(pTargetPointer)[0] = _mm_shuffle_epi8(in0, shuffleMask);
        reinterpret_cast<__m128i*>(pTargetPointer)[1] = _mm_shuffle_epi8(in1, shuffleMask);

        pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride * uiElementsPerBatch);
        pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride * uiElementsPerBatch);

        uiElementCount -= uiElementsPerBatch;
      }
#  else
      const xiiUInt32 uiElementsPerBatch = 8;

      __m128i mask1 = _mm_set1_epi32(0xff00ff00);
      __m128i mask2 = _mm_set1_epi32(0x00ff00ff);

      // Intel optimization manual, Color Pixel Format Conversion Using SSE2
      while (uiElementCount >= uiElementsPerBatch)
      {
        __m128i in0 = reinterpret_cast<const __m128i*>(pSourcePointer)[0];
        __m128i in1 = reinterpret_cast<const __m128i*>(pSourcePointer)[1];

        reinterpret_cast<__m128i*>(pTargetPointer)[0] = _mm_or_si128(_mm_and_si128(in0, mask1), _mm_and_si128(_mm_or_si128(_mm_slli_epi32(in0, 16), _mm_srli_epi32(in0, 16)), mask2));
        reinterpret_cast<__m128i*>(pTargetPointer)[1] = _mm_or_si128(_mm_and_si128(in1, mask1), _mm_and_si128(_mm_or_si128(_mm_slli_epi32(in1, 16), _mm_srli_epi32(in1, 16)), mask2));

        pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride * uiElementsPerBatch);
        pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride * uiElementsPerBatch);
        uiElementCount -= uiElementsPerBatch;
      }
#  endif
    }
#endif

    while (uiElementCount)
    {
      xiiUInt8 a, b, c, d;
      a                                              = reinterpret_cast<const xiiUInt8*>(pSourcePointer)[2];
      b                                              = reinterpret_cast<const xiiUInt8*>(pSourcePointer)[1];
      c                                              = reinterpret_cast<const xiiUInt8*>(pSourcePointer)[0];
      d                                              = reinterpret_cast<const xiiUInt8*>(pSourcePointer)[3];
      reinterpret_cast<xiiUInt8*>(pTargetPointer)[0] = a;
      reinterpret_cast<xiiUInt8*>(pTargetPointer)[1] = b;
      reinterpret_cast<xiiUInt8*>(pTargetPointer)[2] = c;
      reinterpret_cast<xiiUInt8*>(pTargetPointer)[3] = d;

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);

      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

struct xiiImageConversion_BGRX_BGRA : public xiiImageConversionStepLinear
{
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::BGRX8UNormalized, xiiGALResourceFormat::BGRA8UNormalized, xiiImageConversionFlags::InPlace),
      xiiImageConversionEntry(xiiGALResourceFormat::BGRX8UNormalizedSRGB, xiiGALResourceFormat::BGRA8UNormalizedSRGB, xiiImageConversionFlags::InPlace),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);
    XII_IGNORE_UNUSED(targetFormat);

    xiiUInt32 uiSourceStride = 4;
    xiiUInt32 uiTargetStride = 4;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && XII_SSE_LEVEL >= XII_SSE_20
    if (IsAligned(pSourcePointer) && IsAligned(pTargetPointer))
    {
      const xiiUInt32 uiElementsPerBatch = 4;

      __m128i mask = _mm_set1_epi32(0xFF000000);

      while (uiElementCount >= uiElementsPerBatch)
      {
        const __m128i* pSourcePtr = reinterpret_cast<const __m128i*>(pSourcePointer);
        __m128i*       pTargetPtr = reinterpret_cast<__m128i*>(pTargetPointer);

        pTargetPtr[0] = _mm_or_si128(pSourcePtr[0], mask);

        pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride * uiElementsPerBatch);
        pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride * uiElementsPerBatch);
        uiElementCount -= uiElementsPerBatch;
      }
    }
#endif

    while (uiElementCount)
    {
      xiiUInt32 x = *(reinterpret_cast<const xiiUInt32*>(pSourcePointer));

#if XII_ENABLED(XII_PLATFORM_LITTLE_ENDIAN)
      x |= 0xFF000000;
#else
      x |= 0x000000FF;
#endif

      *(reinterpret_cast<xiiUInt32*>(pTargetPointer)) = x;

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);

      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

class xiiImageConversion_F32_U8 : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R32Float, xiiGALResourceFormat::R8UNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG32Float, xiiGALResourceFormat::RG8UNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32Float, xiiGALResourceFormat::RGBA8UNormalized, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels.
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);
    uiElementCount *= targetFormatDescription.GetElementSize();

    xiiUInt32 uiSourceStride = 4;
    xiiUInt32 uiTargetStride = 1;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

#if (XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_SSE || XII_SIMD_IMPLEMENTATION == XII_SIMD_IMPLEMENTATION_AVX) && XII_SSE_LEVEL >= XII_SSE_20
    {
      const xiiUInt32 uiElementsPerBatch = 16;

      __m128 zero  = _mm_setzero_ps();
      __m128 one   = _mm_set1_ps(1.0f);
      __m128 scale = _mm_set1_ps(255.0f);
      __m128 half  = _mm_set1_ps(0.5f);

      while (uiElementCount >= uiElementsPerBatch)
      {
        __m128 float0 = _mm_loadu_ps(static_cast<const float*>(pSourcePointer) + 0);
        __m128 float1 = _mm_loadu_ps(static_cast<const float*>(pSourcePointer) + 4);
        __m128 float2 = _mm_loadu_ps(static_cast<const float*>(pSourcePointer) + 8);
        __m128 float3 = _mm_loadu_ps(static_cast<const float*>(pSourcePointer) + 12);

        // Clamp NaN to zero
        float0 = _mm_and_ps(_mm_cmpord_ps(float0, zero), float0);
        float1 = _mm_and_ps(_mm_cmpord_ps(float1, zero), float1);
        float2 = _mm_and_ps(_mm_cmpord_ps(float2, zero), float2);
        float3 = _mm_and_ps(_mm_cmpord_ps(float3, zero), float3);

        // Saturate
        float0 = _mm_max_ps(zero, _mm_min_ps(one, float0));
        float1 = _mm_max_ps(zero, _mm_min_ps(one, float1));
        float2 = _mm_max_ps(zero, _mm_min_ps(one, float2));
        float3 = _mm_max_ps(zero, _mm_min_ps(one, float3));

        float0 = _mm_mul_ps(float0, scale);
        float1 = _mm_mul_ps(float1, scale);
        float2 = _mm_mul_ps(float2, scale);
        float3 = _mm_mul_ps(float3, scale);

        // Add 0.5f and truncate for rounding as required by D3D spec
        float0 = _mm_add_ps(float0, half);
        float1 = _mm_add_ps(float1, half);
        float2 = _mm_add_ps(float2, half);
        float3 = _mm_add_ps(float3, half);

        __m128i int0 = _mm_cvttps_epi32(float0);
        __m128i int1 = _mm_cvttps_epi32(float1);
        __m128i int2 = _mm_cvttps_epi32(float2);
        __m128i int3 = _mm_cvttps_epi32(float3);

        __m128i short0 = _mm_packs_epi32(int0, int1);
        __m128i short1 = _mm_packs_epi32(int2, int3);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(pTargetPointer), _mm_packus_epi16(short0, short1));

        pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride * uiElementsPerBatch);
        pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride * uiElementsPerBatch);
        uiElementCount -= uiElementsPerBatch;
      }
    }
#endif

    while (uiElementCount)
    {
      *reinterpret_cast<xiiUInt8*>(pTargetPointer) = xiiMath::ColorFloatToByte(*reinterpret_cast<const float*>(pSourcePointer));

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);

      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

class xiiImageConversion_F32_sRGB : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32Float, xiiGALResourceFormat::RGBA8UNormalizedSRGB, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);
    XII_IGNORE_UNUSED(targetFormat);

    xiiUInt32 uiSourceStride = 16;
    xiiUInt32 uiTargetStride = 4;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      *reinterpret_cast<xiiColorGammaUB*>(pTargetPointer) = *reinterpret_cast<const xiiColor*>(pSourcePointer);

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);
      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

class xiiImageConversion_F32_U16 : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R32Float, xiiGALResourceFormat::R16UNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG32Float, xiiGALResourceFormat::RG16UNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32Float, xiiGALResourceFormat::RGBA16UNormalized, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels.
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);
    uiElementCount *= targetFormatDescription.GetElementSize();

    xiiUInt32 uiSourceStride = 4;
    xiiUInt32 uiTargetStride = 2;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      *reinterpret_cast<xiiUInt16*>(pTargetPointer) = xiiMath::ColorFloatToShort(*reinterpret_cast<const float*>(pSourcePointer));

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);

      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

class xiiImageConversion_F32_F16 : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R32Float, xiiGALResourceFormat::R16Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG32Float, xiiGALResourceFormat::RG16Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32Float, xiiGALResourceFormat::RGBA16Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels.
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);
    uiElementCount *= targetFormatDescription.GetElementSize();

    xiiUInt32 uiSourceStride = 4;
    xiiUInt32 uiTargetStride = 2;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {

      *reinterpret_cast<xiiFloat16*>(pTargetPointer) = *reinterpret_cast<const float*>(pSourcePointer);

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);
      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

class xiiImageConversion_F32_S8 : public xiiImageConversionStepLinear
{
public:
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R32Float, xiiGALResourceFormat::R8SNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG32Float, xiiGALResourceFormat::RG8SNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32Float, xiiGALResourceFormat::RGBA8SNormalized, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels.
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);
    uiElementCount *= targetFormatDescription.GetElementSize();

    xiiUInt32 uiSourceStride = 4;
    xiiUInt32 uiTargetStride = 1;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      *reinterpret_cast<xiiInt8*>(pTargetPointer) = xiiMath::ColorFloatToSignedByte(*reinterpret_cast<const float*>(pSourcePointer));

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);

      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

class xiiImageConversion_U8_F32 : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R8UNormalized, xiiGALResourceFormat::R32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG8UNormalized, xiiGALResourceFormat::RG32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UNormalized, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels.
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);
    uiElementCount *= targetFormatDescription.GetElementSize();

    xiiUInt32 uiSourceStride = 1;
    xiiUInt32 uiTargetStride = 4;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      *reinterpret_cast<float*>(pTargetPointer) = xiiMath::ColorByteToFloat(*reinterpret_cast<const xiiUInt8*>(pSourcePointer));

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);

      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

class xiiImageConversion_sRGB_F32 : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UNormalizedSRGB, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);
    XII_IGNORE_UNUSED(targetFormat);

    xiiUInt32 uiSourceStride = 4;
    xiiUInt32 uiTargetStride = 16;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      *reinterpret_cast<xiiColor*>(pTargetPointer) = *reinterpret_cast<const xiiColorGammaUB*>(pSourcePointer);

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);
      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

class xiiImageConversion_U16_F32 : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R16UNormalized, xiiGALResourceFormat::R32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG16UNormalized, xiiGALResourceFormat::RG32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16UNormalized, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels.
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);
    uiElementCount *= targetFormatDescription.GetElementSize();

    xiiUInt32 uiSourceStride = 2;
    xiiUInt32 uiTargetStride = 4;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      *reinterpret_cast<float*>(pTargetPointer) = xiiMath::ColorShortToFloat(*reinterpret_cast<const xiiUInt16*>(pSourcePointer));

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);

      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

class xiiImageConversion_S16_F32 : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R16SNormalized, xiiGALResourceFormat::R32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG16SNormalized, xiiGALResourceFormat::RG32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16SNormalized, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);
    XII_IGNORE_UNUSED(targetFormat);

    // Work with single channels instead of pixels.
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);
    uiElementCount *= targetFormatDescription.GetElementSize();

    xiiUInt32 uiSourceStride = 2;
    xiiUInt32 uiTargetStride = 4;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      *reinterpret_cast<float*>(pTargetPointer) = xiiMath::ColorSignedShortToFloat(*reinterpret_cast<const xiiInt16*>(pSourcePointer));

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);

      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

class xiiImageConversion_F16_F32 : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R16Float, xiiGALResourceFormat::R32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG16Float, xiiGALResourceFormat::RG32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16Float, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels.
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);
    uiElementCount *= targetFormatDescription.GetElementSize();

    xiiUInt32 uiSourceStride = 2;
    xiiUInt32 uiTargetStride = 4;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      *reinterpret_cast<float*>(pTargetPointer) = *reinterpret_cast<const xiiFloat16*>(pSourcePointer);

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);

      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

class xiiImageConversion_S8_F32 : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R8SNormalized, xiiGALResourceFormat::R32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG8SNormalized, xiiGALResourceFormat::RG32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8SNormalized, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels.
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);
    uiElementCount *= targetFormatDescription.GetElementSize();

    xiiUInt32 uiSourceStride = 1;
    xiiUInt32 uiTargetStride = 4;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      *reinterpret_cast<float*>(pTargetPointer) = xiiMath::ColorSignedByteToFloat(*reinterpret_cast<const xiiInt8*>(pSourcePointer));

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);

      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

struct xiiImageConversion_Pad_To_RGBA_U8 : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R8UNormalized, xiiGALResourceFormat::RGBA8UNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG8UNormalized, xiiGALResourceFormat::RGBA8UNormalized, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    const xiiGALResourceFormatDescription& sourceFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(sourceFormat);
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);

    xiiUInt32 uiSourceStride = sourceFormatDescription.GetElementSize();
    xiiUInt32 uiTargetStride = targetFormatDescription.GetElementSize();

    const xiiUInt8* pSourcePointer = static_cast<const xiiUInt8*>(pSource.GetPtr());
    xiiUInt8*       pTargetPointer = static_cast<xiiUInt8*>(pTarget.GetPtr());

    while (uiElementCount--)
    {
      // Copy existing channels (R or RG).
      memcpy(pTargetPointer, pSourcePointer, uiSourceStride);

      // Zero-fill remaining channels (G/B if missing).
      if (uiTargetStride > uiSourceStride)
      {
        memset(pTargetPointer + uiSourceStride, 0, uiTargetStride - uiSourceStride);
      }

      // Set alpha = 255 (UNORM8)
      pTargetPointer[3] = 0xFF;

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);
    }

    return XII_SUCCESS;
  }
};

struct xiiImageConversion_Pad_To_RGBA_F32 : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R32Float, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG32Float, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGB32Float, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    const xiiGALResourceFormatDescription& sourceFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(sourceFormat);
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);

    xiiUInt32 uiSourceStride = sourceFormatDescription.GetElementSize();
    xiiUInt32 uiTargetStride = targetFormatDescription.GetElementSize();

    const float* pSourcePointer = static_cast<const float*>(static_cast<const void*>(pSource.GetPtr()));
    float*       pTargetPointer = static_cast<float*>(static_cast<void*>(pTarget.GetPtr()));

    const xiiUInt32 uiSourceChannels = sourceFormatDescription.m_uiComponentCount; // 1, 2, or 3
    const xiiUInt32 uiTargetChannels = targetFormatDescription.m_uiComponentCount; // always 4

    while (uiElementCount--)
    {
      // Copy existing float channels.
      for (xiiUInt32 i = 0; i < uiSourceChannels; ++i)
      {
        pTargetPointer[i] = pSourcePointer[i];
      }

      // Zero-fill missing channels (up to RGB).
      for (xiiUInt32 i = uiSourceChannels; i < uiTargetChannels - 1; ++i)
      {
        pTargetPointer[i] = 0.0f;
      }

      // Set alpha = 1.0f.
      pTargetPointer[3] = 1.0f;

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);
    }

    return XII_SUCCESS;
  }
};

struct xiiImageConversion_DiscardChannels : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32Float, xiiGALResourceFormat::RGB32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32Float, xiiGALResourceFormat::RG32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32Float, xiiGALResourceFormat::R32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32UInt, xiiGALResourceFormat::RGB32UInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32UInt, xiiGALResourceFormat::RG32UInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32UInt, xiiGALResourceFormat::R32UInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32SInt, xiiGALResourceFormat::RGB32SInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32SInt, xiiGALResourceFormat::RG32SInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32SInt, xiiGALResourceFormat::R32SInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGB32Float, xiiGALResourceFormat::RG32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGB32Float, xiiGALResourceFormat::R32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGB32UInt, xiiGALResourceFormat::RG32UInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGB32UInt, xiiGALResourceFormat::R32UInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGB32SInt, xiiGALResourceFormat::RG32SInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGB32SInt, xiiGALResourceFormat::R32SInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16Float, xiiGALResourceFormat::RG16Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16Float, xiiGALResourceFormat::R16Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16UNormalized, xiiGALResourceFormat::RG16UNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16UNormalized, xiiGALResourceFormat::R16UNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16UInt, xiiGALResourceFormat::RG16UInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16UInt, xiiGALResourceFormat::R16UInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16SNormalized, xiiGALResourceFormat::RG16SNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16SNormalized, xiiGALResourceFormat::R16SNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16SInt, xiiGALResourceFormat::RG16SInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16SInt, xiiGALResourceFormat::R16SInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG32Float, xiiGALResourceFormat::R32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG32UInt, xiiGALResourceFormat::R32UInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG32SInt, xiiGALResourceFormat::R32SInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::D32FloatS8X24UInt, xiiGALResourceFormat::D32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UNormalized, xiiGALResourceFormat::R8UNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UInt, xiiGALResourceFormat::RG8UInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UInt, xiiGALResourceFormat::R8UInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8SNormalized, xiiGALResourceFormat::RG8SNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8SNormalized, xiiGALResourceFormat::R8SNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8SInt, xiiGALResourceFormat::RG8SInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8SInt, xiiGALResourceFormat::R8SInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG16Float, xiiGALResourceFormat::R16Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG16UNormalized, xiiGALResourceFormat::R16UNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG16UInt, xiiGALResourceFormat::R16UInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG16SNormalized, xiiGALResourceFormat::R16SNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG16SInt, xiiGALResourceFormat::R16SInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG8UInt, xiiGALResourceFormat::R8UInt, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG8SNormalized, xiiGALResourceFormat::R8SNormalized, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG8SInt, xiiGALResourceFormat::R8SInt, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    const xiiGALResourceFormatDescription& sourceFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(sourceFormat);
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);

    xiiUInt32 uiSourceStride = sourceFormatDescription.GetElementSize();
    xiiUInt32 uiTargetStride = targetFormatDescription.GetElementSize();

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    bool bIsRGBA8 = !sourceFormatDescription.IsCompressed() && sourceFormatDescription.m_uiComponentSize == 1 && sourceFormatDescription.m_uiComponentCount == 4;
    bool bIsRGB8  = !targetFormatDescription.IsCompressed() && targetFormatDescription.m_uiComponentSize == 1 && targetFormatDescription.m_uiComponentCount == 3;

    if (bIsRGBA8 && bIsRGB8)
    {
      // Fast path for RGBA -> RGB
      while (uiElementCount)
      {
        const xiiUInt8* src = static_cast<const xiiUInt8*>(pSourcePointer);
        xiiUInt8*       dst = static_cast<xiiUInt8*>(pTargetPointer);

        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];

        pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
        pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);
        uiElementCount--;
      }
    }

    while (uiElementCount)
    {
      memcpy(pTargetPointer, pSourcePointer, uiTargetStride);

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);
      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

class xiiImageConversion_FLOAT_to_R11G11B10 : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32Float, xiiGALResourceFormat::RG11B10Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGB32Float, xiiGALResourceFormat::RG11B10Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    const xiiGALResourceFormatDescription& sourceFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(sourceFormat);
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);

    xiiUInt32 uiSourceStride = sourceFormatDescription.GetElementSize();
    xiiUInt32 uiTargetStride = targetFormatDescription.GetElementSize();

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      // Adapted from DirectXMath's XMStoreFloat3PK
      xiiUInt32 IValue[3];
      memcpy(IValue, pSourcePointer, 12);

      xiiUInt32 Result[3];

      // X & Y Channels (5-bit exponent, 6-bit mantissa)
      for (xiiUInt32 j = 0; j < 2; ++j)
      {
        xiiUInt32 Sign = IValue[j] & 0x80000000;
        xiiUInt32 I    = IValue[j] & 0x7FFFFFFF;

        if ((I & 0x7F800000) == 0x7F800000)
        {
          // INF or NAN
          Result[j] = 0x7c0;
          if ((I & 0x7FFFFF) != 0)
          {
            Result[j] = 0x7c0 | (((I >> 17) | (I >> 11) | (I >> 6) | (I)) & 0x3f);
          }
          else if (Sign)
          {
            // -INF is clamped to 0 since 3PK is positive only
            Result[j] = 0;
          }
        }
        else if (Sign)
        {
          // 3PK is positive only, so clamp to zero
          Result[j] = 0;
        }
        else if (I > 0x477E0000U)
        {
          // The number is too large to be represented as a float11, set to max
          Result[j] = 0x7BF;
        }
        else
        {
          if (I < 0x38800000U)
          {
            // The number is too small to be represented as a normalized float11
            // Convert it to a denormalized value.
            xiiUInt32 Shift = 113U - (I >> 23U);
            I               = (0x800000U | (I & 0x7FFFFFU)) >> Shift;
          }
          else
          {
            // Rebias the exponent to represent the value as a normalized float11
            I += 0xC8000000U;
          }

          Result[j] = ((I + 0xFFFFU + ((I >> 17U) & 1U)) >> 17U) & 0x7ffU;
        }
      }

      // Z Channel (5-bit exponent, 5-bit mantissa)
      xiiUInt32 Sign = IValue[2] & 0x80000000;
      xiiUInt32 I    = IValue[2] & 0x7FFFFFFF;

      if ((I & 0x7F800000) == 0x7F800000)
      {
        // INF or NAN
        Result[2] = 0x3e0;
        if (I & 0x7FFFFF)
        {
          Result[2] = 0x3e0 | (((I >> 18) | (I >> 13) | (I >> 3) | (I)) & 0x1f);
        }
        else if (Sign)
        {
          // -INF is clamped to 0 since 3PK is positive only
          Result[2] = 0;
        }
      }
      else if (Sign)
      {
        // 3PK is positive only, so clamp to zero
        Result[2] = 0;
      }
      else if (I > 0x477C0000U)
      {
        // The number is too large to be represented as a float10, set to max
        Result[2] = 0x3df;
      }
      else
      {
        if (I < 0x38800000U)
        {
          // The number is too small to be represented as a normalized float10
          // Convert it to a denormalized value.
          xiiUInt32 Shift = 113U - (I >> 23U);
          I               = (0x800000U | (I & 0x7FFFFFU)) >> Shift;
        }
        else
        {
          // Rebias the exponent to represent the value as a normalized float10
          I += 0xC8000000U;
        }

        Result[2] = ((I + 0x1FFFFU + ((I >> 18U) & 1U)) >> 18U) & 0x3ffU;
      }

      // Pack Result into memory
      *reinterpret_cast<xiiUInt32*>(pTargetPointer) = (Result[0] & 0x7ff) | ((Result[1] & 0x7ff) << 11) | ((Result[2] & 0x3ff) << 22);

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);
      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

class xiiImageConversion_R11G11B10_to_FLOAT : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::RG11B10Float, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG11B10Float, xiiGALResourceFormat::RGB32Float, xiiImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    const xiiGALResourceFormatDescription& sourceFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(sourceFormat);
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);

    xiiUInt32 uiSourceStride = sourceFormatDescription.GetElementSize();
    xiiUInt32 uiTargetStride = targetFormatDescription.GetElementSize();

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      const R11G11B10* pSourcePtr = reinterpret_cast<const R11G11B10*>(pSourcePointer);
      xiiUInt32*       targetUi   = reinterpret_cast<xiiUInt32*>(pTargetPointer);

      // Adapted from XMLoadFloat3PK
      xiiUInt32 Mantissa;
      xiiUInt32 Exponent;

      // X Channel (6-bit mantissa)
      Mantissa = pSourcePtr->p.xm;

      if (pSourcePtr->p.xe == 0x1f) // INF or NAN
      {
        targetUi[0] = 0x7f800000 | (pSourcePtr->p.xm << 17);
      }
      else
      {
        if (pSourcePtr->p.xe != 0) // The value is normalized
        {
          Exponent = pSourcePtr->p.xe;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x40) == 0);

          Mantissa &= 0x3F;
        }
        else // The value is zero
        {
          Exponent = (xiiUInt32)-112;
        }

        targetUi[0] = ((Exponent + 112) << 23) | (Mantissa << 17);
      }

      // Y Channel (6-bit mantissa)
      Mantissa = pSourcePtr->p.ym;

      if (pSourcePtr->p.ye == 0x1f) // INF or NAN
      {
        targetUi[1] = 0x7f800000 | (pSourcePtr->p.ym << 17);
      }
      else
      {
        if (pSourcePtr->p.ye != 0) // The value is normalized
        {
          Exponent = pSourcePtr->p.ye;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x40) == 0);

          Mantissa &= 0x3F;
        }
        else // The value is zero
        {
          Exponent = (xiiUInt32)-112;
        }

        targetUi[1] = ((Exponent + 112) << 23) | (Mantissa << 17);
      }

      // Z Channel (5-bit mantissa)
      Mantissa = pSourcePtr->p.zm;

      if (pSourcePtr->p.ze == 0x1f) // INF or NAN
      {
        targetUi[2] = 0x7f800000 | (pSourcePtr->p.zm << 17);
      }
      else
      {
        if (pSourcePtr->p.ze != 0) // The value is normalized
        {
          Exponent = pSourcePtr->p.ze;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x20) == 0);

          Mantissa &= 0x1F;
        }
        else // The value is zero
        {
          Exponent = (xiiUInt32)-112;
        }

        targetUi[2] = ((Exponent + 112) << 23) | (Mantissa << 18);
      }

      if (uiTargetStride > sizeof(float) * 3)
      {
        reinterpret_cast<float*>(pTargetPointer)[3] = 1.0f; // Write alpha channel.
      }
      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);

      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

class xiiImageConversion_R11G11B10_to_HALF : public xiiImageConversionStepLinear
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::RG11B10Float, xiiGALResourceFormat::RGBA16Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    const xiiGALResourceFormatDescription& sourceFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(sourceFormat);
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);

    xiiUInt32 uiSourceStride = sourceFormatDescription.GetElementSize();
    xiiUInt32 uiTargetStride = targetFormatDescription.GetElementSize();

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      xiiUInt16*       pResult    = reinterpret_cast<xiiUInt16*>(pTargetPointer);
      const R11G11B10* pR11G11B10 = reinterpret_cast<const R11G11B10*>(pSourcePointer);

      // We can do a straight forward conversion here because R11G11B10 uses the same number of bits for the exponent as a half
      // This means that all special values, e.g. denormals, inf, nan map exactly.
      pResult[0] = static_cast<xiiUInt16>((pR11G11B10->p.xe << 10) | (pR11G11B10->p.xm << 4));
      pResult[1] = static_cast<xiiUInt16>((pR11G11B10->p.ye << 10) | (pR11G11B10->p.ym << 4));
      pResult[2] = static_cast<xiiUInt16>((pR11G11B10->p.ze << 10) | (pR11G11B10->p.zm << 5));
      pResult[3] = 0x3C00; // Hex value of 1.0f as half.

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);

      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};

template <typename T>
class xiiImageConversion_Int_To_F32 : public xiiImageConversionStepLinear
{
public:
  virtual xiiResult ConvertPixels(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt64 uiElementCount, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels.
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);
    uiElementCount *= targetFormatDescription.GetElementSize();

    const xiiUInt32 uiSourceStride = sizeof(T);
    const xiiUInt32 uiTargetStride = 4;

    const void* pSourcePointer = pSource.GetPtr();
    void*       pTargetPointer = pTarget.GetPtr();

    while (uiElementCount)
    {
      *reinterpret_cast<float*>(pTargetPointer) = static_cast<float>(*reinterpret_cast<const T*>(pSourcePointer));

      pSourcePointer = xiiMemoryUtils::AddByteOffset(pSourcePointer, uiSourceStride);
      pTargetPointer = xiiMemoryUtils::AddByteOffset(pTargetPointer, uiTargetStride);
      uiElementCount--;
    }

    return XII_SUCCESS;
  }
};


class xiiImageConversion_UINT8_F32 : public xiiImageConversion_Int_To_F32<xiiUInt8>
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R8UInt, xiiGALResourceFormat::R32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG8UInt, xiiGALResourceFormat::RG32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UInt, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class xiiImageConversion_SINT8_F32 : public xiiImageConversion_Int_To_F32<xiiInt8>
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R8SInt, xiiGALResourceFormat::R32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG8SInt, xiiGALResourceFormat::RG32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8SInt, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class xiiImageConversion_UINT16_F32 : public xiiImageConversion_Int_To_F32<xiiUInt16>
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R16UInt, xiiGALResourceFormat::R32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG16UInt, xiiGALResourceFormat::RG32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16UInt, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class xiiImageConversion_SINT16_F32 : public xiiImageConversion_Int_To_F32<xiiInt16>
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R16SInt, xiiGALResourceFormat::R32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG16SInt, xiiGALResourceFormat::RG32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA16SInt, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class xiiImageConversion_UINT32_F32 : public xiiImageConversion_Int_To_F32<xiiUInt32>
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R32UInt, xiiGALResourceFormat::R32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG32UInt, xiiGALResourceFormat::RG32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGB32UInt, xiiGALResourceFormat::RGB32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32UInt, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class xiiImageConversion_SINT32_F32 : public xiiImageConversion_Int_To_F32<xiiInt32>
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::R32SInt, xiiGALResourceFormat::R32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RG32SInt, xiiGALResourceFormat::RG32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGB32SInt, xiiGALResourceFormat::RGB32Float, xiiImageConversionFlags::Default),
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA32SInt, xiiGALResourceFormat::RGBA32Float, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};


#define ADD_16BPP_CONVERSION(format)                                                                                                                  \
  static xiiImageConversionStep_Decompress16bpp<xiiDecompress##format, xiiGALResourceFormat::format##UNormalized> s_conversion_xiiDecompress##format; \
  static xiiImageConversionStep_Compress16bpp<xiiCompress##format, xiiGALResourceFormat::format##UNormalized>     s_conversion_xiiCompress##format

ADD_16BPP_CONVERSION(B5G6R5);
ADD_16BPP_CONVERSION(B5G5R5A1);

XII_STATICLINK_FORCE
static xiiImageSwizzleConversion32_2103 s_conversion_swizzle2103;
static xiiImageConversion_BGRX_BGRA     s_conversion_BGRX_BGRA;
static xiiImageConversion_F32_U8        s_conversion_F32_U8;
static xiiImageConversion_F32_sRGB      s_conversion_F32_sRGB;
static xiiImageConversion_F32_U16       s_conversion_F32_U16;
static xiiImageConversion_F32_F16       s_conversion_F32_F16;
static xiiImageConversion_F32_S8        s_conversion_F32_S8;
static xiiImageConversion_U8_F32        s_conversion_U8_F32;
static xiiImageConversion_sRGB_F32      s_conversion_sRGB_F32;
static xiiImageConversion_U16_F32       s_conversion_U16_F32;
static xiiImageConversion_S16_F32       s_conversion_S16_F32;
static xiiImageConversion_F16_F32       s_conversion_F16_F32;
static xiiImageConversion_S8_F32        s_conversion_S8_F32;
static xiiImageConversion_UINT8_F32     s_conversion_UINT8_F32;
static xiiImageConversion_SINT8_F32     s_conversion_SINT8_F32;
static xiiImageConversion_UINT16_F32    s_conversion_UINT16_F32;
static xiiImageConversion_SINT16_F32    s_conversion_SINT16_F32;
static xiiImageConversion_UINT32_F32    s_conversion_UINT32_F32;
static xiiImageConversion_SINT32_F32    s_conversion_SINT32_F32;

static xiiImageConversion_Pad_To_RGBA_U8  s_conversion_Pad_To_RGBA_U8;
static xiiImageConversion_Pad_To_RGBA_F32 s_conversion_Pad_To_RGBA_F32;
static xiiImageConversion_DiscardChannels s_conversion_DiscardChannels;

static xiiImageConversion_R11G11B10_to_FLOAT s_conversion_R11G11B10_to_FLOAT;
static xiiImageConversion_R11G11B10_to_HALF  s_conversion_R11G11B10_to_HALF;
static xiiImageConversion_FLOAT_to_R11G11B10 s_conversion_FLOAT_to_R11G11B10;

XII_STATICLINK_FILE(Texture, Texture_Image_Conversions_PixelConversions);
