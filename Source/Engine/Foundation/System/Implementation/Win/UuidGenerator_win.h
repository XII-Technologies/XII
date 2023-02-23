#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <combaseapi.h>
#include <rpc.h>

XII_CHECK_AT_COMPILETIME(sizeof(xiiUInt64) * 2 == sizeof(UUID));

void xiiUuid::CreateNewUuid()
{
  xiiUInt64 uiUuidData[2];

  // This works on desktop Windows
  // UuidCreate(reinterpret_cast<UUID*>(uiUuidData));

  // This also works on UWP
  GUID*   guid = reinterpret_cast<GUID*>(&uiUuidData[0]);
  HRESULT hr   = CoCreateGuid(guid);
  XII_ASSERT_DEBUG(SUCCEEDED(hr), "CoCreateGuid failed, guid might be invalid!");

  m_uiHigh = uiUuidData[0];
  m_uiLow  = uiUuidData[1];
}
