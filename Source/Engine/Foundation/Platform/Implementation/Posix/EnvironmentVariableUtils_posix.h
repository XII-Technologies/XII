/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <stdlib.h>

xiiString xiiEnvironmentVariableUtils::GetValueStringImpl(xiiStringView sName, xiiStringView sDefault)
{
  const char* szValue = getenv(sName.GetStartPointer());
  return szValue != nullptr ? szValue : sDefault;
}

xiiResult xiiEnvironmentVariableUtils::SetValueStringImpl(xiiStringView sName, xiiStringView sValue)
{
  if (setenv(sName.GetStartPointer(), sValue.GetStartPointer(), 1) == 0)
    return XII_SUCCESS;
  else
    return XII_FAILURE;
}

bool xiiEnvironmentVariableUtils::IsVariableSetImpl(xiiStringView sName)
{
  return getenv(sName.GetStartPointer()) != nullptr;
}

xiiResult xiiEnvironmentVariableUtils::UnsetVariableImpl(xiiStringView sName)
{
  if (unsetenv(sName.GetStartPointer()) == 0)
    return XII_SUCCESS;
  else
    return XII_FAILURE;
}
