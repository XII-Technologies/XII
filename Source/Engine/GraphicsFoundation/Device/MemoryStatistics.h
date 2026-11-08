/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Types/Enum.h>

/// Attribution follows allocation ownership, including deferred native destruction.
/// Categories share the device budget; these are diagnostic and cache-admission tags.
struct xiiGALMemoryCategory
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    Other,
    Geometry,
    GeometryPages,
    Textures,
    TexturePages,
    StreamingMetadata,
    Materials,
    AccelerationStructures,
    RenderGraph,
    Upload,
    Readback,
    ENUM_COUNT,
    Default = Other
  };
  static constexpr const char* Names[] = {
    "Other", "Geometry", "Geometry pages", "Textures", "Texture pages",
    "Streaming metadata", "Materials", "Acceleration structures", "Render graph",
    "Upload", "Readback"};
};

/// Native resource suballocations, counted once per allocation. Heap-level
/// committed pool capacity and OS budget/usage are reported separately.
struct xiiGALMemoryCategoryStatistics
{
  xiiEnum<xiiGALMemoryCategory> m_Category;
  xiiUInt64                     m_uiAllocatedBytes   = 0U;
  xiiUInt64                     m_uiDeviceLocalBytes = 0U;
  xiiUInt64                     m_uiHostVisibleBytes = 0U; ///< May overlap device-local bytes on unified or BAR-visible heaps.
  xiiUInt64                     m_uiRetiringBytes    = 0U; ///< Still charged until the native allocator frees the allocation.
  xiiUInt32                     m_uiAllocationCount  = 0U;
};
