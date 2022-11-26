#include <Foundation/FoundationPCH.h>

void* xiiAllocatorBase::Reallocate(void* ptr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
{
  void* pNewMem = Allocate(uiNewSize, uiAlign);
  memcpy(pNewMem, ptr, uiCurrentSize);
  Deallocate(ptr);
  return pNewMem;
}



XII_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_AllocatorBase);
