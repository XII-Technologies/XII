#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <stdlib.h>

xiiString xiiEnvironmentVariableUtils::GetValueStringImpl(xiiStringView sName, xiiStringView sDefault)
{
  xiiStringBuilder tmp;
  const char* value = getenv(sName.GetData(tmp));
  return value != nullptr ? value : sDefault;
}

xiiResult xiiEnvironmentVariableUtils::SetValueStringImpl(xiiStringView sName, xiiStringView sValue)
{
  xiiStringBuilder tmp;
  if (setenv(sName, sValue.GetData(tmp), 1) == 0)
    return XII_SUCCESS;
  else
    return XII_FAILURE;
}

bool xiiEnvironmentVariableUtils::IsVariableSetImpl(xiiStringView sName)
{
  xiiStringBuilder tmp;
  return getenv(sName.GetData(tmp)) != nullptr;
}

xiiResult xiiEnvironmentVariableUtils::UnsetVariableImpl(xiiStringView sName)
{
  xiiStringBuilder tmp;
  if (unsetenv(sName.GetData(tmp)) == 0)
    return XII_SUCCESS;
  else
    return XII_FAILURE;
}
