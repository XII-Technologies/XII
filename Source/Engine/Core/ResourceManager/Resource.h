/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Timestamp.h>

/// The base class for all resources.
class XII_CORE_DLL xiiResource : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiResource, xiiReflectedClass);

public:
  /// Specifies the thread type on which update operations should be performed.
  enum class DoUpdate : xiiUInt8
  {
    OnMainThread = 0U, ///< Perform update operations on the main thread.
    OnAnyThread,       ///< Allow update operations to occur on any available thread.
  };

protected:
  /// Defines unloading behavior for quality levels.
  ///
  /// Used to control how quality levels are removed or managed during runtime or resource cleanup.
  enum class Unload : xiiUInt8
  {
    AllQualityLevels = 0U, ///< Unloads all available quality levels. This option is typically used when a full cleanup is required.
    OneQualityLevel        ///< Unloads only one quality level. Useful for selectively freeing memory or optimizing transitions.
  };

  /// Default constructor.
  xiiResource(DoUpdate resourceUpdateThread, xiiUInt8 uiQualityLevelsLoadable);

  /// virtual destructor.
  virtual ~xiiResource();

public:
  struct MemoryUsage
  {
    MemoryUsage()
    {
      m_uiMemoryCPU = 0;
      m_uiMemoryGPU = 0;
    }

    xiiUInt64 m_uiMemoryCPU;
    xiiUInt64 m_uiMemoryGPU;
  };

  /// Returns the unique ID that identifies this resource. On a file resource this might be a path. Can also be a GUID or any other
  /// scheme that uniquely identifies the resource.
  XII_ALWAYS_INLINE xiiStringView GetResourceID() const { return m_sUniqueID; }

  /// Returns the hash of the unique ID.
  XII_ALWAYS_INLINE xiiUInt64 GetResourceIDHash() const { return m_uiUniqueIDHash; }

  /// The resource description allows to store an additional string that might be more descriptive during debugging, than the unique
  /// ID.
  void SetResourceDescription(xiiStringView sDescription);

  /// The resource description allows to store an additional string that might be more descriptive during debugging, than the unique
  /// ID.
  const xiiString& GetResourceDescription() const { return m_sResourceDescription; }

  /// The returns the resource description, if available, otherwise the resource ID.
  ///
  /// This is mainly for logging, where you want the more user friendly description, but the ID, if no description is available.
  const xiiString& GetResourceIdOrDescription() const { return m_sResourceDescription.IsEmpty() ? m_sUniqueID : m_sResourceDescription; }

  /// Returns the current state in which this resource is in.
  XII_ALWAYS_INLINE xiiResourceState GetLoadingState() const { return m_LoadingState; }

  /// Returns the current maximum quality level that the resource could have.
  ///
  /// This is used to scale the amount data used. Once a resource is in the 'Loaded' state, it can still have different
  /// quality levels. E.g. a texture can be fully used with n mipmap levels, but there might be more that could be loaded.
  /// On the other hand a resource could have a higher 'loaded quality level' then the 'max quality level', if the user
  /// just changed settings and reduced the maximum quality level that should be used. In this case the resource manager
  /// will instruct the resource to unload some of its data soon.
  ///
  /// The quality level is a purely logical concept that can be handled very different by different resource types.
  /// E.g. a texture resource could theoretically use one quality level per available mipmap level. However, since
  /// the resource should generally be able to load and unload each quality level separately, it might make more sense
  /// for a texture resource, to use one quality level for everything up to 64*64, and then one quality level for each
  /// mipmap above that, which would result in 5 quality levels for a 1024*1024 texture.
  ///
  /// Most resource will have zero or one quality levels (which is the same) as they are either loaded or not.
  XII_ALWAYS_INLINE xiiUInt8 GetNumQualityLevelsDiscardable() const { return m_uiQualityLevelsDiscardable; }

  /// Returns how many quality levels the resource may additionally load.
  XII_ALWAYS_INLINE xiiUInt8 GetNumQualityLevelsLoadable() const { return m_uiQualityLevelsLoadable; }

  /// Returns the priority that is used by the resource manager to determine which resource to load next.
  float GetLoadingPriority(xiiTime now) const;

  /// Returns the current resource priority.
  xiiResourcePriority GetPriority() const { return m_Priority; }

  /// Changes the current resource priority.
  void SetPriority(xiiResourcePriority priority);

  /// Returns the basic flags for the resource type. Mostly used the resource manager.
  XII_ALWAYS_INLINE const xiiBitflags<xiiResourceFlags>& GetBaseResourceFlags() const { return m_Flags; }

  /// Returns the information about the current memory usage of the resource.
  XII_ALWAYS_INLINE const MemoryUsage& GetMemoryUsage() const { return m_MemoryUsage; }

  /// Returns the time at which the resource was (tried to be) acquired last.
  /// If a resource is acquired using xiiResourceAcquireMode::PointerOnly, this does not update the last acquired time, since the resource is
  /// not acquired for full use.
  XII_ALWAYS_INLINE xiiTime GetLastAcquireTime() const { return m_LastAcquire; }

  /// Returns the reference count of this resource.
  XII_ALWAYS_INLINE xiiInt32 GetReferenceCount() const { return m_iReferenceCount; }

  /// Returns the modification date of the file from which this resource was loaded.
  ///
  /// The date may be invalid, if it cannot be retrieved or the resource was created and not loaded.
  XII_ALWAYS_INLINE const xiiTimestamp& GetLoadedFileModificationTime() const { return m_LoadedFileModificationTime; }

  /// Returns the current value of the resource change counter.
  /// Can be used to detect whether the resource has changed since using it last time.
  ///
  /// The resource change counter is increased by calling IncResourceChangeCounter() or
  /// whenever the resource content is updated.
  XII_ALWAYS_INLINE xiiUInt32 GetCurrentResourceChangeCounter() const { return m_uiResourceChangeCounter; }

  /// Allows to manually increase the resource change counter to signal that dependent code might need to update.
  XII_ALWAYS_INLINE void IncResourceChangeCounter() { ++m_uiResourceChangeCounter; }

  /// If the resource has modifications from the original state, it should reset itself to that state now (or force a reload on
  /// itself).
  virtual void ResetResource() {}

  /// Prints the stack-traces for all handles that currently reference this resource.
  ///
  /// Only implemented if XII_RESOURCEHANDLE_STACK_TRACES is XII_ON.
  /// Otherwise the function does nothing.
  void PrintHandleStackTraces();

  mutable xiiEvent<const xiiResourceEvent&, xiiMutex> m_ResourceEvents;

private:
  friend class xiiResourceManager;
  friend class xiiResourceManagerWorkerDataLoad;
  friend class xiiResourceManagerWorkerUpdateContent;

  /// Called by xiiResourceManager shortly after resource creation.
  void SetUniqueID(xiiStringView sUniqueID, bool bIsReloadable);

  void CallUnloadData(Unload WhatToUnload);

  /// Requests the resource to unload another quality level. If bFullUnload is true, the resource should unload all data, because it
  /// is going to be deleted afterwards.
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) = 0;

  void CallUpdateContent(xiiStreamReader* pStream);

  /// Called whenever more data for the resource is available. The resource must read the stream to update it's data.
  ///
  /// pStream may be nullptr in case the resource data could not be found.
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) = 0;

  /// Returns the resource type loader that should be used for this type of resource, unless it has been overridden on the
  /// xiiResourceManager.
  ///
  /// By default, this redirects to xiiResourceManager::GetDefaultResourceLoader. So there is one global default loader, that can be set
  /// on the resource manager. Overriding this function will then allow to use a different resource loader on a specific type.
  /// Additionally, one can override the resource loader from the outside, by setting it via xiiResourceManager::SetResourceTypeLoader.
  /// That last method always takes precedence and allows to modify the behavior without modifying the code for the resource.
  /// But in the default case, the resource defines which loader is used.
  virtual xiiResourceTypeLoader* GetDefaultResourceTypeLoader() const;

private:
  xiiAtomicInteger<xiiResourceState> m_LoadingState               = xiiResourceState::Unloaded;
  xiiAtomicInteger8                  m_uiQualityLevelsDiscardable = 0U;
  xiiAtomicInteger8                  m_uiQualityLevelsLoadable    = 0U;

protected:
  /// Non-const version for resources that want to write this variable directly.
  MemoryUsage& ModifyMemoryUsage() { return m_MemoryUsage; }

  /// Call this to specify whether a resource is reloadable.
  ///
  /// By default all created resources are flagged as not reloadable.
  /// All resources loaded from file are automatically flagged as reloadable.
  void SetIsReloadable(bool bIsReloadable) { m_Flags.AddOrRemove(xiiResourceFlags::IsReloadable, bIsReloadable); }

  /// Used internally by the code injection macros
  void SetHasLoadingFallback(bool bHasLoadingFallback) { m_Flags.AddOrRemove(xiiResourceFlags::ResourceHasFallback, bHasLoadingFallback); }

private:
  template <typename ResourceType>
  friend class xiiTypedResourceHandle;

  friend XII_CORE_DLL_FRIEND void IncreaseResourceRefCount(xiiResource* pResource, const void* pOwner);
  friend XII_CORE_DLL_FRIEND void DecreaseResourceRefCount(xiiResource* pResource, const void* pOwner);

#if XII_ENABLED(XII_RESOURCEHANDLE_STACK_TRACES)
  friend XII_CORE_DLL_FRIEND void MigrateResourceRefCount(xiiResource* pResource, const void* pOldOwner, const void* pNewOwner);

  struct HandleStackTrace
  {
    xiiUInt32 m_uiNumPtrs = 0;
    void*     m_Ptrs[64];
  };

  xiiMutex                                    m_HandleStackTraceMutex;
  xiiHashTable<const void*, HandleStackTrace> m_HandleStackTraces;
#endif


  /// This function must be overridden by all resource types.
  ///
  /// It has to compute the memory used by this resource.
  /// It is called by the resource manager whenever the resource's data has been loaded or unloaded.
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) = 0;

  virtual void ReportResourceIsMissing();

  virtual bool HasResourceTypeLoadingFallback() const = 0;

  /// Called by xiiResourceMananger::CreateResource
  void VerifyAfterCreateResource(const xiiResourceLoadDescription& ld);

  xiiUInt64          m_uiUniqueIDHash          = 0;
  xiiUInt32          m_uiResourceChangeCounter = 0;
  xiiAtomicInteger32 m_iReferenceCount         = 0;
  // xiiAtomicInteger32 m_iLockCount = 0; // currently not used
  xiiString                     m_sUniqueID;
  xiiString                     m_sResourceDescription;
  MemoryUsage                   m_MemoryUsage;
  xiiBitflags<xiiResourceFlags> m_Flags;

  xiiTime             m_LastAcquire;
  xiiResourcePriority m_Priority = xiiResourcePriority::Medium;
  xiiTimestamp        m_LoadedFileModificationTime;

private:
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  static const xiiResource* GetCurrentlyUpdatingContent();
#endif
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// GLORIOUS MACROS FOR RESOURCE CLASS CODE GENERATION
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Core/ResourceManager/ResourceManager.h>

#define XII_RESOURCE_DECLARE_COMMON_CODE(SELF)                                                                                                 \
  friend class ::xiiResourceManager;                                                                                                           \
                                                                                                                                               \
public:                                                                                                                                        \
  /*                                                                                                                                     \ \ \
  /// Unfortunately this has to be called manually from within dynamic plugins during core engine shutdown.                       \ \ \
  ///                                                                                                                                    \ \ \
  /// Without this, the dynamic plugin might still be referenced by the core engine during later shutdown phases and will crash, because \ \ \
  /// memory and code is still referenced, that is already unloaded.                                                                     \ \ \
  */ \
  static void CleanupDynamicPluginReferences();                                                                                                \
                                                                                                                                               \
  /*                                                                                                                                     \ \ \
  /// Returns a typed resource handle to this resource                                                                            \ \ \
  */ \
  xiiTypedResourceHandle<SELF> GetResourceHandle() const;                                                                                      \
                                                                                                                                               \
  /*                                                                                                                                     \ \ \
  /// Sets the fallback resource that can be used while this resource is not yet loaded.                                          \ \ \
  ///                                                                                                                                    \ \ \
  /// By default there is no fallback resource, so all resource will block the application when requested for the first time.            \ \ \
  */ \
  void SetLoadingFallbackResource(const xiiTypedResourceHandle<SELF>& hResource);                                                              \
                                                                                                                                               \
private:                                                                                                                                       \
  /* These functions are needed to access the static members, such that they get DLL exported, otherwise you get unresolved symbols */         \
  static void                                SetResourceTypeLoadingFallback(const xiiTypedResourceHandle<SELF>& hResource);                    \
  static void                                SetResourceTypeMissingFallback(const xiiTypedResourceHandle<SELF>& hResource);                    \
  static const xiiTypedResourceHandle<SELF>& GetResourceTypeLoadingFallback()                                                                  \
  {                                                                                                                                            \
    return s_TypeLoadingFallback;                                                                                                              \
  }                                                                                                                                            \
  static const xiiTypedResourceHandle<SELF>& GetResourceTypeMissingFallback()                                                                  \
  {                                                                                                                                            \
    return s_TypeMissingFallback;                                                                                                              \
  }                                                                                                                                            \
  virtual bool HasResourceTypeLoadingFallback() const override                                                                                 \
  {                                                                                                                                            \
    return s_TypeLoadingFallback.IsValid();                                                                                                    \
  }                                                                                                                                            \
                                                                                                                                               \
  static xiiTypedResourceHandle<SELF> s_TypeLoadingFallback;                                                                                   \
  static xiiTypedResourceHandle<SELF> s_TypeMissingFallback;                                                                                   \
                                                                                                                                               \
  xiiTypedResourceHandle<SELF> m_hLoadingFallback;



#define XII_RESOURCE_IMPLEMENT_COMMON_CODE(SELF)                                                                         \
  xiiTypedResourceHandle<SELF> SELF::s_TypeLoadingFallback;                                                              \
  xiiTypedResourceHandle<SELF> SELF::s_TypeMissingFallback;                                                              \
                                                                                                                         \
  void SELF::CleanupDynamicPluginReferences()                                                                            \
  {                                                                                                                      \
    s_TypeLoadingFallback.Invalidate();                                                                                  \
    s_TypeMissingFallback.Invalidate();                                                                                  \
    xiiResourceManager::ClearResourceCleanupCallback(&SELF::CleanupDynamicPluginReferences);                             \
  }                                                                                                                      \
                                                                                                                         \
  xiiTypedResourceHandle<SELF> SELF::GetResourceHandle() const                                                           \
  {                                                                                                                      \
    XII_ASSERT_DEV(GetReferenceCount() > 0, "This resource is being deallocated, do not store a handle to it anymore!"); \
    xiiTypedResourceHandle<SELF> handle((SELF*)this);                                                                    \
    return handle;                                                                                                       \
  }                                                                                                                      \
                                                                                                                         \
  void SELF::SetLoadingFallbackResource(const xiiTypedResourceHandle<SELF>& hResource)                                   \
  {                                                                                                                      \
    m_hLoadingFallback = hResource;                                                                                      \
    SetHasLoadingFallback(m_hLoadingFallback.IsValid());                                                                 \
  }                                                                                                                      \
                                                                                                                         \
  void SELF::SetResourceTypeLoadingFallback(const xiiTypedResourceHandle<SELF>& hResource)                               \
  {                                                                                                                      \
    s_TypeLoadingFallback = hResource;                                                                                   \
    XII_RESOURCE_VALIDATE_FALLBACK(SELF);                                                                                \
    xiiResourceManager::AddResourceCleanupCallback(&SELF::CleanupDynamicPluginReferences);                               \
  }                                                                                                                      \
  void SELF::SetResourceTypeMissingFallback(const xiiTypedResourceHandle<SELF>& hResource)                               \
  {                                                                                                                      \
    s_TypeMissingFallback = hResource;                                                                                   \
    XII_RESOURCE_VALIDATE_FALLBACK(SELF);                                                                                \
    xiiResourceManager::AddResourceCleanupCallback(&SELF::CleanupDynamicPluginReferences);                               \
  }


#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
#  define XII_RESOURCE_VALIDATE_FALLBACK(SELF)                                        \
    if (hResource.IsValid())                                                          \
    {                                                                                 \
      xiiResourceLock<SELF> lock(hResource, xiiResourceAcquireMode::BlockTillLoaded); \
      /* if this fails, the 'fallback resource' is missing itself*/                   \
    }
#else
#  define XII_RESOURCE_VALIDATE_FALLBACK(SELF)
#endif

#define XII_RESOURCE_DECLARE_CREATEABLE(SELF, SELF_DESCRIPTOR)             \
protected:                                                                 \
  xiiResourceLoadDescription CreateResource(SELF_DESCRIPTOR&& descriptor); \
                                                                           \
private:

#define XII_RESOURCE_IMPLEMENT_CREATEABLE(SELF, SELF_DESCRIPTOR) xiiResourceLoadDescription SELF::CreateResource(SELF_DESCRIPTOR&& descriptor)
