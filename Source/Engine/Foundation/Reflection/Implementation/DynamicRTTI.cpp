#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <Foundation/Reflection/Implementation/RTTI.h>

bool xiiReflectedClass::IsInstanceOf(const xiiRTTI* pType) const
{
  return GetDynamicRTTI()->IsDerivedFrom(pType);
}
