/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Implementation/WorkerTasks.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/LockedObject.h>
#include <Foundation/Types/UniquePtr.h>

class xiiResourceManagerState;

/// The central class for managing all types derived from xiiResource
class XII_CORE_DLL xiiResourceManager
{
  friend class xiiResourceManagerState;
  static xiiUniquePtr<xiiResourceManagerState> s_pState;

  /// \name Events
  ///@{

public:
  /// Events on individual resources. Subscribe to this to get a notification for events happening on any resource.
  /// If you are only interested in events for a specific resource, subscribe on directly on that instance.
  static const xiiEvent<const xiiResourceEvent&, xiiMutex>& GetResourceEvents();

  /// Events for the resource manager that affect broader things.
  static const xiiEvent<const xiiResourceManagerEvent&, xiiMutex>& GetManagerEvents();

  /// Goes through all existing resources and broadcasts the 'Exists' event.
  ///
  /// Used to announce all currently existing resources to interested event listeners (ie tools).
  static void BroadcastExistsEvent();

  ///@}
  /// \name Loading and creating resources
  ///@{

public:
  /// Returns a handle to the requested resource. sResourceID must uniquely identify the resource, different spellings / casing
  /// will result in different resources.
  ///
  /// After the call to this function the resource definitely exists in memory. Upon access through BeginAcquireResource / xiiResourceLock
  /// the resource will be loaded. If it is not possible to load the resource it will change to a 'missing' state. If the code accessing the
  /// resource cannot handle that case, the application will 'terminate' (that means crash).
  template <typename ResourceType>
  static xiiTypedResourceHandle<ResourceType> LoadResource(xiiStringView sResourceID);

  /// Same as LoadResource(), but additionally allows to set a priority on the resource and a custom fallback resource for this
  /// instance.
  ///
  /// Pass in xiiResourcePriority::Unchanged, if you only want to specify a custom fallback resource.
  /// If a resource priority is specified, the target resource will get that priority.
  /// If a valid fallback resource is specified, the resource will store that as its instance specific fallback resource. This will be used
  /// when trying to acquire the resource later.
  template <typename ResourceType>
  static xiiTypedResourceHandle<ResourceType> LoadResource(xiiStringView sResourceID, xiiTypedResourceHandle<ResourceType> hLoadingFallback);


  /// Same as LoadResource(), but instead of a template argument, the resource type to use is given as xiiRTTI info. Returns a
  /// typeless handle due to the missing template argument.
  static xiiTypelessResourceHandle LoadResourceByType(const xiiRTTI* pResourceType, xiiStringView sResourceID);

  /// Checks whether any resource loading is in progress
  static bool IsAnyLoadingInProgress();

  /// Generates a unique resource ID with the given prefix.
  ///
  /// Provide a prefix that is preferably not used anywhere else (i.e., closely related to your code).
  /// If the prefix is not also used to manually generate resource IDs, this function is guaranteed to return a unique resource ID.
  static xiiString GenerateUniqueResourceID(xiiStringView sResourceIDPrefix);

  /// Creates a resource from a descriptor.
  ///
  /// \param sResourceID The unique ID by which the resource is identified. E.g. in GetExistingResource()
  /// \param descriptor A type specific descriptor that holds all the information to create the resource.
  /// \param sResourceDescription An optional description that might help during debugging. Often a human readable name or path is stored
  /// here, to make it easier to identify this resource.
  template <typename ResourceType, typename DescriptorType>
  static xiiTypedResourceHandle<ResourceType> CreateResource(xiiStringView sResourceID, DescriptorType&& descriptor, xiiStringView sResourceDescription = nullptr);

  /// Returns a handle to the resource with the given ID if it exists or creates it from a descriptor.
  ///
  /// \param sResourceID The unique ID by which the resource is identified. E.g. in GetExistingResource()
  /// \param descriptor A type specific descriptor that holds all the information to create the resource.
  /// \param sResourceDescription An optional description that might help during debugging. Often a human readable name or path is stored here, to make it easier to identify this resource.
  template <typename ResourceType, typename DescriptorType>
  static xiiTypedResourceHandle<ResourceType> GetOrCreateResource(xiiStringView sResourceID, DescriptorType&& descriptor, xiiStringView sResourceDescription = nullptr);

  /// Returns a handle to the resource with the given ID. If the resource does not exist, the handle is invalid.
  ///
  /// Use this if a resource needs to be created procedurally (with CreateResource()), but might already have been created.
  /// If the returned handle is invalid, then just go through the resource creation step.
  template <typename ResourceType>
  static xiiTypedResourceHandle<ResourceType> GetExistingResource(xiiStringView sResourceID);

  /// Same as GetExistingResourceByType() but allows to specify the resource type as a xiiRTTI.
  static xiiTypelessResourceHandle GetExistingResourceByType(const xiiRTTI* pResourceType, xiiStringView sResourceID);

  template <typename ResourceType>
  static xiiTypedResourceHandle<ResourceType> GetExistingResourceOrCreateAsync(xiiStringView sResourceID, xiiUniquePtr<xiiResourceTypeLoader>&& pLoader, xiiTypedResourceHandle<ResourceType> hLoadingFallback = {})
  {
    xiiTypelessResourceHandle hTypeless = GetExistingResourceOrCreateAsync(xiiGetStaticRTTI<ResourceType>(), sResourceID, std::move(pLoader));

    auto hTyped = xiiTypedResourceHandle<ResourceType>((ResourceType*)hTypeless.m_pResource);

    if (hLoadingFallback.IsValid())
    {
      ((ResourceType*)hTypeless.m_pResource)->SetLoadingFallbackResource(hLoadingFallback);
    }

    return hTyped;
  }

  static xiiTypelessResourceHandle GetExistingResourceOrCreateAsync(const xiiRTTI* pResourceType, xiiStringView sResourceID, xiiUniquePtr<xiiResourceTypeLoader>&& pLoader);

  /// Triggers loading of the given resource. tShouldBeAvailableIn specifies how long the resource is not yet needed, thus allowing
  /// other resources to be loaded first. This is only a hint and there are no guarantees when the resource is available.
  static void PreloadResource(const xiiTypelessResourceHandle& hResource);

  /// Similar to locking a resource with 'BlockTillLoaded' acquire mode, but can be done with a typeless handle and does not return a result.
  static void ForceLoadResourceNow(const xiiTypelessResourceHandle& hResource);

  /// Returns the current loading state of the given resource.
  static xiiResourceState GetLoadingState(const xiiTypelessResourceHandle& hResource);

  ///@}
  /// \name Reloading resources
  ///@{

public:
  /// Goes through all resources and makes sure they are reloaded, if they have changed. If bForce is true, all resources
  /// are updated, even if there is no indication that they have changed.
  static xiiUInt32 ReloadAllResources(bool bForce);

  /// Goes through all resources of the given type and makes sure they are reloaded, if they have changed. If bForce is true,
  /// resources are updated, even if there is no indication that they have changed.
  template <typename ResourceType>
  static xiiUInt32 ReloadResourcesOfType(bool bForce);

  /// Goes through all resources of the given type and makes sure they are reloaded, if they have changed. If bForce is true,
  /// resources are updated, even if there is no indication that they have changed.
  static xiiUInt32 ReloadResourcesOfType(const xiiRTTI* pType, bool bForce);

  /// Reloads only the one specific resource. If bForce is true, it is updated, even if there is no indication that it has changed.
  template <typename ResourceType>
  static bool ReloadResource(const xiiTypedResourceHandle<ResourceType>& hResource, bool bForce);

  /// Reloads only the one specific resource. If bForce is true, it is updated, even if there is no indication that it has changed.
  static bool ReloadResource(const xiiRTTI* pType, const xiiTypelessResourceHandle& hResource, bool bForce);


  /// Calls ReloadResource() on the given resource, but makes sure that the reload happens with the given custom loader.
  ///
  /// Use this e.g. with a xiiResourceLoaderFromMemory to replace an existing resource with new data that was created on-the-fly.
  /// Using this function will set the 'PreventFileReload' flag on the resource and thus prevent further reload actions.
  ///
  /// \sa RestoreResource()
  static void UpdateResourceWithCustomLoader(const xiiTypelessResourceHandle& hResource, xiiUniquePtr<xiiResourceTypeLoader>&& pLoader);

  /// Removes the 'PreventFileReload' flag and forces a reload on the resource.
  ///
  /// \sa UpdateResourceWithCustomLoader()
  static void RestoreResource(const xiiTypelessResourceHandle& hResource);

  ///@}
  /// \name Acquiring resources
  ///@{

public:
  /// Acquires a resource pointer from a handle. Prefer to use xiiResourceLock, which wraps BeginAcquireResource / EndAcquireResource
  ///
  /// \param hResource The resource to acquire
  /// \param mode The desired way to acquire the resource. See xiiResourceAcquireMode for details.
  /// \param hLoadingFallback A custom fallback resource that should be returned if hResource is not yet available. Allows to use domain specific knowledge to get a better fallback.
  /// \param Priority Allows to adjust the priority of the resource. This will affect how fast the resource is loaded, in case it is not yet available.
  /// \param out_AcquireResult Returns how successful the acquisition was. See xiiResourceAcquireResult for details.
  template <typename ResourceType>
  static ResourceType* BeginAcquireResource(const xiiTypedResourceHandle<ResourceType>& hResource, xiiResourceAcquireMode mode, const xiiTypedResourceHandle<ResourceType>& hLoadingFallback = xiiTypedResourceHandle<ResourceType>(), xiiResourceAcquireResult* out_pAcquireResult = nullptr);

  /// Same as BeginAcquireResource but only for the base resource pointer.
  static xiiResource* BeginAcquireResourcePointer(const xiiRTTI* pType, const xiiTypelessResourceHandle& hResource);

  /// Needs to be called in concert with BeginAcquireResource() after accessing a resource has been finished. Prefer to use
  /// xiiResourceLock instead.
  template <typename ResourceType>
  static void EndAcquireResource(ResourceType* pResource);

  /// Same as EndAcquireResource but without the template parameter. See also BeginAcquireResourcePointer.
  static void EndAcquireResourcePointer(xiiResource* pResource);

  /// Forces the resource manager to treat xiiResourceAcquireMode::AllowLoadingFallback as xiiResourceAcquireMode::BlockTillLoaded on
  /// BeginAcquireResource.
  static void ForceNoFallbackAcquisition(xiiUInt32 uiNumFrames = 0xFFFFFFFFU);

  /// If the returned number is greater 0 the resource manager treats xiiResourceAcquireMode::AllowLoadingFallback as
  /// xiiResourceAcquireMode::BlockTillLoaded on BeginAcquireResource.
  static xiiUInt32 GetForceNoFallbackAcquisition();

  /// Retrieves an array of pointers to resources of the indicated type which
  /// are loaded at the moment. Destroy the returned object as soon as possible as it
  /// holds the entire resource manager locked.
  template <typename ResourceType>
  static xiiLockedObject<xiiMutex, xiiDynamicArray<xiiResource*>> GetAllResourcesOfType();

  ///@}
  /// \name Unloading resources
  ///@{

public:
  /// Deallocates all resources whose refcount has reached 0. Returns the number of deleted resources.
  static xiiUInt32 FreeAllUnusedResources();

  /// Deallocates resources whose refcount has reached 0. Returns the number of deleted resources.
  static xiiUInt32 FreeUnusedResources(xiiTime timeout, xiiTime lastAcquireThreshold);

  /// If timeout is not zero, FreeUnusedResources() is called once every frame with the given parameters.
  static void SetAutoFreeUnused(xiiTime timeout, xiiTime lastAcquireThreshold);

  /// If set to 'false' resources of the given type will not be incrementally unloaded in the background, when they are not referenced anymore.
  template <typename ResourceType>
  static void SetIncrementalUnloadForResourceType(bool bActive);

  template <typename TypeBeingUpdated, typename TypeItWantsToAcquire>
  static void AllowResourceTypeAcquireDuringUpdateContent()
  {
    AllowResourceTypeAcquireDuringUpdateContent(xiiGetStaticRTTI<TypeBeingUpdated>(), xiiGetStaticRTTI<TypeItWantsToAcquire>());
  }

  static void AllowResourceTypeAcquireDuringUpdateContent(const xiiRTTI* pTypeBeingUpdated, const xiiRTTI* pTypeItWantsToAcquire);

  static bool IsResourceTypeAcquireDuringUpdateContentAllowed(const xiiRTTI* pTypeBeingUpdated, const xiiRTTI* pTypeItWantsToAcquire);

private:
  static xiiResult DeallocateResource(xiiResource* pResource);

  ///@}
  /// \name Miscellaneous
  ///@{

public:
  /// Returns the resource manager mutex. Allows to lock the manager on a thread when multiple operations need to be done in
  /// sequence.
  static xiiMutex& GetMutex() { return s_ResourceMutex; }

  /// Must be called once per frame for some bookkeeping.
  static void PerFrameUpdate();

  /// Makes sure that no further resource loading will take place.
  static void EngineAboutToShutdown();

  /// Calls xiiResource::ResetResource() on all resources.
  ///
  /// This is mostly for usage in tools to reset resource whose state can be modified at runtime, to reset them to their original state.
  static void ResetAllResources();

  /// Calls xiiResource::UpdateContent() to fill the resource with 'low resolution' data
  ///
  /// This will early out, if the resource has gotten low-res data before.
  /// The resource itself may ignore the data, if it has already gotten low/high res data before.
  ///
  /// The typical use case is, that some other piece of code stores a low-res version of a resource to be able to get
  /// a resource into a usable state. For instance, a material may store low resolution texture data for every texture that it references.
  /// Then when 'loading' the textures, it can pass this low-res data to the textures, such that rendering can give decent results right
  /// away. If the textures have already been loaded before, or some other material already had low-res data, the call exits quickly.
  static void SetResourceLowResData(const xiiTypelessResourceHandle& hResource, xiiStreamReader* pStream);

  ///@}
  /// \name Type specific loaders
  ///@{

public:
  /// Sets the resource loader to use when no type specific resource loader is available.
  static void SetDefaultResourceLoader(xiiResourceTypeLoader* pDefaultLoader);

  /// Returns the resource loader to use when no type specific resource loader is available.
  static xiiResourceTypeLoader* GetDefaultResourceLoader();

  /// Sets the resource loader to use for the given resource type.
  ///
  /// \note This is bound to one specific type. Derived types do not inherit the type loader.
  template <typename ResourceType>
  static void SetResourceTypeLoader(xiiResourceTypeLoader* pCreator);

  ///@}
  /// \name Named resources
  ///@{

public:
  /// Registers a 'named' resource. When a resource is looked up using \a sLookupName, the lookup will be redirected to \a
  /// sRedirectionResource.
  ///
  /// This can be used to register a resource under an easier to use name. For example one can register "MenuBackground" as the name for "{
  /// E50DCC85-D375-4999-9CFE-42F1377FAC85 }". If the lookup name already exists, it will be overwritten.
  static void RegisterNamedResource(xiiStringView sLookupName, xiiStringView sRedirectionResource);

  /// Removes a previously registered name from the redirection table.
  static void UnregisterNamedResource(xiiStringView sLookupName);


  ///@}
  /// \name Asset system interaction
  ///@{

public:
  /// Registers which resource type to use to load an asset with the given type name
  static void RegisterResourceForAssetType(xiiStringView sAssetTypeName, const xiiRTTI* pResourceType);

  /// Returns the resource type that was registered to handle the given asset type for loading. nullptr if no resource type was
  /// registered for this asset type.
  static const xiiRTTI* FindResourceForAssetType(xiiStringView sAssetTypeName);

  ///@}
  /// \name Export mode
  ///@{

public:
  /// Enables export mode. In this mode the resource manager will assert when it actually tries to load a resource.
  /// This can be useful when exporting resource handles but the actual resource content is not needed.
  static void EnableExportMode(bool bEnable);

  /// Returns whether export mode is active.
  static bool IsExportModeEnabled();

  /// Creates a resource handle for the given resource ID. This method can only be used if export mode is enabled.
  /// Internally it will create a resource but does not load the content. This way it can be ensured that the resource handle is always only
  /// the size of a pointer.
  template <typename ResourceType>
  static xiiTypedResourceHandle<ResourceType> GetResourceHandleForExport(xiiStringView sResourceID);


  ///@}
  /// \name Resource Type Overrides
  ///@{

public:
  /// Registers a resource type to be used instead of any of it's base classes, when loading specific data
  ///
  /// When resource B is derived from A it can be registered to be instantiated when loading data, even if the code specifies to use a
  /// resource of type A.
  /// Whenever LoadResource<A>() is executed, the registered callback \a OverrideDecider is run to figure out whether B should be
  /// instantiated instead. If OverrideDecider returns true, B is used.
  ///
  /// OverrideDecider is given the resource ID after it has been resolved by the xiiFileSystem. So it has to be able to make its decision
  /// from the file path, name or extension.
  /// The override is registered for all base classes of \a pDerivedTypeToUse, in case the derivation hierarchy is longer.
  ///
  /// Without calling this at startup, a derived resource type has to be manually requested in code.
  static void RegisterResourceOverrideType(const xiiRTTI* pDerivedTypeToUse, xiiDelegate<bool(const xiiStringBuilder&)> overrideDecider);

  /// Unregisters \a pDerivedTypeToUse as an override resource
  ///
  /// \sa RegisterResourceOverrideType()
  static void UnregisterResourceOverrideType(const xiiRTTI* pDerivedTypeToUse);

  ///@}
  /// \name Resource Fallbacks
  ///@{

public:
  /// Specifies which resource to use as a loading fallback for the given type, while a resource is not yet loaded.
  template <typename RESOURCE_TYPE>
  static void SetResourceTypeLoadingFallback(const xiiTypedResourceHandle<RESOURCE_TYPE>& hResource)
  {
    RESOURCE_TYPE::SetResourceTypeLoadingFallback(hResource);
  }

  /// \sa SetResourceTypeLoadingFallback()
  template <typename RESOURCE_TYPE>
  static inline const xiiTypedResourceHandle<RESOURCE_TYPE>& GetResourceTypeLoadingFallback()
  {
    return RESOURCE_TYPE::GetResourceTypeLoadingFallback();
  }

  /// Specifies which resource to use as a missing fallback for the given type, when a resource cannot be loaded.
  ///
  /// \note If no missing fallback is specified, trying to load a resource that does not exist will assert at runtime.
  template <typename RESOURCE_TYPE>
  static void SetResourceTypeMissingFallback(const xiiTypedResourceHandle<RESOURCE_TYPE>& hResource)
  {
    RESOURCE_TYPE::SetResourceTypeMissingFallback(hResource);
  }

  /// \sa SetResourceTypeMissingFallback()
  template <typename RESOURCE_TYPE>
  static inline const xiiTypedResourceHandle<RESOURCE_TYPE>& GetResourceTypeMissingFallback()
  {
    return RESOURCE_TYPE::GetResourceTypeMissingFallback();
  }

  using ResourceCleanupCB = xiiDelegate<void()>;

  /// [internal] Used by xiiResource to register a cleanup function to be called at resource manager shutdown.
  static void AddResourceCleanupCallback(ResourceCleanupCB cb);

  /// \sa AddResourceCleanupCallback()
  static void ClearResourceCleanupCallback(ResourceCleanupCB cb);

  /// This will clear ALL resources that were registered as 'missing' or 'loading' fallback resources. This is called early during
  /// system shutdown to clean up resources.
  static void ExecuteAllResourceCleanupCallbacks();

  ///@}
  /// \name Resource Priorities
  ///@{

public:
  /// Specifies which resource to use as a loading fallback for the given type, while a resource is not yet loaded.
  template <typename RESOURCE_TYPE>
  static void SetResourceTypeDefaultPriority(xiiResourcePriority priority)
  {
    GetResourceTypePriorities()[xiiGetStaticRTTI<RESOURCE_TYPE>()] = priority;
  }

private:
  static xiiMap<const xiiRTTI*, xiiResourcePriority>& GetResourceTypePriorities();
  ///@}

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

private:
  friend class xiiResource;
  friend class xiiResourceManagerWorkerDataLoad;
  friend class xiiResourceManagerWorkerUpdateContent;
  friend class xiiResourceHandleReadContext;

  // Events
private:
  static void BroadcastResourceEvent(const xiiResourceEvent& e);

  // Miscellaneous
private:
  static xiiMutex s_ResourceMutex;

  // Startup / shutdown
private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, ResourceManager);
  static void OnEngineShutdown();
  static void OnCoreShutdown();
  static void OnCoreStartup();
  static void PluginEventHandler(const xiiPluginEvent& e);

  // Loading / reloading / creating resources
private:
  struct LoadedResources
  {
    xiiHashTable<xiiTempHashedString, xiiResource*> m_Resources;
  };

  struct LoadingInfo
  {
    float        m_fPriority = 0;
    xiiResource* m_pResource = nullptr;

    XII_ALWAYS_INLINE bool operator==(const LoadingInfo& rhs) const { return m_pResource == rhs.m_pResource; }
    XII_ALWAYS_INLINE bool operator<(const LoadingInfo& rhs) const { return m_fPriority < rhs.m_fPriority; }
  };
  static void EnsureResourceLoadingState(xiiResource* pResource, const xiiResourceState RequestedState);
  static void PreloadResource(xiiResource* pResource);
  static void InternalPreloadResource(xiiResource* pResource, bool bHighestPriority);

  template <typename ResourceType>
  static ResourceType* GetResource(xiiStringView sResourceID, bool bIsReloadable);
  static xiiResource*  GetResource(const xiiRTTI* pRtti, xiiStringView sResourceID, bool bIsReloadable);
  static void          RunWorkerTask();
  static void          UpdateLoadingDeadlines();
  static void          ReverseBubbleSortStep(xiiDeque<LoadingInfo>& data);
  static bool          ReloadResource(xiiResource* pResource, bool bForce);

  static void                                           SetupWorkerTasks();
  static xiiTime                                        GetLastFrameUpdate();
  static xiiHashTable<const xiiRTTI*, LoadedResources>& GetLoadedResources();
  static xiiDynamicArray<xiiResource*>&                 GetLoadedResourceOfTypeTempContainer();

  XII_ALWAYS_INLINE static bool  IsQueuedForLoading(xiiResource* pResource) { return pResource->m_Flags.IsSet(xiiResourceFlags::IsQueuedForLoading); }
  [[nodiscard]] static xiiResult RemoveFromLoadingQueue(xiiResource* pResource);
  static void                    AddToLoadingQueue(xiiResource* pResource, bool bHighPriority);

  struct ResourceTypeInfo
  {
    bool m_bIncrementalUnload        = true;
    bool m_bAllowNestedAcquireCached = false;

    xiiHybridArray<const xiiRTTI*, 8> m_NestedTypes;
  };

  static ResourceTypeInfo& GetResourceTypeInfo(const xiiRTTI* pRtti);

  // Type loaders
private:
  static xiiResourceTypeLoader* GetResourceTypeLoader(const xiiRTTI* pRTTI);

  static xiiMap<const xiiRTTI*, xiiResourceTypeLoader*>& GetResourceTypeLoaders();

  // Override / derived resources
private:
  struct DerivedTypeInfo
  {
    const xiiRTTI*                             m_pDerivedType = nullptr;
    xiiDelegate<bool(const xiiStringBuilder&)> m_Decider;
  };

  /// Checks whether there is a type override for pRtti given sResourceID and returns that
  static const xiiRTTI* FindResourceTypeOverride(const xiiRTTI* pRtti, xiiStringView sResourceID);
};

#include <Core/ResourceManager/Implementation/ResourceHandleReflection.h>
#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/ResourceManager/Implementation/ResourceManager_inl.h>
