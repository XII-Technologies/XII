/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>

///The compression method to be used
enum class xiiCompressionMethod : xiiUInt16
{
  ZStd = 0 ///< Only available when ZStd support is enabled in the build (default)
};

/// This namespace contains utilities which can be used to compress and decompress data.
namespace xiiCompressionUtils
{
  ///Compresses the given data using the compression method eMethod into the dynamic array given in out_Data.
  XII_FOUNDATION_DLL xiiResult Compress(xiiArrayPtr<const xiiUInt8> uncompressedData, xiiCompressionMethod method, xiiDynamicArray<xiiUInt8>& out_data);

  ///Decompresses the given data using the compression method eMethod into the dynamic array given in out_Data.
  XII_FOUNDATION_DLL xiiResult Decompress(xiiArrayPtr<const xiiUInt8> compressedData, xiiCompressionMethod method, xiiDynamicArray<xiiUInt8>& out_data);
} // namespace xiiCompressionUtils
