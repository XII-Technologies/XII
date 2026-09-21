/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Time/Time.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/Id.h>
#include <utility>


#ifdef new
#  undef new
#endif

#ifdef delete
#  undef delete
#endif

using xiiAllocatorId = xiiGenericId<24, 8>;

/// Base class for all memory allocators.
///
/// This abstract base class defines the interface for all allocators in xiiEngine. Allocators are responsible
/// for memory management and can implement different allocation strategies (heap, linear, frame, etc.).
///
/// Key concepts:
/// - All allocators provide aligned memory allocation
/// - Optional tracking of allocation statistics and debugging information
/// - Virtual interface allows swapping allocator implementations
/// - Thread safety depends on specific allocator implementation
///
/// Usage:
/// - Use XII_NEW/XII_DELETE macros instead of calling Allocate/Deallocate directly
/// - Different allocator types optimize for different usage patterns
/// - Always pair allocations with deallocations using the same allocator instance
class XII_FOUNDATION_DLL xiiAllocator
{
public:
  struct Stats
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt64 m_uiAllocationCount   = 0; ///< The total number of allocations.
    xiiUInt64 m_uiDeallocationCount = 0; ///< The total number of deallocations.
    xiiUInt64 m_uiAllocationSize    = 0; ///< The total allocation size in bytes.

    xiiUInt64 m_uiPerFrameAllocationSize = 0; ///< The allocation size in bytes in this frame.
    xiiTime   m_PerFrameAllocationTime;       ///< Time spent on allocations in this frame.
  };

  xiiAllocator();
  virtual ~xiiAllocator();

  /// Interface, do not use this directly, always use the new/delete macros below
  ///
  /// Allocates aligned memory of the specified size. The destructorFunc parameter is used for
  /// automatic cleanup when the allocator is reset or destroyed (mainly used by linear allocators).
  virtual void* Allocate(size_t uiSize, size_t uiAlign, xiiMemoryUtils::DestructorFunction destructorFunc = nullptr) = 0;

  /// Deallocates memory previously allocated by this allocator.
  ///
  /// The pointer must have been returned by a previous call to Allocate() on this allocator instance.
  /// Passing nullptr is safe and will be ignored.
  virtual void Deallocate(void* pPtr) = 0;

  /// Reallocates memory, potentially moving the data to a new location.
  ///
  /// Default implementation allocates new memory, copies old data, and deallocates the old memory.
  /// Some allocators may provide more efficient implementations.
  virtual void* Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign);

  /// Returns the number of bytes allocated at this address.
  ///
  /// This information is only available if allocation tracking is enabled (see xiiAllocatorTrackingMode
  /// and XII_ALLOC_TRACKING_DEFAULT). Returns 0 when tracking is disabled or for invalid pointers.
  /// Primarily used for debugging and memory analysis.
  virtual size_t AllocatedSize(const void* pPtr) = 0;

  virtual xiiAllocatorId GetId() const    = 0;
  virtual Stats          GetStats() const = 0;

private:
  XII_DISALLOW_COPY_AND_ASSIGN(xiiAllocator);
};

#include <Foundation/Memory/Implementation/Allocator_inl.h>

/// creates a new instance of type using the given allocator
#define XII_NEW(allocator, type, ...) \
  xiiInternal::NewInstance<type>(     \
    new ((allocator)->Allocate(sizeof(type), alignof(type), xiiMemoryUtils::MakeDestructorFunction<type>())) type(__VA_ARGS__), (allocator))

/// deletes the instance stored in ptr using the given allocator and sets ptr to nullptr
#define XII_DELETE(allocator, ptr)       \
  {                                      \
    xiiInternal::Delete(allocator, ptr); \
    ptr = nullptr;                       \
  }

/// creates a new array of type using the given allocator with count elements, calls default constructor for non-POD types
#define XII_NEW_ARRAY(allocator, type, count) xiiInternal::CreateArray<type>(allocator, count)

/// Calls destructor on every element for non-POD types and deletes the array stored in arrayPtr using the given allocator
#define XII_DELETE_ARRAY(allocator, arrayPtr)      \
  {                                                \
    xiiInternal::DeleteArray(allocator, arrayPtr); \
    arrayPtr.Clear();                              \
  }

/// creates a raw buffer of type using the given allocator with count elements, but does NOT call the default constructor
#define XII_NEW_RAW_BUFFER(allocator, type, count) xiiInternal::CreateRawBuffer<type>(allocator, count)

/// deletes a raw buffer stored in ptr using the given allocator, but does NOT call destructor
#define XII_DELETE_RAW_BUFFER(allocator, ptr)     \
  {                                               \
    xiiInternal::DeleteRawBuffer(allocator, ptr); \
    ptr = nullptr;                                \
  }

/// extends a given raw buffer to the new size, taking care of calling constructors / assignment operators.
#define XII_EXTEND_RAW_BUFFER(allocator, ptr, oldSize, newSize) xiiInternal::ExtendRawBuffer(ptr, allocator, oldSize, newSize)



/// creates a new instance of type using the default allocator
#define XII_DEFAULT_NEW(type, ...) XII_NEW(xiiFoundation::GetDefaultAllocator(), type, __VA_ARGS__)

/// deletes the instance stored in ptr using the default allocator and sets ptr to nullptr
#define XII_DEFAULT_DELETE(ptr) XII_DELETE(xiiFoundation::GetDefaultAllocator(), ptr)

/// creates a new array of type using the default allocator with count elements, calls default constructor for non-POD types
#define XII_DEFAULT_NEW_ARRAY(type, count) XII_NEW_ARRAY(xiiFoundation::GetDefaultAllocator(), type, count)

/// calls destructor on every element for non-POD types and deletes the array stored in arrayPtr using the default allocator
#define XII_DEFAULT_DELETE_ARRAY(arrayPtr) XII_DELETE_ARRAY(xiiFoundation::GetDefaultAllocator(), arrayPtr)

/// creates a raw buffer of type using the default allocator with count elements, but does NOT call the default constructor
#define XII_DEFAULT_NEW_RAW_BUFFER(type, count) XII_NEW_RAW_BUFFER(xiiFoundation::GetDefaultAllocator(), type, count)

/// deletes a raw buffer stored in ptr using the default allocator, but does NOT call destructor
#define XII_DEFAULT_DELETE_RAW_BUFFER(ptr) XII_DELETE_RAW_BUFFER(xiiFoundation::GetDefaultAllocator(), ptr)
