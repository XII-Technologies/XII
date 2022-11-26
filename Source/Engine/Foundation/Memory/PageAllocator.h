#pragma once

#include <Foundation/Basics.h>

/// \brief This helper class can reserve and allocate whole memory pages.
class XII_FOUNDATION_DLL xiiPageAllocator
{
public:
  static void* AllocatePage(size_t uiSize);
  static void  DeallocatePage(void* ptr);

  static xiiAllocatorId GetId();
};
