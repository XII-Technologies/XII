/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Singleton.h>

xiiMap<size_t, xiiSingletonRegistry::SingletonEntry> xiiSingletonRegistry::s_Singletons;

const xiiMap<size_t, xiiSingletonRegistry::SingletonEntry>& xiiSingletonRegistry::GetAllRegisteredSingletons()
{
  return s_Singletons;
}

XII_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_Singleton);
