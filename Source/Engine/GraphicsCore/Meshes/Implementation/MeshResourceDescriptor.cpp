#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GraphicsCore/Meshes/MeshResourceDescriptor.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

xiiMeshResourceDescriptor::xiiMeshResourceDescriptor()
{
  m_Bounds = xiiBoundingBoxSphere::MakeInvalid();
}

void xiiMeshResourceDescriptor::Clear()
{
  m_Bounds = xiiBoundingBoxSphere::MakeInvalid();
  m_hMeshBuffer.Invalidate();
  m_Materials.Clear();
  m_MeshBufferDescriptor.Clear();
  m_SubMeshes.Clear();
}

xiiMeshBufferResourceDescriptor& xiiMeshResourceDescriptor::MeshBufferDesc()
{
  return m_MeshBufferDescriptor;
}

const xiiMeshBufferResourceDescriptor& xiiMeshResourceDescriptor::MeshBufferDesc() const
{
  return m_MeshBufferDescriptor;
}

void xiiMeshResourceDescriptor::UseExistingMeshBuffer(const xiiMeshBufferResourceHandle& hBuffer)
{
  m_hMeshBuffer = hBuffer;
}

const xiiMeshBufferResourceHandle& xiiMeshResourceDescriptor::GetExistingMeshBuffer() const
{
  return m_hMeshBuffer;
}

xiiArrayPtr<const xiiMeshResourceDescriptor::Material> xiiMeshResourceDescriptor::GetMaterials() const
{
  return m_Materials;
}

xiiArrayPtr<const xiiMeshResourceDescriptor::SubMesh> xiiMeshResourceDescriptor::GetSubMeshes() const
{
  return m_SubMeshes;
}

void xiiMeshResourceDescriptor::CollapseSubMeshes()
{
  for (xiiUInt32 idx = 1; idx < m_SubMeshes.GetCount(); ++idx)
  {
    m_SubMeshes[0].m_uiFirstPrimitive = xiiMath::Min(m_SubMeshes[0].m_uiFirstPrimitive, m_SubMeshes[idx].m_uiFirstPrimitive);
    m_SubMeshes[0].m_uiPrimitiveCount += m_SubMeshes[idx].m_uiPrimitiveCount;

    if (m_SubMeshes[0].m_Bounds.IsValid() && m_SubMeshes[idx].m_Bounds.IsValid())
    {
      m_SubMeshes[0].m_Bounds.ExpandToInclude(m_SubMeshes[idx].m_Bounds);
    }
  }

  m_SubMeshes.SetCount(1);
  m_SubMeshes[0].m_uiMaterialIndex = 0;

  m_Materials.SetCount(1);
}

const xiiBoundingBoxSphere& xiiMeshResourceDescriptor::GetBounds() const
{
  return m_Bounds;
}

void xiiMeshResourceDescriptor::AddSubMesh(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiMaterialIndex)
{
  SubMesh p;
  p.m_uiFirstPrimitive = uiFirstPrimitive;
  p.m_uiPrimitiveCount = uiPrimitiveCount;
  p.m_uiMaterialIndex  = uiMaterialIndex;
  p.m_Bounds           = xiiBoundingBoxSphere::MakeInvalid();

  m_SubMeshes.PushBack(p);
}

void xiiMeshResourceDescriptor::SetMaterial(xiiUInt32 uiMaterialIndex, const char* szPathToMaterial)
{
  m_Materials.EnsureCount(uiMaterialIndex + 1);

  m_Materials[uiMaterialIndex].m_sPath = szPathToMaterial;
}

xiiResult xiiMeshResourceDescriptor::Save(const char* szFile)
{
  XII_LOG_BLOCK("xiiMeshResourceDescriptor::Save", szFile);

  xiiFileWriter file;
  if (file.Open(szFile, 1024 * 1024).Failed())
  {
    xiiLog::Error("Failed to open file '{0}'", szFile);
    return XII_FAILURE;
  }

  Save(file);
  return XII_SUCCESS;
}

void xiiMeshResourceDescriptor::Save(xiiStreamWriter& inout_stream)
{
  xiiUInt8 uiVersion = 7;
  inout_stream << uiVersion;

  xiiUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  xiiCompressedStreamWriterZstd compressor(&inout_stream, 0, xiiCompressedStreamWriterZstd::Compression::Average);
  xiiChunkStreamWriter          chunk(compressor);
#else
  xiiChunkStreamWriter chunk(stream);
#endif

  inout_stream << uiCompressionMode;

  chunk.BeginStream(1);

  {
    chunk.BeginChunk("Materials", 1);

    // number of materials
    chunk << m_Materials.GetCount();

    // each material
    for (xiiUInt32 idx = 0; idx < m_Materials.GetCount(); ++idx)
    {
      chunk << idx;                      // Material Index
      chunk << m_Materials[idx].m_sPath; // Material Path (data directory relative)
      /// \todo Material Path (relative to mesh file)
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("SubMeshes", 1);

    // number of sub-meshes
    chunk << m_SubMeshes.GetCount();

    for (xiiUInt32 idx = 0; idx < m_SubMeshes.GetCount(); ++idx)
    {
      chunk << idx;                                // Sub-Mesh index
      chunk << m_SubMeshes[idx].m_uiMaterialIndex; // The material to use
      chunk << m_SubMeshes[idx].m_uiFirstPrimitive;
      chunk << m_SubMeshes[idx].m_uiPrimitiveCount;
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("MeshInfo", 4);

    // Number of vertices
    chunk << m_MeshBufferDescriptor.GetVertexCount();

    // Number of triangles
    chunk << m_MeshBufferDescriptor.GetPrimitiveCount();

    // Whether any index buffer is used
    chunk << m_MeshBufferDescriptor.HasIndexBuffer();

    // Whether the indices are 16 or 32 Bit, always false, if no index buffer is used
    chunk << (m_MeshBufferDescriptor.HasIndexBuffer() && m_MeshBufferDescriptor.Uses32BitIndices());

    // Number of vertex streams
    chunk << m_MeshBufferDescriptor.GetInputLayout().m_VertexStreams.GetCount();

    // Version 3: Topology
    chunk << (xiiUInt8)m_MeshBufferDescriptor.GetTopology();

    for (xiiUInt32 idx = 0; idx < m_MeshBufferDescriptor.GetInputLayout().m_VertexStreams.GetCount(); ++idx)
    {
      const auto& vs = m_MeshBufferDescriptor.GetInputLayout().m_VertexStreams[idx];

      chunk << idx; // Vertex stream index
      chunk << (xiiInt32)vs.m_Format;
      chunk << (xiiInt32)vs.m_Semantic;
      chunk << vs.m_uiElementSize; // not needed, but can be used to check that memory layout has not changed
      chunk << vs.m_uiOffset;      // not needed, but can be used to check that memory layout has not changed
    }

    // Version 2
    if (!m_Bounds.IsValid())
    {
      ComputeBounds();
    }

    chunk << m_Bounds.m_vCenter;
    chunk << m_Bounds.m_vBoxHalfExtends;
    chunk << m_Bounds.m_fSphereRadius;
    // Version 4
    chunk << m_fMaxBoneVertexOffset;

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("VertexBuffer", 1);

    // size in bytes
    chunk << m_MeshBufferDescriptor.GetVertexBufferData().GetCount();

    if (!m_MeshBufferDescriptor.GetVertexBufferData().IsEmpty())
    {
      chunk.WriteBytes(m_MeshBufferDescriptor.GetVertexBufferData().GetData(), m_MeshBufferDescriptor.GetVertexBufferData().GetCount()).IgnoreResult();
    }

    chunk.EndChunk();
  }

  // always write the index buffer chunk, even if it is empty
  {
    chunk.BeginChunk("IndexBuffer", 1);

    // size in bytes
    chunk << m_MeshBufferDescriptor.GetIndexBufferData().GetCount();

    if (!m_MeshBufferDescriptor.GetIndexBufferData().IsEmpty())
    {
      chunk.WriteBytes(m_MeshBufferDescriptor.GetIndexBufferData().GetData(), m_MeshBufferDescriptor.GetIndexBufferData().GetCount()).IgnoreResult();
    }

    chunk.EndChunk();
  }

  if (!m_Bones.IsEmpty())
  {
    chunk.BeginChunk("BindPose", 1);

    chunk.WriteHashTable(m_Bones).IgnoreResult();

    chunk.EndChunk();
  }

  if (m_hDefaultSkeleton.IsValid())
  {
    chunk.BeginChunk("Skeleton", 1);

    chunk << m_hDefaultSkeleton;

    chunk.EndChunk();
  }

  chunk.EndStream();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  compressor.FinishCompressedStream().IgnoreResult();

  xiiLog::Dev("Compressed mesh data from {0} KB to {1} KB ({2}%%)", xiiArgF((float)compressor.GetUncompressedSize() / 1024.0f, 1), xiiArgF((float)compressor.GetCompressedSize() / 1024.0f, 1), xiiArgF(100.0f * compressor.GetCompressedSize() / compressor.GetUncompressedSize(), 1));
#endif
}

xiiResult xiiMeshResourceDescriptor::Load(const char* szFile)
{
  XII_LOG_BLOCK("xiiMeshResourceDescriptor::Load", szFile);

  xiiFileReader file;
  if (file.Open(szFile, 1024 * 1024).Failed())
  {
    xiiLog::Error("Failed to open file '{0}'", szFile);
    return XII_FAILURE;
  }

  // skip asset header
  xiiAssetFileHeader assetHeader;
  XII_SUCCEED_OR_RETURN(assetHeader.Read(file));

  return Load(file);
}

xiiResult xiiMeshResourceDescriptor::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  // version 4 and below is broken
  if (uiVersion <= 4)
    return XII_FAILURE;

  xiiUInt8 uiCompressionMode = 0;
  if (uiVersion >= 6)
  {
    inout_stream >> uiCompressionMode;
  }

  xiiStreamReader* pCompressor = &inout_stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  xiiCompressedStreamReaderZstd decompressorZstd;
#endif

  switch (uiCompressionMode)
  {
    case 0:
      break;

    case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      decompressorZstd.SetInputStream(&inout_stream);
      pCompressor = &decompressorZstd;
      break;
#else
      xiiLog::Error("Mesh is compressed with zstandard, but support for this compressor is not compiled in.");
      return XII_FAILURE;
#endif

    default:
      xiiLog::Error("Mesh is compressed with an unknown algorithm.");
      return XII_FAILURE;
  }

  xiiChunkStreamReader chunk(*pCompressor);
  chunk.BeginStream();

  xiiUInt32 count;
  bool      bHasIndexBuffer  = false;
  bool      b32BitIndices    = false;
  bool      bCalculateBounds = true;

  while (chunk.GetCurrentChunk().m_bValid)
  {
    const auto& ci = chunk.GetCurrentChunk();

    if (ci.m_sChunkName == "Materials")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        xiiLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return XII_FAILURE;
      }

      // number of materials
      chunk >> count;
      m_Materials.SetCount(count);

      // each material
      for (xiiUInt32 i = 0; i < m_Materials.GetCount(); ++i)
      {
        xiiUInt32 idx;
        chunk >> idx;                      // Material Index
        chunk >> m_Materials[idx].m_sPath; // Material Path (data directory relative)
        /// \todo Material Path (relative to mesh file)
      }
    }

    if (chunk.GetCurrentChunk().m_sChunkName == "SubMeshes")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        xiiLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return XII_FAILURE;
      }

      // number of sub-meshes
      chunk >> count;
      m_SubMeshes.SetCount(count);

      for (xiiUInt32 i = 0; i < m_SubMeshes.GetCount(); ++i)
      {
        xiiUInt32 idx;
        chunk >> idx;                                // Sub-Mesh index
        chunk >> m_SubMeshes[idx].m_uiMaterialIndex; // The material to use
        chunk >> m_SubMeshes[idx].m_uiFirstPrimitive;
        chunk >> m_SubMeshes[idx].m_uiPrimitiveCount;

        /// \todo load from file
        m_SubMeshes[idx].m_Bounds = xiiBoundingBoxSphere::MakeInvalid();
      }
    }

    if (ci.m_sChunkName == "MeshInfo")
    {
      if (ci.m_uiChunkVersion > 4)
      {
        xiiLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return XII_FAILURE;
      }

      // Number of vertices
      xiiUInt32 uiVertexCount = 0;
      chunk >> uiVertexCount;

      // Number of primitives
      xiiUInt32 uiPrimitiveCount = 0;
      chunk >> uiPrimitiveCount;

      // Whether any index buffer is used
      chunk >> bHasIndexBuffer;

      // Whether the indices are 16 or 32 Bit, always false, if no index buffer is used
      chunk >> b32BitIndices;

      // Number of vertex streams
      xiiUInt32 uiStreamCount = 0;
      chunk >> uiStreamCount;

      xiiUInt8 uiTopology = xiiGALPrimitiveTopology::Triangles;
      if (ci.m_uiChunkVersion >= 3)
      {
        chunk >> uiTopology;
      }

      for (xiiUInt32 i = 0; i < uiStreamCount; ++i)
      {
        xiiUInt32 idx;
        chunk >> idx; // Vertex stream index
        XII_ASSERT_DEV(idx == i, "Invalid stream index ({0}) in file (should be {1})", idx, i);

        xiiInt32  iFormat, iSemantic;
        xiiUInt16 uiElementSize, uiOffset;

        chunk >> iFormat;
        chunk >> iSemantic;
        chunk >> uiElementSize; // not needed, but can be used to check that memory layout has not changed
        chunk >> uiOffset;      // not needed, but can be used to check that memory layout has not changed

        if (uiVersion < 7)
        {
          // xiiGALVertexAttributeSemantic got new elements inserted
          // need to adjust old file formats accordingly

          if (iSemantic >= xiiGALVertexAttributeSemantic::Color2) // should be xiiGALVertexAttributeSemantic::TexCoord0 instead
          {
            iSemantic += 6;
          }
        }

        m_MeshBufferDescriptor.AddStream((xiiGALVertexAttributeSemantic::Enum)iSemantic, (xiiEnum<xiiGALTextureFormat>)iFormat);
      }

      m_MeshBufferDescriptor.AllocateStreams(uiVertexCount, (xiiGALPrimitiveTopology::Enum)uiTopology, uiPrimitiveCount);

      // Version 2
      if (ci.m_uiChunkVersion >= 2)
      {
        bCalculateBounds = false;
        chunk >> m_Bounds.m_vCenter;
        chunk >> m_Bounds.m_vBoxHalfExtends;
        chunk >> m_Bounds.m_fSphereRadius;
      }
      if (ci.m_uiChunkVersion >= 4)
      {
        chunk >> m_fMaxBoneVertexOffset;
      }
    }

    if (ci.m_sChunkName == "VertexBuffer")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        xiiLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return XII_FAILURE;
      }

      // size in bytes
      chunk >> count;
      m_MeshBufferDescriptor.GetVertexBufferData().SetCountUninitialized(count);

      if (!m_MeshBufferDescriptor.GetVertexBufferData().IsEmpty())
        chunk.ReadBytes(m_MeshBufferDescriptor.GetVertexBufferData().GetData(), m_MeshBufferDescriptor.GetVertexBufferData().GetCount());
    }

    if (ci.m_sChunkName == "IndexBuffer")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        xiiLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return XII_FAILURE;
      }

      // size in bytes
      chunk >> count;
      m_MeshBufferDescriptor.GetIndexBufferData().SetCountUninitialized(count);

      if (!m_MeshBufferDescriptor.GetIndexBufferData().IsEmpty())
        chunk.ReadBytes(m_MeshBufferDescriptor.GetIndexBufferData().GetData(), m_MeshBufferDescriptor.GetIndexBufferData().GetCount());
    }

    if (ci.m_sChunkName == "BindPose")
    {
      XII_SUCCEED_OR_RETURN(chunk.ReadHashTable(m_Bones));
    }

    if (ci.m_sChunkName == "Skeleton")
    {
      chunk >> m_hDefaultSkeleton;
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  if (bCalculateBounds)
  {
    ComputeBounds();

    auto b = m_Bounds;
    xiiLog::Info("Calculated Bounds: {0} | {1} | {2} - {3} | {4} | {5}", xiiArgF(b.m_vCenter.x, 2), xiiArgF(b.m_vCenter.y, 2), xiiArgF(b.m_vCenter.z, 2), xiiArgF(b.m_vBoxHalfExtends.x, 2), xiiArgF(b.m_vBoxHalfExtends.y, 2), xiiArgF(b.m_vBoxHalfExtends.z, 2));
  }

  return XII_SUCCESS;
}

void xiiMeshResourceDescriptor::ComputeBounds()
{
  if (m_hMeshBuffer.IsValid())
  {
    xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(m_hMeshBuffer, xiiResourceAcquireMode::AllowLoadingFallback);
    m_Bounds = pMeshBuffer->GetBounds();
  }
  else
  {
    m_Bounds = m_MeshBufferDescriptor.ComputeBounds();
  }
}

xiiResult xiiMeshResourceDescriptor::BoneData::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_GlobalInverseRestPoseMatrix;
  inout_stream << m_uiBoneIndex;

  return XII_SUCCESS;
}

xiiResult xiiMeshResourceDescriptor::BoneData::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_GlobalInverseRestPoseMatrix;
  inout_stream >> m_uiBoneIndex;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_MeshResourceDescriptor);
