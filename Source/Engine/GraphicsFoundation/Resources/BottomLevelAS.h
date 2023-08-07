#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief This describes the acceleration structures build flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRaytTracingBuildASFlags
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    None            = 0U,         ///< No raytracing acceleration structure build flags.
    AllowUpdate     = XII_BIT(0), ///< Indicates that the specified acceleration structure can be updated with the build blas/tlas device functions. With this flag, the acceleration structure may allocate more memory and take more time to build.
    AllowCompaction = XII_BIT(1), ///< Indicates that the specified acceleration structure can act as the source for a copy acceleration structure command with the copy AS compact mode to produce a compacted acceleration structure. With this flag acceleration structure may allocate more memory and take more time on build.
    PreferFastTrace = XII_BIT(2), ///< Indicates that the given acceleration structure build should prioritize trace performance over build time.
    PreferFastBuild = XII_BIT(3), ///< Indicates that the given acceleration structure build should prioritize build time over trace performance.
    LowMemory       = XII_BIT(4), ///< Indicates that this acceleration structure should minimize the size of the scratch memory and the final result build, potentially at the expense of build time or trace performance.

    ENUM_COUNT,

    Default = None
  };

  struct Bits
  {
    StorageType AllowUpdate : 1;
    StorageType AllowCompaction : 1;
    StorageType PreferFastTrace : 1;
    StorageType PreferFastBuild : 1;
    StorageType LowMemory : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALRaytTracingBuildASFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALRaytTracingBuildASFlags);

struct XII_GRAPHICSFOUNDATION_DLL xiiGALBLASTriangleDescription : public xiiHashableStruct<xiiGALBLASTriangleDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiStringView            m_sGeometryName;               ///< The geometry name used to map triangle data.
  xiiUInt32                m_uiMaxVertexCount = 0U;       ///< The maximum vertex count in this geometry.
  xiiEnum<xiiGALValueType> m_VertexValueType;             ///< The type of vertices in this geometry.
  xiiUInt8                 m_uiVertexComponentCount = 0U; ///< The number of components in the vertex.
  xiiUInt32                m_uiMaxPrimitiveCount    = 0U; ///< The maximum primitive count in this geometry.
  xiiEnum<xiiGALValueType> m_IndexType;                   ///< The index type of this geometry.
};

struct XII_GRAPHICSFOUNDATION_DLL xiiGALBLASBoundingBoxDescription : public xiiHashableStruct<xiiGALBLASBoundingBoxDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiStringView m_sGeometryName; ///< The geometry name.
  xiiUInt32     m_uiMaxBoxCount; ///< The maximum axis aligned bounding box (AABB) count.
};

struct XII_GRAPHICSFOUNDATION_DLL xiiGALBottomLevelASDescription : public xiiHashableStruct<xiiGALBottomLevelASDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiStringView                              m_sName;
  const xiiGALBLASTriangleDescription*       m_pTriangles         = nullptr;
  xiiUInt32                                  m_uiTriangleCount    = 0U;
  const xiiGALBLASBoundingBoxDescription*    m_pBoundingBoxes     = nullptr;
  xiiUInt32                                  m_uiBoundingBoxCount = 0U;
  xiiBitflags<xiiGALRaytTracingBuildASFlags> m_BuildASFlags;
  xiiUInt64                                  m_uiCompactedSize        = 0U;
  xiiUInt64                                  m_uiImmediateContextMask = XII_BIT(0);
};

struct XII_GRAPHICSFOUNDATION_DLL xiiGALScratchBufferSizeDescription : public xiiHashableStruct<xiiGALScratchBufferSizeDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt64 m_uiBuild  = 0U; ///< Scratch buffer size for acceleration structure building. May be zero if the acceleration structure was created with a non-zero compacted size.
  xiiUInt64 m_uiUpdate = 0U; ///< Scratch buffer size for acceleration structure updating. May be zero if acceleration structure was created without raytracing build allow update flag or with a non-zero compacted size.
};

#include <GraphicsFoundation/Resources/Implementation/BottomLevelAS_inl.h>
