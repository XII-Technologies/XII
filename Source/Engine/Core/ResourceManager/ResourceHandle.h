/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>

#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Foundation/Reflection/Reflection.h>

/// If this is set to XII_ON, stack traces are recorded for every resource handle.
///
/// This can be used to find the places that create resource handles but do not properly clean them up.
#define XII_RESOURCEHANDLE_STACK_TRACES XII_OFF

class xiiResource;

template <typename T>
class xiiResourceLock;

// These out-of-line helper functions allow to forward declare resource handles without knowledge about the resource class.
XII_CORE_DLL void IncreaseResourceRefCount(xiiResource* pResource, const void* pOwner);
XII_CORE_DLL void DecreaseResourceRefCount(xiiResource* pResource, const void* pOwner);

#if XII_ENABLED(XII_RESOURCEHANDLE_STACK_TRACES)
XII_CORE_DLL void MigrateResourceRefCount(xiiResource* pResource, const void* pOldOwner, const void* pNewOwner);
#else
XII_ALWAYS_INLINE void MigrateResourceRefCount(xiiResource* pResource, const void* pOldOwner, const void* pNewOwner)
{
  XII_IGNORE_UNUSED(pResource);
  XII_IGNORE_UNUSED(pOldOwner);
  XII_IGNORE_UNUSED(pNewOwner);
}
#endif

/// The typeless implementation of resource handles. A typed interface is provided by xiiTypedResourceHandle.
class XII_CORE_DLL xiiTypelessResourceHandle
{
public:
  XII_ALWAYS_INLINE xiiTypelessResourceHandle() = default;

  /// [internal] Increases the refcount of the given resource.
  xiiTypelessResourceHandle(xiiResource* pResource);

  /// Increases the refcount of the given resource
  XII_ALWAYS_INLINE xiiTypelessResourceHandle(const xiiTypelessResourceHandle& rhs)
  {
    m_pResource = rhs.m_pResource;

    if (m_pResource)
    {
      IncreaseResourceRefCount(m_pResource, this);
    }
  }

  /// Move constructor, no refcount change is necessary.
  XII_ALWAYS_INLINE xiiTypelessResourceHandle(xiiTypelessResourceHandle&& rhs)
  {
    m_pResource     = rhs.m_pResource;
    rhs.m_pResource = nullptr;

    if (m_pResource)
    {
      MigrateResourceRefCount(m_pResource, &rhs, this);
    }
  }

  /// Releases any referenced resource.
  XII_ALWAYS_INLINE ~xiiTypelessResourceHandle() { Invalidate(); }

  /// Returns whether the handle stores a valid pointer to a resource.
  XII_ALWAYS_INLINE bool IsValid() const { return m_pResource != nullptr; }

  /// Clears any reference to a resource and reduces its refcount.
  void Invalidate();

  /// Returns the Resource ID hash of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  xiiUInt64 GetResourceIDHash() const;

  /// Returns the Resource ID of the exact resource that this handle points to, without acquiring the resource.
  /// If the handle is not valid, an empty string is returned.
  xiiStringView GetResourceID() const;

  /// The returns the resource description, if available, otherwise the resource ID.
  /// This is mainly for logging, where you want the more user friendly description, but the ID, if no description is available.
  /// If the handle is not valid, an empty string is returned.
  xiiStringView GetResourceIdOrDescription() const;

  /// Releases the current reference and increases the refcount of the given resource.
  void operator=(const xiiTypelessResourceHandle& rhs);

  /// Move operator, no refcount change is necessary.
  void operator=(xiiTypelessResourceHandle&& rhs);

  /// Checks whether the two handles point to the same resource.
  XII_ALWAYS_INLINE bool operator==(const xiiTypelessResourceHandle& rhs) const { return m_pResource == rhs.m_pResource; }

  /// For storing handles as keys in maps
  XII_ALWAYS_INLINE bool operator<(const xiiTypelessResourceHandle& rhs) const { return m_pResource < rhs.m_pResource; }

  /// Checks whether the handle points to the given resource.
  XII_ALWAYS_INLINE bool operator==(const xiiResource* rhs) const { return m_pResource == rhs; }

  /// Returns the type information of the resource or nullptr if the handle is invalid.
  const xiiRTTI* GetResourceType() const;

protected:
  xiiResource* m_pResource = nullptr;

private:
  // you must go through the resource manager to get access to the resource pointer
  friend class xiiResourceManager;
  friend class xiiResourceHandleWriteContext;
  friend class xiiResourceHandleReadContext;
  friend class xiiResourceHandleStreamOperations;
};

template <>
struct xiiHashHelper<xiiTypelessResourceHandle>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiTypelessResourceHandle& value) { return xiiHashingUtils::StringHashTo32(value.GetResourceIDHash()); }

  XII_ALWAYS_INLINE static bool Equal(const xiiTypelessResourceHandle& a, const xiiTypelessResourceHandle& b) { return a == b; }
};

/// The xiiTypedResourceHandle controls access to a xiiResource.
///
/// All resources must be referenced using xiiTypedResourceHandle instances (instantiated with the proper resource type as the template
/// argument). You must not store a direct pointer to a resource anywhere. Instead always store resource handles. To actually access a
/// resource, use xiiResourceManager::BeginAcquireResource and xiiResourceManager::EndAcquireResource after you have finished using it.
///
/// xiiTypedResourceHandle implements reference counting on resources. It also allows to redirect resources to fallback resources when they
/// are not yet loaded (if possible).
///
/// As long as there is one resource handle that references a resource, it is considered 'in use' and thus might not get unloaded.
/// So be careful where you store resource handles.
/// If necessary you can call Invalidate() to clear a resource handle and thus also remove the reference to the resource.
template <typename RESOURCE_TYPE>
class xiiTypedResourceHandle
{
public:
  using ResourceType = RESOURCE_TYPE;

  /// A default constructed handle is invalid and does not reference any resource.
  xiiTypedResourceHandle() = default;

  /// Increases the refcount of the given resource.
  explicit xiiTypedResourceHandle(ResourceType* pResource) :
    m_hTypeless(pResource)
  {
  }

  /// Increases the refcount of the given resource.
  xiiTypedResourceHandle(const xiiTypedResourceHandle<ResourceType>& rhs) :
    m_hTypeless(rhs.m_hTypeless)
  {
  }

  /// Move constructor, no refcount change is necessary.
  xiiTypedResourceHandle(xiiTypedResourceHandle<ResourceType>&& rhs) :
    m_hTypeless(std::move(rhs.m_hTypeless))
  {
  }

  template <typename BaseOrDerivedType>
  xiiTypedResourceHandle(const xiiTypedResourceHandle<BaseOrDerivedType>& rhs) :
    m_hTypeless(rhs.m_hTypeless)
  {
    static_assert(std::is_base_of<ResourceType, BaseOrDerivedType>::value || std::is_base_of<BaseOrDerivedType, ResourceType>::value, "Only related types can be assigned to handles of this type");

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (std::is_base_of<BaseOrDerivedType, ResourceType>::value)
    {
      XII_ASSERT_DEBUG(rhs.IsValid(), "Cannot cast invalid base handle to derived type!");
      xiiResourceLock<BaseOrDerivedType> lock(rhs, xiiResourceAcquireMode::PointerOnly);
      XII_ASSERT_DEBUG(xiiDynamicCast<const ResourceType*>(lock.GetPointer()) != nullptr, "Types are not related!");
    }
#endif
  }

  /// Releases the current reference and increases the refcount of the given resource.
  void operator=(const xiiTypedResourceHandle<ResourceType>& rhs) { m_hTypeless = rhs.m_hTypeless; }

  /// Move operator, no refcount change is necessary.
  void operator=(xiiTypedResourceHandle<ResourceType>&& rhs) { m_hTypeless = std::move(rhs.m_hTypeless); }

  /// Checks whether the two handles point to the same resource.
  XII_ALWAYS_INLINE bool operator==(const xiiTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless == rhs.m_hTypeless; }

  /// Checks whether the two handles point to the same resource.
  XII_ALWAYS_INLINE bool operator!=(const xiiTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless != rhs.m_hTypeless; }

  /// For storing handles as keys in maps
  XII_ALWAYS_INLINE bool operator<(const xiiTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless < rhs.m_hTypeless; }

  /// Checks whether the handle points to the given resource.
  XII_ALWAYS_INLINE bool operator==(const xiiResource* rhs) const { return m_hTypeless == rhs; }

  /// Checks whether the handle points to the given resource.
  XII_ALWAYS_INLINE bool operator!=(const xiiResource* rhs) const { return m_hTypeless != rhs; }


  /// Returns the corresponding typeless resource handle.
  XII_ALWAYS_INLINE operator const xiiTypelessResourceHandle() const { return m_hTypeless; }

  /// Returns the corresponding typeless resource handle.
  XII_ALWAYS_INLINE operator xiiTypelessResourceHandle() { return m_hTypeless; }

  /// Returns whether the handle stores a valid pointer to a resource.
  XII_ALWAYS_INLINE bool IsValid() const { return m_hTypeless.IsValid(); }

  /// Returns whether the handle stores a valid pointer to a resource.
  XII_ALWAYS_INLINE explicit operator bool() const { return m_hTypeless.IsValid(); }

  /// Clears any reference to a resource and reduces its refcount.
  XII_ALWAYS_INLINE void Invalidate() { m_hTypeless.Invalidate(); }

  /// Returns the Resource ID hash of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  XII_ALWAYS_INLINE xiiUInt64 GetResourceIDHash() const { return m_hTypeless.GetResourceIDHash(); }

  /// Returns the Resource ID of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  XII_ALWAYS_INLINE xiiStringView GetResourceID() const { return m_hTypeless.GetResourceID(); }

  /// The returns the resource description, if available, otherwise the resource ID.
  /// This is mainly for logging, where you want the more user friendly description, but the ID, if no description is available.
  /// If the handle is not valid, an empty string is returned.
  XII_ALWAYS_INLINE xiiStringView GetResourceIdOrDescription() const { return m_hTypeless.GetResourceIdOrDescription(); }

  /// Attempts to copy the given typeless handle to this handle.
  ///
  /// It is an error to assign a typeless handle that references a resource with a mismatching type.
  void AssignFromTypelessHandle(const xiiTypelessResourceHandle& hHandle)
  {
    if (!hHandle.IsValid())
      return;

    XII_ASSERT_DEV(hHandle.GetResourceType()->IsDerivedFrom<RESOURCE_TYPE>(), "Type '{}' does not match resource type '{}' in typeless handle.", xiiGetStaticRTTI<RESOURCE_TYPE>()->GetTypeName(), hHandle.GetResourceType()->GetTypeName());

    m_hTypeless = hHandle;
  }

private:
  template <typename T>
  friend class xiiTypedResourceHandle;

  // you must go through the resource manager to get access to the resource pointer
  friend class xiiResourceManager;
  friend class xiiResourceHandleWriteContext;
  friend class xiiResourceHandleReadContext;
  friend class xiiResourceHandleStreamOperations;

  xiiTypelessResourceHandle m_hTypeless;
};

template <typename T>
struct xiiHashHelper<xiiTypedResourceHandle<T>>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiTypedResourceHandle<T>& value) { return xiiHashingUtils::StringHashTo32(value.GetResourceIDHash()); }

  XII_ALWAYS_INLINE static bool Equal(const xiiTypedResourceHandle<T>& a, const xiiTypedResourceHandle<T>& b) { return a == b; }
};

// Stream operations
class xiiResource;

class XII_CORE_DLL xiiResourceHandleStreamOperations
{
public:
  template <typename ResourceType>
  static void WriteHandle(xiiStreamWriter& ref_stream, const xiiTypedResourceHandle<ResourceType>& hResource)
  {
    WriteHandle(ref_stream, hResource.m_hTypeless.m_pResource);
  }

  template <typename ResourceType>
  static void ReadHandle(xiiStreamReader& ref_stream, xiiTypedResourceHandle<ResourceType>& ref_hResourceHandle)
  {
    ReadHandle(ref_stream, ref_hResourceHandle.m_hTypeless);
  }

private:
  static void WriteHandle(xiiStreamWriter& Stream, const xiiResource* pResource);
  static void ReadHandle(xiiStreamReader& Stream, xiiTypelessResourceHandle& ResourceHandle);
};

/// Operator to serialize resource handles
template <typename ResourceType>
void operator<<(xiiStreamWriter& ref_stream, const xiiTypedResourceHandle<ResourceType>& hValue)
{
  xiiResourceHandleStreamOperations::WriteHandle(ref_stream, hValue);
}

/// Operator to deserialize resource handles
template <typename ResourceType>
void operator>>(xiiStreamReader& ref_stream, xiiTypedResourceHandle<ResourceType>& ref_hValue)
{
  xiiResourceHandleStreamOperations::ReadHandle(ref_stream, ref_hValue);
}
