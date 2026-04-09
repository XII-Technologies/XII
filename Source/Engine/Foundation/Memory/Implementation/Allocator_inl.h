
XII_ALWAYS_INLINE xiiAllocator::xiiAllocator() = default;

XII_ALWAYS_INLINE xiiAllocator::~xiiAllocator() = default;

namespace xiiMath
{
  // due to #include order issues, we have to forward declare this function here

  XII_FOUNDATION_DLL xiiUInt64 SafeMultiply64(xiiUInt64 a, xiiUInt64 b, xiiUInt64 c, xiiUInt64 d);
} // namespace xiiMath

namespace xiiInternal
{
  template <typename T>
  struct NewInstance
  {
    XII_ALWAYS_INLINE NewInstance(T* pInstance, xiiAllocator* pAllocator)
    {
      m_pInstance  = pInstance;
      m_pAllocator = pAllocator;
    }

    template <typename U>
    XII_ALWAYS_INLINE NewInstance(NewInstance<U>&& other)
    {
      m_pInstance  = other.m_pInstance;
      m_pAllocator = other.m_pAllocator;

      other.m_pInstance  = nullptr;
      other.m_pAllocator = nullptr;
    }

    XII_ALWAYS_INLINE NewInstance(std::nullptr_t) {}

    template <typename U>
    XII_ALWAYS_INLINE NewInstance<U> Cast()
    {
      return NewInstance<U>(static_cast<U*>(m_pInstance), m_pAllocator);
    }

    XII_ALWAYS_INLINE operator T*() { return m_pInstance; }

    XII_ALWAYS_INLINE T* operator->() { return m_pInstance; }

    T*            m_pInstance  = nullptr;
    xiiAllocator* m_pAllocator = nullptr;
  };

  template <typename T>
  XII_ALWAYS_INLINE bool operator<(const NewInstance<T>& lhs, T* rhs)
  {
    return lhs.m_pInstance < rhs;
  }

  template <typename T>
  XII_ALWAYS_INLINE bool operator<(T* lhs, const NewInstance<T>& rhs)
  {
    return lhs < rhs.m_pInstance;
  }

  template <typename T>
  XII_FORCE_INLINE void Delete(xiiAllocator* pAllocator, T* pPtr)
  {
    if (pPtr != nullptr)
    {
      xiiMemoryUtils::Destruct(pPtr, 1);
      pAllocator->Deallocate(pPtr);
    }
  }

  template <typename T>
  XII_FORCE_INLINE T* CreateRawBuffer(xiiAllocator* pAllocator, size_t uiCount)
  {
    xiiUInt64 safeAllocationSize = xiiMath::SafeMultiply64(uiCount, sizeof(T));
    return static_cast<T*>(pAllocator->Allocate(static_cast<size_t>(safeAllocationSize), alignof(T))); // Down-cast to size_t for 32-bit.
  }

  XII_FORCE_INLINE void DeleteRawBuffer(xiiAllocator* pAllocator, void* pPtr)
  {
    if (pPtr != nullptr)
    {
      pAllocator->Deallocate(pPtr);
    }
  }

  template <typename T>
  inline xiiArrayPtr<T> CreateArray(xiiAllocator* pAllocator, xiiUInt32 uiCount)
  {
    T* buffer = CreateRawBuffer<T>(pAllocator, uiCount);
    xiiMemoryUtils::Construct<SkipTrivialTypes>(buffer, uiCount);

    return xiiArrayPtr<T>(buffer, uiCount);
  }

  template <typename T>
  inline void DeleteArray(xiiAllocator* pAllocator, xiiArrayPtr<T> arrayPtr)
  {
    T* buffer = arrayPtr.GetPtr();
    if (buffer != nullptr)
    {
      xiiMemoryUtils::Destruct(buffer, arrayPtr.GetCount());
      pAllocator->Deallocate(buffer);
    }
  }

  template <typename T>
  XII_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, xiiAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount, xiiTypeIsPod)
  {
    return (T*)pAllocator->Reallocate(pPtr, uiCurrentCount * sizeof(T), uiNewCount * sizeof(T), alignof(T));
  }

  template <typename T>
  XII_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, xiiAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount, xiiTypeIsMemRelocatable)
  {
    return (T*)pAllocator->Reallocate(pPtr, uiCurrentCount * sizeof(T), uiNewCount * sizeof(T), alignof(T));
  }

  template <typename T>
  XII_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, xiiAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount, xiiTypeIsClass)
  {
    static_assert(!std::is_trivial<T>::value, "POD type is treated as class. Use XII_DECLARE_POD_TYPE(YourClass) or XII_DEFINE_AS_POD_TYPE(ExternalClass) to mark it as POD.");

    T* pNewMemory = CreateRawBuffer<T>(pAllocator, uiNewCount);
    xiiMemoryUtils::RelocateConstruct(pNewMemory, pPtr, uiCurrentCount);
    DeleteRawBuffer(pAllocator, pPtr);
    return pNewMemory;
  }

  template <typename T>
  XII_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, xiiAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount)
  {
    XII_ASSERT_DEV(uiCurrentCount < uiNewCount, "Shrinking of a buffer is not implemented yet");
    XII_ASSERT_DEV(!(uiCurrentCount == uiNewCount), "Same size passed in twice.");
    if (pPtr == nullptr)
    {
      XII_ASSERT_DEV(uiCurrentCount == 0, "current count must be 0 if ptr is nullptr");

      return CreateRawBuffer<T>(pAllocator, uiNewCount);
    }
    return ExtendRawBuffer(pPtr, pAllocator, uiCurrentCount, uiNewCount, xiiGetTypeClass<T>());
  }
} // namespace xiiInternal
