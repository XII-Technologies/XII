/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/Threading/Lock.h>
#include <GraphicsFoundation/Resources/BindlessResource.h>

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALBindlessResourceHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGALBindlessResourceHandle>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Index", m_uiIndex),
      XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

void xiiGALBindlessResourceAllocator::Initialize(xiiUInt32 uiCapacity)
{
  XII_LOCK(m_Mutex);
  m_Generations.SetCount(uiCapacity, 1U);
  m_Allocated.SetCount(uiCapacity, 0U);
  m_FreeIndices.Clear();
  m_FreeIndices.Reserve(uiCapacity);
  for (xiiUInt32 i = uiCapacity; i > 0U; --i)
    m_FreeIndices.PushBack(i - 1U);
  m_RetiredIndices.Clear();
  m_uiAllocatedCount = 0U;
}

void xiiGALBindlessResourceAllocator::Clear()
{
  XII_LOCK(m_Mutex);
  m_Generations.Clear();
  m_FreeIndices.Clear();
  m_Allocated.Clear();
  m_RetiredIndices.Clear();
  m_uiAllocatedCount = 0U;
}

xiiGALBindlessResourceHandle xiiGALBindlessResourceAllocator::Allocate()
{
  XII_LOCK(m_Mutex);
  if (m_FreeIndices.IsEmpty())
    return {};
  const xiiUInt32 uiIndex = m_FreeIndices.PeekBack();
  m_FreeIndices.PopBack();
  m_Allocated[uiIndex] = 1U;
  ++m_uiAllocatedCount;
  xiiGALBindlessResourceHandle handle;
  handle.m_uiIndex      = uiIndex;
  handle.m_uiGeneration = m_Generations[uiIndex];
  return handle;
}

bool xiiGALBindlessResourceAllocator::Retire(xiiGALBindlessResourceHandle handle, xiiUInt64 uiLastUseFenceValue)
{
  XII_LOCK(m_Mutex);
  if (!handle.IsValid() || handle.m_uiIndex >= m_Generations.GetCount() || m_Allocated[handle.m_uiIndex] == 0U || m_Generations[handle.m_uiIndex] != handle.m_uiGeneration)
    return false;
  m_Allocated[handle.m_uiIndex] = 0U;
  ++m_Generations[handle.m_uiIndex];
  if (m_Generations[handle.m_uiIndex] == 0U) m_Generations[handle.m_uiIndex] = 1U;
  m_RetiredIndices.PushBack({handle.m_uiIndex, uiLastUseFenceValue});
  --m_uiAllocatedCount;
  return true;
}

void xiiGALBindlessResourceAllocator::Collect(xiiUInt64 uiCompletedFenceValue)
{
  XII_LOCK(m_Mutex);
  for (xiiUInt32 i = m_RetiredIndices.GetCount(); i > 0U; --i)
  {
    if (m_RetiredIndices[i - 1U].m_uiFenceValue > uiCompletedFenceValue) continue;
    m_FreeIndices.PushBack(m_RetiredIndices[i - 1U].m_uiIndex);
    m_RetiredIndices.RemoveAtAndSwap(i - 1U);
  }
}

bool xiiGALBindlessResourceAllocator::IsAlive(xiiGALBindlessResourceHandle handle) const
{
  XII_LOCK(m_Mutex);
  return handle.IsValid() && handle.m_uiIndex < m_Generations.GetCount() && m_Allocated[handle.m_uiIndex] != 0U && m_Generations[handle.m_uiIndex] == handle.m_uiGeneration;
}

xiiUInt32 xiiGALBindlessResourceAllocator::GetAllocatedCount() const
{
  XII_LOCK(m_Mutex);
  return m_uiAllocatedCount;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_BindlessResource);
