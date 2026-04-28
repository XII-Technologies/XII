/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#define XII_CHECK_CLASS(T) \
  static_assert(!std::is_trivial<T>::value, "Trivial POD type is treated as class. Use XII_DECLARE_POD_TYPE(YourClass) or XII_DEFINE_AS_POD_TYPE(ExternalClass) to mark it as POD.")

template <xiiConstructionMode mode, typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::Construct(T* pDestination, size_t uiCount)
{
  if constexpr (mode == SkipTrivialTypes && std::is_trivial<T>::value)
  {
    // Do nothing.
  }
  else
  {
    for (size_t i = 0; i < uiCount; ++i)
    {
      ::new (pDestination + i) T();
    }
  }
}

template <xiiConstructionMode mode, typename T>
XII_ALWAYS_INLINE xiiMemoryUtils::ConstructorFunction xiiMemoryUtils::MakeConstructorFunction()
{
  if constexpr (mode == SkipTrivialTypes && std::is_trivial<T>::value)
  {
    return nullptr;
  }
  else
  {
    struct Helper
    {
      static void Construct(void* pDestination) { xiiMemoryUtils::Construct<mode>(static_cast<T*>(pDestination), 1); }
    };

    return &Helper::Construct;
  }
}

template <typename Destination, typename Source>
XII_ALWAYS_INLINE void xiiMemoryUtils::CopyConstruct(Destination* pDestination, const Source& copy, size_t uiCount)
{
  if constexpr (xiiIsPodType<Destination>::value)
  {
    static_assert(std::is_same<Destination, Source>::value || (std::is_base_of<Destination, Source>::value == false && std::is_base_of<Source, Destination>::value == false),
                  "Can't copy POD types that are derived from each other. Are you certain any of these types should be POD?");

    const Destination& copyConverted = copy;

    for (size_t i = 0; i < uiCount; ++i)
    {
      memcpy(pDestination + i, &copyConverted, sizeof(Destination));
    }
  }
  else
  {
    XII_CHECK_CLASS(Destination);

    for (size_t i = 0; i < uiCount; ++i)
    {
      ::new (pDestination + i) Destination(copy); // Note that until now copy has not been converted to Destination. This allows for calling
                                                  // specialized constructors if available.
    }
  }
}

template <typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::CopyConstructArray(T* pDestination, const T* pSource, size_t uiCount)
{
  XII_ASSERT_DEV(pDestination + uiCount <= pSource || pSource + uiCount <= pDestination, "Memory regions must not overlap when using CopyConstruct.");

  if constexpr (xiiIsPodType<T>::value)
  {
    memcpy(pDestination, pSource, uiCount * sizeof(T));
  }
  else
  {
    XII_CHECK_CLASS(T);

    for (size_t i = 0; i < uiCount; ++i)
    {
      ::new (pDestination + i) T(pSource[i]);
    }
  }
}

template <typename T>
XII_ALWAYS_INLINE xiiMemoryUtils::CopyConstructorFunction xiiMemoryUtils::MakeCopyConstructorFunction()
{
  struct Helper
  {
    static void CopyConstruct(void* pDestination, const void* pSource)
    {
      xiiMemoryUtils::CopyConstruct(static_cast<T*>(pDestination), *static_cast<const T*>(pSource), 1);
    }
  };

  return &Helper::CopyConstruct;
}

template <typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::MoveConstruct(T* pDestination, T&& source)
{
  // Make sure source is actually an rvalue reference (T&& is a universal reference).
  static_assert(std::is_rvalue_reference<decltype(source)>::value, "'source' parameter is not an rvalue reference.");
  ::new (pDestination) T(std::forward<T>(source));
}

template <typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::MoveConstruct(T* pDestination, T* pSource, size_t uiCount)
{
  XII_ASSERT_DEV(pDestination + uiCount <= pSource || pSource + uiCount <= pDestination, "Memory regions must not overlap when using MoveConstruct.");

  // Enforce move construction.
  static_assert(std::is_move_constructible<T>::value, "Type is not move constructible!");

  for (size_t i = 0; i < uiCount; ++i)
  {
    ::new (pDestination + i) T(std::move(pSource[i]));
  }
}

template <typename Destination, typename Source>
XII_ALWAYS_INLINE void xiiMemoryUtils::CopyOrMoveConstruct(Destination* pDestination, Source&& source)
{
  if constexpr (std::is_rvalue_reference<decltype(source)>::value)
  {
    static_assert(std::is_rvalue_reference<decltype(source)>::value, "This version of CopyOrMoveConstruct should only be called with a rvalue reference!");

    ::new (pDestination) Destination(std::move(source));
  }
  else
  {
    CopyConstruct<Destination, Source>(pDestination, source, 1);
  }
}

template <typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::RelocateConstruct(T* pDestination, T* pSource, size_t uiCount)
{
  XII_ASSERT_DEV(pDestination + uiCount <= pSource || pSource + uiCount <= pDestination, "Memory regions must not overlap when using RelocateConstruct.");

  if constexpr (xiiGetTypeClass<T>::value != 0) // POD or mem-relocatable
  {
    memcpy(pDestination, pSource, uiCount * sizeof(T));
  }
  else // class
  {
    XII_CHECK_CLASS(T);

    for (size_t i = 0; i < uiCount; ++i)
    {
      // Note that this calls the move constructor only if available and will copy otherwise.
      ::new (pDestination + i) T(std::move(pSource[i]));
    }

    Destruct(pSource, uiCount);
  }
}

template <typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::Destruct(T* pDestination, size_t uiCount)
{
  if constexpr (xiiIsPodType<T>::value == 1)
  {
    static_assert(std::is_trivially_destructible<T>::value != 0, "Class is declared as POD but has a non-trivial destructor. Remove the destructor or don't declare it as POD.");
  }
  else if constexpr (std::is_trivially_destructible<T>::value == 0)
  {
    for (size_t i = 0; i < uiCount; ++i)
    {
      pDestination[i].~T();
    }
  }
}

template <typename T>
XII_ALWAYS_INLINE xiiMemoryUtils::DestructorFunction xiiMemoryUtils::MakeDestructorFunction()
{
  if constexpr (xiiIsPodType<T>::value)
  {
    return nullptr;
  }
  else
  {
    XII_CHECK_CLASS(T);

    struct Helper
    {
      static void Destruct(void* pDestination) { xiiMemoryUtils::Destruct(static_cast<T*>(pDestination), 1); }
    };

    return &Helper::Destruct;
  }
}

XII_ALWAYS_INLINE void xiiMemoryUtils::RawByteCopy(void* pDestination, const void* pSource, size_t uiNumBytesToCopy)
{
  memcpy(pDestination, pSource, uiNumBytesToCopy);
}

template <typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::Copy(T* pDestination, const T* pSource, size_t uiCount)
{
  XII_ASSERT_DEV(pDestination < pSource || pSource + uiCount <= pDestination, "Memory regions must not overlap when using Copy. Use CopyOverlapped instead.");

  if constexpr (xiiIsPodType<T>::value)
  {
    memcpy(pDestination, pSource, uiCount * sizeof(T));
  }
  else
  {
    XII_CHECK_CLASS(T);

    for (size_t i = 0; i < uiCount; ++i)
    {
      pDestination[i] = pSource[i];
    }
  }
}

template <typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::CopyOverlapped(T* pDestination, const T* pSource, size_t uiCount)
{
  if constexpr (xiiIsPodType<T>::value)
  {
    memmove(pDestination, pSource, uiCount * sizeof(T));
  }
  else
  {
    XII_CHECK_CLASS(T);

    if (pDestination == pSource)
      return;

    if (pDestination < pSource)
    {
      for (size_t i = 0; i < uiCount; ++i)
      {
        pDestination[i] = pSource[i];
      }
    }
    else
    {
      for (size_t i = uiCount; i > 0; --i)
      {
        pDestination[i - 1] = pSource[i - 1];
      }
    }
  }
}

template <typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::Relocate(T* pDestination, T* pSource, size_t uiCount)
{
  XII_ASSERT_DEV(pDestination + uiCount <= pSource || pSource + uiCount <= pDestination, "Memory regions must not overlap when using Relocate.");

  if constexpr (xiiGetTypeClass<T>::value != 0) // POD or mem-relocatable
  {
    memcpy(pDestination, pSource, uiCount * sizeof(T));
  }
  else // class
  {
    XII_CHECK_CLASS(T);

    for (size_t i = 0; i < uiCount; ++i)
    {
      // Note that this calls the move constructor only if available and will copy otherwise.
      pDestination[i] = std::move(pSource[i]);
    }

    Destruct(pSource, uiCount);
  }
}

template <typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::RelocateOverlapped(T* pDestination, T* pSource, size_t uiCount)
{
  if constexpr (xiiGetTypeClass<T>::value == 2) // mem-relocatable
  {
    if (pDestination < pSource)
    {
      size_t uiDestructCount = pSource - pDestination;
      Destruct(pDestination, uiDestructCount);
    }
    else
    {
      size_t uiDestructCount = pDestination - pSource;
      Destruct(pSource + uiCount, uiDestructCount);
    }
    memmove(pDestination, pSource, uiCount * sizeof(T));
  }
  else if constexpr (xiiGetTypeClass<T>::value == 1) // POD
  {
    memmove(pDestination, pSource, uiCount * sizeof(T));
  }
  else
  {
    XII_CHECK_CLASS(T);

    if (pDestination == pSource)
      return;

    if (pDestination < pSource)
    {
      for (size_t i = 0; i < uiCount; ++i)
      {
        pDestination[i] = std::move(pSource[i]);
      }

      size_t uiDestructCount = pSource - pDestination;
      Destruct(pSource + uiCount - uiDestructCount, uiDestructCount);
    }
    else
    {
      for (size_t i = uiCount; i > 0; --i)
      {
        pDestination[i - 1] = std::move(pSource[i - 1]);
      }

      size_t uiDestructCount = pDestination - pSource;
      Destruct(pSource, uiDestructCount);
    }
  }
}

template <typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::Prepend(T* pDestination, const T& source, size_t uiCount)
{
  if constexpr (xiiGetTypeClass<T>::value != 0) // POD or mem-relocatable
  {
    memmove(pDestination + 1, pDestination, uiCount * sizeof(T));
    CopyConstruct(pDestination, source, 1);
  }
  else // class
  {
    XII_CHECK_CLASS(T);

    if (uiCount > 0)
    {
      MoveConstruct(pDestination + uiCount, std::move(pDestination[uiCount - 1]));

      for (size_t i = uiCount - 1; i > 0; --i)
      {
        pDestination[i] = std::move(pDestination[i - 1]);
      }

      *pDestination = source;
    }
    else
    {
      CopyConstruct(pDestination, source, 1);
    }
  }
}

template <typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::Prepend(T* pDestination, T&& source, size_t uiCount)
{
  if constexpr (xiiGetTypeClass<T>::value != 0) // POD or mem-relocatable
  {
    memmove(pDestination + 1, pDestination, uiCount * sizeof(T));
    MoveConstruct(pDestination, std::move(source));
  }
  else // class
  {
    XII_CHECK_CLASS(T);

    if (uiCount > 0)
    {
      MoveConstruct(pDestination + uiCount, std::move(pDestination[uiCount - 1]));

      for (size_t i = uiCount - 1; i > 0; --i)
      {
        pDestination[i] = std::move(pDestination[i - 1]);
      }

      *pDestination = std::move(source);
    }
    else
    {
      MoveConstruct(pDestination, std::move(source));
    }
  }
}

template <typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::Prepend(T* pDestination, const T* pSource, size_t uiSourceCount, size_t uiCount)
{
  if constexpr (xiiGetTypeClass<T>::value != 0) // POD or mem-relocatable
  {
    memmove(pDestination + uiSourceCount, pDestination, uiCount * sizeof(T));
    CopyConstructArray(pDestination, pSource, uiSourceCount);
  }
  else // class
  {
    XII_CHECK_CLASS(T);

    if (uiCount > 0)
    {
      MoveConstruct(pDestination + uiSourceCount, pDestination, uiCount);
      CopyConstructArray(pDestination, pSource, uiSourceCount);
    }
    else
    {
      CopyConstructArray(pDestination, pSource, uiSourceCount);
    }
  }
}

template <typename T>
XII_ALWAYS_INLINE bool xiiMemoryUtils::IsEqual(const T* a, const T* b, size_t uiCount /*= 1*/)
{
  if constexpr (xiiIsPodType<T>::value)
  {
    return memcmp(a, b, uiCount * sizeof(T)) == 0;
  }
  else
  {
    XII_CHECK_CLASS(T);

    for (size_t i = 0; i < uiCount; ++i)
    {
      if (!(a[i] == b[i]))
        return false;
    }
    return true;
  }
}

template <typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::ZeroFill(T* pDestination, size_t uiCount)
{
  memset(pDestination, 0, uiCount * sizeof(T));
}

template <typename T, size_t N>
XII_ALWAYS_INLINE void xiiMemoryUtils::ZeroFillArray(T (&destination)[N])
{
  return ZeroFill(destination, N);
}

template <typename T>
XII_ALWAYS_INLINE void xiiMemoryUtils::PatternFill(T* pDestination, xiiUInt8 uiBytePattern, size_t uiCount)
{
  memset(pDestination, uiBytePattern, uiCount * sizeof(T));
}

template <typename T, size_t N>
XII_ALWAYS_INLINE void xiiMemoryUtils::PatternFillArray(T (&destination)[N], xiiUInt8 uiBytePattern)
{
  return PatternFill(destination, uiBytePattern, N);
}

template <typename T>
XII_ALWAYS_INLINE xiiInt32 xiiMemoryUtils::Compare(const T* a, const T* b, size_t uiCount /*= 1*/)
{
  return memcmp(a, b, uiCount * sizeof(T));
}

XII_ALWAYS_INLINE xiiInt32 xiiMemoryUtils::RawByteCompare(const void* a, const void* b, size_t uiNumBytesToCompare)
{
  return memcmp(a, b, uiNumBytesToCompare);
}

template <typename T>
XII_ALWAYS_INLINE T* xiiMemoryUtils::AddByteOffset(T* pPtr, std::ptrdiff_t offset)
{
  return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(pPtr) + offset);
}

template <typename T>
XII_ALWAYS_INLINE T* xiiMemoryUtils::AlignBackwards(T* pPtr, size_t uiAlignment)
{
  return reinterpret_cast<T*>(reinterpret_cast<size_t>(pPtr) & ~(uiAlignment - 1));
}

template <typename T>
XII_ALWAYS_INLINE T* xiiMemoryUtils::AlignForwards(T* pPtr, size_t uiAlignment)
{
  return reinterpret_cast<T*>((reinterpret_cast<size_t>(pPtr) + uiAlignment - 1) & ~(uiAlignment - 1));
}

template <typename T>
XII_ALWAYS_INLINE T xiiMemoryUtils::AlignSize(T uiSize, T uiAlignment)
{
  return ((uiSize + (uiAlignment - 1)) & ~(uiAlignment - 1));
}

template <typename T>
XII_ALWAYS_INLINE bool xiiMemoryUtils::IsAligned(const T* pPtr, size_t uiAlignment)
{
  return (reinterpret_cast<size_t>(pPtr) & (uiAlignment - 1)) == 0;
}

template <typename T>
XII_ALWAYS_INLINE bool xiiMemoryUtils::IsSizeAligned(T uiSize, T uiAlignment)
{
  return (uiSize & (uiAlignment - 1)) == 0;
}

#undef XII_CHECK_CLASS
