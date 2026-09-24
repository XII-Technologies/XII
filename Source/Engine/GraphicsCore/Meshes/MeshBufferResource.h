/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <GraphicsFoundation/Resources/Buffer.h>

class xiiGeometry;

/// Input layout semantic for mesh vertex streams, used to identify the purpose of a vertex stream and how it should be interpreted by the renderer.
struct XII_GRAPHICSCORE_DLL xiiMeshVertexSemantic
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Position = 0U, ///< The vertex position stream, expected to be present in all meshes.
    Normal,        ///< The vertex normal stream, used for lighting calculations and expected to be present in most meshes.
    Tangent,       ///< The vertex tangent stream, used for normal mapping and expected to be present in most meshes.
    TexCoord0,     ///< The first vertex texture coordinate stream, used for UV mapping and expected to be present in most meshes.
    TexCoord1,     ///< The second vertex texture coordinate stream, used for lightmaps or additional UV sets and expected to be present in some meshes.
    Color0,        ///< The vertex color stream, used for vertex coloring and expected to be present in some meshes.
    BoneIndices0,  ///< The vertex bone index stream, used for skinning and expected to be present in skinned meshes.
    BoneWeights0,  ///< The vertex bone weight stream, used for skinning and expected to be present in skinned meshes.
    Custom0,       ///< Custom vertex stream semantic, used for user-defined vertex data and may be present in any mesh.
    Custom1,       ///< Custom vertex stream semantic, used for user-defined vertex data and may be present in any mesh.
    Custom2,       ///< Custom vertex stream semantic, used for user-defined vertex data and may be present in any mesh.
    Custom3,       ///< Custom vertex stream semantic, used for user-defined vertex data and may be present in any mesh.

    ENUM_COUNT,

    Default = Position
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMeshVertexSemantic);

/// Precision and storage format for mesh vertex streams, used to identify the data type and layout of a vertex stream and how it should be interpreted by the renderer.
struct XII_GRAPHICSCORE_DLL xiiMeshVertexStreamFormat
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Float1 = 0U,       ///< Single-component floating point format, used for simple vertex data like scalar values or single texture coordinates.
    Float2,            ///< Two-component floating point format, used for vertex data like texture coordinates or 2D vectors.
    Float3,            ///< Three-component floating point format, used for vertex data like positions, normals, or 3D vectors.
    Float4,            ///< Four-component floating point format, used for vertex data like tangents, colors, or 4D vectors.
    UByte4Normalized,  ///< Four-component unsigned byte format with normalized values, used for compact vertex data like colors or bone weights where values are expected to be in the [0, 1] range.
    UShort4,           ///< Four-component unsigned short format, used for vertex data like bone indices where compact integer storage is sufficient and values are not normalized.
    UShort4Normalized, ///< Four-component unsigned short format with normalized values, used for vertex data like bone weights where compact integer storage is sufficient and values are expected to be in the [0, 1] range.
    UInt,              ///< Single unsigned integer format, used for vertex data like object IDs or other per-vertex integer data.

    ENUM_COUNT,

    Default = Float3
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMeshVertexStreamFormat);

/// CPU/GPU vertex layout entry for xiiMeshBufferResource.
///
/// This describes the layout of a single vertex stream, including its semantic meaning, data format, byte offset from the start of the vertex, and stride between vertices.
struct XII_GRAPHICSCORE_DLL xiiMeshVertexStream
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiMeshVertexSemantic>     m_Semantic;      ///< The semantic meaning of this vertex stream, used to identify how the renderer should interpret the data in this stream.
  xiiEnum<xiiMeshVertexStreamFormat> m_Format;        ///< The data format of this vertex stream, used to identify the type and layout of the vertex data for correct interpretation by the renderer.
  xiiUInt16                          m_uiOffset = 0U; ///< The byte offset of this vertex stream from the start of the vertex, used to calculate the memory address of this stream's data for each vertex.
  xiiUInt16                          m_uiStride = 0U; ///< The byte stride between vertices for this vertex stream, used to calculate the memory address of this stream's data for each vertex when vertices are tightly packed or interleaved.

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

/// Full fidelity authoring vertex used by AddCommonStreams() and geometry conversion.
///
/// The renderer is expected to read the declared stream layout. Mesh shader paths usually bind this
/// as a structured SRV, while fallback vertex pipelines can bind the same buffer as a vertex buffer.
struct XII_GRAPHICSCORE_DLL xiiMeshPackedVertex
{
  XII_DECLARE_POD_TYPE();

  xiiVec3          m_vPosition     = xiiVec3::MakeZero();                  ///< The position of this vertex in object space.
  xiiVec3          m_vNormal       = xiiVec3(0.0f, 0.0f, 1.0f);            ///< The normal vector of this vertex.
  xiiVec4          m_vTangent      = xiiVec4(1.0f, 0.0f, 0.0f, 1.0f);      ///< The tangent vector of this vertex.
  xiiVec2          m_vTexCoord0    = xiiVec2(0.0f);                        ///< The first texture coordinate of this vertex.
  xiiVec2          m_vTexCoord1    = xiiVec2(0.0f);                        ///< The second texture coordinate of this vertex.
  xiiColorLinearUB m_Color0        = xiiColorLinearUB(255, 255, 255, 255); ///< The color of this vertex.
  xiiVec4U16       m_vBoneIndices0 = xiiVec4U16::MakeZero();               ///< The bone indices for skinning this vertex, where each component represents the index of a bone that influences this vertex.
  xiiColorLinearUB m_BoneWeights0  = xiiColorLinearUB(255, 0, 0, 0);       ///< The bone weights for skinning this vertex, where each component represents the weight of the corresponding bone index in m_vBoneIndices0, normalized to the [0, 1] range and packed into an unsigned byte format for compact storage.
};

/// GPU-visible meshlet header.
///
/// The first four fields are laid out for compact structured-buffer consumption. The bounds and cone
/// are duplicated from authoring data so compute culling and mesh shaders do not need CPU-side lookups.
struct XII_GRAPHICSCORE_DLL xiiMeshlet
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32         m_uiFirstPrimitive       = 0U;                            ///< The index of the first primitive in this meshlet, used to identify the starting point of the primitive data for this meshlet in the meshlet primitive index buffer.
  xiiUInt32         m_uiVertexRemapOffset    = 0U;                            ///< The byte offset into the meshlet vertex remap buffer where the vertex remapping data for this meshlet begins, used to identify the starting point of the vertex remapping data for this meshlet.
  xiiUInt32         m_uiPrimitiveIndexOffset = 0U;                            ///< The byte offset into the meshlet primitive index buffer where the primitive index data for this meshlet begins, used to identify the starting point of the primitive index data for this meshlet.
  xiiUInt32         m_uiMaterialIndex        = 0U;                            ///< The index of the material for this meshlet, used to identify which material should be applied to the primitives in this meshlet during rendering.
  xiiUInt16         m_uiPrimitiveCount       = 0U;                            ///< The number of primitives in this meshlet, used to identify how many primitives belong to this meshlet for rendering and culling purposes.
  xiiUInt16         m_uiVertexCount          = 0U;                            ///< The number of unique vertices in this meshlet, used to identify how many unique vertices are referenced by the primitives in this meshlet for rendering and culling purposes.
  xiiUInt16         m_uiLodIndex             = 0U;                            ///< The index of the LOD this meshlet belongs to, used to identify which level of detail this meshlet is part of for LOD-based rendering and culling.
  xiiUInt16         m_uiSectionIndex         = 0U;                            ///< The index of the section this meshlet belongs to, used to identify which section of the mesh this meshlet is part of for rendering and culling purposes.
  xiiBoundingSphere m_Bounds                 = xiiBoundingSphere::MakeZero(); ///< The bounding sphere of this meshlet, used for frustum culling and other spatial queries during rendering.
  xiiVec3           m_vConeAxis              = xiiVec3(0.0f, 0.0f, 1.0f);     ///< The axis of the normal cone for this meshlet, used for backface culling and other orientation-based culling techniques during rendering.
  float             m_fConeCutoff            = -1.0f;                         ///< The cutoff angle for the normal cone of this meshlet, where values in the range [-1, 1] represent the cosine of the cutoff angle and are used for backface culling and other orientation-based culling techniques during rendering. A value of -1 means no normal cone culling, while a value of 0 means a 90-degree cutoff angle.

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

/// DrawMeshIndirect-compatible command payload.
struct XII_GRAPHICSCORE_DLL xiiMeshDrawCommand
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiThreadGroupCountX = 0U; ///< The number of thread groups to dispatch in the X dimension for this draw command, used to identify how many thread groups should be dispatched for this draw call when using mesh shaders or compute shader-based rendering techniques.
  xiiUInt32 m_uiThreadGroupCountY = 1U; ///< The number of thread groups to dispatch in the Y dimension for this draw command, used to identify how many thread groups should be dispatched for this draw call when using mesh shaders or compute shader-based rendering techniques. This is typically set to 1 for non-instanced draws, but can be greater than 1 for instanced draws where each thread group processes multiple instances.
  xiiUInt32 m_uiThreadGroupCountZ = 1U; ///< The number of thread groups to dispatch in the Z dimension for this draw command, used to identify how many thread groups should be dispatched for this draw call when using mesh shaders or compute shader-based rendering techniques. This is typically set to 1, but can be greater than 1 for certain advanced rendering techniques that utilize 3D thread group dispatch.

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

/// Per-instance data consumed by GPU culling and mesh shading passes.
struct XII_GRAPHICSCORE_DLL xiiMeshInstanceData
{
  XII_DECLARE_POD_TYPE();

  xiiMat4   m_GlobalTransform = xiiMat4::MakeIdentity(); ///< The global transformation matrix for this mesh instance, used to transform the vertices of this mesh instance from object space to world space during rendering and culling operations.
  xiiUInt32 m_uiObjectId      = 0U;                      ///< The object ID for this mesh instance, used to identify this instance in GPU-driven rendering techniques such as mesh shaders or compute shader-based rendering, where the object ID can be used for per-instance data lookups, material selection, or other instance-specific logic during rendering.
  xiiUInt32 m_uiMeshletOffset = 0U;                      ///< The byte offset into the meshlet buffer where the meshlet data for this instance begins, used to identify the starting point of the meshlet data for this instance in GPU-driven rendering techniques such as mesh shaders or compute shader-based rendering, where each instance may reference a different subset of meshlets for rendering.
  xiiUInt32 m_uiMeshletCount  = 0U;                      ///< The number of meshlets referenced by this instance, used to identify how many meshlets belong to this instance for rendering and culling purposes in GPU-driven rendering techniques such as mesh shaders or compute shader-based rendering, where each instance may reference a different subset of meshlets for rendering.
  xiiUInt32 m_uiMaterialBase  = 0U;                      ///< The base material index for this instance, used to identify the starting point of the material data for this instance in GPU-driven rendering techniques such as mesh shaders or compute shader-based rendering, where each instance may reference a different subset of materials for rendering.

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

/// Descriptor for creating an xiiMeshBufferResource, containing all necessary information and data to initialize a mesh buffer resource, including vertex stream layout, vertex and index data, meshlet data, and resource usage flags.
struct XII_GRAPHICSCORE_DLL xiiMeshBufferResourceDescriptor
{
public:
  /// Returns the vertex data for this mesh buffer resource descriptor, used to access the raw vertex data that will be uploaded to the GPU for rendering this mesh. The returned array pointer contains the vertex data in a tightly packed format based on the vertex stream layout and formats specified in this descriptor, and can be used for direct memory copying to GPU buffers or for CPU-side processing of the vertex data before uploading it to the GPU.
  xiiArrayPtr<xiiUInt8> GetVertexData();

  /// Returns the vertex data for this mesh buffer resource descriptor, used to access the raw vertex data that will be uploaded to the GPU for rendering this mesh. The returned array pointer contains the vertex data in a tightly packed format based on the vertex stream layout and formats specified in this descriptor, and can be used for direct memory copying to GPU buffers or for CPU-side processing of the vertex data before uploading it to the GPU.
  xiiArrayPtr<const xiiUInt8> GetVertexData() const;

  /// Returns the index data for this mesh buffer resource descriptor, used to access the raw index data that will be uploaded to the GPU for rendering this mesh. The returned array pointer contains the index data in a tightly packed format based on the index format specified in this descriptor, and can be used for direct memory copying to GPU buffers or for CPU-side processing of the index data before uploading it to the GPU.
  xiiArrayPtr<xiiUInt8> GetIndexData();

  /// Returns the index data for this mesh buffer resource descriptor, used to access the raw index data that will be uploaded to the GPU for rendering this mesh. The returned array pointer contains the index data in a tightly packed format based on the index format specified in this descriptor, and can be used for direct memory copying to GPU buffers or for CPU-side processing of the index data before uploading it to the GPU.
  xiiArrayPtr<const xiiUInt8> GetIndexData() const;

  /// Returns the vertex count for this mesh buffer resource descriptor, used to identify the number of vertices in this mesh and to calculate the required memory size for the vertex data based on the vertex stream layout and formats specified in this descriptor. The vertex count is typically determined by the number of unique vertices in the geometry data used to create this mesh buffer resource descriptor.
  xiiUInt32 GetVertexCount() const;

  /// Returns the index count for this mesh buffer resource descriptor, used to identify the number of indices in this mesh and to calculate the required memory size for the index data based on the index format specified in this descriptor. The index count is typically determined by the number of primitives (triangles, lines, etc.) in the geometry data used to create this mesh buffer resource descriptor, multiplied by the number of vertices per primitive (e.g., 3 for triangles).
  xiiUInt32 GetIndexCount() const;

  /// Returns the primitive count for this mesh buffer resource descriptor, used to identify the number of primitives (triangles, lines, etc.) in this mesh and to calculate the required memory size for the index data based on the index format specified in this descriptor. The primitive count is typically determined by the number of primitives in the geometry data used to create this mesh buffer resource descriptor.
  xiiUInt32 GetPrimitiveCount() const;

  /// Returns the vertex data size for this mesh buffer resource descriptor, used to identify the total memory size of the vertex data for this mesh based on the vertex count and the vertex stream layout and formats specified in this descriptor. The vertex data size is typically calculated as the vertex count multiplied by the vertex stride, where the vertex stride is determined by the sum of the sizes of all vertex streams in the vertex layout, taking into account any padding or alignment requirements based on the formats of the vertex streams.
  xiiUInt32 GetVertexDataSize() const;

  /// Returns the index data size for this mesh buffer resource descriptor, used to identify the total memory size of the index data for this mesh based on the index count and the index format specified in this descriptor. The index data size is typically calculated as the index count multiplied by the index stride, where the index stride is determined by the size of the index format (e.g., 2 bytes for 16-bit indices, 4 bytes for 32-bit indices).
  xiiUInt32 GetIndexDataSize() const;

  /// Returns the vertex stride for this mesh buffer resource descriptor, used to identify the byte stride between vertices in the vertex data based on the vertex stream layout and formats specified in this descriptor. The vertex stride is typically calculated as the sum of the sizes of all vertex streams in the vertex layout, taking into account any padding or alignment requirements based on the formats of the vertex streams.
  xiiUInt32 GetVertexStride() const;

  /// Returns the bounding box sphere for this mesh buffer resource descriptor, used to identify the spatial bounds of this mesh for frustum culling, collision detection, and other spatial queries during rendering. The bounding box sphere is typically calculated based on the vertex positions in the geometry data used to create this mesh buffer resource descriptor, and can be used for efficient culling and intersection tests during rendering.
  const xiiBoundingBoxSphere& GetBounds() const;

public:
  /// Default constructor.
  xiiMeshBufferResourceDescriptor();

  /// Clears all data from this descriptor, resetting it to an empty state.
  void Clear();

  /// Adds a vertex stream to this mesh buffer resource descriptor with the specified semantic, format, byte offset, and stride.
  ///
  /// \param semantic The semantic meaning of the vertex stream, used to identify how the renderer should interpret the data in this stream.
  /// \param format The data format of the vertex stream, used to identify the type and layout of the vertex data for correct interpretation by the renderer.
  /// \param uiOffset The byte offset of this vertex stream from the start of the vertex, used to calculate the memory address of this stream's data for each vertex.
  /// \param uiStride The byte stride between vertices for this vertex stream, used to calculate the memory address of this stream's data for each vertex when vertices are tightly packed or interleaved.
  void AddStream(xiiEnum<xiiMeshVertexSemantic> semantic, xiiEnum<xiiMeshVertexStreamFormat> format, xiiUInt16 uiOffset, xiiUInt16 uiStride);

  /// Adds a set of common vertex streams (position, normal, tangent, texcoord0, texcoord1, color0, bone indices, bone weights) to this mesh buffer resource descriptor with standard formats and offsets for use with the provided geometry.
  void AddCommonStreams();

  /// Allocates memory for the vertex and index data in this mesh buffer resource descriptor based on the specified vertex count, primitive count, and index format.
  ///
  /// \param uiVertexCount The number of vertices in the mesh, used to calculate the required memory size for the vertex data based on the vertex stream layout and formats.
  /// \param uiPrimitiveCount The number of primitives in the mesh, used to calculate the required memory size for the index data based on the index format.
  /// \param bUse32BitIndices Whether to use 32-bit unsigned integers for the index data, which allows for more than 65535 vertices but increases memory usage. If false, 16-bit unsigned integers will be used, which limits the maximum vertex count to 65535 but reduces memory usage.
  void AllocateStreams(xiiUInt32 uiVertexCount, xiiUInt32 uiPrimitiveCount, bool bUse32BitIndices = true);

  /// Allocates memory for the vertex and index data in this mesh buffer resource descriptor based on the provided geometry, including vertex count, primitive count, and index format inferred from the geometry data.
  ///
  /// \param geometry The geometry data used to determine the vertex count, primitive count, and index format for allocating the vertex and index streams in this mesh buffer resource descriptor. The vertex count is typically determined by the number of unique vertices in the geometry, while the primitive count is determined by the number of primitives (triangles, lines, etc.) in the geometry. The index format is determined based on whether the vertex count exceeds 65535, which would require 32-bit indices instead of 16-bit indices.
  /// \param topology The primitive topology of the geometry, used to determine how the primitives are defined and rendered (e.g., triangle list, triangle strip, line list, etc.) and may influence how the vertex and index data is organized in memory for optimal rendering performance.
  /// \param bBuildMeshlets Whether to build meshlets for this geometry, which can improve rendering performance by allowing for more efficient culling and draw call batching, but may increase memory usage and build time. If true, meshlets will be built based on the geometry data and included in this mesh buffer resource descriptor. If false, no meshlets will be built and the mesh will be rendered using traditional vertex and index buffers without meshlet-based culling or batching optimizations.
  void AllocateStreamsFromGeometry(const xiiGeometry& geometry, xiiEnum<xiiGALPrimitiveTopology> topology, bool bBuildMeshlets = true);

  /// Sets the vertex and index data for this mesh buffer resource descriptor, copying the provided data into the internal storage of this descriptor and updating the vertex count, index count, and vertex stride based on the provided data and the vertex stream layout and formats specified in this descriptor.
  ///
  /// \param pData The raw vertex data to be copied into this mesh buffer resource descriptor, formatted according to the vertex stream layout and formats specified in this descriptor. The data should be tightly packed based on the vertex stream layout, with each vertex's data organized according to the specified offsets and strides for each vertex stream. The size of the data should match the expected vertex data size based on the vertex count and vertex stride for this descriptor.
  /// \param uiVertexCount The number of vertices represented by the provided vertex data, used to update the vertex count for this mesh buffer resource descriptor and to validate that the size of the provided vertex data matches the expected vertex data size based on the vertex count and vertex stride for this descriptor.
  /// \param uiVertexStride The byte stride between vertices in the provided vertex data, used to validate that the vertex data is organized according to the vertex stream layout and formats specified in this descriptor, and to calculate the expected vertex data size based on the vertex count and vertex stride for this descriptor. The vertex stride should match the expected vertex stride calculated from the vertex stream layout and formats specified in this descriptor to ensure correct interpretation of the vertex data by the renderer.
  void SetVertexData(xiiArrayPtr<const xiiUInt8> pData, xiiUInt32 uiVertexCount, xiiUInt32 uiVertexStride);

  /// Sets the vertex and index data for this mesh buffer resource descriptor, copying the provided data into the internal storage of this descriptor and updating the vertex count, index count, and vertex stride based on the provided data and the vertex stream layout and formats specified in this descriptor.
  ///
  /// \param pData The raw index data to be copied into this mesh buffer resource descriptor, formatted according to the index format specified in this descriptor. The data should be tightly packed based on the index format, with each index represented as either a 16-bit unsigned integer or a 32-bit unsigned integer depending on the index format specified in this descriptor. The size of the data should match the expected index data size based on the index count and index stride for this descriptor.
  /// \param uiIndexCount The number of indices represented by the provided index data, used to update the index count for this mesh buffer resource descriptor and to validate that the size of the provided index data matches the expected index data size based on the index count and index stride for this descriptor.
  /// \param indexType The data format of the index data, used to validate that the index data is organized according to the index format specified in this descriptor, and to calculate the expected index data size based on the index count and index stride for this descriptor. The index type should match the expected index format specified in this descriptor (e.g., 16-bit unsigned integer or 32-bit unsigned integer) to ensure correct interpretation of the index data by the renderer.
  void SetIndexData(xiiArrayPtr<const xiiUInt8> pData, xiiUInt32 uiIndexCount, xiiEnum<xiiGALValueType> indexType);

  /// Builds meshlets for this mesh buffer resource descriptor based on the vertex and index data, vertex stream layout, and specified maximum vertices and primitives per meshlet. Meshlets are small clusters of geometry that can be efficiently culled and rendered by modern GPU pipelines, improving rendering performance by reducing draw calls and enabling more efficient culling. The meshlet building process typically involves clustering the geometry into meshlets based on spatial locality and other heuristics, and generating the necessary data for each meshlet such as vertex remapping, primitive indices, and material indices.
  ///
  /// \param uiMaxVertices The maximum number of unique vertices allowed in each meshlet, used to control the size of the meshlets and to ensure that the vertex remapping data for each meshlet can fit within the specified limits for efficient GPU processing. A common value for this parameter is 64, which allows for efficient GPU processing while still providing good culling granularity.
  /// \param uiMaxPrimitives The maximum number of primitives (triangles, lines, etc.) allowed in each meshlet, used to control the size of the meshlets and to ensure that the primitive index data for each meshlet can fit within the specified limits for efficient GPU processing. A common value for this parameter is 124, which allows for efficient GPU processing while still providing good culling granularity. The specific values for these parameters may need to be adjusted based on the target hardware capabilities and the complexity of the geometry being processed to achieve optimal rendering performance.
  void BuildMeshlets(xiiUInt32 uiMaxVertices = 64U, xiiUInt32 uiMaxPrimitives = 124U);

  /// Computes the bounding box sphere for this mesh buffer resource descriptor based on the vertex positions in the vertex data and the vertex stream layout specified in this descriptor. The bounding box sphere is typically calculated by iterating through the vertex positions in the vertex data, applying any necessary transformations based on the vertex stream layout, and computing the minimum enclosing sphere that contains all the vertex positions. This bounding box sphere can then be used for efficient frustum culling, collision detection, and other spatial queries during rendering.
  void ComputeBounds();

  /// Sets the bounding box sphere for this mesh buffer resource descriptor, allowing for manual specification of the spatial bounds of this mesh. This can be useful in cases where the bounding box sphere has been precomputed or optimized based on specific requirements, or when the vertex data is not available for computing the bounds directly. The provided bounding box sphere should encompass all the vertex positions in the geometry represented by this mesh buffer resource descriptor to ensure correct culling and spatial queries during rendering.
  ///
  /// \param bounds The bounding box sphere to be set for this mesh buffer resource descriptor, used to identify the spatial bounds of this mesh for frustum culling, collision detection, and other spatial queries during rendering. The bounding box sphere should be defined in object space and should encompass all the vertex positions in the geometry represented by this mesh buffer resource descriptor to ensure correct culling and spatial queries during rendering.
  void SetBounds(const xiiBoundingBoxSphere& bounds);

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

public:
  xiiEnum<xiiGALPrimitiveTopology> m_Topology            = xiiGALPrimitiveTopology::TriangleList; ///< The primitive topology for this mesh, used to identify how the primitives in this mesh are defined and rendered (e.g., triangle list, triangle strip, line list, etc.) and may influence how the vertex and index data is organized in memory for optimal rendering performance.
  xiiEnum<xiiGALValueType>         m_IndexType           = xiiGALValueType::UInt32;               ///< The data format of the index data for this mesh, used to identify how the index data is organized and interpreted by the renderer (e.g., 16-bit unsigned integer or 32-bit unsigned integer) and to calculate the expected index data size based on the index count and index stride for this descriptor.
  xiiEnum<xiiGALResourceUsage>     m_ResourceUsage       = xiiGALResourceUsage::Immutable;        ///< The resource usage pattern for this mesh buffer resource, used to identify how the vertex and index data will be used and updated during rendering (e.g., immutable, dynamic, staging, etc.) and to optimize the memory allocation and access patterns for the GPU based on the expected usage of this mesh.
  bool                             m_bKeepCpuMeshData    = false;                                 ///< Whether to keep a copy of the vertex and index data in CPU memory after uploading it to the GPU, which can be useful for CPU-side processing, collision detection, or other operations that require access to the mesh data on the CPU, but may increase memory usage. If true, a copy of the vertex and index data will be retained in CPU memory after uploading it to the GPU. If false, the vertex and index data will be released from CPU memory after uploading it to the GPU to reduce memory usage.
  bool                             m_bAllowGpuDrivenDraw = true;                                  ///< Whether to allow GPU-driven rendering techniques such as mesh shaders or compute shader-based rendering for this mesh, which can improve rendering performance by reducing draw calls and enabling more efficient culling, but may require additional GPU resources and may not be supported on all hardware. If true, this mesh buffer resource can be used with GPU-driven rendering techniques if the hardware supports it. If false, this mesh buffer resource will not be used with GPU-driven rendering techniques and will fall back to traditional vertex and index buffer rendering.
  bool                             m_bAllowCpuFallback   = true;                                  ///< Whether to allow CPU-based rendering fallback for this mesh, which can be useful for compatibility with older hardware or for debugging purposes, but may reduce rendering performance compared to GPU-driven rendering techniques. If true, this mesh buffer resource can be rendered using CPU-based rendering techniques if GPU-driven rendering is not available or not allowed. If false, this mesh buffer resource will not be rendered using CPU-based rendering techniques and may require GPU-driven rendering techniques to be enabled for rendering.

  xiiDynamicArray<xiiMeshVertexStream> m_VertexStreams; ///< The vertex stream layout for this mesh, used to identify the semantic meaning, data format, byte offset, and stride for each vertex stream in the vertex data, which is essential for correctly interpreting the vertex data and for configuring the vertex input layout for rendering this mesh.
  xiiDynamicArray<xiiUInt8>            m_VertexData;    ///< The raw vertex data for this mesh, formatted according to the vertex stream layout specified in m_VertexStreams. This data is typically tightly packed based on the vertex stream layout, with each vertex's data organized according to the specified offsets and strides for each vertex stream. This vertex data will be uploaded to the GPU for rendering this mesh, and can also be retained in CPU memory if m_bKeepCpuMeshData is true for CPU-side processing or other operations that require access to the mesh data on the CPU.
  xiiDynamicArray<xiiUInt8>            m_IndexData;     ///< The raw index data for this mesh, formatted according to the index format specified in m_IndexType. This data is typically tightly packed based on the index format, with each index represented as either a 16-bit unsigned integer or a 32-bit unsigned integer depending on the index format specified in m_IndexType. This index data will be uploaded to the GPU for rendering this mesh, and can also be retained in CPU memory if m_bKeepCpuMeshData is true for CPU-side processing or other operations that require access to the mesh data on the CPU.

  xiiDynamicArray<xiiMeshlet>         m_Meshlets;                ///< The meshlet data for this mesh, used for GPU-driven rendering techniques such as mesh shaders or compute shader-based rendering to enable more efficient culling and draw call batching. Each meshlet contains information about the primitives and vertices it contains, as well as its bounding sphere and normal cone for culling purposes. The meshlet data will be uploaded to the GPU for use in rendering this mesh with GPU-driven techniques, and can also be retained in CPU memory if m_bKeepCpuMeshData is true for CPU-side processing or other operations that require access to the meshlet data on the CPU.
  xiiDynamicArray<xiiUInt32>          m_MeshletVertexRemap;      ///< The vertex remapping data for the meshlets in this mesh, used to remap the vertex indices for each meshlet to a compact set of unique vertices that are referenced by the primitives in that meshlet. This data is essential for GPU-driven rendering techniques such as mesh shaders or compute shader-based rendering, where each meshlet may reference a different subset of vertices for optimal culling and rendering performance. The vertex remapping data will be uploaded to the GPU for use in rendering this mesh with GPU-driven techniques, and can also be retained in CPU memory if m_bKeepCpuMeshData is true for CPU-side processing or other operations that require access to the vertex remapping data on the CPU.
  xiiDynamicArray<xiiUInt8>           m_MeshletPrimitiveIndices; ///< The primitive index data for the meshlets in this mesh, used to identify the primitives (triangles, lines, etc.) that belong to each meshlet for rendering and culling purposes in GPU-driven rendering techniques such as mesh shaders or compute shader-based rendering. This data is essential for GPU-driven rendering techniques, where each meshlet may reference a different subset of primitives for optimal culling and rendering performance. The primitive index data will be uploaded to the GPU for use in rendering this mesh with GPU-driven techniques, and can also be retained in CPU memory if m_bKeepCpuMeshData is true for CPU-side processing or other operations that require access to the primitive index data on the CPU.
  xiiDynamicArray<xiiUInt32>          m_MeshletMaterialIndices;  ///< The material index data for the meshlets in this mesh, used to identify the material that should be applied to each meshlet during rendering in GPU-driven rendering techniques such as mesh shaders or compute shader-based rendering. This data is essential for GPU-driven rendering techniques, where each meshlet may reference a different material for optimal rendering performance. The material index data will be uploaded to the GPU for use in rendering this mesh with GPU-driven techniques, and can also be retained in CPU memory if m_bKeepCpuMeshData is true for CPU-side processing or other operations that require access to the material index data on the CPU.
  xiiDynamicArray<xiiMeshDrawCommand> m_DrawCommands;            ///< The draw command data for this mesh, used for GPU-driven rendering techniques such as mesh shaders or compute shader-based rendering to specify the number of thread groups to dispatch for rendering this mesh. Each draw command contains information about the thread group counts in the X, Y, and Z dimensions, which are essential for configuring the dispatch parameters for rendering this mesh with GPU-driven techniques. The draw command data will be uploaded to the GPU for use in rendering this mesh with GPU-driven techniques, and can also be retained in CPU memory if m_bKeepCpuMeshData is true for CPU-side processing or other operations that require access to the draw command data on the CPU.

private:
  xiiUInt32            m_uiVertexCount  = 0U;                                  ///< The number of vertices in this mesh, used to identify the vertex count for this mesh and to calculate the required memory size for the vertex data based on the vertex stream layout and formats specified in this descriptor. The vertex count is typically determined by the number of unique vertices in the geometry data used to create this mesh buffer resource descriptor.
  xiiUInt32            m_uiIndexCount   = 0U;                                  ///< The number of indices in this mesh, used to identify the index count for this mesh and to calculate the required memory size for the index data based on the index format specified in this descriptor. The index count is typically determined by the number of primitives (triangles, lines, etc.) in the geometry data used to create this mesh buffer resource descriptor, multiplied by the number of vertices per primitive (e.g., 3 for triangles).
  xiiUInt32            m_uiVertexStride = 0U;                                  ///< The byte stride between vertices in the vertex data for this mesh, used to identify the vertex stride for this mesh and to calculate the expected vertex data size based on the vertex count and vertex stream layout and formats specified in this descriptor. The vertex stride is typically calculated as the sum of the sizes of all vertex streams in the vertex layout, taking into account any padding or alignment requirements based on the formats of the vertex streams.
  xiiBoundingBoxSphere m_Bounds         = xiiBoundingBoxSphere::MakeInvalid(); ///< The bounding box sphere for this mesh, used to identify the spatial bounds of this mesh for frustum culling, collision detection, and other spatial queries during rendering. The bounding box sphere is typically calculated based on the vertex positions in the geometry data used to create this mesh buffer resource descriptor, and can be used for efficient culling and intersection tests during rendering.
};

/// Resource class for mesh buffers, responsible for managing the GPU resources and data associated with a mesh, including vertex and index buffers, meshlet buffers, and draw command buffers. This resource class handles the loading, unloading, and updating of the mesh data on the GPU, as well as providing access to the mesh data and properties for rendering and other operations.
class XII_GRAPHICSCORE_DLL xiiMeshBufferResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshBufferResource, xiiResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiMeshBufferResource);

  XII_RESOURCE_DECLARE_CREATEABLE(xiiMeshBufferResource, xiiMeshBufferResourceDescriptor);

public:
  xiiMeshBufferResource();
  ~xiiMeshBufferResource();

  /// Returns the mesh buffer resource descriptor for this mesh buffer resource, used to identify the properties and data of this mesh buffer resource for rendering and other operations. The mesh buffer resource descriptor contains information about the vertex stream layout, vertex and index data, meshlet data, and other properties of this mesh buffer resource that are essential for correctly interpreting the mesh data and for configuring the rendering pipeline for this mesh.
  const xiiMeshBufferResourceDescriptor& GetDescriptor() const;

  /// Returns the bounding box sphere for this mesh buffer resource, used to identify the spatial bounds of this mesh for frustum culling, collision detection, and other spatial queries during rendering. The bounding box sphere is typically calculated based on the vertex positions in the geometry data used to create this mesh buffer resource, and can be used for efficient culling and intersection tests during rendering.
  const xiiBoundingBoxSphere& GetBounds() const;

  /// Returns the vertex count for this mesh buffer resource, used to identify the number of vertices in this mesh and to calculate the required memory size for the vertex data based on the vertex stream layout and formats specified in this resource. The vertex count is typically determined by the number of unique vertices in the geometry data used to create this mesh buffer resource.
  xiiUInt32 GetVertexCount() const;

  /// Returns the index count for this mesh buffer resource, used to identify the number of indices in this mesh and to calculate the required memory size for the index data based on the index format specified in this resource. The index count is typically determined by the number of primitives (triangles, lines, etc.) in the geometry data used to create this mesh buffer resource, multiplied by the number of vertices per primitive (e.g., 3 for triangles).
  xiiUInt32 GetIndexCount() const;

  /// Returns the primitive count for this mesh buffer resource, used to identify the number of primitives (triangles, lines, etc.) in this mesh and to calculate the required memory size for the index data based on the index format specified in this resource. The primitive count is typically determined by the number of primitives in the geometry data used to create this mesh buffer resource.
  xiiUInt32 GetPrimitiveCount() const;

  /// Returns the meshlet count for this mesh buffer resource, used to identify the number of meshlets in this mesh and to calculate the required memory size for the meshlet data based on the maximum vertices and primitives per meshlet specified in the mesh buffer resource descriptor. The meshlet count is typically determined by the number of meshlets generated from the geometry data used to create this mesh buffer resource, based on the specified maximum vertices and primitives per meshlet.
  xiiUInt32 GetMeshletCount() const;

  /// Returns the vertex stride for this mesh buffer resource, used to identify the byte stride between vertices in the vertex data based on the vertex stream layout and formats specified in this resource. The vertex stride is typically calculated as the sum of the sizes of all vertex streams in the vertex layout, taking into account any padding or alignment requirements based on the formats of the vertex streams.
  xiiUInt32 GetVertexStride() const;

  /// Returns the primitive topology for this mesh buffer resource, used to identify how the primitives in this mesh are defined and rendered (e.g., triangle list, triangle strip, line list, etc.) and may influence how the vertex and index data is organized in memory for optimal rendering performance.
  xiiEnum<xiiGALPrimitiveTopology> GetTopology() const;

  /// Returns the index format for this mesh buffer resource, used to identify how the index data is organized and interpreted by the renderer (e.g., 16-bit unsigned integer or 32-bit unsigned integer) and to calculate the expected index data size based on the index count and index stride for this resource.
  xiiEnum<xiiGALValueType> GetIndexType() const;

  /// Returns the vertex stream layout for this mesh buffer resource, used to identify the semantic meaning, data format, byte offset, and stride for each vertex stream in the vertex data, which is essential for correctly interpreting the vertex data and for configuring the vertex input layout for rendering this mesh.
  xiiArrayPtr<const xiiMeshVertexStream> GetVertexStreams() const;

  /// Returns the meshlet data for this mesh buffer resource, used for GPU-driven rendering techniques such as mesh shaders or compute shader-based rendering to enable more efficient culling and draw call batching. Each meshlet contains information about the primitives and vertices it contains, as well as its bounding sphere and normal cone for culling purposes. The meshlet data will be uploaded to the GPU for use in rendering this mesh with GPU-driven techniques, and can also be retained in CPU memory if m_bKeepCpuMeshData is true for CPU-side processing or other operations that require access to the meshlet data on the CPU.
  xiiArrayPtr<const xiiMeshlet> GetMeshlets() const;

  /// Returns the vertex remapping data for the meshlets in this mesh buffer resource, used to remap the vertex indices for each meshlet to a compact set of unique vertices that are referenced by the primitives in that meshlet. This data is essential for GPU-driven rendering techniques such as mesh shaders or compute shader-based rendering, where each meshlet may reference a different subset of vertices for optimal culling and rendering performance. The vertex remapping data will be uploaded to the GPU for use in rendering this mesh with GPU-driven techniques, and can also be retained in CPU memory if m_bKeepCpuMeshData is true for CPU-side processing or other operations that require access to the vertex remapping data on the CPU.
  xiiArrayPtr<const xiiUInt32> GetMeshletVertexRemap() const;

  /// Returns the primitive index data for the meshlets in this mesh buffer resource, used to identify the primitives (triangles, lines, etc.) that belong to each meshlet for rendering and culling purposes in GPU-driven rendering techniques such as mesh shaders or compute shader-based rendering. This data is essential for GPU-driven rendering techniques, where each meshlet may reference a different subset of primitives for optimal culling and rendering performance. The primitive index data will be uploaded to the GPU for use in rendering this mesh with GPU-driven techniques, and can also be retained in CPU memory if m_bKeepCpuMeshData is true for CPU-side processing or other operations that require access to the primitive index data on the CPU.
  xiiArrayPtr<const xiiUInt8> GetMeshletPrimitiveIndices() const;

  /// Returns the GPU vertex buffer for this mesh buffer resource, used to identify the GPU resource that contains the vertex data for this mesh and to bind it for rendering. The vertex buffer is typically created from the vertex data in the mesh buffer resource descriptor and uploaded to the GPU for efficient rendering of this mesh.
  xiiSharedPtr<xiiGALBuffer> GetVertexBuffer() const;

  /// Returns the GPU index buffer for this mesh buffer resource, used to identify the GPU resource that contains the index data for this mesh and to bind it for rendering. The index buffer is typically created from the index data in the mesh buffer resource descriptor and uploaded to the GPU for efficient rendering of this mesh.
  xiiSharedPtr<xiiGALBuffer> GetIndexBuffer() const;

  /// Returns the GPU meshlet buffer for this mesh buffer resource, used to identify the GPU resource that contains the meshlet data for this mesh and to bind it for rendering with GPU-driven techniques such as mesh shaders or compute shader-based rendering. The meshlet buffer is typically created from the meshlet data in the mesh buffer resource descriptor and uploaded to the GPU for efficient rendering of this mesh with GPU-driven techniques.
  xiiSharedPtr<xiiGALBuffer> GetMeshletBuffer() const;

  /// Returns the GPU vertex remapping buffer for the meshlets in this mesh buffer resource, used to identify the GPU resource that contains the vertex remapping data for the meshlets in this mesh and to bind it for rendering with GPU-driven techniques such as mesh shaders or compute shader-based rendering. The vertex remapping buffer is typically created from the vertex remapping data in the mesh buffer resource descriptor and uploaded to the GPU for efficient rendering of this mesh with GPU-driven techniques.
  xiiSharedPtr<xiiGALBuffer> GetMeshletVertexRemapBuffer() const;

  /// Returns the GPU primitive index buffer for the meshlets in this mesh buffer resource, used to identify the GPU resource that contains the primitive index data for the meshlets in this mesh and to bind it for rendering with GPU-driven techniques such as mesh shaders or compute shader-based rendering. The primitive index buffer is typically created from the primitive index data in the mesh buffer resource descriptor and uploaded to the GPU for efficient rendering of this mesh with GPU-driven techniques.
  xiiSharedPtr<xiiGALBuffer> GetMeshletPrimitiveIndexBuffer() const;

  /// Returns the GPU draw command buffer for this mesh buffer resource, used to identify the GPU resource that contains the draw command data for this mesh and to bind it for rendering with GPU-driven techniques such as mesh shaders or compute shader-based rendering. The draw command buffer is typically created from the draw command data in the mesh buffer resource descriptor and uploaded to the GPU for efficient rendering of this mesh with GPU-driven techniques.
  xiiSharedPtr<xiiGALBuffer> GetDrawCommandBuffer() const;

private:
  virtual xiiResourceLoadDescription UnloadData(Unload whatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  virtual void                       UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void CreateGpuBuffer(xiiSharedPtr<xiiGALBuffer>& out_pBuffer, xiiArrayPtr<const xiiUInt8> pData, xiiUInt32 uiStride, xiiBitflags<xiiGALBindFlags> bindFlags, xiiStringView sDebugName);
  void CreateGpuBuffer(xiiSharedPtr<xiiGALBuffer>& out_pBuffer, xiiArrayPtr<const xiiUInt32> pData, xiiUInt32 uiStride, xiiBitflags<xiiGALBindFlags> bindFlags, xiiStringView sDebugName);
  void CreateGpuBuffer(xiiSharedPtr<xiiGALBuffer>& out_pBuffer, xiiArrayPtr<const xiiMeshlet> pData, xiiUInt32 uiStride, xiiBitflags<xiiGALBindFlags> bindFlags, xiiStringView sDebugName);
  void CreateGpuBuffer(xiiSharedPtr<xiiGALBuffer>& out_pBuffer, xiiArrayPtr<const xiiMeshDrawCommand> pData, xiiUInt32 uiStride, xiiBitflags<xiiGALBindFlags> bindFlags, xiiStringView sDebugName);

  xiiMeshBufferResourceDescriptor m_Descriptor; ///< The mesh buffer resource descriptor for this mesh buffer resource, used to identify the properties and data of this mesh buffer resource for rendering and other operations. The mesh buffer resource descriptor contains information about the vertex stream layout, vertex and index data, meshlet data, and other properties of this mesh buffer resource that are essential for correctly interpreting the mesh data and for configuring the rendering pipeline for this mesh.

  xiiSharedPtr<xiiGALBuffer> m_pVertexBuffer;                ///< The GPU vertex buffer for this mesh buffer resource, used to identify the GPU resource that contains the vertex data for this mesh and to bind it for rendering. The vertex buffer is typically created from the vertex data in the mesh buffer resource descriptor and uploaded to the GPU for efficient rendering of this mesh.
  xiiSharedPtr<xiiGALBuffer> m_pIndexBuffer;                 ///< The GPU index buffer for this mesh buffer resource, used to identify the GPU resource that contains the index data for this mesh and to bind it for rendering. The index buffer is typically created from the index data in the mesh buffer resource descriptor and uploaded to the GPU for efficient rendering of this mesh.
  xiiSharedPtr<xiiGALBuffer> m_pMeshletBuffer;               ///< The GPU meshlet buffer for this mesh buffer resource, used to identify the GPU resource that contains the meshlet data for this mesh and to bind it for rendering with GPU-driven techniques such as mesh shaders or compute shader-based rendering. The meshlet buffer is typically created from the meshlet data in the mesh buffer resource descriptor and uploaded to the GPU for efficient rendering of this mesh with GPU-driven techniques.
  xiiSharedPtr<xiiGALBuffer> m_pMeshletVertexRemapBuffer;    ///< The GPU vertex remapping buffer for the meshlets in this mesh buffer resource, used to identify the GPU resource that contains the vertex remapping data for the meshlets in this mesh and to bind it for rendering with GPU-driven techniques such as mesh shaders or compute shader-based rendering. The vertex remapping buffer is typically created from the vertex remapping data in the mesh buffer resource descriptor and uploaded to the GPU for efficient rendering of this mesh with GPU-driven techniques.
  xiiSharedPtr<xiiGALBuffer> m_pMeshletPrimitiveIndexBuffer; ///< The GPU primitive index buffer for the meshlets in this mesh buffer resource, used to identify the GPU resource that contains the primitive index data for the meshlets in this mesh and to bind it for rendering with GPU-driven techniques such as mesh shaders or compute shader-based rendering. The primitive index buffer is typically created from the primitive index data in the mesh buffer resource descriptor and uploaded to the GPU for efficient rendering of this mesh with GPU-driven techniques.
  xiiSharedPtr<xiiGALBuffer> m_pDrawCommandBuffer;           ///< The GPU draw command buffer for this mesh buffer resource, used to identify the GPU resource that contains the draw command data for this mesh and to bind it for rendering with GPU-driven techniques such as mesh shaders or compute shader-based rendering. The draw command buffer is typically created from the draw command data in the mesh buffer resource descriptor and uploaded to the GPU for efficient rendering of this mesh with GPU-driven techniques.

  xiiUInt64 m_uiMemoryGPU = 0U; ///< The amount of GPU memory used by this mesh buffer resource, used to track the memory usage of this resource on the GPU for budgeting and optimization purposes. The GPU memory usage is typically calculated based on the sizes of the vertex buffer, index buffer, meshlet buffer, vertex remapping buffer, primitive index buffer, and draw command buffer for this mesh buffer resource.
};
