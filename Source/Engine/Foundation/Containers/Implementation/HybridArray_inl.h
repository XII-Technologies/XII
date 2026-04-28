/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename T, xiiUInt32 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
xiiHybridArray<T, Size, AllocatorWrapper>::xiiHybridArray() :
  xiiDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, AllocatorWrapper::GetAllocator())
{
}

template <typename T, xiiUInt32 Size, typename AllocatorWrapper /*= xiiDefaultAllocatorWrapper*/>
xiiHybridArray<T, Size, AllocatorWrapper>::xiiHybridArray(xiiAllocator* pAllocator) :
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

//////////////////////////////////////////////////////////////////////////

template <typename T, xiiUInt32 Size>
xiiTemporaryHybridArray<T, Size>::xiiTemporaryHybridArray() :
  xiiHybridArray<T, Size>(xiiTemporaryAllocator::Get())
{
}

template <typename T, xiiUInt32 Size>
template <typename AllocatorWrapper>
xiiTemporaryHybridArray<T, Size>::xiiTemporaryHybridArray(const xiiHybridArray<T, Size, AllocatorWrapper>& other) :
  xiiHybridArray<T, Size>(xiiTemporaryAllocator::Get())
{
  *this = other;
}

template <typename T, xiiUInt32 Size>
xiiTemporaryHybridArray<T, Size>::xiiTemporaryHybridArray(const xiiArrayPtr<const T>& other) :
  xiiHybridArray<T, Size>(xiiTemporaryAllocator::Get())
{
  *this = other;
}

template <typename T, xiiUInt32 Size>
template <typename AllocatorWrapper>
void xiiTemporaryHybridArray<T, Size>::operator=(const xiiHybridArray<T, Size, AllocatorWrapper>& rhs)
{
  xiiDynamicArray<T>::operator=(rhs);
}

template <typename T, xiiUInt32 Size>
void xiiTemporaryHybridArray<T, Size>::operator=(const xiiArrayPtr<const T>& rhs)
{
  xiiDynamicArray<T>::operator=(rhs);
}

template <typename T, xiiUInt32 Size>
void xiiTemporaryHybridArray<T, Size>::operator=(xiiHybridArray<T, Size>&& rhs) noexcept
{
  xiiDynamicArray<T>::operator=(std::move(rhs));
}
