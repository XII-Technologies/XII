#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/Compression.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  define ZSTD_STATIC_LINKING_ONLY // ZSTD_findDecompressedSize
#  include <zstd/zstd.h>
#endif

namespace xiiCompressionUtils
{
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  static xiiResult CompressZStd(xiiArrayPtr<const xiiUInt8> pUncompressedData, xiiDynamicArray<xiiUInt8>& out_Data)
  {
    size_t uiSizeBound = ZSTD_compressBound(pUncompressedData.GetCount());
    if (uiSizeBound > xiiMath::MaxValue<xiiUInt32>())
    {
      xiiLog::Error("Can't compress since the output container can't hold enough elements ({0})", static_cast<xiiUInt64>(uiSizeBound));
      return XII_FAILURE;
    }

    out_Data.SetCountUninitialized(static_cast<xiiUInt32>(uiSizeBound));

    size_t const cSize = ZSTD_compress(out_Data.GetData(), uiSizeBound, pUncompressedData.GetPtr(), pUncompressedData.GetCount(), 1);
    if (ZSTD_isError(cSize))
    {
      xiiLog::Error("Compression failed with error: '{0}'.", ZSTD_getErrorName(cSize));
      return XII_FAILURE;
    }

    out_Data.SetCount(static_cast<xiiUInt32>(cSize));

    return XII_SUCCESS;
  }

  static xiiResult DecompressZStd(xiiArrayPtr<const xiiUInt8> pCompressedData, xiiDynamicArray<xiiUInt8>& out_Data)
  {
    xiiUInt64 uiSize = ZSTD_findDecompressedSize(pCompressedData.GetPtr(), pCompressedData.GetCount());

    if (uiSize == ZSTD_CONTENTSIZE_ERROR)
    {
      xiiLog::Error("Can't decompress since it wasn't compressed with ZStd");
      return XII_FAILURE;
    }
    else if (uiSize == ZSTD_CONTENTSIZE_UNKNOWN)
    {
      xiiLog::Error("Can't decompress since the original size can't be determined, was the data compressed using the streaming variant?");
      return XII_FAILURE;
    }

    if (uiSize > xiiMath::MaxValue<xiiUInt32>())
    {
      xiiLog::Error("Can't compress since the output container can't hold enough elements ({0})", uiSize);
      return XII_FAILURE;
    }

    out_Data.SetCountUninitialized(static_cast<xiiUInt32>(uiSize));

    size_t const uiActualSize = ZSTD_decompress(out_Data.GetData(), xiiMath::SafeConvertToSizeT(uiSize), pCompressedData.GetPtr(), pCompressedData.GetCount());

    if (uiActualSize != uiSize)
    {
      xiiLog::Error("Error during ZStd decompression: '{0}'.", ZSTD_getErrorName(uiActualSize));
      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }
#endif

  xiiResult Compress(xiiArrayPtr<const xiiUInt8> pUncompressedData, xiiCompressionMethod eMethod, xiiDynamicArray<xiiUInt8>& out_Data)
  {
    out_Data.Clear();

    if (pUncompressedData.IsEmpty())
    {
      return XII_SUCCESS;
    }

    switch (eMethod)
    {
      case xiiCompressionMethod::ZStd:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        return CompressZStd(pUncompressedData, out_Data);
#else
        xiiLog::Error("ZStd compression disabled in build settings!");
        return XII_FAILURE;
#endif
      default:
        xiiLog::Error("Unsupported compression method {0}!", static_cast<xiiUInt32>(eMethod));
        return XII_FAILURE;
    }
  }

  xiiResult Decompress(xiiArrayPtr<const xiiUInt8> pCompressedData, xiiCompressionMethod eMethod, xiiDynamicArray<xiiUInt8>& out_Data)
  {
    out_Data.Clear();

    if (pCompressedData.IsEmpty())
    {
      return XII_SUCCESS;
    }

    switch (eMethod)
    {
      case xiiCompressionMethod::ZStd:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        return DecompressZStd(pCompressedData, out_Data);
#else
        xiiLog::Error("ZStd compression disabled in build settings!");
        return XII_FAILURE;
#endif
      default:
        xiiLog::Error("Unsupported compression method {0}!", static_cast<xiiUInt32>(eMethod));
        return XII_FAILURE;
    }
  }
} // namespace xiiCompressionUtils


XII_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_Compression);
