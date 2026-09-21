/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/Resource.h>

/// This describes the acceleration structure build flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingBuildASFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None            = 0U,         ///< No raytracing acceleration structure build flags.
    AllowUpdate     = XII_BIT(0), ///< Indicates that the specified acceleration structure can be updated with the build BLAS/TLAS device functions. With this flag, the acceleration structure may allocate more memory and take more time to build.
    AllowCompaction = XII_BIT(1), ///< Indicates that the specified acceleration structure can act as the source for a copy acceleration structure command with the copy AS compact mode to produce a compacted acceleration structure. With this flag acceleration structure may allocate more memory and take more time on build.
    PreferFastTrace = XII_BIT(2), ///< Indicates that the given acceleration structure build should prioritize trace performance over build time.
    PreferFastBuild = XII_BIT(3), ///< Indicates that the given acceleration structure build should prioritize build time over trace performance.
    LowMemory       = XII_BIT(4), ///< Indicates that this acceleration structure should minimize the size of the scratch memory and the final result build, potentially at the expense of build time or trace performance.

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

XII_DECLARE_FLAGS_OPERATORS(xiiGALRayTracingBuildASFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALRayTracingBuildASFlags);

/// This describes the bottom level acceleration structure triangles.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBLASTriangleDescription : public xiiHashableStruct<xiiGALBLASTriangleDescription>
{
  xiiString                m_sGeometryName;                                    ///< The geometry name used to map triangle data. The default is an empty string view.
  xiiUInt32                m_uiMaxVertexCount = 0U;                            ///< The maximum vertex count in this geometry. The default is 0.
  xiiEnum<xiiGALValueType> m_VertexValueType  = xiiGALValueType::Undefined;    ///< The type of vertices in this geometry. The default is Undefined.
                                                                               ///<
                                                                               ///< \remarks Only Float32, Float16, and Int16 are valid. Int16 defines 16-bit signed-normalized vertex components.
  xiiUInt8 m_uiVertexComponentCount = 0U;                                      ///< The number of components in the vertex. The default is 0.
                                                                               ///<
                                                                               ///< \remarks Valid values are only 2 or 3. For 2-component formats, the third component is assumed to be 0.
  xiiUInt32                m_uiMaxPrimitiveCount = 0U;                         ///< The maximum primitive count in this geometry. The default is 0.
  xiiEnum<xiiGALValueType> m_IndexType           = xiiGALValueType::Undefined; ///< The index type of this geometry. This must be UInt16, UInt32 or Undefined. If Undefined, then the vertex array is used instead of the indexed vertices. The default is Undefined.
};

/// This describes the bottom level acceleration structure axis-aligned bounding box.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBLASBoundingBoxDescription : public xiiHashableStruct<xiiGALBLASBoundingBoxDescription>
{
  xiiString m_sGeometryName;      ///< The geometry name. This is used to map axis-aligned bounding box data to this geometry. The default is an empty string view.
  xiiUInt32 m_uiMaxBoxCount = 0U; ///< The maximum axis-aligned bounding box (AABB) count. The default is 0.
};

/// This describes bottom level acceleration structure creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBottomLevelASCreationDescription : public xiiHashableStruct<xiiGALBottomLevelASCreationDescription>
{
  xiiDynamicArray<xiiGALBLASTriangleDescription>    m_Triangles;                                            ///< Array of triangle geometry descriptions.
  xiiDynamicArray<xiiGALBLASBoundingBoxDescription> m_BoundingBoxes;                                        ///< Array of AABB geometry descriptions.
  xiiBitflags<xiiGALRayTracingBuildASFlags>         m_BuildASFlags    = xiiGALRayTracingBuildASFlags::None; ///< Ray tracing build flags. The default is None.
  xiiUInt64                                         m_uiCompactedSize = 0U;                                 ///< Compacted size, if this acceleration structure will be the target of a compacted copy operation. The default is 0.
};

/// This describes the scratch buffer information for the acceleration structure.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALScratchBufferSizeDescription : public xiiHashableStruct<xiiGALScratchBufferSizeDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt64 m_uiBuild  = 0U; ///< Scratch buffer size for acceleration structure building. May be zero if the acceleration structure was created with a non-zero compacted size. The default is 0.
  xiiUInt64 m_uiUpdate = 0U; ///< Scratch buffer size for acceleration structure updating. May be zero if acceleration structure was created without raytracing build allow update flag or with a non-zero compacted size. The default is 0.
};

/// Interface that defines methods to manipulate a bottom level acceleration structure (BLAS) object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALBottomLevelAS : public xiiGALResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBottomLevelAS, xiiGALResource);

public:
  /// This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALBottomLevelASCreationDescription& GetDescription() const { return m_Description; }

  /// This returns the geometry description index in the BLAS triangle or axis-aligned bounding box descriptions.
  ///
  /// \param sName - The geometry name that is specified in the BLAS triangle or axis-aligned bounding box descriptions.
  ///
  /// \return The geometry the index or xiiInvalidIndex if the geometry does not exist.
  ///
  /// \note Access to the BLAS must be externally synchronized.
  [[nodiscard]] xiiUInt32 GetGeometryDescriptionIndex(xiiStringView sName) const;

  /// This returns the geometry index that can be used in a shader binding table.
  ///
  /// \param sName - The geometry name that is specified in the BLAS triangle or axis-aligned bounding box descriptions.
  ///
  /// \return The geometry index or xiiInvalidIndex if the geometry does not exist.
  ///
  /// \note Access to the BLAS must be externally synchronized.
  [[nodiscard]] xiiUInt32 GetGeometryIndex(xiiStringView sName) const;

  /// This returns the geometry count that was used to build the acceleration structure.
  ///
  /// \return The number of geometries that was used to build the acceleration structure.
  ///
  /// \note Access to the BLAS must be externally synchronized.
  [[nodiscard]] xiiUInt32 GetActualGeometryCount() const;

  /// This returns the scratch buffer information for the current acceleration structure.
  ///
  /// \return The scratch buffer size description, see xiiGALScratchBufferSizeDescription.
  [[nodiscard]] const xiiGALScratchBufferSizeDescription& GetScratchBufferSizeDescription() const;

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALBottomLevelAS(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALBottomLevelASCreationDescription& creationDescription);

  virtual ~xiiGALBottomLevelAS();

  virtual xiiResult InitPlatform() = 0;

protected:
  xiiGALBottomLevelASCreationDescription m_Description;
  xiiGALScratchBufferSizeDescription     m_ScratchBufferSizeDescription;
  xiiHashTable<xiiStringView, xiiUInt32> m_NameToIndex;
};
