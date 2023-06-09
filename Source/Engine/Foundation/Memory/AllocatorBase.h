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

/// \brief Base class for all memory allocators.
class XII_FOUNDATION_DLL xiiAllocatorBase
{
public:
  struct Stats
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt64 m_uiNumAllocations   = 0u; ///< total number of allocations
    xiiUInt64 m_uiNumDeallocations = 0u; ///< total number of deallocations
    xiiUInt64 m_uiAllocationSize   = 0u; ///< total allocation size in bytes

    xiiUInt64 m_uiPerFrameAllocationSize = 0u; ///< allocation size in bytes in this frame
    xiiTime   m_PerFrameAllocationTime;        ///< time spend on allocations in this frame
  };

  xiiAllocatorBase();
  virtual ~xiiAllocatorBase();

  /// \brief Interface, do not use this directly, always use the new/delete macros below
  virtual void* Allocate(size_t uiSize, size_t uiAlign, xiiMemoryUtils::DestructorFunction destructorFunc = nullptr) = 0;
  virtual void  Deallocate(void* pPtr)                                                                               = 0;
  virtual void* Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign);

  /// \brief Returns the number of bytes allocated at this address.
  ///
  /// \note Careful! This information is only available, if allocation tracking is enabled!
  /// Otherwise 0 is returned.
  /// See xiiMemoryTrackingFlags::EnableAllocationTracking and XII_USE_ALLOCATION_TRACKING.
  virtual size_t AllocatedSize(const void* pPtr) = 0;

  virtual xiiAllocatorId GetId() const    = 0;
  virtual Stats          GetStats() const = 0;

private:
  XII_DISALLOW_COPY_AND_ASSIGN(xiiAllocatorBase);
};

#include <Foundation/Memory/Implementation/AllocatorBase_inl.h>

/// \brief creates a new instance of type using the given allocator
#define XII_NEW(allocator, type, ...) \
  xiiInternal::NewInstance<type>(     \
    new ((allocator)->Allocate(sizeof(type), XII_ALIGNMENT_OF(type), xiiMemoryUtils::MakeDestructorFunction<type>())) type(__VA_ARGS__), (allocator))

/// \brief deletes the instance stored in ptr using the given allocator and sets ptr to nullptr
#define XII_DELETE(allocator, ptr)       \
  {                                      \
    xiiInternal::Delete(allocator, ptr); \
    ptr = nullptr;                       \
  }

/// \brief creates a new array of type using the given allocator with count elements, calls default constructor for non-POD types
#define XII_NEW_ARRAY(allocator, type, count) xiiInternal::CreateArray<type>(allocator, count)

/// \brief Calls destructor on every element for non-POD types and deletes the array stored in arrayPtr using the given allocator
#define XII_DELETE_ARRAY(allocator, arrayPtr)      \
  {                                                \
    xiiInternal::DeleteArray(allocator, arrayPtr); \
    arrayPtr.Clear();                              \
  }

/// \brief creates a raw buffer of type using the given allocator with count elements, but does NOT call the default constructor
#define XII_NEW_RAW_BUFFER(allocator, type, count) xiiInternal::CreateRawBuffer<type>(allocator, count)

/// \brief deletes a raw buffer stored in ptr using the given allocator, but does NOT call destructor
#define XII_DELETE_RAW_BUFFER(allocator, ptr)     \
  {                                               \
    xiiInternal::DeleteRawBuffer(allocator, ptr); \
    ptr = nullptr;                                \
  }

/// \brief extends a given raw buffer to the new size, taking care of calling constructors / assignment operators.
#define XII_EXTEND_RAW_BUFFER(allocator, ptr, oldSize, newSize) xiiInternal::ExtendRawBuffer(ptr, allocator, oldSize, newSize)


/// \brief creates a new instance of type using the default allocator
#define XII_DEFAULT_NEW(type, ...) XII_NEW(xiiFoundation::GetDefaultAllocator(), type, __VA_ARGS__)

/// \brief deletes the instance stored in ptr using the default allocator and sets ptr to nullptr
#define XII_DEFAULT_DELETE(ptr) XII_DELETE(xiiFoundation::GetDefaultAllocator(), ptr)

/// \brief creates a new array of type using the default allocator with count elements, calls default constructor for non-POD types
#define XII_DEFAULT_NEW_ARRAY(type, count) XII_NEW_ARRAY(xiiFoundation::GetDefaultAllocator(), type, count)

/// \brief calls destructor on every element for non-POD types and deletes the array stored in arrayPtr using the default allocator
#define XII_DEFAULT_DELETE_ARRAY(arrayPtr) XII_DELETE_ARRAY(xiiFoundation::GetDefaultAllocator(), arrayPtr)

/// \brief creates a raw buffer of type using the default allocator with count elements, but does NOT call the default constructor
#define XII_DEFAULT_NEW_RAW_BUFFER(type, count) XII_NEW_RAW_BUFFER(xiiFoundation::GetDefaultAllocator(), type, count)

/// \brief deletes a raw buffer stored in ptr using the default allocator, but does NOT call destructor
#define XII_DEFAULT_DELETE_RAW_BUFFER(ptr) XII_DELETE_RAW_BUFFER(xiiFoundation::GetDefaultAllocator(), ptr)
