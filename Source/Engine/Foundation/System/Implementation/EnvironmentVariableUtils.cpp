#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Utilities/ConversionUtils.h>

// The POSIX functions are not thread safe by definition.
static xiiMutex s_EnvVarMutex;


xiiString xiiEnvironmentVariableUtils::GetValueString(xiiStringView sName, xiiStringView sDefault /*= nullptr*/)
{
  XII_ASSERT_DEV(!sName.IsEmpty(), "Null or empty name passed to xiiEnvironmentVariableUtils::GetValueString()");

  XII_LOCK(s_EnvVarMutex);

  return GetValueStringImpl(sName, sDefault);
}

xiiResult xiiEnvironmentVariableUtils::SetValueString(xiiStringView sName, xiiStringView sValue)
{
  XII_LOCK(s_EnvVarMutex);

  return SetValueStringImpl(sName, sValue);
}

xiiInt32 xiiEnvironmentVariableUtils::GetValueInt(xiiStringView sName, xiiInt32 iDefault /*= -1*/)
{
  XII_LOCK(s_EnvVarMutex);

  xiiString value = GetValueString(sName);

  if (value.IsEmpty())
    return iDefault;

  xiiInt32 iRetVal = 0;
  if (xiiConversionUtils::StringToInt(value, iRetVal).Succeeded())
    return iRetVal;
  else
    return iDefault;
}

xiiResult xiiEnvironmentVariableUtils::SetValueInt(xiiStringView sName, xiiInt32 iValue)
{
  xiiStringBuilder sb;
  sb.SetFormat("{}", iValue);

  return SetValueString(sName, sb);
}

bool xiiEnvironmentVariableUtils::IsVariableSet(xiiStringView sName)
{
  XII_LOCK(s_EnvVarMutex);

  return IsVariableSetImpl(sName);
}

xiiResult xiiEnvironmentVariableUtils::UnsetVariable(xiiStringView sName)
{
  XII_LOCK(s_EnvVarMutex);

  return UnsetVariableImpl(sName);
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Platform/Implementation/Windows/EnvironmentVariableUtils_win.h>
#elif XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  include <Foundation/Platform/Implementation/UWP/EnvironmentVariableUtils_win_uwp.h>
#else
#  include <Foundation/Platform/Implementation/Posix/EnvironmentVariableUtils_posix.h>
#endif

XII_STATICLINK_FILE(Foundation, Foundation_System_Implementation_EnvironmentVariableUtils);
