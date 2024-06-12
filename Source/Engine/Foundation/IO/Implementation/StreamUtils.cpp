#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/StreamUtils.h>

void xiiStreamUtils::ReadAllAndAppend(xiiStreamReader& ref_stream, xiiDynamicArray<xiiUInt8>& ref_destination)
{
  xiiUInt8 temp[1024 * 4];

  while (true)
  {
    const xiiUInt32 uiRead = (xiiUInt32)ref_stream.ReadBytes(temp, XII_ARRAY_SIZE(temp));

    if (uiRead == 0)
      return;

    ref_destination.PushBackRange(xiiArrayPtr<xiiUInt8>(temp, uiRead));
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StreamUtils);
