/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Lock.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>

XII_IMPLEMENT_SINGLETON(xiiGALBindlessResourceTable);

static xiiUniquePtr<xiiGALBindlessResourceTable> s_pBindlessResourceTable;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsFoundation, BindlessResourceTable)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    s_pBindlessResourceTable = XII_DEFAULT_NEW(xiiGALBindlessResourceTable);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiGALBindlessResourceTable* pTable = xiiGALBindlessResourceTable::GetSingleton();
    XII_ASSERT_DEV(pTable != nullptr, "The bindless resource table core subsystem must be started first.");

    if (!pTable->IsInitialized())
    {
      XII_VERIFY(pTable->Configure(pTable->GetConfiguration()).Succeeded(), "Failed to restore the bindless resource table.");
    }
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (xiiGALBindlessResourceTable* pTable = xiiGALBindlessResourceTable::GetSingleton())
    {
      // Drop strong references to device objects while the GAL device and its allocators
      // are still alive. The table object itself remains available until core shutdown.
      pTable->Clear();
    }
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    s_pBindlessResourceTable.Clear();
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

xiiGALBindlessResourceTable::xiiGALBindlessResourceTable() :
  m_SingletonRegistrar(this)
{
  Initialize(m_Description);
}

xiiGALBindlessResourceTable::~xiiGALBindlessResourceTable()
{
  Clear();
}

template <typename TObject>
xiiGALBindlessResourceHandle xiiGALBindlessResourceTable::Register(TableStorage<TObject>& table, xiiSharedPtr<TObject> pObject)
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

template <typename TObject>
bool xiiGALBindlessResourceTable::Update(TableStorage<TObject>& table, xiiGALBindlessResourceHandle handle, xiiSharedPtr<TObject> pObject)
{
  if (pObject == nullptr || !table.m_Allocator.IsAlive(handle))
    return false;
  table.m_Objects[handle.m_uiIndex] = std::move(pObject);
  return true;
}

template <typename TObject>
bool xiiGALBindlessResourceTable::Retire(TableStorage<TObject>& table, xiiGALBindlessResourceHandle handle, xiiUInt64 uiFenceValue)
{
  if (!table.m_Allocator.Retire(handle, uiFenceValue))
    return false;
  table.m_RetireFences[handle.m_uiIndex] = uiFenceValue;
  return true;
}

template <typename TObject>
void xiiGALBindlessResourceTable::Collect(TableStorage<TObject>& table, xiiUInt64 uiCompletedFenceValue)
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

xiiResult xiiGALBindlessResourceTable::Configure(const xiiGALBindlessResourceTableDescription& description)
{
  if (description.m_uiBufferSRVCapacity == 0U || description.m_uiBufferUAVCapacity == 0U || description.m_uiTextureSRVCapacity == 0U || description.m_uiTextureUAVCapacity == 0U || description.m_uiSamplerCapacity == 0U ||
      description.m_uiBufferSRVCapacity > XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY || description.m_uiBufferUAVCapacity > XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY || description.m_uiTextureSRVCapacity > XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY || description.m_uiTextureUAVCapacity > XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY || description.m_uiSamplerCapacity > XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY)
    return XII_FAILURE;

  XII_LOCK(m_Mutex);
  const bool bSameConfiguration =
    m_Description.m_uiBufferSRVCapacity == description.m_uiBufferSRVCapacity &&
    m_Description.m_uiBufferUAVCapacity == description.m_uiBufferUAVCapacity &&
    m_Description.m_uiTextureSRVCapacity == description.m_uiTextureSRVCapacity &&
    m_Description.m_uiTextureUAVCapacity == description.m_uiTextureUAVCapacity &&
    m_Description.m_uiSamplerCapacity == description.m_uiSamplerCapacity;
  if (bSameConfiguration && m_bInitialized)
    return XII_SUCCESS;

  auto hasOccupiedSlots = [](const auto& table) {
    for (const auto& pObject : table.m_Objects)
    {
      if (pObject != nullptr)
        return true;
    }
    return false;
  };
  if (hasOccupiedSlots(m_BufferSRVs) || hasOccupiedSlots(m_BufferUAVs) || hasOccupiedSlots(m_TextureSRVs) || hasOccupiedSlots(m_TextureUAVs) || hasOccupiedSlots(m_Samplers))
  {
    XII_ASSERT_DEV(false, "The bindless resource table cannot be reconfigured while handles are active or retired.");
    return XII_FAILURE;
  }

  auto clear = [](auto& table) {
    table.m_Allocator.Clear();
    table.m_Objects.Clear();
    table.m_RetireFences.Clear();
  };
  clear(m_BufferSRVs);
  clear(m_BufferUAVs);
  clear(m_TextureSRVs);
  clear(m_TextureUAVs);
  clear(m_Samplers);

  m_Description = description;
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
  initialize(m_BufferSRVs, description.m_uiBufferSRVCapacity);
  initialize(m_BufferUAVs, description.m_uiBufferUAVCapacity);
  initialize(m_TextureSRVs, description.m_uiTextureSRVCapacity);
  initialize(m_TextureUAVs, description.m_uiTextureUAVCapacity);
  initialize(m_Samplers, description.m_uiSamplerCapacity);
  m_bInitialized = true;
}

void xiiGALBindlessResourceTable::Clear()
{
  XII_LOCK(m_Mutex);
  auto clear = [](auto& table) {
    table.m_Allocator.Clear();
    table.m_Objects.Clear();
    table.m_RetireFences.Clear();
  };
  clear(m_BufferSRVs);
  clear(m_BufferUAVs);
  clear(m_TextureSRVs);
  clear(m_TextureUAVs);
  clear(m_Samplers);
  m_bInitialized = false;
}

xiiGALBindlessResourceHandle xiiGALBindlessResourceTable::RegisterBufferSRV(xiiSharedPtr<xiiGALBufferView> p)
{
  XII_LOCK(m_Mutex);
  return Register(m_BufferSRVs, std::move(p));
}
xiiGALBindlessResourceHandle xiiGALBindlessResourceTable::RegisterBufferUAV(xiiSharedPtr<xiiGALBufferView> p)
{
  XII_LOCK(m_Mutex);
  return Register(m_BufferUAVs, std::move(p));
}
xiiGALBindlessResourceHandle xiiGALBindlessResourceTable::RegisterTextureSRV(xiiSharedPtr<xiiGALTextureView> p)
{
  XII_LOCK(m_Mutex);
  return Register(m_TextureSRVs, std::move(p));
}
xiiGALBindlessResourceHandle xiiGALBindlessResourceTable::RegisterTextureUAV(xiiSharedPtr<xiiGALTextureView> p)
{
  XII_LOCK(m_Mutex);
  return Register(m_TextureUAVs, std::move(p));
}
xiiGALBindlessResourceHandle xiiGALBindlessResourceTable::RegisterSampler(xiiSharedPtr<xiiGALSampler> p)
{
  XII_LOCK(m_Mutex);
  return Register(m_Samplers, std::move(p));
}

bool xiiGALBindlessResourceTable::UpdateBufferSRV(xiiGALBindlessResourceHandle h, xiiSharedPtr<xiiGALBufferView> p)
{
  XII_LOCK(m_Mutex);
  return Update(m_BufferSRVs, h, std::move(p));
}
bool xiiGALBindlessResourceTable::UpdateBufferUAV(xiiGALBindlessResourceHandle h, xiiSharedPtr<xiiGALBufferView> p)
{
  XII_LOCK(m_Mutex);
  return Update(m_BufferUAVs, h, std::move(p));
}
bool xiiGALBindlessResourceTable::UpdateTextureSRV(xiiGALBindlessResourceHandle h, xiiSharedPtr<xiiGALTextureView> p)
{
  XII_LOCK(m_Mutex);
  return Update(m_TextureSRVs, h, std::move(p));
}
bool xiiGALBindlessResourceTable::UpdateTextureUAV(xiiGALBindlessResourceHandle h, xiiSharedPtr<xiiGALTextureView> p)
{
  XII_LOCK(m_Mutex);
  return Update(m_TextureUAVs, h, std::move(p));
}
bool xiiGALBindlessResourceTable::UpdateSampler(xiiGALBindlessResourceHandle h, xiiSharedPtr<xiiGALSampler> p)
{
  XII_LOCK(m_Mutex);
  return Update(m_Samplers, h, std::move(p));
}

bool xiiGALBindlessResourceTable::RetireBufferSRV(xiiGALBindlessResourceHandle h, xiiUInt64 f)
{
  XII_LOCK(m_Mutex);
  return Retire(m_BufferSRVs, h, f);
}
bool xiiGALBindlessResourceTable::RetireBufferUAV(xiiGALBindlessResourceHandle h, xiiUInt64 f)
{
  XII_LOCK(m_Mutex);
  return Retire(m_BufferUAVs, h, f);
}
bool xiiGALBindlessResourceTable::RetireTextureSRV(xiiGALBindlessResourceHandle h, xiiUInt64 f)
{
  XII_LOCK(m_Mutex);
  return Retire(m_TextureSRVs, h, f);
}
bool xiiGALBindlessResourceTable::RetireTextureUAV(xiiGALBindlessResourceHandle h, xiiUInt64 f)
{
  XII_LOCK(m_Mutex);
  return Retire(m_TextureUAVs, h, f);
}
bool xiiGALBindlessResourceTable::RetireSampler(xiiGALBindlessResourceHandle h, xiiUInt64 f)
{
  XII_LOCK(m_Mutex);
  return Retire(m_Samplers, h, f);
}

void xiiGALBindlessResourceTable::Collect(xiiUInt64 uiCompletedFenceValue)
{
  XII_LOCK(m_Mutex);
  Collect(m_BufferSRVs, uiCompletedFenceValue);
  Collect(m_BufferUAVs, uiCompletedFenceValue);
  Collect(m_TextureSRVs, uiCompletedFenceValue);
  Collect(m_TextureUAVs, uiCompletedFenceValue);
  Collect(m_Samplers, uiCompletedFenceValue);
}

void xiiGALBindlessResourceTable::BindBufferSRVs(xiiGALCommandList& commandList, const xiiTempHashedString& name, xiiBitflags<xiiGALShaderType> stages) const
{
  XII_LOCK(m_Mutex);
  auto raw = MakeRawTable(m_BufferSRVs.m_Objects);
  commandList.ResolveAndSetShaderResourceBufferViews(name, 0U, raw, stages);
}
void xiiGALBindlessResourceTable::BindBufferUAVs(xiiGALCommandList& commandList, const xiiTempHashedString& name, xiiBitflags<xiiGALShaderType> stages) const
{
  XII_LOCK(m_Mutex);
  auto raw = MakeRawTable(m_BufferUAVs.m_Objects);
  commandList.ResolveAndSetUnorderedAccessBufferViews(name, 0U, raw, stages);
}
void xiiGALBindlessResourceTable::BindTextureSRVs(xiiGALCommandList& commandList, const xiiTempHashedString& name, xiiBitflags<xiiGALShaderType> stages) const
{
  XII_LOCK(m_Mutex);
  auto raw = MakeRawTable(m_TextureSRVs.m_Objects);
  commandList.ResolveAndSetShaderResourceTextureViews(name, 0U, raw, stages);
}
void xiiGALBindlessResourceTable::BindTextureUAVs(xiiGALCommandList& commandList, const xiiTempHashedString& name, xiiBitflags<xiiGALShaderType> stages) const
{
  XII_LOCK(m_Mutex);
  auto raw = MakeRawTable(m_TextureUAVs.m_Objects);
  commandList.ResolveAndSetUnorderedAccessTextureViews(name, 0U, raw, stages);
}
void xiiGALBindlessResourceTable::BindSamplers(xiiGALCommandList& commandList, const xiiTempHashedString& name, xiiBitflags<xiiGALShaderType> stages) const
{
  XII_LOCK(m_Mutex);
  auto raw = MakeRawTable(m_Samplers.m_Objects);
  commandList.ResolveAndSetSamplers(name, 0U, raw, stages);
}

xiiGALBindlessResourceTableStats xiiGALBindlessResourceTable::GetStats() const
{
  XII_LOCK(m_Mutex);
  xiiGALBindlessResourceTableStats stats;
  stats.m_uiBufferSRVCount  = m_BufferSRVs.m_Allocator.GetAllocatedCount();
  stats.m_uiBufferUAVCount  = m_BufferUAVs.m_Allocator.GetAllocatedCount();
  stats.m_uiTextureSRVCount = m_TextureSRVs.m_Allocator.GetAllocatedCount();
  stats.m_uiTextureUAVCount = m_TextureUAVs.m_Allocator.GetAllocatedCount();
  stats.m_uiSamplerCount    = m_Samplers.m_Allocator.GetAllocatedCount();
  return stats;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_BindlessResourceTable);
