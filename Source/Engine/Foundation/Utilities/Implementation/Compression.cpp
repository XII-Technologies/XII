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
  static xiiResult CompressZStd(xiiArrayPtr<const xiiUInt8> uncompressedData, xiiDynamicArray<xiiUInt8>& out_data)
  {
    size_t uiSizeBound = ZSTD_compressBound(uncompressedData.GetCount());
    if (uiSizeBound > xiiMath::MaxValue<xiiUInt32>())
    {
      xiiLog::Error("Can't compress since the output container can't hold enough elements ({0})", static_cast<xiiUInt64>(uiSizeBound));
      return XII_FAILURE;
    }

    out_data.SetCountUninitialized(static_cast<xiiUInt32>(uiSizeBound));

    size_t const cSize = ZSTD_compress(out_data.GetData(), uiSizeBound, uncompressedData.GetPtr(), uncompressedData.GetCount(), 1);
    if (ZSTD_isError(cSize))
    {
      xiiLog::Error("Compression failed with error: '{0}'.", ZSTD_getErrorName(cSize));
      return XII_FAILURE;
    }

    out_data.SetCount(static_cast<xiiUInt32>(cSize));

    return XII_SUCCESS;
  }

  static xiiResult DecompressZStd(xiiArrayPtr<const xiiUInt8> compressedData, xiiDynamicArray<xiiUInt8>& out_data)
  {
    xiiUInt64 uiSize = ZSTD_findDecompressedSize(compressedData.GetPtr(), compressedData.GetCount());

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

    out_data.SetCountUninitialized(static_cast<xiiUInt32>(uiSize));

    size_t const uiActualSize = ZSTD_decompress(out_data.GetData(), xiiMath::SafeConvertToSizeT(uiSize), compressedData.GetPtr(), compressedData.GetCount());

    if (uiActualSize != uiSize)
    {
      xiiLog::Error("Error during ZStd decompression: '{0}'.", ZSTD_getErrorName(uiActualSize));
      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }
#endif

  xiiResult Compress(xiiArrayPtr<const xiiUInt8> uncompressedData, xiiCompressionMethod method, xiiDynamicArray<xiiUInt8>& out_data)
  {
    out_data.Clear();

    if (uncompressedData.IsEmpty())
    {
      return XII_SUCCESS;
    }

    switch (method)
    {
      case xiiCompressionMethod::ZStd:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        return CompressZStd(uncompressedData, out_data);
#else
        xiiLog::Error("ZStd compression disabled in build settings!");
        return XII_FAILURE;
#endif
      default:
        xiiLog::Error("Unsupported compression method {0}!", static_cast<xiiUInt32>(method));
        return XII_FAILURE;
    }
  }

  xiiResult Decompress(xiiArrayPtr<const xiiUInt8> compressedData, xiiCompressionMethod method, xiiDynamicArray<xiiUInt8>& out_data)
  {
    out_data.Clear();

    if (compressedData.IsEmpty())
    {
      return XII_SUCCESS;
    }

    switch (method)
    {
      case xiiCompressionMethod::ZStd:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        return DecompressZStd(compressedData, out_data);
#else
        xiiLog::Error("ZStd compression disabled in build settings!");
        return XII_FAILURE;
#endif
      default:
        xiiLog::Error("Unsupported compression method {0}!", static_cast<xiiUInt32>(method));
        return XII_FAILURE;
    }
  }
} // namespace xiiCompressionUtils


XII_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_Compression);
