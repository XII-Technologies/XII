/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Types/ScopeExit.h>

XII_CREATE_SIMPLE_TEST_GROUP(ResourceManager);

namespace
{
  using TestResourceHandle = xiiTypedResourceHandle<class TestResource>;

  class TestResource : public xiiResource
  {
    XII_ADD_DYNAMIC_REFLECTION(TestResource, xiiResource);
    XII_RESOURCE_DECLARE_COMMON_CODE(TestResource);

  public:
    TestResource() :
      xiiResource(xiiResource::DoUpdate::OnAnyThread, 1)
    {
    }

  protected:
    virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override
    {
      xiiResourceLoadDescription ld;
      ld.m_State                      = xiiResourceState::Unloaded;
      ld.m_uiQualityLevelsDiscardable = 0;
      ld.m_uiQualityLevelsLoadable    = 0;

      return ld;
    }

    virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* Stream) override
    {
      xiiResourceLoadDescription ld;
      ld.m_State                      = xiiResourceState::Loaded;
      ld.m_uiQualityLevelsDiscardable = 0;
      ld.m_uiQualityLevelsLoadable    = 0;

      xiiStreamReader& s = *Stream;

      xiiUInt32 uiNumElements = 0;
      s >> uiNumElements;

      if (GetResourceID().StartsWith("NonBlockingLevel1-"))
      {
        m_hNested = xiiResourceManager::LoadResource<TestResource>("Level0-0");
      }

      if (GetResourceID().StartsWith("BlockingLevel1-"))
      {
        m_hNested = xiiResourceManager::LoadResource<TestResource>("Level0-0");

        xiiResourceLock<TestResource> pTestResource(m_hNested, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

        XII_ASSERT_ALWAYS(pTestResource.GetAcquireResult() == xiiResourceAcquireResult::Final, "");
      }

      m_Data.SetCountUninitialized(uiNumElements);

      for (xiiUInt32 i = 0; i < uiNumElements; ++i)
      {
        s >> m_Data[i];
      }

      return ld;
    }

    virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override
    {
      out_NewMemoryUsage.m_uiMemoryCPU = sizeof(TestResource);
      out_NewMemoryUsage.m_uiMemoryGPU = 0;
    }

  public:
    void Test() { XII_TEST_BOOL(!m_Data.IsEmpty()); }

  private:
    TestResourceHandle         m_hNested;
    xiiDynamicArray<xiiUInt32> m_Data;
  };

  class TestResourceTypeLoader : public xiiResourceTypeLoader
  {
  public:
    struct LoadedData
    {
      xiiDefaultMemoryStreamStorage m_StreamData;
      xiiMemoryStreamReader         m_Reader;
    };

    virtual xiiResourceLoadData OpenDataStream(const xiiResource* pResource) override
    {
      LoadedData* pData = XII_DEFAULT_NEW(LoadedData);

      const xiiUInt32 uiNumElements = 1024 * 10;
      pData->m_StreamData.Reserve(uiNumElements * sizeof(xiiUInt32) + 1);

      xiiMemoryStreamWriter writer(&pData->m_StreamData);
      pData->m_Reader.SetStorage(&pData->m_StreamData);

      writer << uiNumElements;

      for (xiiUInt32 i = 0; i < uiNumElements; ++i)
      {
        writer << i;
      }

      xiiResourceLoadData ld;
      ld.m_pCustomLoaderData    = pData;
      ld.m_pDataStream          = &pData->m_Reader;
      ld.m_sResourceDescription = pResource->GetResourceID();

      return ld;
    }

    virtual void CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& loaderData) override
    {
      LoadedData* pData = static_cast<LoadedData*>(loaderData.m_pCustomLoaderData);
      XII_DEFAULT_DELETE(pData);
    }
  };

  XII_RESOURCE_IMPLEMENT_COMMON_CODE(TestResource);
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(TestResource, 1, xiiRTTIDefaultAllocator<TestResource>)
  XII_END_DYNAMIC_REFLECTED_TYPE;

} // namespace

XII_CREATE_SIMPLE_TEST(ResourceManager, Basics)
{
  TestResourceTypeLoader TypeLoader;
  xiiResourceManager::AllowResourceTypeAcquireDuringUpdateContent<TestResource, TestResource>();
  xiiResourceManager::SetResourceTypeLoader<TestResource>(&TypeLoader);
  XII_SCOPE_EXIT(xiiResourceManager::SetResourceTypeLoader<TestResource>(nullptr));

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Main")
  {
    XII_TEST_INT(xiiResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const xiiUInt32 uiNumResources = 200;

    xiiDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    xiiStringBuilder sResourceID;
    for (xiiUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.SetFormat("Level0-{}", i);
      hResources.PushBack(xiiResourceManager::LoadResource<TestResource>(sResourceID));
    }

    XII_TEST_INT(xiiResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (xiiUInt32 i = 0; i < uiNumResources; ++i)
    {
      xiiResourceManager::PreloadResource(hResources[i]);
    }

    XII_TEST_INT(xiiResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (xiiUInt32 i = 0; i < uiNumResources; ++i)
    {
      xiiResourceLock<TestResource> pTestResource(hResources[i], xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

      XII_TEST_BOOL(pTestResource.GetAcquireResult() == xiiResourceAcquireResult::Final);

      pTestResource->Test();
    }

    XII_TEST_INT(xiiResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    hResources.Clear();

    xiiUInt32 uiUnloaded = 0;

    for (xiiUInt32 tries = 0; tries < 3; ++tries)
    {
      // if a resource is in a loading queue, unloading it can actually 'fail' for a short time
      uiUnloaded += xiiResourceManager::FreeAllUnusedResources();

      if (uiUnloaded == uiNumResources)
        break;

      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(100));
    }

    XII_TEST_INT(xiiResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }
}

XII_CREATE_SIMPLE_TEST(ResourceManager, NestedLoading)
{
  TestResourceTypeLoader TypeLoader;
  xiiResourceManager::AllowResourceTypeAcquireDuringUpdateContent<TestResource, TestResource>();
  xiiResourceManager::SetResourceTypeLoader<TestResource>(&TypeLoader);
  XII_SCOPE_EXIT(xiiResourceManager::SetResourceTypeLoader<TestResource>(nullptr));

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "NonBlocking")
  {
    XII_TEST_INT(xiiResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const xiiUInt32 uiNumResources = 200;

    xiiDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    xiiStringBuilder sResourceID;
    for (xiiUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.SetFormat("NonBlockingLevel1-{}", i);
      hResources.PushBack(xiiResourceManager::LoadResource<TestResource>(sResourceID));
    }

    XII_TEST_INT(xiiResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (xiiUInt32 i = 0; i < uiNumResources; ++i)
    {
      xiiResourceManager::PreloadResource(hResources[i]);
    }

    for (xiiUInt32 i = 0; i < uiNumResources; ++i)
    {
      xiiResourceLock<TestResource> pTestResource(hResources[i], xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

      XII_TEST_BOOL(pTestResource.GetAcquireResult() == xiiResourceAcquireResult::Final);

      pTestResource->Test();
    }

    XII_TEST_INT(xiiResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources + 1);

    hResources.Clear();

    xiiResourceManager::FreeAllUnusedResources();
    XII_TEST_INT(xiiResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }

  // Test disabled as it deadlocks
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Blocking")
  {
    XII_TEST_INT(xiiResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const xiiUInt32 uiNumResources = 500;

    xiiDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    xiiStringBuilder sResourceID;
    for (xiiUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.SetFormat("BlockingLevel1-{}", i);
      hResources.PushBack(xiiResourceManager::LoadResource<TestResource>(sResourceID));
    }

    XII_TEST_INT(xiiResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (xiiUInt32 i = 0; i < uiNumResources; ++i)
    {
      xiiResourceManager::PreloadResource(hResources[i]);
    }

    for (xiiUInt32 i = 0; i < uiNumResources; ++i)
    {
      xiiResourceLock<TestResource> pTestResource(hResources[i], xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

      XII_TEST_BOOL(pTestResource.GetAcquireResult() == xiiResourceAcquireResult::Final);

      pTestResource->Test();
    }

    XII_TEST_INT(xiiResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources + 1);

    hResources.Clear();

    while (xiiResourceManager::IsAnyLoadingInProgress())
    {
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(100));
    }

    xiiResourceManager::FreeAllUnusedResources();
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(100));
    xiiResourceManager::FreeAllUnusedResources();

    XII_TEST_INT(xiiResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }
}
