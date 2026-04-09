#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/Allocator.h>

void* xiiAllocator::Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
{
  void* pNewMemory = Allocate(uiNewSize, uiAlign);
  memcpy(pNewMemory, pPtr, uiCurrentSize);
  Deallocate(pPtr);
  return pNewMemory;
}
