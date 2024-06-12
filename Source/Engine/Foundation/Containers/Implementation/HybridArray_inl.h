#include <Foundation/Containers/HybridArray.h>

template <typename T, xiiUInt32 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
xiiHybridArray<T, Size, AllocatorWrapper>::xiiHybridArray() :
  xiiDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, AllocatorWrapper::GetAllocator())
{
}

template <typename T, xiiUInt32 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
xiiHybridArray<T, Size, AllocatorWrapper>::xiiHybridArray(xiiAllocatorBase* pAllocator) :
  xiiDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, pAllocator)
{
}

template <typename T, xiiUInt32 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
xiiHybridArray<T, Size, AllocatorWrapper>::xiiHybridArray(const xiiHybridArray<T, Size, AllocatorWrapper>& other) :
  xiiDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, AllocatorWrapper::GetAllocator())
{
  *this = other;
}

template <typename T, xiiUInt32 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
xiiHybridArray<T, Size, AllocatorWrapper>::xiiHybridArray(const xiiArrayPtr<const T>& other) :
  xiiDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, AllocatorWrapper::GetAllocator())
{
  *this = other;
}

template <typename T, xiiUInt32 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
xiiHybridArray<T, Size, AllocatorWrapper>::xiiHybridArray(xiiHybridArray<T, Size, AllocatorWrapper>&& other) noexcept :
  xiiDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, other.GetAllocator())
{
  *this = std::move(other);
}

template <typename T, xiiUInt32 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
void xiiHybridArray<T, Size, AllocatorWrapper>::operator=(const xiiHybridArray<T, Size, AllocatorWrapper>& rhs)
{
  xiiDynamicArray<T, AllocatorWrapper>::operator=(rhs);
}

template <typename T, xiiUInt32 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
void xiiHybridArray<T, Size, AllocatorWrapper>::operator=(const xiiArrayPtr<const T>& rhs)
{
  xiiDynamicArray<T, AllocatorWrapper>::operator=(rhs);
}

template <typename T, xiiUInt32 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
void xiiHybridArray<T, Size, AllocatorWrapper>::operator=(xiiHybridArray<T, Size, AllocatorWrapper>&& rhs) noexcept
{
  xiiDynamicArray<T, AllocatorWrapper>::operator=(std::move(rhs));
}
