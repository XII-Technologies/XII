#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <uuid/uuid.h>


XII_CHECK_AT_COMPILETIME(sizeof(xiiUInt64) * 2 == sizeof(uuid_t));

void xiiUuid::CreateNewUuid()
{
  uuid_t uuid;
  uuid_generate(uuid);

  xiiUInt64* uiUuidData = reinterpret_cast<xiiUInt64*>(uuid);

  m_uiHigh = uiUuidData[0];
  m_uiLow  = uiUuidData[1];
}
