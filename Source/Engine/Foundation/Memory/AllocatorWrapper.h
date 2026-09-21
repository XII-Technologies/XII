/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Memory/Allocator.h>

/// Allocator wrapper that should never be used - causes assertion failures.
///
/// This wrapper is used as a template parameter to indicate that no allocator
/// should be used. Any attempt to call GetAllocator() will trigger an assertion.
/// Useful for container types that should never allocate.
struct xiiNullAllocatorWrapper
{
  XII_FORCE_INLINE static xiiAllocator* GetAllocator()
  {
    XII_REPORT_FAILURE("This method should never be called");
    return nullptr;
  }
};

/// Wrapper for the engine's default general-purpose allocator.
struct xiiDefaultAllocatorWrapper
{
  XII_ALWAYS_INLINE static xiiAllocator* GetAllocator() { return xiiFoundation::GetDefaultAllocator(); }
};

/// Wrapper for the allocator used for static/global objects.
struct xiiStaticAllocatorWrapper
{
  XII_ALWAYS_INLINE static xiiAllocator* GetAllocator() { return xiiFoundation::GetStaticAllocator(); }
};

/// Wrapper for the allocator that provides memory with specific alignment guarantees.
struct xiiAlignedAllocatorWrapper
{
  XII_ALWAYS_INLINE static xiiAllocator* GetAllocator() { return xiiFoundation::GetAlignedAllocator(); }
};

/// Helper function to facilitate setting the allocator on member containers of a class
/// Allocators can be either template arguments or a ctor parameter. Using the ctor parameter requires the class ctor to reference each member container in the initialization list. This can be very tedious. On the other hand, the template variant only support template parameter so you can't simply pass in a member allocator.
/// This class solves this problem provided the following rules are followed:
/// 1. The `xiiAllocator` must be the declared at the earliest in the class, before any container.
/// 2. The `xiiLocalAllocatorWrapper` should be declared right afterwards.
/// 3. Any container needs to be declared below these two and must include the `xiiLocalAllocatorWrapper` as a template argument to the allocator.
/// 4. In the ctor initializer list, init the xiiAllocator first, then the xiiLocalAllocatorWrapper. With this approach all containers can be omitted.
/// \code{.cpp}
///   class MyClass
///   {
///     xiiAllocator m_SpecialAlloc;
///     xiiLocalAllocatorWrapper m_Wrapper;
///
///     xiiDynamicArray<int, xiiLocalAllocatorWrapper> m_Data;
///
///     MyClass()
///       : m_SpecialAlloc("MySpecialAlloc")
///       , m_Wrapper(&m_SpecialAlloc)
///     {
///     }
///   }
/// \endcode
struct XII_FOUNDATION_DLL xiiLocalAllocatorWrapper
{
  xiiLocalAllocatorWrapper(xiiAllocator* pAllocator);

  void Reset();

  static xiiAllocator* GetAllocator();
};
