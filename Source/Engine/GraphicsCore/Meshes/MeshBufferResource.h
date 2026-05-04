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

/// \brief Vertex attributes understood by the high level mesh system.
struct XII_GRAPHICSCORE_DLL xiiMeshVertexSemantic
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Position,
    Normal,
    Tangent,
    TexCoord0,
    TexCoord1,
    Color0,
    BoneIndices0,
    BoneWeights0,
    Custom0,
    Custom1,
    Custom2,
    Custom3,

    ENUM_COUNT,

    Default = Position
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMeshVertexSemantic);

/// \brief Precision/storage policy for a vertex stream.
struct XII_GRAPHICSCORE_DLL xiiMeshVertexStreamFormat
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Float1,
    Float2,
    Float3,
    Float4,
    UByte4Normalized,
    UShort4,
    UShort4Normalized,
    UInt,

    ENUM_COUNT,

    Default = Float3
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMeshVertexStreamFormat);

/// \brief CPU/GPU vertex layout entry for xiiMeshBufferResource.
struct XII_GRAPHICSCORE_DLL xiiMeshVertexStream
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiMeshVertexSemantic>     m_Semantic;
  xiiEnum<xiiMeshVertexStreamFormat> m_Format;
  xiiUInt16                          m_uiOffset = 0U;
  xiiUInt16                          m_uiStride = 0U;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

/// \brief Full fidelity authoring vertex used by AddCommonStreams() and geometry conversion.
///
/// The renderer is expected to read the declared stream layout. Mesh shader paths usually bind this
/// as a structured SRV, while fallback vertex pipelines can bind the same buffer as a vertex buffer.
struct XII_GRAPHICSCORE_DLL xiiMeshPackedVertex
{
  XII_DECLARE_POD_TYPE();

  xiiVec3          m_vPosition       = xiiVec3::MakeZero();
  xiiVec3          m_vNormal         = xiiVec3(0.0f, 0.0f, 1.0f);
  xiiVec4          m_vTangent        = xiiVec4(1.0f, 0.0f, 0.0f, 1.0f);
  xiiVec2          m_vTexCoord0      = xiiVec2(0.0f);
  xiiVec2          m_vTexCoord1      = xiiVec2(0.0f);
  xiiColorLinearUB m_Color0          = xiiColorLinearUB(255, 255, 255, 255);
  xiiVec4U16       m_vBoneIndices0   = xiiVec4U16::MakeZero();
  xiiColorLinearUB m_BoneWeights0    = xiiColorLinearUB(255, 0, 0, 0);
};

/// \brief GPU-visible meshlet header.
///
/// The first four fields are laid out for compact structured-buffer consumption. The bounds and cone
/// are duplicated from authoring data so compute culling and mesh shaders do not need CPU-side lookups.
struct XII_GRAPHICSCORE_DLL xiiMeshlet
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32         m_uiFirstPrimitive        = 0U;
  xiiUInt32         m_uiVertexRemapOffset     = 0U;
  xiiUInt32         m_uiPrimitiveIndexOffset  = 0U;
  xiiUInt32         m_uiMaterialIndex         = 0U;
  xiiUInt16         m_uiPrimitiveCount        = 0U;
  xiiUInt16         m_uiVertexCount           = 0U;
  xiiUInt16         m_uiLodIndex              = 0U;
  xiiUInt16         m_uiSectionIndex          = 0U;
  xiiBoundingSphere m_Bounds                  = xiiBoundingSphere::MakeZero();
  xiiVec3           m_vConeAxis               = xiiVec3(0.0f, 0.0f, 1.0f);
  float             m_fConeCutoff             = -1.0f;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

/// \brief DrawMeshIndirect-compatible command payload.
struct XII_GRAPHICSCORE_DLL xiiMeshDrawCommand
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiThreadGroupCountX = 0U;
  xiiUInt32 m_uiThreadGroupCountY = 1U;
  xiiUInt32 m_uiThreadGroupCountZ = 1U;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

/// \brief Per-instance data consumed by GPU culling and mesh shading passes.
struct XII_GRAPHICSCORE_DLL xiiMeshInstanceData
{
  XII_DECLARE_POD_TYPE();

  xiiMat4   m_GlobalTransform = xiiMat4::MakeIdentity();
  xiiUInt32 m_uiObjectId      = 0U;
  xiiUInt32 m_uiMeshletOffset = 0U;
  xiiUInt32 m_uiMeshletCount  = 0U;
  xiiUInt32 m_uiMaterialBase  = 0U;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

struct XII_GRAPHICSCORE_DLL xiiMeshBufferResourceDescriptor
{
  xiiMeshBufferResourceDescriptor();

  void Clear();

  void AddStream(xiiEnum<xiiMeshVertexSemantic> semantic, xiiEnum<xiiMeshVertexStreamFormat> format, xiiUInt16 uiOffset, xiiUInt16 uiStride);
  void AddCommonStreams();

  void AllocateStreams(xiiUInt32 uiVertexCount, xiiUInt32 uiPrimitiveCount, bool bUse32BitIndices = true);
  void AllocateStreamsFromGeometry(const xiiGeometry& geometry, xiiEnum<xiiGALPrimitiveTopology> topology, bool bBuildMeshlets = true);

  xiiArrayPtr<xiiUInt8>       GetVertexData();
  xiiArrayPtr<const xiiUInt8> GetVertexData() const;

  xiiArrayPtr<xiiUInt8>       GetIndexData();
  xiiArrayPtr<const xiiUInt8> GetIndexData() const;

  void SetVertexData(xiiArrayPtr<const xiiUInt8> data, xiiUInt32 uiVertexCount, xiiUInt32 uiVertexStride);
  void SetIndexData(xiiArrayPtr<const xiiUInt8> data, xiiUInt32 uiIndexCount, xiiEnum<xiiGALValueType> indexType);

  void BuildMeshlets(xiiUInt32 uiMaxVertices = 64U, xiiUInt32 uiMaxPrimitives = 124U);
  void ComputeBounds();

  xiiUInt32 GetVertexCount() const;
  xiiUInt32 GetIndexCount() const;
  xiiUInt32 GetPrimitiveCount() const;
  xiiUInt32 GetVertexDataSize() const;
  xiiUInt32 GetIndexDataSize() const;
  xiiUInt32 GetVertexStride() const;

  const xiiBoundingBoxSphere& GetBounds() const;
  void                        SetBounds(const xiiBoundingBoxSphere& bounds);

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

  xiiEnum<xiiGALPrimitiveTopology> m_Topology            = xiiGALPrimitiveTopology::TriangleList;
  xiiEnum<xiiGALValueType>         m_IndexType           = xiiGALValueType::UInt32;
  xiiEnum<xiiGALResourceUsage>     m_ResourceUsage       = xiiGALResourceUsage::Immutable;
  bool                             m_bKeepCpuMeshData    = false;
  bool                             m_bAllowGpuDrivenDraw = true;
  bool                             m_bAllowCpuFallback   = true;

  xiiDynamicArray<xiiMeshVertexStream> m_VertexStreams;
  xiiDynamicArray<xiiUInt8>            m_VertexData;
  xiiDynamicArray<xiiUInt8>            m_IndexData;

  xiiDynamicArray<xiiMeshlet>  m_Meshlets;
  xiiDynamicArray<xiiUInt32>   m_MeshletVertexRemap;
  xiiDynamicArray<xiiUInt8>    m_MeshletPrimitiveIndices;
  xiiDynamicArray<xiiUInt32>   m_MeshletMaterialIndices;
  xiiDynamicArray<xiiMeshDrawCommand> m_DrawCommands;

private:
  xiiUInt32            m_uiVertexCount  = 0U;
  xiiUInt32            m_uiIndexCount   = 0U;
  xiiUInt32            m_uiVertexStride = 0U;
  xiiBoundingBoxSphere m_Bounds         = xiiBoundingBoxSphere::MakeInvalid();
};

class XII_GRAPHICSCORE_DLL xiiMeshBufferResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshBufferResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiMeshBufferResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiMeshBufferResource, xiiMeshBufferResourceDescriptor);

public:
  xiiMeshBufferResource();
  ~xiiMeshBufferResource();

  const xiiBoundingBoxSphere& GetBounds() const;
  xiiUInt32                   GetVertexCount() const;
  xiiUInt32                   GetIndexCount() const;
  xiiUInt32                   GetPrimitiveCount() const;
  xiiUInt32                   GetMeshletCount() const;
  xiiUInt32                   GetVertexStride() const;
  xiiEnum<xiiGALPrimitiveTopology> GetTopology() const;
  xiiEnum<xiiGALValueType>         GetIndexType() const;

  xiiArrayPtr<const xiiMeshVertexStream> GetVertexStreams() const;
  xiiArrayPtr<const xiiMeshlet>          GetMeshlets() const;
  xiiArrayPtr<const xiiUInt32>           GetMeshletVertexRemap() const;
  xiiArrayPtr<const xiiUInt8>            GetMeshletPrimitiveIndices() const;

  xiiSharedPtr<xiiGALBuffer> GetVertexBuffer() const;
  xiiSharedPtr<xiiGALBuffer> GetIndexBuffer() const;
  xiiSharedPtr<xiiGALBuffer> GetMeshletBuffer() const;
  xiiSharedPtr<xiiGALBuffer> GetMeshletVertexRemapBuffer() const;
  xiiSharedPtr<xiiGALBuffer> GetMeshletPrimitiveIndexBuffer() const;
  xiiSharedPtr<xiiGALBuffer> GetDrawCommandBuffer() const;

  const xiiMeshBufferResourceDescriptor& GetDescriptor() const;

private:
  virtual xiiResourceLoadDesc UnloadData(Unload whatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void CreateGpuBuffer(xiiSharedPtr<xiiGALBuffer>& out_pBuffer, xiiArrayPtr<const xiiUInt8> data, xiiUInt32 uiStride, xiiBitflags<xiiGALBindFlags> bindFlags, xiiStringView sDebugName);
  void CreateGpuBuffer(xiiSharedPtr<xiiGALBuffer>& out_pBuffer, xiiArrayPtr<const xiiUInt32> data, xiiUInt32 uiStride, xiiBitflags<xiiGALBindFlags> bindFlags, xiiStringView sDebugName);
  void CreateGpuBuffer(xiiSharedPtr<xiiGALBuffer>& out_pBuffer, xiiArrayPtr<const xiiMeshlet> data, xiiUInt32 uiStride, xiiBitflags<xiiGALBindFlags> bindFlags, xiiStringView sDebugName);
  void CreateGpuBuffer(xiiSharedPtr<xiiGALBuffer>& out_pBuffer, xiiArrayPtr<const xiiMeshDrawCommand> data, xiiUInt32 uiStride, xiiBitflags<xiiGALBindFlags> bindFlags, xiiStringView sDebugName);

  xiiMeshBufferResourceDescriptor m_Descriptor;

  xiiSharedPtr<xiiGALBuffer> m_pVertexBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pIndexBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pMeshletBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pMeshletVertexRemapBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pMeshletPrimitiveIndexBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pDrawCommandBuffer;

  xiiUInt64 m_uiMemoryGPU = 0U;
};
