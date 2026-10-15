/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/Bitflags.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// Demand for world-derived rendering data. These categories describe data, not render paths.
/// Unknown/custom render components conservatively advertise All until they provide a narrower
/// declaration. Passes declare only data they consume. No scene ownership is implied by a flag.
struct xiiRenderExtractionCategory
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    None       = 0,
    Geometry   = XII_BIT(0),
    Lights     = XII_BIT(1),
    Decals     = XII_BIT(2),
    Particles  = XII_BIT(3),
    Atmosphere = XII_BIT(4),
    Custom     = XII_BIT(5),
    All        = Geometry | Lights | Decals | Particles | Atmosphere | Custom,
    Default    = None,
  };
  struct Bits
  {
    StorageType Geometry : 1;
    StorageType Lights : 1;
    StorageType Decals : 1;
    StorageType Particles : 1;
    StorageType Atmosphere : 1;
    StorageType Custom : 1;
  };
};
XII_DECLARE_FLAGS_OPERATORS(xiiRenderExtractionCategory);

/// GPU services required by a pipeline, independent of which CPU scene categories it reads.
/// For example, a CPU light inspector need not initialize the GPU lighting system.
struct xiiRenderPipelineFeature
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    None           = 0,
    GpuScene       = XII_BIT(0),
    Lighting       = XII_BIT(1),
    VirtualShadows = XII_BIT(2),
    RayTracing     = XII_BIT(3),
    All            = GpuScene | Lighting | VirtualShadows | RayTracing,
    Default        = None,
  };
  struct Bits
  {
    StorageType GpuScene : 1;
    StorageType Lighting : 1;
    StorageType VirtualShadows : 1;
    StorageType RayTracing : 1;
  };
};
XII_DECLARE_FLAGS_OPERATORS(xiiRenderPipelineFeature);

struct xiiRenderExtractionStatistics
{
  xiiUInt32 m_uiDispatchedComponents = 0;
  xiiUInt32 m_uiSkippedComponents    = 0;
  xiiUInt32 m_uiReusedComponents     = 0;
};
