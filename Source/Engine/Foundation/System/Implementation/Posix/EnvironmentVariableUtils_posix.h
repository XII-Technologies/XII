#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <stdlib.h>

xiiString xiiEnvironmentVariableUtils::GetValueStringImpl(const char* szName, const char* szDefault)
{
  const char* value = getenv(szName);
  return value != nullptr ? value : szDefault;
}

xiiResult xiiEnvironmentVariableUtils::SetValueStringImpl(const char* szName, const char* szValue)
{
  if (setenv(szName, szValue, 1) == 0)
    return XII_SUCCESS;
  else
    return XII_FAILURE;
}

bool xiiEnvironmentVariableUtils::IsVariableSetImpl(const char* szName)
{
  return getenv(szName) != nullptr;
}

xiiResult xiiEnvironmentVariableUtils::UnsetVariableImpl(const char* szName)
{
  if (unsetenv(szName) == 0)
    return XII_SUCCESS;
  else
    return XII_FAILURE;
}
