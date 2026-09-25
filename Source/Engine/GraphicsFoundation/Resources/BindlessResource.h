/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsFoundation/GraphicsFoundationDLL.h>

/// Generation-checked index into a backend bindless descriptor table.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBindlessResourceHandle
{
  XII_DECLARE_POD_TYPE();
  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex && m_uiGeneration != 0U; }
  xiiUInt32                            m_uiIndex      = xiiInvalidIndex;
  xiiUInt32                            m_uiGeneration = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALBindlessResourceHandle);

/// Fence-aware descriptor index allocator shared by bindless texture, buffer and sampler tables.
/// Indices are not reused until Collect() observes the frame/fence on which the old descriptor was
/// last referenced, preventing descriptor aliasing across frames in flight.
class XII_GRAPHICSFOUNDATION_DLL xiiGALBindlessResourceAllocator
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALBindlessResourceAllocator);

public:
  xiiGALBindlessResourceAllocator() = default;

  void Initialize(xiiUInt32 uiCapacity);
  void Clear();

  [[nodiscard]] xiiGALBindlessResourceHandle Allocate();
  bool                                       Retire(xiiGALBindlessResourceHandle handle, xiiUInt64 uiLastUseFenceValue);
  void                                       Collect(xiiUInt64 uiCompletedFenceValue);

  [[nodiscard]] bool      IsAlive(xiiGALBindlessResourceHandle handle) const;
  [[nodiscard]] xiiUInt32 GetCapacity() const { return m_Generations.GetCount(); }
  [[nodiscard]] xiiUInt32 GetAllocatedCount() const;

private:
  struct RetiredIndex
  {
    xiiUInt32 m_uiIndex      = xiiInvalidIndex;
    xiiUInt64 m_uiFenceValue = 0U;
  };

  mutable xiiMutex              m_Mutex;
  xiiDynamicArray<xiiUInt32>    m_Generations;
  xiiDynamicArray<xiiUInt32>    m_FreeIndices;
  xiiDynamicArray<xiiUInt8>     m_Allocated;
  xiiDynamicArray<RetiredIndex> m_RetiredIndices;
  xiiUInt32                     m_uiAllocatedCount = 0U;
};
