#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <intsafe.h>

xiiString xiiEnvironmentVariableUtils::GetValueStringImpl(xiiStringView sName, xiiStringView sDefault)
{
  xiiStringWChar szwName(sName);
  wchar_t        szStaticValueBuffer[64] = {0};
  size_t         uiRequiredSize          = 0;

  errno_t res = _wgetenv_s(&uiRequiredSize, szStaticValueBuffer, szwName);

  // Variable doesn't exist
  if (uiRequiredSize == 0)
  {
    return sDefault;
  }

  // Succeeded
  if (res == 0)
  {
    return xiiString(szStaticValueBuffer);
  }
  // Static buffer was too small, do a heap allocation to query the value
  else if (res == ERANGE)
  {
    XII_ASSERT_DEV(uiRequiredSize != SIZE_T_MAX, "");
    const size_t uiDynamicSize   = uiRequiredSize + 1;
    wchar_t*     szDynamicBuffer = XII_DEFAULT_NEW_RAW_BUFFER(wchar_t, uiDynamicSize);
    xiiMemoryUtils::ZeroFill(szDynamicBuffer, uiDynamicSize);

    res = _wgetenv_s(&uiRequiredSize, szDynamicBuffer, uiDynamicSize, szwName);

    if (res != 0)
    {
      xiiLog::Error("Error getting environment variable \"{0}\" with dynamic buffer.", sName);
      XII_DEFAULT_DELETE_RAW_BUFFER(szDynamicBuffer);
      return sDefault;
    }
    else
    {
      xiiString retVal(szDynamicBuffer);
      XII_DEFAULT_DELETE_RAW_BUFFER(szDynamicBuffer);
      return retVal;
    }
  }
  else
  {
    xiiLog::Warning("Couldn't get environment variable value for \"{0}\", got {1} as a result.", sName, res);
    return sDefault;
  }
}

xiiResult xiiEnvironmentVariableUtils::SetValueStringImpl(xiiStringView sName, xiiStringView sValue)
{
  xiiStringWChar szwName(sName);
  xiiStringWChar szwValue(sValue);

  if (_wputenv_s(szwName, szwValue) == 0)
    return XII_SUCCESS;
  else
    return XII_FAILURE;
}

bool xiiEnvironmentVariableUtils::IsVariableSetImpl(xiiStringView sName)
{
  xiiStringWChar szwName(sName);
  wchar_t        szStaticValueBuffer[16] = {0};
  size_t         uiRequiredSize          = 0;

  errno_t res = _wgetenv_s(&uiRequiredSize, szStaticValueBuffer, szwName);

  if (res == 0 || res == ERANGE)
  {
    // Variable doesn't exist if uiRequiredSize is 0
    return uiRequiredSize > 0;
  }
  else
  {
    xiiLog::Error("xiiEnvironmentVariableUtils::IsVariableSet(\"{0}\") got {1} from _wgetenv_s.", sName, res);
    return false;
  }
}

xiiResult xiiEnvironmentVariableUtils::UnsetVariableImpl(xiiStringView sName)
{
  xiiStringWChar szwName(sName);

  if (_wputenv_s(szwName, L"") == 0)
    return XII_SUCCESS;
  else
    return XII_FAILURE;
}
