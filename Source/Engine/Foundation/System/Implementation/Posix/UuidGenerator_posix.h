#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#if __has_include(<uuid/uuid.h>)
#  include <uuid/uuid.h>
#  define HAS_UUID 1
#else
// #  error "uuid.h does not exist on this distro."
#  define HAS_UUID 0
#endif

#if HAS_UUID

static_assert(sizeof(xiiUInt64) * 2 == sizeof(uuid_t));

xiiUuid xiiUuid::MakeUuid()
{
  uuid_t uuid;
  uuid_generate(uuid);

  xiiUInt64* uiUuidData = reinterpret_cast<xiiUInt64*>(uuid);

  return xiiUuid(uiUuidData[1], uiUuidData[0]);
}

#else

xiiUuid xiiUuid::MakeUuid()
{
  XII_REPORT_FAILURE("This distro doesn't have support for UUID generation.");
  return xiiUuid();
}

#endif
