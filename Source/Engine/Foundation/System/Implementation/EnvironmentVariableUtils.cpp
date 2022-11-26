#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Utilities/ConversionUtils.h>

// The POSIX functions are not thread safe by definition.
static xiiMutex s_EnvVarMutex;


xiiString xiiEnvironmentVariableUtils::GetValueString(const char* szName, const char* szDefault /*= nullptr*/)
{
  XII_ASSERT_DEV(!xiiStringUtils::IsNullOrEmpty(szName), "Null or empty name passed to xiiEnvironmentVariableUtils::GetValueString()");

  XII_LOCK(s_EnvVarMutex);

  return GetValueStringImpl(szName, szDefault);
}

xiiResult xiiEnvironmentVariableUtils::SetValueString(const char* szName, const char* szValue)
{
  XII_LOCK(s_EnvVarMutex);

  return SetValueStringImpl(szName, szValue);
}

xiiInt32 xiiEnvironmentVariableUtils::GetValueInt(const char* szName, xiiInt32 iDefault /*= -1*/)
{
  XII_LOCK(s_EnvVarMutex);

  xiiString value = GetValueString(szName);

  if (value.IsEmpty())
    return iDefault;

  xiiInt32 iRetVal = 0;
  if (xiiConversionUtils::StringToInt(value, iRetVal).Succeeded())
    return iRetVal;
  else
    return iDefault;
}

xiiResult xiiEnvironmentVariableUtils::SetValueInt(const char* szName, xiiInt32 iValue)
{
  xiiStringBuilder sb;
  sb.Format("{}", iValue);

  return SetValueString(szName, sb);
}

bool xiiEnvironmentVariableUtils::IsVariableSet(const char* szName)
{
  XII_LOCK(s_EnvVarMutex);

  return IsVariableSetImpl(szName);
}

xiiResult xiiEnvironmentVariableUtils::UnsetVariable(const char* szName)
{
  XII_LOCK(s_EnvVarMutex);

  return UnsetVariableImpl(szName);
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/System/Implementation/Win/EnvironmentVariableUtils_win.h>
#elif XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  include <Foundation/System/Implementation/Win/EnvironmentVariableUtils_win_uwp.h>
#else
#  include <Foundation/System/Implementation/Posix/EnvironmentVariableUtils_posix.h>
#endif


XII_STATICLINK_FILE(Foundation, Foundation_System_Implementation_EnvironmentVariableUtils);
