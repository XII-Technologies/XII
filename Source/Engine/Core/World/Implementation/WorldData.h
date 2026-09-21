/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Communication/MessageQueue.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Types/SharedPtr.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/GameObject.h>
#include <Core/World/WorldDesc.h>

namespace xiiInternal
{
  class XII_CORE_DLL WorldData
  {
  private:
    friend class ::xiiWorld;
    friend class ::xiiComponentManagerBase;

    WorldData(xiiWorldDescription& desc);
    ~WorldData();

    void Clear();

    xiiHashedString                       m_sName;
    mutable xiiProxyAllocator             m_Allocator;
    xiiLocalAllocatorWrapper              m_AllocatorWrapper;
    xiiInternal::WorldLargeBlockAllocator m_BlockAllocator;
    xiiDoubleBufferedLinearAllocator      m_LinearAllocator;

    enum
    {
      GAME_OBJECTS_PER_BLOCK        = xiiDataBlock<xiiGameObject, xiiInternal::DEFAULT_BLOCK_SIZE>::CAPACITY,
      TRANSFORMATION_DATA_PER_BLOCK = xiiDataBlock<xiiGameObject::TransformationData, xiiInternal::DEFAULT_BLOCK_SIZE>::CAPACITY
    };

    // Object Storage
    using ObjectStorage = xiiBlockStorage<xiiGameObject, xiiInternal::DEFAULT_BLOCK_SIZE, xiiBlockStorageType::Compact>;
    xiiIdTable<xiiGameObjectId, xiiGameObject*, xiiLocalAllocatorWrapper> m_Objects;
    ObjectStorage                                                         m_ObjectStorage;

    xiiSet<xiiGameObject*, xiiCompareHelper<xiiGameObject*>, xiiLocalAllocatorWrapper> m_DeadObjects;
    xiiEvent<const xiiGameObject*>                                                     m_ObjectDeletionEvent;

  public:
    class XII_CORE_DLL ConstObjectIterator
    {
    public:
      const xiiGameObject& operator*() const;
      const xiiGameObject* operator->() const;

      operator const xiiGameObject*() const;

      /// Advances the iterator to the next object. The iterator will not be valid anymore, if the last object is reached.
      void Next();

      /// Checks whether this iterator points to a valid object.
      bool IsValid() const;

      /// Shorthand for 'Next'
      void operator++();

    private:
      friend class ::xiiWorld;

      ConstObjectIterator(ObjectStorage::ConstIterator iterator);

      ObjectStorage::ConstIterator m_Iterator;
    };

    class XII_CORE_DLL ObjectIterator
    {
    public:
      xiiGameObject& operator*();
      xiiGameObject* operator->();

      operator xiiGameObject*();

      /// Advances the iterator to the next object. The iterator will not be valid anymore, if the last object is reached.
      void Next();

      /// Checks whether this iterator points to a valid object.
      bool IsValid() const;

      /// Shorthand for 'Next'
      void operator++();

    private:
      friend class ::xiiWorld;

      ObjectIterator(ObjectStorage::Iterator iterator);

      ObjectStorage::Iterator m_Iterator;
    };

  private:
    // hierarchy structures
    struct Hierarchy
    {
      using DataBlock      = xiiDataBlock<xiiGameObject::TransformationData, xiiInternal::DEFAULT_BLOCK_SIZE>;
      using DataBlockArray = xiiDynamicArray<DataBlock>;

      xiiHybridArray<DataBlockArray*, 8, xiiLocalAllocatorWrapper> m_Data;
    };

    struct HierarchyType
    {
      enum Enum
      {
        Static,
        Dynamic,
        COUNT
      };
    };

    Hierarchy m_Hierarchies[HierarchyType::COUNT];

    static HierarchyType::Enum GetHierarchyType(bool bDynamic);

    xiiGameObject::TransformationData* CreateTransformationData(bool bDynamic, xiiUInt32 uiHierarchyLevel);

    void DeleteTransformationData(bool bDynamic, xiiUInt32 uiHierarchyLevel, xiiGameObject::TransformationData* pData);

    template <typename VISITOR>
    static xiiVisitorExecution::Enum TraverseHierarchyLevel(Hierarchy::DataBlockArray& blocks, void* pUserData = nullptr);
    template <typename VISITOR>
    xiiVisitorExecution::Enum TraverseHierarchyLevelMultiThreaded(Hierarchy::DataBlockArray& blocks, void* pUserData = nullptr);

    using VisitorFunc = xiiDelegate<xiiVisitorExecution::Enum(xiiGameObject*)>;
    void                             TraverseBreadthFirst(VisitorFunc& func);
    void                             TraverseDepthFirst(VisitorFunc& func);
    static xiiVisitorExecution::Enum TraverseObjectDepthFirst(xiiGameObject* pObject, VisitorFunc& func);

    static void UpdateGlobalTransform(xiiGameObject::TransformationData* pData, xiiUInt32 uiUpdateCounter);
    static void UpdateGlobalTransformWithParent(xiiGameObject::TransformationData* pData, xiiUInt32 uiUpdateCounter);

    static void UpdateGlobalTransformAndSpatialData(xiiGameObject::TransformationData* pData, xiiUInt32 uiUpdateCounter, xiiSpatialSystem& spatialSystem);
    static void UpdateGlobalTransformWithParentAndSpatialData(xiiGameObject::TransformationData* pData, xiiUInt32 uiUpdateCounter, xiiSpatialSystem& spatialSystem);

    void UpdateGlobalTransforms();

    void ResourceEventHandler(const xiiResourceEvent& e);

    // Game Object Lookups.
    xiiHashTable<xiiUInt64, xiiGameObjectId, xiiHashHelper<xiiUInt64>, xiiLocalAllocatorWrapper> m_GlobalKeyToIdTable;
    xiiHashTable<xiiUInt64, xiiHashedString, xiiHashHelper<xiiUInt64>, xiiLocalAllocatorWrapper> m_IdToGlobalKeyTable;

    // Modules.
    xiiDynamicArray<xiiWorldModule*, xiiLocalAllocatorWrapper> m_Modules;
    xiiDynamicArray<xiiWorldModule*, xiiLocalAllocatorWrapper> m_ModulesToStartSimulation;

    // Component Management.
    xiiSet<xiiComponent*, xiiCompareHelper<xiiComponent*>, xiiLocalAllocatorWrapper> m_DeadComponents;

    struct InitBatch
    {
      InitBatch(xiiAllocator* pAllocator, xiiStringView sName, bool bMustFinishWithinOneFrame);

      xiiHashedString m_sName;
      bool            m_bMustFinishWithinOneFrame = true;
      bool            m_bIsReady                  = false;

      xiiUInt32                           m_uiNextComponentToInitialize      = 0;
      xiiUInt32                           m_uiNextComponentToStartSimulation = 0;
      xiiDynamicArray<xiiComponentHandle> m_ComponentsToInitialize;
      xiiDynamicArray<xiiComponentHandle> m_ComponentsToStartSimulation;
    };

    xiiTime                                                                                m_MaxInitializationTimePerFrame;
    xiiIdTable<xiiComponentInitBatchId, xiiUniquePtr<InitBatch>, xiiLocalAllocatorWrapper> m_InitBatches;
    InitBatch*                                                                             m_pDefaultInitBatch = nullptr;
    InitBatch*                                                                             m_pCurrentInitBatch = nullptr;

    struct RegisteredUpdateFunction
    {
      xiiWorldModule::UpdateFunction m_Function;
      xiiHashedString                m_sFunctionName;
      float                          m_fPriority;
      xiiUInt16                      m_uiAsyncPhaseBatchSize;
      bool                           m_bOnlyUpdateWhenSimulating;

      void FillFromDesc(const xiiWorldModule::UpdateFunctionDesc& desc);
      bool operator<(const RegisteredUpdateFunction& other) const;
    };

    struct UpdateTask final : public xiiTask
    {
      virtual void Execute() override;

      xiiWorldModule::UpdateFunction m_Function;
      xiiUInt32                      m_uiStartIndex;
      xiiUInt32                      m_uiCount;
    };

    xiiDynamicArray<RegisteredUpdateFunction, xiiLocalAllocatorWrapper>           m_UpdateFunctions[xiiWorldUpdatePhase::COUNT];
    xiiDynamicArray<xiiWorldModule::UpdateFunctionDesc, xiiLocalAllocatorWrapper> m_UpdateFunctionsToRegister;

    xiiDynamicArray<xiiSharedPtr<UpdateTask>, xiiLocalAllocatorWrapper> m_UpdateTasks;

    xiiUniquePtr<xiiSpatialSystem>            m_pSpatialSystem;
    xiiSharedPtr<xiiCoordinateSystemProvider> m_pCoordinateSystemProvider;
    xiiUniquePtr<xiiTimeStepSmoothing>        m_pTimeStepSmoothing;

    xiiClock  m_Clock;
    xiiRandom m_Random;

    struct QueuedMsgMetaData
    {
      XII_DECLARE_POD_TYPE();

      XII_ALWAYS_INLINE QueuedMsgMetaData() :
        m_uiReceiverData(0)
      {
      }

      union
      {
        struct
        {
          xiiUInt64 m_uiReceiverObjectOrComponent : 62;
          xiiUInt64 m_uiReceiverIsComponent : 1;
          xiiUInt64 m_uiRecursive : 1;
        };

        xiiUInt64 m_uiReceiverData;
      };

      xiiTime m_Due;
    };

    using MessageQueue = xiiMessageQueue<QueuedMsgMetaData, xiiLocalAllocatorWrapper>;
    mutable MessageQueue        m_MessageQueues[xiiObjectMsgQueueType::COUNT];
    mutable MessageQueue        m_TimedMessageQueues[xiiObjectMsgQueueType::COUNT];
    xiiObjectMsgQueueType::Enum m_ProcessingMessageQueue = xiiObjectMsgQueueType::COUNT;

    xiiThreadID                m_WriteThreadID;
    xiiInt32                   m_iWriteCounter = 0;
    mutable xiiAtomicInteger32 m_iReadCounter;

    xiiUInt32 m_uiUpdateCounter                   = 0;
    bool      m_bSimulateWorld                    = true;
    bool      m_bReportErrorWhenStaticObjectMoves = true;

    /// Maps some data (given as void*) to a xiiGameObjectHandle. Only available in special situations (e.g. Editor use cases).
    xiiDelegate<xiiGameObjectHandle(const void*, xiiComponentHandle, xiiStringView)> m_GameObjectReferenceResolver;

    struct ResourceReloadContext
    {
      xiiWorld*     m_pWorld     = nullptr;
      xiiComponent* m_pComponent = nullptr;
      void*         m_pUserData  = nullptr;
    };

    using ResourceReloadFunc = xiiDelegate<void(ResourceReloadContext&)>;

    struct ResourceReloadFunctionData
    {
      xiiComponentHandle m_hComponent;
      void*              m_pUserData = nullptr;
      ResourceReloadFunc m_Func;
    };

    using ReloadFunctionList = xiiHybridArray<ResourceReloadFunctionData, 8>;
    xiiHashTable<xiiTypelessResourceHandle, ReloadFunctionList> m_ReloadFunctions;
    xiiHashSet<xiiTypelessResourceHandle>                       m_NeedReload;
    ReloadFunctionList                                          m_TempReloadFunctions;

  public:
    class ReadMarker
    {
    public:
      void Lock();
      void Unlock();

    private:
      friend class ::xiiInternal::WorldData;

      ReadMarker(const WorldData& data);
      const WorldData& m_Data;
    };

    class WriteMarker
    {
    public:
      void Lock();
      void Unlock();

    private:
      friend class ::xiiInternal::WorldData;

      WriteMarker(WorldData& data);
      WorldData& m_Data;
    };

  private:
    mutable ReadMarker m_ReadMarker;
    WriteMarker        m_WriteMarker;

    void* m_pUserData = nullptr;
  };
} // namespace xiiInternal

#include <Core/World/Implementation/WorldData_inl.h>
