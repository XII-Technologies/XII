#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

xiiString xiiEnvironmentVariableUtils::GetValueStringImpl(const char* szName, const char* szDefault)
{
  XII_ASSERT_NOT_IMPLEMENTED
  return "";
}

xiiResult xiiEnvironmentVariableUtils::SetValueStringImpl(const char* szName, const char* szValue)
{
  XII_ASSERT_NOT_IMPLEMENTED
  return XII_FAILURE;
}

bool xiiEnvironmentVariableUtils::IsVariableSetImpl(const char* szName)
{
  return false;
}

xiiResult xiiEnvironmentVariableUtils::UnsetVariableImpl(const char* szName)
{
  return XII_FAILURE;
}
