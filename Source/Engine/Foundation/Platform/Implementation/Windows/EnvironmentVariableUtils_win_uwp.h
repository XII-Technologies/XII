#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

xiiString xiiEnvironmentVariableUtils::GetValueStringImpl(xiiStringView sName, xiiStringView sDefault)
{
  XII_ASSERT_NOT_IMPLEMENTED;
  return "";
}

xiiResult xiiEnvironmentVariableUtils::SetValueStringImpl(xiiStringView sName, xiiStringView sValue)
{
  XII_ASSERT_NOT_IMPLEMENTED;
  return XII_FAILURE;
}

bool xiiEnvironmentVariableUtils::IsVariableSetImpl(xiiStringView sName)
{
  return false;
}

xiiResult xiiEnvironmentVariableUtils::UnsetVariableImpl(xiiStringView sName)
{
  return XII_FAILURE;
}
