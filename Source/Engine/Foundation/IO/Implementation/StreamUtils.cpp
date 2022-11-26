#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/StreamUtils.h>

void xiiStreamUtils::ReadAllAndAppend(xiiStreamReader& stream, xiiDynamicArray<xiiUInt8>& destination)
{
  xiiUInt8 temp[1024 * 4];

  while (true)
  {
    const xiiUInt32 uiRead = (xiiUInt32)stream.ReadBytes(temp, XII_ARRAY_SIZE(temp));

    if (uiRead == 0)
      return;

    destination.PushBackRange(xiiArrayPtr<xiiUInt8>(temp, uiRead));
  }
}
