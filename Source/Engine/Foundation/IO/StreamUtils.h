/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>

namespace xiiStreamUtils
{
  /// Reads all the remaining data in \a stream and appends it to \a destination.
  XII_FOUNDATION_DLL void ReadAllAndAppend(xiiStreamReader& ref_stream, xiiDynamicArray<xiiUInt8>& ref_destination);

} // namespace xiiStreamUtils
