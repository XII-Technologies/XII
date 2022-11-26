#include <Texture/TexturePCH.h>

#if XII_ENABLED(XII_PLATFORM_LINUX)

#  include <Texture/DirectXTex/BC.h>
#  include <Texture/Image/ImageConversion.h>

#  include <Foundation/Threading/TaskSystem.h>


xiiImageConversionEntry g_DXTexCpuConversions[] = {
  xiiImageConversionEntry(xiiImageFormat::R32G32B32A32_FLOAT, xiiImageFormat::BC6H_UF16, xiiImageConversionFlags::Default),

  xiiImageConversionEntry(xiiImageFormat::R8G8B8A8_UNORM, xiiImageFormat::BC1_UNORM, xiiImageConversionFlags::Default),
  xiiImageConversionEntry(xiiImageFormat::R8G8B8A8_UNORM, xiiImageFormat::BC7_UNORM, xiiImageConversionFlags::Default),

  xiiImageConversionEntry(xiiImageFormat::R8G8B8A8_UNORM_SRGB, xiiImageFormat::BC1_UNORM_SRGB, xiiImageConversionFlags::Default),
  xiiImageConversionEntry(xiiImageFormat::R8G8B8A8_UNORM_SRGB, xiiImageFormat::BC7_UNORM_SRGB, xiiImageConversionFlags::Default),
};

class xiiImageConversion_CompressDxTexCpu : public xiiImageConversionStepCompressBlocks
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    return g_DXTexCpuConversions;
  }

  virtual xiiResult CompressBlocks(xiiConstByteBlobPtr source, xiiByteBlobPtr target, xiiUInt32 numBlocksX, xiiUInt32 numBlocksY, xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat) const override
  {
    if (targetFormat == xiiImageFormat::BC7_UNORM || targetFormat == xiiImageFormat::BC7_UNORM_SRGB)
    {
      const xiiUInt32 srcStride    = numBlocksX * 4 * 4;
      const xiiUInt32 targetStride = numBlocksX * 16;

      xiiTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](xiiUInt32 startIndex, xiiUInt32 endIndex) {
        const xiiUInt8* srcIt    = source.GetPtr() + srcStride * startIndex * 4;
        xiiUInt8*       targetIt = target.GetPtr() + targetStride * startIndex;
        for (xiiUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (xiiUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (xiiUInt32 y = 0; y < 4; y++)
            {
              for (xiiUInt32 x = 0; x < 4; x++)
              {
                const xiiUInt8* pixel = srcIt + y * srcStride + x * 4;
                temp[y * 4 + x]       = DirectX::XMVectorSet(pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, pixel[3] / 255.0f);
              }
            }
            DirectX::D3DXEncodeBC7(targetIt, temp, 0);

            srcIt += 4 * 4;
            targetIt += 16;
          }
          srcIt += 3 * srcStride;
        }
      });

      return XII_SUCCESS;
    }
    else if (targetFormat == xiiImageFormat::BC1_UNORM || targetFormat == xiiImageFormat::BC1_UNORM_SRGB)
    {
      const xiiUInt32 srcStride    = numBlocksX * 4 * 4;
      const xiiUInt32 targetStride = numBlocksX * 8;

      xiiTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](xiiUInt32 startIndex, xiiUInt32 endIndex) {
        const xiiUInt8* srcIt    = source.GetPtr() + srcStride * startIndex * 4;
        xiiUInt8*       targetIt = target.GetPtr() + targetStride * startIndex;
        for (xiiUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (xiiUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (xiiUInt32 y = 0; y < 4; y++)
            {
              for (xiiUInt32 x = 0; x < 4; x++)
              {
                const xiiUInt8* pixel = srcIt + y * srcStride + x * 4;
                temp[y * 4 + x]       = DirectX::XMVectorSet(pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, pixel[3] / 255.0f);
              }
            }
            DirectX::D3DXEncodeBC1(targetIt, temp, 1.0f, 0);

            srcIt += 4 * 4;
            targetIt += 8;
          }
          srcIt += 3 * srcStride;
        }
      });

      return XII_SUCCESS;
    }
    else if (targetFormat == xiiImageFormat::BC6H_UF16)
    {
      const xiiUInt32 srcStride    = numBlocksX * 4 * 4 * sizeof(float);
      const xiiUInt32 targetStride = numBlocksX * 16;

      xiiTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](xiiUInt32 startIndex, xiiUInt32 endIndex) {
        const xiiUInt8* srcIt    = source.GetPtr() + srcStride * startIndex * 4;
        xiiUInt8*       targetIt = target.GetPtr() + targetStride * startIndex;
        for (xiiUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (xiiUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (xiiUInt32 y = 0; y < 4; y++)
            {
              for (xiiUInt32 x = 0; x < 4; x++)
              {
                const float* pixel = reinterpret_cast<const float*>(srcIt + y * srcStride + x * 4 * sizeof(float));
                temp[y * 4 + x]    = DirectX::XMVectorSet(pixel[0], pixel[1], pixel[2], pixel[3]);
              }
            }
            DirectX::D3DXEncodeBC6HU(targetIt, temp, 0);

            srcIt += 4 * 4 * sizeof(float);
            targetIt += 16;
          }
          srcIt += 3 * srcStride;
        }
      });

      return XII_SUCCESS;
    }

    return XII_FAILURE;
  }
};

static xiiImageConversion_CompressDxTexCpu s_conversion_compressDxTexCpu;

#endif

XII_STATICLINK_FILE(Texture, Texture_Image_Conversions_DXTexCpuConversions);