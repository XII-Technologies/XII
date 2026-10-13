/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Singleton.h>

xiiMap<size_t, xiiSingletonRegistry::SingletonEntry>& xiiSingletonRegistry::GetSingletons()
{
  // Static plugin constructors may register before other translation units initialize.
  // Constructing storage here also keeps it alive until those singletons are destroyed.
  static xiiMap<size_t, SingletonEntry> s_Singletons(xiiStaticAllocatorWrapper::GetAllocator());
  return s_Singletons;
}

const xiiMap<size_t, xiiSingletonRegistry::SingletonEntry>& xiiSingletonRegistry::GetAllRegisteredSingletons()
{
  return GetSingletons();
}
