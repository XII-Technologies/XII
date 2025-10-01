#include <Foundation/FoundationPCH.h>

#include <Foundation/Types/Uuid.h>

// Include inline file
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Implementation/Windows/UuidGenerator_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX)
#  include <Foundation/Platform/Implementation/Posix/UuidGenerator_posix.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Platform/Implementation/Posix/UuidGenerator_posix.h>
#else
#  error "Uuid generation functions are not implemented on current platform"
#endif

xiiUuid xiiUuid::MakeStableUuidFromString(xiiStringView sString)
{
  xiiUuid NewUuid;
  NewUuid.m_uiLow  = xiiHashingUtils::xxHash64String(sString);
  NewUuid.m_uiHigh = xiiHashingUtils::xxHash64String(sString, 0x7FFFFFFFFFFFFFE7U);

  return NewUuid;
}

xiiUuid xiiUuid::MakeStableUuidFromInt(xiiInt64 iInt)
{
  xiiUuid NewUuid;
  NewUuid.m_uiLow  = xiiHashingUtils::xxHash64(&iInt, sizeof(xiiInt64));
  NewUuid.m_uiHigh = xiiHashingUtils::xxHash64(&iInt, sizeof(xiiInt64), 0x7FFFFFFFFFFFFFE7U);

  return NewUuid;
}

XII_STATICLINK_FILE(Foundation, Foundation_System_Implementation_UuidGenerator);
