#include <Foundation/FoundationPCH.h>

#include <Foundation/Types/Uuid.h>


// Include inline file
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/System/Implementation/Win/UuidGenerator_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX)
#  include <Foundation/System/Implementation/Posix/UuidGenerator_posix.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/System/Implementation/Posix/UuidGenerator_posix.h>
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/System/Implementation/Android/UuidGenerator_android.h>
#else
#  error "Uuid generation functions are not implemented on current platform"
#endif

xiiUuid xiiUuid::StableUuidForString(xiiStringView sString)
{
  xiiUuid NewUuid;
  NewUuid.m_uiLow  = xiiHashingUtils::xxHash64String(sString);
  NewUuid.m_uiHigh = xiiHashingUtils::xxHash64String(sString, 0x7FFFFFFFFFFFFFE7U);

  return NewUuid;
}

xiiUuid xiiUuid::StableUuidForInt(xiiInt64 iInt)
{
  xiiUuid NewUuid;
  NewUuid.m_uiLow  = xiiHashingUtils::xxHash64(&iInt, sizeof(xiiInt64));
  NewUuid.m_uiHigh = xiiHashingUtils::xxHash64(&iInt, sizeof(xiiInt64), 0x7FFFFFFFFFFFFFE7U);

  return NewUuid;
}

XII_STATICLINK_FILE(Foundation, Foundation_System_Implementation_UuidGenerator);
