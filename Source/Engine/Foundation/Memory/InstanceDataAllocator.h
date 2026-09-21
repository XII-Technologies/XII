/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>

/// Structure to describe an instance data type.
///
/// Many resources, such as VMs, state machines and visual scripts of various types have shared state (their configuration)
/// as well as per-instance state (for their execution).
///
/// This structure describes the type of instance data used by a such a resource (or a node inside it).
/// Instance data is allocated through the xiiInstanceDataAllocator.
///
/// Use the templated Fill() method to fill the desc from a data type.
struct XII_FOUNDATION_DLL xiiInstanceDataDesc
{
  xiiUInt32                           m_uiTypeSize          = 0;
  xiiUInt32                           m_uiTypeAlignment     = 0;
  xiiMemoryUtils::ConstructorFunction m_ConstructorFunction = nullptr;
  xiiMemoryUtils::DestructorFunction  m_DestructorFunction  = nullptr;

  template <typename T>
  XII_ALWAYS_INLINE void FillFromType()
  {
    m_uiTypeSize          = sizeof(T);
    m_uiTypeAlignment     = alignof(T);
    m_ConstructorFunction = xiiMemoryUtils::MakeConstructorFunction<SkipTrivialTypes, T>();
    m_DestructorFunction  = xiiMemoryUtils::MakeDestructorFunction<T>();
  }
};

/// Manages complex multi-type instance data allocation with proper construction and destruction.
///
/// This allocator is designed for systems that need to allocate heterogeneous data structures
/// in a single memory block, such as VM instances, state machines, or script execution contexts.
/// It calculates proper alignments for mixed data types and handles construction/destruction
/// automatically.
///
/// Typical workflow:
/// 1. Use AddDesc() to register all data types needed for an instance
/// 2. Call AllocateAndConstruct() to create initialized memory
/// 3. Use GetInstanceData() to access individual data by offset
/// 4. Call DestructAndDeallocate() when the instance is no longer needed
class XII_FOUNDATION_DLL xiiInstanceDataAllocator
{
public:
  /// Adds the given desc to internal list of data that needs to be allocated and returns the byte offset.
  [[nodiscard]] xiiUInt32 AddDesc(const xiiInstanceDataDesc& desc);

  /// Resets all internal state.
  void ClearDescs();

  /// Constructs the instance data objects, within the pre-allocated memory block.
  void Construct(xiiByteBlobPtr blobPtr) const;

  /// Destructs the instance data objects.
  void Destruct(xiiByteBlobPtr blobPtr) const;

  /// Allocates memory and constructs the instance data objects inside it. The returned xiiBlob must be stored somewhere.
  [[nodiscard]] xiiBlob AllocateAndConstruct() const;

  /// Destructs and deallocates the instance data objects and the given memory block.
  void DestructAndDeallocate(xiiBlob& ref_blob) const;

  /// The total size in bytes taken up by all instance data objects that were added.
  xiiUInt32 GetTotalDataSize() const { return m_uiTotalDataSize; }

  /// Retrieves a void pointer to the instance data within the given blob at the given offset, or nullptr if the offset is invalid.
  XII_ALWAYS_INLINE static void* GetInstanceData(const xiiByteBlobPtr& blobPtr, xiiUInt32 uiOffset)
  {
    return (uiOffset != xiiInvalidIndex) ? blobPtr.GetPtr() + uiOffset : nullptr;
  }

private:
  xiiDynamicArray<xiiInstanceDataDesc> m_Descs;
  xiiUInt32                            m_uiTotalDataSize = 0;
};
