/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/StreamWithStats.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Utilities/Compression.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsFoundation/Device/Device.h>

#include <cstddef>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMeshVertexSemantic, 1)
  XII_ENUM_CONSTANT(xiiMeshVertexSemantic::Position),
  XII_ENUM_CONSTANT(xiiMeshVertexSemantic::Normal),
  XII_ENUM_CONSTANT(xiiMeshVertexSemantic::Tangent),
  XII_ENUM_CONSTANT(xiiMeshVertexSemantic::TexCoord0),
  XII_ENUM_CONSTANT(xiiMeshVertexSemantic::TexCoord1),
  XII_ENUM_CONSTANT(xiiMeshVertexSemantic::Color0),
  XII_ENUM_CONSTANT(xiiMeshVertexSemantic::BoneIndices0),
  XII_ENUM_CONSTANT(xiiMeshVertexSemantic::BoneWeights0),
  XII_ENUM_CONSTANT(xiiMeshVertexSemantic::Custom0),
  XII_ENUM_CONSTANT(xiiMeshVertexSemantic::Custom1),
  XII_ENUM_CONSTANT(xiiMeshVertexSemantic::Custom2),
  XII_ENUM_CONSTANT(xiiMeshVertexSemantic::Custom3),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMeshVertexStreamFormat, 1)
  XII_ENUM_CONSTANT(xiiMeshVertexStreamFormat::Float1),
  XII_ENUM_CONSTANT(xiiMeshVertexStreamFormat::Float2),
  XII_ENUM_CONSTANT(xiiMeshVertexStreamFormat::Float3),
  XII_ENUM_CONSTANT(xiiMeshVertexStreamFormat::Float4),
  XII_ENUM_CONSTANT(xiiMeshVertexStreamFormat::UByte4Normalized),
  XII_ENUM_CONSTANT(xiiMeshVertexStreamFormat::UShort4),
  XII_ENUM_CONSTANT(xiiMeshVertexStreamFormat::UShort4Normalized),
  XII_ENUM_CONSTANT(xiiMeshVertexStreamFormat::UInt),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshBufferResource, 1, xiiRTTIDefaultAllocator<xiiMeshBufferResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiMeshBufferResource);
// clang-format on

namespace
{
  static constexpr xiiUInt32 s_uiMeshBufferResourceVersion = 5U;

  static xiiUInt32 GetIndexSize(xiiEnum<xiiGALValueType> indexType)
  {
    return indexType == xiiGALValueType::UInt16 ? sizeof(xiiUInt16) : sizeof(xiiUInt32);
  }

  static xiiUInt32 ReadIndex(xiiArrayPtr<const xiiUInt8> indexData, xiiEnum<xiiGALValueType> indexType, xiiUInt32 uiIndex)
  {
    if (indexType == xiiGALValueType::UInt16)
    {
      const xiiUInt16* pIndices = reinterpret_cast<const xiiUInt16*>(indexData.GetPtr());
      return pIndices[uiIndex];
    }

    const xiiUInt32* pIndices = reinterpret_cast<const xiiUInt32*>(indexData.GetPtr());
    return pIndices[uiIndex];
  }

  static void WriteIndex(xiiArrayPtr<xiiUInt8> indexData, xiiEnum<xiiGALValueType> indexType, xiiUInt32 uiIndex, xiiUInt32 uiValue)
  {
    if (indexType == xiiGALValueType::UInt16)
    {
      xiiUInt16* pIndices = reinterpret_cast<xiiUInt16*>(indexData.GetPtr());
      pIndices[uiIndex]   = static_cast<xiiUInt16>(uiValue);
      return;
    }

    xiiUInt32* pIndices = reinterpret_cast<xiiUInt32*>(indexData.GetPtr());
    pIndices[uiIndex]   = uiValue;
  }

  static const xiiMeshVertexStream* FindStream(xiiArrayPtr<const xiiMeshVertexStream> streams, xiiMeshVertexSemantic::Enum semantic)
  {
    for (const xiiMeshVertexStream& stream : streams)
    {
      if (stream.m_Semantic == semantic)
        return &stream;
    }

    return nullptr;
  }

  static xiiVec3 ReadPosition(const xiiMeshBufferResourceDescriptor& desc, xiiUInt32 uiVertexIndex)
  {
    const xiiMeshVertexStream* pPositionStream = FindStream(desc.m_VertexStreams, xiiMeshVertexSemantic::Position);
    if (pPositionStream == nullptr || pPositionStream->m_Format != xiiMeshVertexStreamFormat::Float3)
      return xiiVec3::MakeZero();

    const xiiUInt8* pVertex = desc.m_VertexData.GetData() + uiVertexIndex * pPositionStream->m_uiStride + pPositionStream->m_uiOffset;
    return *reinterpret_cast<const xiiVec3*>(pVertex);
  }

  template <typename T>
  static xiiArrayPtr<const xiiUInt8> AsBytes(xiiArrayPtr<const T> data)
  {
    return xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(data.GetPtr()), data.GetCount() * sizeof(T));
  }
} // namespace

xiiResult xiiMeshVertexStream::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_Semantic;
  inout_stream << m_Format;
  inout_stream << m_uiOffset;
  inout_stream << m_uiStride;

  return XII_SUCCESS;
}

xiiResult xiiMeshVertexStream::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_Semantic;
  inout_stream >> m_Format;
  inout_stream >> m_uiOffset;
  inout_stream >> m_uiStride;

  return XII_SUCCESS;
}

xiiResult xiiMeshlet::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_uiFirstPrimitive;
  inout_stream << m_uiVertexRemapOffset;
  inout_stream << m_uiPrimitiveIndexOffset;
  inout_stream << m_uiMaterialIndex;
  inout_stream << m_uiPrimitiveCount;
  inout_stream << m_uiVertexCount;
  inout_stream << m_uiLodIndex;
  inout_stream << m_uiSectionIndex;
  inout_stream << m_Bounds.m_vCenter;
  inout_stream << m_Bounds.m_fRadius;
  inout_stream << m_vConeAxis;
  inout_stream << m_fConeCutoff;

  return XII_SUCCESS;
}

xiiResult xiiMeshlet::Deserialize(xiiStreamReader& inout_stream)
{
  xiiMemoryUtils::ZeroFillArray(m_uiGpuPadding);
  inout_stream >> m_uiFirstPrimitive;
  inout_stream >> m_uiVertexRemapOffset;
  inout_stream >> m_uiPrimitiveIndexOffset;
  inout_stream >> m_uiMaterialIndex;
  inout_stream >> m_uiPrimitiveCount;
  inout_stream >> m_uiVertexCount;
  inout_stream >> m_uiLodIndex;
  inout_stream >> m_uiSectionIndex;
  inout_stream >> m_Bounds.m_vCenter;
  inout_stream >> m_Bounds.m_fRadius;
  inout_stream >> m_vConeAxis;
  inout_stream >> m_fConeCutoff;

  return XII_SUCCESS;
}

xiiResult xiiMeshDrawCommand::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_uiThreadGroupCountX;
  inout_stream << m_uiThreadGroupCountY;
  inout_stream << m_uiThreadGroupCountZ;

  return XII_SUCCESS;
}

xiiResult xiiMeshDrawCommand::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_uiThreadGroupCountX;
  inout_stream >> m_uiThreadGroupCountY;
  inout_stream >> m_uiThreadGroupCountZ;

  return XII_SUCCESS;
}

xiiResult xiiMeshInstanceData::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_GlobalTransform;
  inout_stream << m_uiObjectId;
  inout_stream << m_uiMeshletOffset;
  inout_stream << m_uiMeshletCount;
  inout_stream << m_uiMaterialBase;

  return XII_SUCCESS;
}

xiiResult xiiMeshInstanceData::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_GlobalTransform;
  inout_stream >> m_uiObjectId;
  inout_stream >> m_uiMeshletOffset;
  inout_stream >> m_uiMeshletCount;
  inout_stream >> m_uiMaterialBase;

  return XII_SUCCESS;
}

xiiMeshBufferResourceDescriptor::xiiMeshBufferResourceDescriptor()
{
  AddCommonStreams();
}

void xiiMeshBufferResourceDescriptor::Clear()
{
  m_Topology            = xiiGALPrimitiveTopology::TriangleList;
  m_IndexType           = xiiGALValueType::UInt32;
  m_ResourceUsage       = xiiGALResourceUsage::Immutable;
  m_bKeepCpuMeshData    = false;
  m_bAllowGpuDrivenDraw = true;
  m_bAllowCpuFallback   = true;

  m_VertexStreams.Clear();
  m_VertexData.Clear();
  m_IndexData.Clear();
  m_Meshlets.Clear();
  m_MeshletVertexRemap.Clear();
  m_MeshletPrimitiveIndices.Clear();
  m_MeshletMaterialIndices.Clear();
  m_DrawCommands.Clear();
  m_GeometryPages.Clear();
  m_GeometryPageData.Clear();
  m_sGeometryPageFile.Clear();
  m_uiGeometryFileOffset = m_uiGeometryFileHash = m_uiPageDataBytes = 0U;
  m_uiGeometryFileVersion                                           = 0U;

  m_uiVertexCount  = 0U;
  m_uiIndexCount   = 0U;
  m_uiVertexStride = 0U;
  m_Bounds         = xiiBoundingBoxSphere::MakeInvalid();
}

void xiiMeshBufferResourceDescriptor::AddStream(xiiEnum<xiiMeshVertexSemantic> semantic, xiiEnum<xiiMeshVertexStreamFormat> format, xiiUInt16 uiOffset, xiiUInt16 uiStride)
{
  xiiMeshVertexStream& stream = m_VertexStreams.ExpandAndGetRef();
  stream.m_Semantic           = semantic;
  stream.m_Format             = format;
  stream.m_uiOffset           = uiOffset;
  stream.m_uiStride           = uiStride;

  m_uiVertexStride = xiiMath::Max<xiiUInt32>(m_uiVertexStride, uiStride);
}

void xiiMeshBufferResourceDescriptor::AddCommonStreams(bool bIncludeSkinning)
{
  m_VertexStreams.Clear();
  m_uiVertexStride = sizeof(xiiMeshPackedVertex);

  AddStream(xiiMeshVertexSemantic::Position, xiiMeshVertexStreamFormat::Float3, static_cast<xiiUInt16>(offsetof(xiiMeshPackedVertex, m_vPosition)), static_cast<xiiUInt16>(sizeof(xiiMeshPackedVertex)));
  AddStream(xiiMeshVertexSemantic::Normal, xiiMeshVertexStreamFormat::Float3, static_cast<xiiUInt16>(offsetof(xiiMeshPackedVertex, m_vNormal)), static_cast<xiiUInt16>(sizeof(xiiMeshPackedVertex)));
  AddStream(xiiMeshVertexSemantic::Tangent, xiiMeshVertexStreamFormat::Float4, static_cast<xiiUInt16>(offsetof(xiiMeshPackedVertex, m_vTangent)), static_cast<xiiUInt16>(sizeof(xiiMeshPackedVertex)));
  AddStream(xiiMeshVertexSemantic::TexCoord0, xiiMeshVertexStreamFormat::Float2, static_cast<xiiUInt16>(offsetof(xiiMeshPackedVertex, m_vTexCoord0)), static_cast<xiiUInt16>(sizeof(xiiMeshPackedVertex)));
  AddStream(xiiMeshVertexSemantic::TexCoord1, xiiMeshVertexStreamFormat::Float2, static_cast<xiiUInt16>(offsetof(xiiMeshPackedVertex, m_vTexCoord1)), static_cast<xiiUInt16>(sizeof(xiiMeshPackedVertex)));
  AddStream(xiiMeshVertexSemantic::Color0, xiiMeshVertexStreamFormat::UByte4Normalized, static_cast<xiiUInt16>(offsetof(xiiMeshPackedVertex, m_Color0)), static_cast<xiiUInt16>(sizeof(xiiMeshPackedVertex)));
  if (bIncludeSkinning)
  {
    AddStream(xiiMeshVertexSemantic::BoneIndices0, xiiMeshVertexStreamFormat::UShort4, static_cast<xiiUInt16>(offsetof(xiiMeshPackedVertex, m_vBoneIndices0)), static_cast<xiiUInt16>(sizeof(xiiMeshPackedVertex)));
    AddStream(xiiMeshVertexSemantic::BoneWeights0, xiiMeshVertexStreamFormat::UByte4Normalized, static_cast<xiiUInt16>(offsetof(xiiMeshPackedVertex, m_BoneWeights0)), static_cast<xiiUInt16>(sizeof(xiiMeshPackedVertex)));
  }
}

void xiiMeshBufferResourceDescriptor::AllocateStreams(xiiUInt32 uiVertexCount, xiiUInt32 uiPrimitiveCount, bool bUse32BitIndices)
{
  if (m_VertexStreams.IsEmpty())
  {
    AddCommonStreams();
  }

  m_uiVertexCount = uiVertexCount;
  m_IndexType     = bUse32BitIndices ? xiiGALValueType::UInt32 : xiiGALValueType::UInt16;
  m_uiIndexCount  = uiPrimitiveCount * xiiGALPrimitiveTopology::VerticesPerPrimitive(m_Topology);

  m_VertexData.SetCountUninitialized(m_uiVertexCount * m_uiVertexStride);
  m_IndexData.SetCountUninitialized(m_uiIndexCount * GetIndexSize(m_IndexType));
}

void xiiMeshBufferResourceDescriptor::AllocateStreamsFromGeometry(const xiiGeometry& geometry, xiiEnum<xiiGALPrimitiveTopology> topology, bool bBuildMeshlets)
{
  Clear();
  AddCommonStreams();

  m_Topology       = topology;
  m_uiVertexCount  = geometry.GetVertices().GetCount();
  m_uiVertexStride = sizeof(xiiMeshPackedVertex);

  xiiUInt32 uiPrimitiveCount = 0U;
  if (topology == xiiGALPrimitiveTopology::TriangleList)
  {
    for (const xiiGeometry::Polygon& polygon : geometry.GetPolygons())
    {
      if (polygon.m_Vertices.GetCount() >= 3U)
      {
        uiPrimitiveCount += polygon.m_Vertices.GetCount() - 2U;
      }
    }
  }
  else if (topology == xiiGALPrimitiveTopology::LineList)
  {
    uiPrimitiveCount = geometry.GetLines().GetCount();
  }

  m_IndexType    = m_uiVertexCount <= xiiMath::MaxValue<xiiUInt16>() ? xiiGALValueType::UInt16 : xiiGALValueType::UInt32;
  m_uiIndexCount = uiPrimitiveCount * xiiGALPrimitiveTopology::VerticesPerPrimitive(topology);

  m_VertexData.SetCountUninitialized(m_uiVertexCount * sizeof(xiiMeshPackedVertex));
  m_IndexData.SetCountUninitialized(m_uiIndexCount * GetIndexSize(m_IndexType));

  xiiMeshPackedVertex* pVertices = reinterpret_cast<xiiMeshPackedVertex*>(m_VertexData.GetData());
  for (xiiUInt32 i = 0; i < m_uiVertexCount; ++i)
  {
    const xiiGeometry::Vertex& src = geometry.GetVertices()[i];

    pVertices[i].m_vPosition     = src.m_vPosition;
    pVertices[i].m_vNormal       = src.m_vNormal;
    pVertices[i].m_vTangent      = xiiVec4(src.m_vTangent, src.m_fBiTangentSign);
    pVertices[i].m_vTexCoord0    = src.m_vTexCoord;
    pVertices[i].m_vTexCoord1    = xiiVec2(0.0f);
    pVertices[i].m_Color0        = src.m_Color;
    pVertices[i].m_vBoneIndices0 = src.m_BoneIndices;
    pVertices[i].m_BoneWeights0  = src.m_BoneWeights;
  }

  xiiUInt32 uiIndex = 0U;
  if (topology == xiiGALPrimitiveTopology::TriangleList)
  {
    for (const xiiGeometry::Polygon& polygon : geometry.GetPolygons())
    {
      if (polygon.m_Vertices.GetCount() < 3U)
        continue;

      for (xiiUInt32 i = 2U; i < polygon.m_Vertices.GetCount(); ++i)
      {
        WriteIndex(m_IndexData, m_IndexType, uiIndex++, polygon.m_Vertices[0]);
        WriteIndex(m_IndexData, m_IndexType, uiIndex++, polygon.m_Vertices[i - 1U]);
        WriteIndex(m_IndexData, m_IndexType, uiIndex++, polygon.m_Vertices[i]);
      }
    }
  }
  else if (topology == xiiGALPrimitiveTopology::LineList)
  {
    for (const xiiGeometry::Line& line : geometry.GetLines())
    {
      WriteIndex(m_IndexData, m_IndexType, uiIndex++, line.m_uiStartVertex);
      WriteIndex(m_IndexData, m_IndexType, uiIndex++, line.m_uiEndVertex);
    }
  }

  ComputeBounds();

  if (bBuildMeshlets && topology == xiiGALPrimitiveTopology::TriangleList)
  {
    if (BuildMeshlets().Failed())
      xiiLog::Error("Cannot build meshlets from invalid geometry.");
  }
}

xiiArrayPtr<xiiUInt8> xiiMeshBufferResourceDescriptor::GetVertexData()
{
  return m_VertexData;
}

xiiArrayPtr<const xiiUInt8> xiiMeshBufferResourceDescriptor::GetVertexData() const
{
  return m_VertexData;
}

xiiArrayPtr<xiiUInt8> xiiMeshBufferResourceDescriptor::GetIndexData()
{
  return m_IndexData;
}

xiiArrayPtr<const xiiUInt8> xiiMeshBufferResourceDescriptor::GetIndexData() const
{
  return m_IndexData;
}

void xiiMeshBufferResourceDescriptor::SetVertexData(xiiArrayPtr<const xiiUInt8> data, xiiUInt32 uiVertexCount, xiiUInt32 uiVertexStride)
{
  m_uiVertexCount  = uiVertexCount;
  m_uiVertexStride = uiVertexStride;

  m_VertexData.SetCountUninitialized(data.GetCount());
  if (!data.IsEmpty())
  {
    xiiMemoryUtils::Copy(m_VertexData.GetData(), data.GetPtr(), data.GetCount());
  }
}

void xiiMeshBufferResourceDescriptor::SetIndexData(xiiArrayPtr<const xiiUInt8> data, xiiUInt32 uiIndexCount, xiiEnum<xiiGALValueType> indexType)
{
  m_uiIndexCount = uiIndexCount;
  m_IndexType    = indexType;

  m_IndexData.SetCountUninitialized(data.GetCount());
  if (!data.IsEmpty())
  {
    xiiMemoryUtils::Copy(m_IndexData.GetData(), data.GetPtr(), data.GetCount());
  }
}

void xiiMeshBufferResourceDescriptor::ComputeBounds()
{
  if (m_uiVertexCount == 0U || m_VertexData.IsEmpty())
  {
    m_Bounds = xiiBoundingBoxSphere::MakeInvalid();
    return;
  }

  xiiHybridArray<xiiVec3, 256> positions;
  positions.SetCountUninitialized(m_uiVertexCount);
  for (xiiUInt32 i = 0; i < m_uiVertexCount; ++i)
  {
    positions[i] = ReadPosition(*this, i);
  }

  m_Bounds = xiiBoundingBoxSphere::MakeFromPoints(positions.GetData(), positions.GetCount());
}

xiiUInt32 xiiMeshBufferResourceDescriptor::GetVertexCount() const
{
  return m_uiVertexCount;
}

xiiUInt32 xiiMeshBufferResourceDescriptor::GetIndexCount() const
{
  return m_uiIndexCount;
}

xiiUInt32 xiiMeshBufferResourceDescriptor::GetPrimitiveCount() const
{
  const xiiUInt32 uiVerticesPerPrimitive = xiiGALPrimitiveTopology::VerticesPerPrimitive(m_Topology);
  return uiVerticesPerPrimitive == 0U ? 0U : m_uiIndexCount / uiVerticesPerPrimitive;
}

xiiUInt32 xiiMeshBufferResourceDescriptor::GetVertexDataSize() const
{
  return m_VertexData.GetCount();
}

xiiUInt32 xiiMeshBufferResourceDescriptor::GetIndexDataSize() const
{
  return m_IndexData.GetCount();
}

xiiUInt32 xiiMeshBufferResourceDescriptor::GetVertexStride() const
{
  return m_uiVertexStride;
}

const xiiBoundingBoxSphere& xiiMeshBufferResourceDescriptor::GetBounds() const
{
  return m_Bounds;
}

void xiiMeshBufferResourceDescriptor::SetBounds(const xiiBoundingBoxSphere& bounds)
{
  m_Bounds = bounds;
}

xiiResult xiiMeshBufferResourceDescriptor::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiMeshBufferResourceVersion);

  inout_stream << m_Topology;
  inout_stream << m_IndexType;
  inout_stream << m_ResourceUsage;
  inout_stream << m_bKeepCpuMeshData;
  inout_stream << m_bAllowGpuDrivenDraw;
  inout_stream << m_bAllowCpuFallback;
  inout_stream << m_uiVertexCount;
  inout_stream << m_uiIndexCount;
  inout_stream << m_uiVertexStride;
  inout_stream << m_Bounds.m_vCenter;
  inout_stream << m_Bounds.m_fSphereRadius;
  inout_stream << m_Bounds.m_vBoxHalfExtents;

  XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_VertexStreams));
  XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_VertexData));
  XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_IndexData));
  XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Meshlets));
  XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_MeshletVertexRemap));
  XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_MeshletPrimitiveIndices));
  XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_MeshletMaterialIndices));
  XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_DrawCommands));
  if (m_sGeometryPageFile.IsEmpty())
  {
    XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_GeometryPages));
    XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_GeometryPageData));
  }
  else
  {
    // A loaded descriptor may be saved again by editor and export consumers.
    auto                      pages = m_GeometryPages;
    xiiDynamicArray<xiiUInt8> payload, decoded, encoded;
    for (xiiUInt32 page = 0U; page < pages.GetCount(); ++page)
    {
      XII_SUCCEED_OR_RETURN(ReadGeometryPage(page, decoded));
      XII_SUCCEED_OR_RETURN(xiiCompressionUtils::Compress(decoded, xiiCompressionMethod::ZStd, encoded));
      if (encoded.GetCount() > xiiMath::MaxValue<xiiUInt32>() - payload.GetCount())
        return XII_FAILURE;
      pages[page].m_uiOffset     = payload.GetCount();
      pages[page].m_uiStoredSize = encoded.GetCount();
      payload.PushBackRange(encoded);
    }
    XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(pages));
    XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(payload));
  }
  for (const auto& meshlet : m_Meshlets)
    inout_stream << meshlet.m_Page << meshlet.m_Hierarchy << meshlet.m_uiGpuPadding[1];

  return XII_SUCCESS;
}

xiiResult xiiMeshBufferResourceDescriptor::Deserialize(xiiStreamReader& source, const xiiResourceLoadData* pSource)
{
  xiiStreamReaderWithStats inout_stream(&source);
  xiiTypeVersion           version = 0U;
  XII_SUCCEED_OR_RETURN(inout_stream.ReadWordValue(&version));
  if (version != 1U && version != s_uiMeshBufferResourceVersion)
  {
    xiiLog::Error("Unsupported mesh buffer version {}. Retransform the mesh asset in the editor.", version);
    return XII_FAILURE;
  }

  inout_stream >> m_Topology;
  inout_stream >> m_IndexType;
  inout_stream >> m_ResourceUsage;
  inout_stream >> m_bKeepCpuMeshData;
  inout_stream >> m_bAllowGpuDrivenDraw;
  inout_stream >> m_bAllowCpuFallback;
  inout_stream >> m_uiVertexCount;
  inout_stream >> m_uiIndexCount;
  inout_stream >> m_uiVertexStride;
  inout_stream >> m_Bounds.m_vCenter;
  inout_stream >> m_Bounds.m_fSphereRadius;
  inout_stream >> m_Bounds.m_vBoxHalfExtents;

  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_VertexStreams));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_VertexData));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_IndexData));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Meshlets));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_MeshletVertexRemap));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_MeshletPrimitiveIndices));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_MeshletMaterialIndices));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_DrawCommands));
  if (version >= 2U)
  {
    XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_GeometryPages));
    inout_stream >> m_uiPageDataBytes;
    if (m_uiPageDataBytes > xiiMath::MaxValue<xiiUInt32>())
      return XII_FAILURE;
    if (pSource && !pSource->m_sSourceFile.IsEmpty())
    {
      m_sGeometryPageFile     = pSource->m_sSourceFile;
      m_uiGeometryFileOffset  = pSource->m_uiSourceOffset + inout_stream.m_uiBytesRead;
      m_uiGeometryFileHash    = pSource->m_uiSourceHash;
      m_uiGeometryFileVersion = pSource->m_uiSourceVersion;
      if (inout_stream.SkipBytes(m_uiPageDataBytes) != m_uiPageDataBytes)
        return XII_FAILURE;
    }
    else
    {
      m_GeometryPageData.SetCountUninitialized(static_cast<xiiUInt32>(m_uiPageDataBytes));
      if (inout_stream.ReadBytes(m_GeometryPageData.GetData(), m_uiPageDataBytes) != m_uiPageDataBytes)
        return XII_FAILURE;
    }
    for (auto& meshlet : m_Meshlets)
    {
      inout_stream >> meshlet.m_Page >> meshlet.m_Hierarchy >> meshlet.m_uiGpuPadding[1];
      if ((!m_GeometryPages.IsEmpty() && meshlet.m_Page.x >= m_GeometryPages.GetCount()) ||
          !meshlet.m_Hierarchy.IsValid() || meshlet.m_Hierarchy.x < 0.0f)
        return XII_FAILURE;
    }
    for (const auto& cluster : m_Meshlets)
    {
      if (m_GeometryPages.IsEmpty())
        break;
      const auto size = m_GeometryPages[cluster.m_Page.x].m_uiSize;
      if (cluster.m_Page.y % 4U != 0U || cluster.m_Page.y > size || cluster.m_uiVertexCount * 4U > size - cluster.m_Page.y ||
          cluster.m_Page.z > size || cluster.m_uiPrimitiveCount * 3U > size - cluster.m_Page.z ||
          xiiUInt64(cluster.m_Page.w) * GetVertexStride() > size ||
          cluster.m_Hierarchy.y < 1.0f || cluster.m_Hierarchy.y > static_cast<float>(m_GeometryPages.GetCount()))
        return XII_FAILURE;
    }
    for (xiiUInt32 index = 0U; index < m_GeometryPages.GetCount(); ++index)
    {
      const auto& page = m_GeometryPages[index];
      if (page.m_uiSize == 0U || page.m_uiSize > 128U * 1024U || page.m_uiClusterCount == 0U ||
          xiiUInt64(page.m_uiClusterCount) * sizeof(xiiMeshlet) >= page.m_uiSize || page.m_uiSize % 4U != 0U ||
          page.m_uiFirstCluster > m_Meshlets.GetCount() || page.m_uiClusterCount > m_Meshlets.GetCount() - page.m_uiFirstCluster ||
          !xiiMath::IsFinite(page.m_fGeometricError) || page.m_fGeometricError < 0.0f ||
          page.m_bRequired != (page.m_uiParent == xiiInvalidIndex) ||
          (page.m_uiParent != xiiInvalidIndex && (page.m_uiParent <= index || page.m_uiParent >= m_GeometryPages.GetCount())) ||
          page.m_uiChildCount > 8U || (page.m_uiChildCount != 0U && (page.m_uiFirstChild >= index || page.m_uiChildCount > index - page.m_uiFirstChild)))
        return XII_FAILURE;
      for (xiiUInt32 child = 0U; child < page.m_uiChildCount; ++child)
        if (m_GeometryPages[page.m_uiFirstChild + child].m_uiParent != index ||
            m_GeometryPages[page.m_uiFirstChild + child].m_fGeometricError > page.m_fGeometricError)
          return XII_FAILURE;
      xiiUInt32 depth = 0U;
      for (auto parent = page.m_uiParent; parent != xiiInvalidIndex; parent = m_GeometryPages[parent].m_uiParent)
        if (parent >= m_GeometryPages.GetCount() || ++depth > 7U)
          return XII_FAILURE;
      if (page.m_uiOffset > m_uiPageDataBytes || (page.m_uiStoredSize == 0U ? page.m_uiSize : page.m_uiStoredSize) > m_uiPageDataBytes - page.m_uiOffset)
        return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

xiiMeshBufferResource::xiiMeshBufferResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiMeshBufferResource::~xiiMeshBufferResource() = default;

const xiiBoundingBoxSphere& xiiMeshBufferResource::GetBounds() const
{
  return m_Descriptor.GetBounds();
}

xiiUInt32 xiiMeshBufferResource::GetVertexCount() const
{
  return m_Descriptor.GetVertexCount();
}

xiiUInt32 xiiMeshBufferResource::GetIndexCount() const
{
  return m_Descriptor.GetIndexCount();
}

xiiUInt32 xiiMeshBufferResource::GetPrimitiveCount() const
{
  return m_Descriptor.GetPrimitiveCount();
}

xiiUInt32 xiiMeshBufferResource::GetMeshletCount() const
{
  return m_Descriptor.m_Meshlets.GetCount();
}

xiiUInt32 xiiMeshBufferResource::GetVertexStride() const
{
  return m_Descriptor.GetVertexStride();
}

xiiEnum<xiiGALPrimitiveTopology> xiiMeshBufferResource::GetTopology() const
{
  return m_Descriptor.m_Topology;
}

xiiEnum<xiiGALValueType> xiiMeshBufferResource::GetIndexType() const
{
  return m_Descriptor.m_IndexType;
}

xiiArrayPtr<const xiiMeshVertexStream> xiiMeshBufferResource::GetVertexStreams() const
{
  return m_Descriptor.m_VertexStreams;
}

xiiArrayPtr<const xiiMeshlet> xiiMeshBufferResource::GetMeshlets() const
{
  return m_Descriptor.m_Meshlets;
}

xiiArrayPtr<const xiiUInt32> xiiMeshBufferResource::GetMeshletVertexRemap() const
{
  return m_Descriptor.m_MeshletVertexRemap;
}

xiiArrayPtr<const xiiUInt8> xiiMeshBufferResource::GetMeshletPrimitiveIndices() const
{
  return m_Descriptor.m_MeshletPrimitiveIndices;
}

xiiSharedPtr<xiiGALBuffer> xiiMeshBufferResource::GetVertexBuffer() const
{
  XII_LOCK(m_BufferMutex);
  if (!m_pVertexBuffer && HasGeometryPages())
    const_cast<xiiMeshBufferResource*>(this)->CreateGpuBuffer(m_pVertexBuffer, m_Descriptor.m_VertexData, GetVertexStride(), xiiGALBindFlags::VertexBuffer | xiiGALBindFlags::ShaderResource | (xiiGALDevice::GetDefaultDevice()->GetFeatures().m_RayTracing == xiiGALDeviceFeatureState::Enabled ? xiiGALBindFlags::RayTracing : xiiGALBindFlags::None), GetResourceIdOrDescription());
  return m_pVertexBuffer;
}

xiiSharedPtr<xiiGALBuffer> xiiMeshBufferResource::GetIndexBuffer() const
{
  XII_LOCK(m_BufferMutex);
  if (!m_pIndexBuffer && HasGeometryPages())
    const_cast<xiiMeshBufferResource*>(this)->CreateGpuBuffer(m_pIndexBuffer, m_Descriptor.m_IndexData, GetIndexSize(GetIndexType()), xiiGALBindFlags::IndexBuffer | xiiGALBindFlags::ShaderResource | (xiiGALDevice::GetDefaultDevice()->GetFeatures().m_RayTracing == xiiGALDeviceFeatureState::Enabled ? xiiGALBindFlags::RayTracing : xiiGALBindFlags::None), GetResourceIdOrDescription());
  return m_pIndexBuffer;
}

xiiSharedPtr<xiiGALBuffer> xiiMeshBufferResource::GetDrawCommandBuffer() const
{
  return m_pDrawCommandBuffer;
}

const xiiMeshBufferResourceDescriptor& xiiMeshBufferResource::GetDescriptor() const
{
  return m_Descriptor;
}

xiiResourceLoadDescription xiiMeshBufferResource::UnloadData(Unload whatToUnload)
{
  XII_IGNORE_UNUSED(whatToUnload);
  XII_LOCK(m_BufferMutex);

  m_pVertexBuffer.Clear();
  m_pIndexBuffer.Clear();
  m_pDrawCommandBuffer.Clear();
  m_Descriptor.Clear();
  m_uiMemoryGPU = 0U;

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0U;
  res.m_uiQualityLevelsLoadable    = 0U;
  res.m_State                      = xiiResourceState::Unloaded;
  return res;
}

xiiResourceLoadDescription xiiMeshBufferResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0U;
  res.m_uiQualityLevelsLoadable    = 0U;
  res.m_State                      = xiiResourceState::Loaded;

  if (pStream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  xiiMeshBufferResourceDescriptor descriptor;
  if (descriptor.Deserialize(*pStream, GetCurrentlyUpdatingLoadData()).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  CreateResource(std::move(descriptor));
  return res;
}

xiiResult xiiMeshBufferResource::ReadGeometryPage(xiiUInt32 page, xiiUInt32 revision, xiiDynamicArray<xiiUInt8>& out_data) const
{
  XII_LOCK(m_BufferMutex);
  if (GetLoadingState() != xiiResourceState::Loaded || GetCurrentResourceChangeCounter() != revision)
    return XII_FAILURE;
  return m_Descriptor.ReadGeometryPage(page, out_data);
}

void xiiMeshBufferResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  XII_LOCK(m_BufferMutex);
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiMeshBufferResource) + static_cast<xiiUInt32>(m_Descriptor.m_VertexStreams.GetHeapMemoryUsage() + m_Descriptor.m_VertexData.GetHeapMemoryUsage() + m_Descriptor.m_IndexData.GetHeapMemoryUsage() + m_Descriptor.m_Meshlets.GetHeapMemoryUsage() + m_Descriptor.m_MeshletVertexRemap.GetHeapMemoryUsage() + m_Descriptor.m_MeshletPrimitiveIndices.GetHeapMemoryUsage() + m_Descriptor.m_MeshletMaterialIndices.GetHeapMemoryUsage() + m_Descriptor.m_DrawCommands.GetHeapMemoryUsage() + m_Descriptor.m_GeometryPages.GetHeapMemoryUsage() + m_Descriptor.m_GeometryPageData.GetHeapMemoryUsage());
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU;
}

void xiiMeshBufferResource::CreateGpuBuffer(xiiSharedPtr<xiiGALBuffer>& out_pBuffer, xiiArrayPtr<const xiiUInt8> data, xiiUInt32 uiStride, xiiBitflags<xiiGALBindFlags> bindFlags, xiiStringView sDebugName)
{
  out_pBuffer.Clear();
  if (data.IsEmpty())
    return;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (!pDevice)
    return;

  xiiGALBufferCreationDescription desc;
  desc.m_MemoryCategory      = xiiGALMemoryCategory::Geometry;
  desc.m_uiSize              = data.GetCount();
  desc.m_BindFlags           = bindFlags;
  desc.m_Usage               = m_Descriptor.m_ResourceUsage;
  desc.m_CPUAccessFlags      = m_Descriptor.m_ResourceUsage == xiiGALResourceUsage::Dynamic ? xiiGALCPUAccessFlag::Write : xiiGALCPUAccessFlag::None;
  desc.m_Mode                = bindFlags.IsSet(xiiGALBindFlags::IndexBuffer) ? xiiGALBufferMode::Raw : xiiGALBufferMode::Structured;
  desc.m_uiElementByteStride = uiStride;

  // Bindless byte-address reads access complete words, including packed primitive and UInt16
  // index tails. Retain logical element counts in the descriptor; pad only the GPU allocation.
  xiiDynamicArray<xiiUInt8> paddedData;
  if (bindFlags.IsSet(xiiGALBindFlags::ShaderResource) && (data.GetCount() & 3U) != 0U)
  {
    paddedData.SetCount(xiiMath::RoundUp(data.GetCount(), 4U));
    xiiMemoryUtils::Copy(paddedData.GetData(), data.GetPtr(), data.GetCount());
    xiiMemoryUtils::ZeroFill(paddedData.GetData() + data.GetCount(), paddedData.GetCount() - data.GetCount());
    data          = paddedData;
    desc.m_uiSize = data.GetCount();
  }
  xiiGALBufferData initialData(const_cast<xiiUInt8*>(data.GetPtr()), data.GetCount());
  out_pBuffer = pDevice->CreateBuffer(desc, &initialData);

  if (out_pBuffer)
  {
    out_pBuffer->SetDebugName(sDebugName);
    m_uiMemoryGPU += out_pBuffer->GetMemoryConsumption();
  }
}

void xiiMeshBufferResource::CreateGpuBuffer(xiiSharedPtr<xiiGALBuffer>& out_pBuffer, xiiArrayPtr<const xiiMeshDrawCommand> data, xiiUInt32 uiStride, xiiBitflags<xiiGALBindFlags> bindFlags, xiiStringView sDebugName)
{
  CreateGpuBuffer(out_pBuffer, AsBytes(data), uiStride, bindFlags, sDebugName);
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiMeshBufferResource, xiiMeshBufferResourceDescriptor)
{
  XII_LOCK(m_BufferMutex);
  m_pVertexBuffer.Clear();
  m_pIndexBuffer.Clear();
  m_pDrawCommandBuffer.Clear();
  m_uiMemoryGPU = 0U;

  if (descriptor.m_bStreamableGeometry && descriptor.m_ResourceUsage == xiiGALResourceUsage::Immutable &&
      !descriptor.m_Meshlets.IsEmpty() && descriptor.m_GeometryPages.IsEmpty())
  {
    const auto* load = GetCurrentlyUpdatingLoadData();
    if (load && !load->m_sSourceFile.IsEmpty())
    {
      xiiLog::Error("Static mesh '{}' lacks compiled geometry pages. Re-transform its source asset.", GetResourceID());
      return {xiiResourceState::LoadedResourceMissing, 0U, 0U};
    }
    if (descriptor.BuildGeometryPages().Failed())
      return {xiiResourceState::LoadedResourceMissing, 0U, 0U};
  }
  m_Descriptor = std::move(descriptor);

  if (HasGeometryPages())
  {
    return {xiiResourceState::Loaded, 0U, 0U};
  }

  // BLAS builds consume the canonical render vertex/index buffers directly.
  // Request device addresses only on ray-tracing-capable devices so the same
  // resource remains valid on integrated/fallback hardware.
  xiiBitflags<xiiGALBindFlags>     vertexBindFlags = xiiGALBindFlags::VertexBuffer | xiiGALBindFlags::ShaderResource;
  xiiBitflags<xiiGALBindFlags>     indexBindFlags  = xiiGALBindFlags::IndexBuffer | xiiGALBindFlags::ShaderResource;
  const xiiSharedPtr<xiiGALDevice> pDevice         = xiiGALDevice::GetDefaultDevice();
  if (pDevice != nullptr && pDevice->GetFeatures().m_RayTracing == xiiGALDeviceFeatureState::Enabled)
  {
    vertexBindFlags.Add(xiiGALBindFlags::RayTracing);
    indexBindFlags.Add(xiiGALBindFlags::RayTracing);
  }

  xiiStringBuilder sDebugName;
  sDebugName.Set(GetResourceIdOrDescription(), " Vertex Data");
  CreateGpuBuffer(m_pVertexBuffer, m_Descriptor.m_VertexData, m_Descriptor.GetVertexStride(), vertexBindFlags, sDebugName);

  sDebugName.Set(GetResourceIdOrDescription(), " Index Data");
  CreateGpuBuffer(m_pIndexBuffer, m_Descriptor.m_IndexData, GetIndexSize(m_Descriptor.m_IndexType), indexBindFlags, sDebugName);

  sDebugName.Set(GetResourceIdOrDescription(), " Mesh Draw Commands");
  CreateGpuBuffer(m_pDrawCommandBuffer, m_Descriptor.m_DrawCommands, sizeof(xiiMeshDrawCommand), xiiGALBindFlags::IndirectDrawArguments | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, sDebugName);

  if (!m_Descriptor.m_bKeepCpuMeshData)
  {
    m_Descriptor.m_VertexData.Clear();
    m_Descriptor.m_IndexData.Clear();
  }

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0U;
  res.m_uiQualityLevelsLoadable    = 0U;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_MeshBufferResource);
