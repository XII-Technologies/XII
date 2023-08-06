#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief This describes the acceleration structures build flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRaytTracingBuildASFlags
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    None,                         ///< No raytracing acceleration structure build flags.
    AllowUpdate     = XII_BIT(0), ///< Indicates that the specified acceleration structure can be updated with the build blas/tlas device functions. With this flag, the acceleration structure may allocate more memory and take more time to build.
    AllowCompaction = XII_BIT(1), ///< Indicates that the specified acceleration structure can act as the source for a copy acceleration structure command with the copy AS compact mode to produce a compacted acceleration structure. With this flag acceleration structure may allocate more memory and take more time on build.
    PreferFastTrace = XII_BIT(2), ///< Indicates that the given acceleration structure build should prioritize trace performance over build time.
    PreferFastBuild = XII_BIT(3), ///< Indicates that the given acceleration structure build should prioritize build time over trace performance.
    LowMemory       = XII_BIT(4), ///< Indicates that this acceleration structure should minimize the size of the scratch memory and the final result build, potentially at the expense of build time or trace performance.

    ENUM_COUNT,

    Default = None
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALRaytTracingBuildASFlags);

#include <GraphicsFoundation/Resources/Implementation/BottomLevelAS_inl.h>
