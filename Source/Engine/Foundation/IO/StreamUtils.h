
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>

namespace xiiStreamUtils
{
  /// \brief Reads all the remaining data in \a stream and appends it to \a destination.
  XII_FOUNDATION_DLL void ReadAllAndAppend(xiiStreamReader& stream, xiiDynamicArray<xiiUInt8>& destination);

} // namespace xiiStreamUtils
