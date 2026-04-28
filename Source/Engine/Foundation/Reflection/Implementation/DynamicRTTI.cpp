/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <Foundation/Reflection/Implementation/RTTI.h>

bool xiiReflectedClass::IsInstanceOf(const xiiRTTI* pType) const
{
  return GetDynamicRTTI()->IsDerivedFrom(pType);
}

XII_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_DynamicRTTI);
