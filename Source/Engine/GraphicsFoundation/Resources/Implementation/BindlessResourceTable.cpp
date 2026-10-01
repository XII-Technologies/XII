/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Lock.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>

class xiiGALBindlessResourceTable::State
{
public:
  template <typename TObject>
  struct TableStorage
  {
    xiiGALBindlessResourceAllocator        m_Allocator;
    xiiDynamicArray<xiiSharedPtr<TObject>> m_Objects;
    xiiDynamicArray<xiiUInt64>             m_RetireFences;
  };

  xiiMutex                                  m_Mutex;
  TableStorage<xiiGALBufferView>            m_BufferSRVs;
  TableStorage<xiiGALBufferView>            m_BufferUAVs;
  TableStorage<xiiGALTextureView>           m_TextureSRVs;
  TableStorage<xiiGALTextureView>           m_TextureUAVs;
  TableStorage<xiiGALSampler>               m_Samplers;
  xiiGALBindlessResourceTableDescription    m_Description;
  bool                                      m_bInitialized = false;
};

xiiUniquePtr<xiiGALBindlessResourceTable::State> xiiGALBindlessResourceTable::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsFoundation, BindlessResourceTable)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiGALBindlessResourceTable::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiGALBindlessResourceTable::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiGALBindlessResourceTable::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGALBindlessResourceTable::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALBindlessResourceTableDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGALBindlessResourceTableDescription>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("BufferSRVCapacity", m_uiBufferSRVCapacity),
      XII_MEMBER_PROPERTY("BufferUAVCapacity", m_uiBufferUAVCapacity),
      XII_MEMBER_PROPERTY("TextureSRVCapacity", m_uiTextureSRVCapacity),
      XII_MEMBER_PROPERTY("TextureUAVCapacity", m_uiTextureUAVCapacity),
      XII_MEMBER_PROPERTY("SamplerCapacity", m_uiSamplerCapacity),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALBindlessResourceTableStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGALBindlessResourceTableStats>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("BufferSRVCount", m_uiBufferSRVCount),
      XII_MEMBER_PROPERTY("BufferUAVCount", m_uiBufferUAVCount),
      XII_MEMBER_PROPERTY("TextureSRVCount", m_uiTextureSRVCount),
      XII_MEMBER_PROPERTY("TextureUAVCount", m_uiTextureUAVCount),
      XII_MEMBER_PROPERTY("SamplerCount", m_uiSamplerCount),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

namespace
{
  constexpr xiiUInt64 s_uiNotRetired = xiiMath::MaxValue<xiiUInt64>();

  template <typename TObject>
  xiiDynamicArray<TObject*> MakeRawTable(const xiiDynamicArray<xiiSharedPtr<TObject>>& objects)
  {
    xiiUInt32 uiCount = objects.GetCount();
    while (uiCount > 0U && objects[uiCount - 1U] == nullptr)
      --uiCount;
    xiiDynamicArray<TObject*> result;
    result.SetCount(uiCount);
    for (xiiUInt32 i = 0; i < uiCount; ++i)
      result[i] = objects[i].Borrow();
    return result;
  }
} // namespace

void xiiGALBindlessResourceTable::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Bindless resource table started twice.");
  s_pState = XII_DEFAULT_NEW(State);
  Initialize(s_pState->m_Description);
}

void xiiGALBindlessResourceTable::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede bindless table engine startup.");
  if (s_pState != nullptr && !s_pState->m_bInitialized)
    XII_VERIFY(Configure(s_pState->m_Description).Succeeded(), "Failed to restore the bindless resource table.");
}

void xiiGALBindlessResourceTable::EngineShutdown()
{
  // Drop strong references to device objects while the GAL device and its allocators are alive.
  Clear();
}

void xiiGALBindlessResourceTable::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

bool xiiGALBindlessResourceTable::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bInitialized;
}

const xiiGALBindlessResourceTableDescription& xiiGALBindlessResourceTable::GetConfiguration()
{
  XII_ASSERT_DEV(s_pState != nullptr, "The bindless resource table subsystem is not started.");
  static const xiiGALBindlessResourceTableDescription s_DefaultDescription;
  return s_pState != nullptr ? s_pState->m_Description : s_DefaultDescription;
}

namespace
{
template <typename TTable, typename TObject>
xiiGALBindlessResourceHandle RegisterResource(TTable& table, xiiSharedPtr<TObject> pObject)
{
  if (pObject == nullptr)
    return {};
  const xiiGALBindlessResourceHandle handle = table.m_Allocator.Allocate();
  if (!handle.IsValid())
    return {};
  table.m_Objects[handle.m_uiIndex]      = std::move(pObject);
  table.m_RetireFences[handle.m_uiIndex] = s_uiNotRetired;
  return handle;
}

template <typename TTable, typename TObject>
bool UpdateResource(TTable& table, xiiGALBindlessResourceHandle handle, xiiSharedPtr<TObject> pObject)
{
  if (pObject == nullptr || !table.m_Allocator.IsAlive(handle))
    return false;
  table.m_Objects[handle.m_uiIndex] = std::move(pObject);
  return true;
}

template <typename TTable>
bool RetireResource(TTable& table, xiiGALBindlessResourceHandle handle, xiiUInt64 uiFenceValue)
{
  if (!table.m_Allocator.Retire(handle, uiFenceValue))
    return false;
  table.m_RetireFences[handle.m_uiIndex] = uiFenceValue;
  return true;
}

template <typename TTable>
void CollectResources(TTable& table, xiiUInt64 uiCompletedFenceValue)
{
  for (xiiUInt32 i = 0; i < table.m_RetireFences.GetCount(); ++i)
  {
    if (table.m_RetireFences[i] != s_uiNotRetired && table.m_RetireFences[i] <= uiCompletedFenceValue)
    {
      table.m_Objects[i].Clear();
      table.m_RetireFences[i] = s_uiNotRetired;
    }
  }
  table.m_Allocator.Collect(uiCompletedFenceValue);
}
} // namespace

xiiResult xiiGALBindlessResourceTable::Configure(const xiiGALBindlessResourceTableDescription& description)
{
  XII_ASSERT_DEV(s_pState != nullptr, "The bindless resource table subsystem is not started.");
  if (s_pState == nullptr)
    return XII_FAILURE;

  if (description.m_uiBufferSRVCapacity == 0U || description.m_uiBufferUAVCapacity == 0U || description.m_uiTextureSRVCapacity == 0U || description.m_uiTextureUAVCapacity == 0U || description.m_uiSamplerCapacity == 0U ||
      description.m_uiBufferSRVCapacity > XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY || description.m_uiBufferUAVCapacity > XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY || description.m_uiTextureSRVCapacity > XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY || description.m_uiTextureUAVCapacity > XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY || description.m_uiSamplerCapacity > XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY)
    return XII_FAILURE;

  XII_LOCK(s_pState->m_Mutex);
  const bool bSameConfiguration =
    s_pState->m_Description.m_uiBufferSRVCapacity == description.m_uiBufferSRVCapacity &&
    s_pState->m_Description.m_uiBufferUAVCapacity == description.m_uiBufferUAVCapacity &&
    s_pState->m_Description.m_uiTextureSRVCapacity == description.m_uiTextureSRVCapacity &&
    s_pState->m_Description.m_uiTextureUAVCapacity == description.m_uiTextureUAVCapacity &&
    s_pState->m_Description.m_uiSamplerCapacity == description.m_uiSamplerCapacity;
  if (bSameConfiguration && s_pState->m_bInitialized)
    return XII_SUCCESS;

  auto hasOccupiedSlots = [](const auto& table) {
    for (const auto& pObject : table.m_Objects)
    {
      if (pObject != nullptr)
        return true;
    }
    return false;
  };
  if (hasOccupiedSlots(s_pState->m_BufferSRVs) || hasOccupiedSlots(s_pState->m_BufferUAVs) || hasOccupiedSlots(s_pState->m_TextureSRVs) || hasOccupiedSlots(s_pState->m_TextureUAVs) || hasOccupiedSlots(s_pState->m_Samplers))
  {
    XII_ASSERT_DEV(false, "The bindless resource table cannot be reconfigured while handles are active or retired.");
    return XII_FAILURE;
  }

  auto clear = [](auto& table) {
    table.m_Allocator.Clear();
    table.m_Objects.Clear();
    table.m_RetireFences.Clear();
  };
  clear(s_pState->m_BufferSRVs);
  clear(s_pState->m_BufferUAVs);
  clear(s_pState->m_TextureSRVs);
  clear(s_pState->m_TextureUAVs);
  clear(s_pState->m_Samplers);

  s_pState->m_Description = description;
  Initialize(description);
  return XII_SUCCESS;
}

void xiiGALBindlessResourceTable::Initialize(const xiiGALBindlessResourceTableDescription& description)
{
  XII_ASSERT_DEV(description.m_uiBufferSRVCapacity <= XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY && description.m_uiBufferUAVCapacity <= XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY && description.m_uiTextureSRVCapacity <= XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY && description.m_uiTextureUAVCapacity <= XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY && description.m_uiSamplerCapacity <= XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY, "Bindless table capacities must fit the reflected runtime descriptor capacity.");
  auto initialize = [](auto& table, xiiUInt32 uiCapacity) {
    table.m_Allocator.Initialize(uiCapacity);
    table.m_Objects.SetCount(uiCapacity);
    table.m_RetireFences.SetCount(uiCapacity, s_uiNotRetired);
  };
  initialize(s_pState->m_BufferSRVs, description.m_uiBufferSRVCapacity);
  initialize(s_pState->m_BufferUAVs, description.m_uiBufferUAVCapacity);
  initialize(s_pState->m_TextureSRVs, description.m_uiTextureSRVCapacity);
  initialize(s_pState->m_TextureUAVs, description.m_uiTextureUAVCapacity);
  initialize(s_pState->m_Samplers, description.m_uiSamplerCapacity);
  s_pState->m_bInitialized = true;
}

void xiiGALBindlessResourceTable::Clear()
{
  if (s_pState == nullptr)
    return;

  XII_LOCK(s_pState->m_Mutex);
  auto clear = [](auto& table) {
    table.m_Allocator.Clear();
    table.m_Objects.Clear();
    table.m_RetireFences.Clear();
  };
  clear(s_pState->m_BufferSRVs);
  clear(s_pState->m_BufferUAVs);
  clear(s_pState->m_TextureSRVs);
  clear(s_pState->m_TextureUAVs);
  clear(s_pState->m_Samplers);
  s_pState->m_bInitialized = false;
}

xiiGALBindlessResourceHandle xiiGALBindlessResourceTable::RegisterBufferSRV(xiiSharedPtr<xiiGALBufferView> p)
{
  if (s_pState == nullptr)
    return {};
  XII_LOCK(s_pState->m_Mutex);
  return RegisterResource(s_pState->m_BufferSRVs, std::move(p));
}
xiiGALBindlessResourceHandle xiiGALBindlessResourceTable::RegisterBufferUAV(xiiSharedPtr<xiiGALBufferView> p)
{
  if (s_pState == nullptr)
    return {};
  XII_LOCK(s_pState->m_Mutex);
  return RegisterResource(s_pState->m_BufferUAVs, std::move(p));
}
xiiGALBindlessResourceHandle xiiGALBindlessResourceTable::RegisterTextureSRV(xiiSharedPtr<xiiGALTextureView> p)
{
  if (s_pState == nullptr)
    return {};
  XII_LOCK(s_pState->m_Mutex);
  return RegisterResource(s_pState->m_TextureSRVs, std::move(p));
}
xiiGALBindlessResourceHandle xiiGALBindlessResourceTable::RegisterTextureUAV(xiiSharedPtr<xiiGALTextureView> p)
{
  if (s_pState == nullptr)
    return {};
  XII_LOCK(s_pState->m_Mutex);
  return RegisterResource(s_pState->m_TextureUAVs, std::move(p));
}
xiiGALBindlessResourceHandle xiiGALBindlessResourceTable::RegisterSampler(xiiSharedPtr<xiiGALSampler> p)
{
  if (s_pState == nullptr)
    return {};
  XII_LOCK(s_pState->m_Mutex);
  return RegisterResource(s_pState->m_Samplers, std::move(p));
}

bool xiiGALBindlessResourceTable::UpdateBufferSRV(xiiGALBindlessResourceHandle h, xiiSharedPtr<xiiGALBufferView> p)
{
  if (s_pState == nullptr)
    return false;
  XII_LOCK(s_pState->m_Mutex);
  return UpdateResource(s_pState->m_BufferSRVs, h, std::move(p));
}
bool xiiGALBindlessResourceTable::UpdateBufferUAV(xiiGALBindlessResourceHandle h, xiiSharedPtr<xiiGALBufferView> p)
{
  if (s_pState == nullptr)
    return false;
  XII_LOCK(s_pState->m_Mutex);
  return UpdateResource(s_pState->m_BufferUAVs, h, std::move(p));
}
bool xiiGALBindlessResourceTable::UpdateTextureSRV(xiiGALBindlessResourceHandle h, xiiSharedPtr<xiiGALTextureView> p)
{
  if (s_pState == nullptr)
    return false;
  XII_LOCK(s_pState->m_Mutex);
  return UpdateResource(s_pState->m_TextureSRVs, h, std::move(p));
}
bool xiiGALBindlessResourceTable::UpdateTextureUAV(xiiGALBindlessResourceHandle h, xiiSharedPtr<xiiGALTextureView> p)
{
  if (s_pState == nullptr)
    return false;
  XII_LOCK(s_pState->m_Mutex);
  return UpdateResource(s_pState->m_TextureUAVs, h, std::move(p));
}
bool xiiGALBindlessResourceTable::UpdateSampler(xiiGALBindlessResourceHandle h, xiiSharedPtr<xiiGALSampler> p)
{
  if (s_pState == nullptr)
    return false;
  XII_LOCK(s_pState->m_Mutex);
  return UpdateResource(s_pState->m_Samplers, h, std::move(p));
}

bool xiiGALBindlessResourceTable::RetireBufferSRV(xiiGALBindlessResourceHandle h, xiiUInt64 f)
{
  if (s_pState == nullptr)
    return false;
  XII_LOCK(s_pState->m_Mutex);
  return RetireResource(s_pState->m_BufferSRVs, h, f);
}
bool xiiGALBindlessResourceTable::RetireBufferUAV(xiiGALBindlessResourceHandle h, xiiUInt64 f)
{
  if (s_pState == nullptr)
    return false;
  XII_LOCK(s_pState->m_Mutex);
  return RetireResource(s_pState->m_BufferUAVs, h, f);
}
bool xiiGALBindlessResourceTable::RetireTextureSRV(xiiGALBindlessResourceHandle h, xiiUInt64 f)
{
  if (s_pState == nullptr)
    return false;
  XII_LOCK(s_pState->m_Mutex);
  return RetireResource(s_pState->m_TextureSRVs, h, f);
}
bool xiiGALBindlessResourceTable::RetireTextureUAV(xiiGALBindlessResourceHandle h, xiiUInt64 f)
{
  if (s_pState == nullptr)
    return false;
  XII_LOCK(s_pState->m_Mutex);
  return RetireResource(s_pState->m_TextureUAVs, h, f);
}
bool xiiGALBindlessResourceTable::RetireSampler(xiiGALBindlessResourceHandle h, xiiUInt64 f)
{
  if (s_pState == nullptr)
    return false;
  XII_LOCK(s_pState->m_Mutex);
  return RetireResource(s_pState->m_Samplers, h, f);
}

void xiiGALBindlessResourceTable::Collect(xiiUInt64 uiCompletedFenceValue)
{
  if (s_pState == nullptr)
    return;
  XII_LOCK(s_pState->m_Mutex);
  CollectResources(s_pState->m_BufferSRVs, uiCompletedFenceValue);
  CollectResources(s_pState->m_BufferUAVs, uiCompletedFenceValue);
  CollectResources(s_pState->m_TextureSRVs, uiCompletedFenceValue);
  CollectResources(s_pState->m_TextureUAVs, uiCompletedFenceValue);
  CollectResources(s_pState->m_Samplers, uiCompletedFenceValue);
}

void xiiGALBindlessResourceTable::BindBufferSRVs(xiiGALCommandList& commandList, const xiiTempHashedString& name, xiiBitflags<xiiGALShaderType> stages)
{
  if (s_pState == nullptr)
    return;
  XII_LOCK(s_pState->m_Mutex);
  auto raw = MakeRawTable(s_pState->m_BufferSRVs.m_Objects);
  commandList.ResolveAndSetShaderResourceBufferViews(name, 0U, raw, stages);
}
void xiiGALBindlessResourceTable::BindBufferUAVs(xiiGALCommandList& commandList, const xiiTempHashedString& name, xiiBitflags<xiiGALShaderType> stages)
{
  if (s_pState == nullptr)
    return;
  XII_LOCK(s_pState->m_Mutex);
  auto raw = MakeRawTable(s_pState->m_BufferUAVs.m_Objects);
  commandList.ResolveAndSetUnorderedAccessBufferViews(name, 0U, raw, stages);
}
void xiiGALBindlessResourceTable::BindTextureSRVs(xiiGALCommandList& commandList, const xiiTempHashedString& name, xiiBitflags<xiiGALShaderType> stages)
{
  if (s_pState == nullptr)
    return;
  XII_LOCK(s_pState->m_Mutex);
  auto raw = MakeRawTable(s_pState->m_TextureSRVs.m_Objects);
  commandList.ResolveAndSetShaderResourceTextureViews(name, 0U, raw, stages);
}
void xiiGALBindlessResourceTable::BindTextureUAVs(xiiGALCommandList& commandList, const xiiTempHashedString& name, xiiBitflags<xiiGALShaderType> stages)
{
  if (s_pState == nullptr)
    return;
  XII_LOCK(s_pState->m_Mutex);
  auto raw = MakeRawTable(s_pState->m_TextureUAVs.m_Objects);
  commandList.ResolveAndSetUnorderedAccessTextureViews(name, 0U, raw, stages);
}
void xiiGALBindlessResourceTable::BindSamplers(xiiGALCommandList& commandList, const xiiTempHashedString& name, xiiBitflags<xiiGALShaderType> stages)
{
  if (s_pState == nullptr)
    return;
  XII_LOCK(s_pState->m_Mutex);
  auto raw = MakeRawTable(s_pState->m_Samplers.m_Objects);
  commandList.ResolveAndSetSamplers(name, 0U, raw, stages);
}

xiiGALBindlessResourceTableStats xiiGALBindlessResourceTable::GetStats()
{
  if (s_pState == nullptr)
    return {};
  XII_LOCK(s_pState->m_Mutex);
  xiiGALBindlessResourceTableStats stats;
  stats.m_uiBufferSRVCount  = s_pState->m_BufferSRVs.m_Allocator.GetAllocatedCount();
  stats.m_uiBufferUAVCount  = s_pState->m_BufferUAVs.m_Allocator.GetAllocatedCount();
  stats.m_uiTextureSRVCount = s_pState->m_TextureSRVs.m_Allocator.GetAllocatedCount();
  stats.m_uiTextureUAVCount = s_pState->m_TextureUAVs.m_Allocator.GetAllocatedCount();
  stats.m_uiSamplerCount    = s_pState->m_Samplers.m_Allocator.GetAllocatedCount();
  return stats;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_BindlessResourceTable);
