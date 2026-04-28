/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <rpc.h>

static_assert(sizeof(xiiUInt64) * 2 == sizeof(UUID));

xiiUuid xiiUuid::MakeUuid()
{
  xiiUInt64 uiUuidData[2];

  if (UuidCreate(reinterpret_cast<UUID*>(&uiUuidData[0])) != RPC_S_OK)
  {
    XII_ASSERT_DEBUG(false, "UuidCreate failed, UUID might be invalid!");
    return xiiUuid(); // Return an empty UUID on failure.
  }

  return xiiUuid(uiUuidData[1], uiUuidData[0]);
}
