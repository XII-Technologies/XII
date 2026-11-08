/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/StreamWithStats.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/Compression.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <meshoptimizer/meshoptimizer.h>

xiiResult xiiGeometryPage::Serialize(xiiStreamWriter& stream) const
{
  stream << m_uiOffset << m_uiSize << m_bRequired << m_uiStoredSize;
  stream << m_uiFirstCluster << m_uiClusterCount << m_uiParent << m_uiFirstChild << m_uiChildCount << m_fGeometricError;
  return XII_SUCCESS;
}

xiiResult xiiGeometryPage::Deserialize(xiiStreamReader& stream)
{
  stream >> m_uiOffset >> m_uiSize >> m_bRequired >> m_uiStoredSize;
  stream >> m_uiFirstCluster >> m_uiClusterCount >> m_uiParent >> m_uiFirstChild >> m_uiChildCount >> m_fGeometricError;
  return XII_SUCCESS;
}

xiiResult xiiMeshBufferResourceDescriptor::BuildGeometryPages()
{
  m_sGeometryPageFile.Clear();
  m_uiGeometryFileOffset = m_uiGeometryFileHash = m_uiPageDataBytes = 0U;
  m_uiGeometryFileVersion                                           = 0U;
  m_GeometryPages.Clear();
  m_GeometryPageData.Clear();
  if (!m_bStreamableGeometry || m_ResourceUsage != xiiGALResourceUsage::Immutable || m_Meshlets.IsEmpty())
    return XII_SUCCESS;

  const auto                 stride   = GetVertexStride();
  const xiiMeshVertexStream* position = nullptr;
  for (const auto& stream : m_VertexStreams)
  {
    if (stream.m_Semantic == xiiMeshVertexSemantic::BoneIndices0 || stream.m_Semantic == xiiMeshVertexSemantic::BoneWeights0)
    {
      m_bStreamableGeometry = false;
      return XII_SUCCESS;
    }
    if (stream.m_Semantic == xiiMeshVertexSemantic::Position && stream.m_Format == xiiMeshVertexStreamFormat::Float3)
      position = &stream;
  }
  if (!position || stride == 0U || position->m_uiOffset + sizeof(xiiVec3) > stride ||
      xiiUInt64(GetVertexCount()) * stride > m_VertexData.GetCount())
    return XII_FAILURE;
  xiiDynamicArray<xiiVec3> positions;
  positions.SetCountUninitialized(GetVertexCount());
  for (xiiUInt32 vertex = 0U; vertex < positions.GetCount(); ++vertex)
    xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&positions[vertex]), m_VertexData.GetData() + vertex * stride + position->m_uiOffset, sizeof(xiiVec3));

  auto                                                    original = std::move(m_Meshlets);
  xiiDynamicArray<xiiMeshlet, xiiAlignedAllocatorWrapper> clusters;
  // A page owns its compact vertex attributes, remaps and triangles. Adjacent
  // meshlets deduplicate vertices here rather than allocating per cluster.
  auto pack = [&](xiiArrayPtr<const xiiMeshlet> input, float error) -> xiiResult {
    xiiDynamicArray<xiiUInt8>          vertices, primitives;
    xiiDynamicArray<xiiUInt32>         remaps;
    xiiHashTable<xiiUInt32, xiiUInt32> vertexMap;
    const auto                         pageIndex    = m_GeometryPages.GetCount();
    const auto                         firstCluster = clusters.GetCount();
    for (auto cluster : input)
    {
      const auto remapOffset     = remaps.GetCount();
      const auto primitiveOffset = primitives.GetCount();
      for (xiiUInt32 vertex = 0U; vertex < cluster.m_uiVertexCount; ++vertex)
      {
        const auto source = m_MeshletVertexRemap[cluster.m_uiVertexRemapOffset + vertex];
        xiiUInt32  local  = 0U;
        if (!vertexMap.TryGetValue(source, local))
        {
          local = vertexMap.GetCount();
          vertexMap.Insert(source, local);
          vertices.PushBackRange(m_VertexData.GetArrayPtr().GetSubArray(source * stride, stride));
        }
        remaps.PushBack(local);
      }
      primitives.PushBackRange(m_MeshletPrimitiveIndices.GetArrayPtr().GetSubArray(cluster.m_uiPrimitiveIndexOffset, cluster.m_uiPrimitiveCount * 3U));
      if (cluster.m_uiGpuPadding[1] == 0U)
        cluster.m_uiGpuPadding[1] = cluster.m_uiPrimitiveCount;
      cluster.m_Page        = xiiVec4U32(pageIndex, remapOffset, primitiveOffset, 0U);
      cluster.m_Hierarchy.x = error;
      cluster.m_Hierarchy.y = static_cast<float>(pageIndex + 1U);
      clusters.PushBack(cluster);
    }
    const auto      vertexBytes = vertices.GetCount();
    const xiiUInt32 remapBytes  = remaps.GetCount() * sizeof(xiiUInt32);
    const auto      size        = xiiMath::RoundUp(vertexBytes + remapBytes + primitives.GetCount(), 4U);
    if (size == 0U || size + xiiUInt64(input.GetCount()) * sizeof(xiiMeshlet) > 128U * 1024U)
      return XII_FAILURE;
    xiiDynamicArray<xiiUInt8> payload;
    payload.SetCount(size, 0U);
    xiiMemoryUtils::Copy(payload.GetData(), vertices.GetData(), vertexBytes);
    xiiMemoryUtils::Copy(payload.GetData() + vertexBytes, reinterpret_cast<const xiiUInt8*>(remaps.GetData()), remapBytes);
    xiiMemoryUtils::Copy(payload.GetData() + vertexBytes + remapBytes, primitives.GetData(), primitives.GetCount());
    for (xiiUInt32 index = firstCluster; index < clusters.GetCount(); ++index)
    {
      auto& cluster    = clusters[index];
      cluster.m_Page.y = vertexBytes + cluster.m_Page.y * sizeof(xiiUInt32);
      cluster.m_Page.z += vertexBytes + remapBytes;
      cluster.m_Page.w = vertexMap.GetCount();
    }
    if (payload.GetCount() > xiiMath::MaxValue<xiiUInt32>() - m_GeometryPageData.GetCount())
      return XII_FAILURE;
    xiiGeometryPage page;
    page.m_uiOffset        = m_GeometryPageData.GetCount();
    page.m_uiSize          = size;
    page.m_uiFirstCluster  = firstCluster;
    page.m_uiClusterCount  = input.GetCount();
    page.m_fGeometricError = error;
    m_GeometryPages.PushBack(page);
    m_GeometryPageData.PushBackRange(payload);
    return XII_SUCCESS;
  };

  for (xiiUInt32 sectionBegin = 0U; sectionBegin < original.GetCount();)
  {
    xiiUInt32 sectionEnd = sectionBegin + 1U;
    while (sectionEnd < original.GetCount() && original[sectionEnd].m_uiSectionIndex == original[sectionBegin].m_uiSectionIndex &&
           original[sectionEnd].m_uiLodIndex == original[sectionBegin].m_uiLodIndex)
      ++sectionEnd;
    xiiDynamicArray<xiiUInt32> level;
    for (xiiUInt32 first = sectionBegin; first < sectionEnd;)
    {
      auto      end      = first;
      xiiUInt32 estimate = 0U;
      while (end < sectionEnd)
      {
        const auto&     cluster = original[end];
        const xiiUInt32 bytes   = cluster.m_uiVertexCount * (stride + static_cast<xiiUInt32>(sizeof(xiiUInt32))) + cluster.m_uiPrimitiveCount * 3U + sizeof(xiiMeshlet);
        if (end != first && estimate + bytes > 64U * 1024U)
          break;
        estimate += bytes;
        ++end;
      }
      level.PushBack(m_GeometryPages.GetCount());
      XII_SUCCEED_OR_RETURN(pack(original.GetArrayPtr().GetSubArray(first, end - first), 0.0f));
      first = end;
    }
    // Each node is a complete representation of up to eight child pages.
    // Only roots remain pinned. All intermediate and leaf payloads are caches.
    for (xiiUInt32 depth = 0U; level.GetCount() > 1U && depth < 7U; ++depth)
    {
      xiiDynamicArray<xiiUInt32> next;
      for (xiiUInt32 first = 0U; first < level.GetCount();)
      {
        const auto childCount = xiiMath::Min(8U, level.GetCount() - first);
        if (childCount == 1U)
        {
          next.PushBack(level[first++]);
          continue;
        }
        // Carried nodes can make the level non-contiguous. They remain roots
        // rather than manufacturing a discontinuous dependency range.
        bool contiguous = true;
        for (xiiUInt32 child = 1U; child < childCount; ++child)
          contiguous &= level[first + child] == level[first] + child;
        if (!contiguous)
        {
          next.PushBack(level[first++]);
          continue;
        }
        xiiDynamicArray<xiiUInt32> indices, simplified;
        auto                       bounds        = xiiBoundingSphere::MakeInvalid();
        float                      childError    = 0.0f;
        xiiUInt32                  coverageFirst = xiiInvalidIndex, coverageEnd = 0U;
        for (xiiUInt32 child = 0U; child < childCount; ++child)
        {
          const auto& page = m_GeometryPages[level[first + child]];
          childError       = xiiMath::Max(childError, page.m_fGeometricError);
          for (xiiUInt32 c = 0U; c < page.m_uiClusterCount; ++c)
          {
            const auto& cluster = clusters[page.m_uiFirstCluster + c];
            if (!bounds.IsValid())
              bounds = cluster.m_Bounds;
            else
              bounds.ExpandToInclude(cluster.m_Bounds);
            coverageFirst      = xiiMath::Min(coverageFirst, cluster.m_uiFirstPrimitive);
            const auto covered = cluster.m_uiGpuPadding[1];
            coverageEnd        = xiiMath::Max(coverageEnd, cluster.m_uiFirstPrimitive + covered);
            for (xiiUInt32 p = 0U; p < cluster.m_uiPrimitiveCount * 3U; ++p)
              indices.PushBack(m_MeshletVertexRemap[cluster.m_uiVertexRemapOffset + m_MeshletPrimitiveIndices[cluster.m_uiPrimitiveIndexOffset + p]]);
          }
        }
        simplified.SetCountUninitialized(indices.GetCount());
        float      error  = 0.0f;
        const auto target = xiiMath::Max(3U, (48U * 1024U / (stride * 3U + 7U)) * 3U);
        const auto count  = meshopt_simplify(simplified.GetData(), indices.GetData(), indices.GetCount(), &positions[0].x, positions.GetCount(),
                                             sizeof(xiiVec3), xiiMath::Min(target, indices.GetCount() / 2U / 3U * 3U), xiiMath::MaxValue<float>(),
                                             meshopt_SimplifyErrorAbsolute | meshopt_SimplifyLockBorder, &error);
        if (count == 0U || count % 3U != 0U || !xiiMath::IsFinite(error))
          return XII_FAILURE;
        if (count >= indices.GetCount() * 3U / 4U)
        {
          for (xiiUInt32 child = 0U; child < childCount; ++child)
            next.PushBack(level[first + child]);
          first += childCount;
          continue;
        }
        simplified.SetCount(static_cast<xiiUInt32>(count));
        const auto                       capacity = meshopt_buildMeshletsBound(count, xiiMeshlet::s_uiMaxVertices, xiiMeshlet::s_uiMaxPrimitives);
        xiiDynamicArray<meshopt_Meshlet> meshlets;
        xiiDynamicArray<xiiUInt32>       remaps;
        xiiDynamicArray<xiiUInt8>        triangles;
        meshlets.SetCount(static_cast<xiiUInt32>(capacity));
        remaps.SetCountUninitialized(static_cast<xiiUInt32>(capacity * xiiMeshlet::s_uiMaxVertices));
        triangles.SetCountUninitialized(static_cast<xiiUInt32>(capacity * xiiMeshlet::s_uiMaxPrimitives * 3U));
        const auto                                              meshletCount = meshopt_buildMeshlets(meshlets.GetData(), remaps.GetData(), triangles.GetData(), simplified.GetData(), count,
                                                                                                     &positions[0].x, positions.GetCount(), sizeof(xiiVec3), xiiMeshlet::s_uiMaxVertices, xiiMeshlet::s_uiMaxPrimitives, 0.5f);
        xiiDynamicArray<xiiMeshlet, xiiAlignedAllocatorWrapper> parents;
        for (size_t index = 0U; index < meshletCount; ++index)
        {
          const auto& built               = meshlets[static_cast<xiiUInt32>(index)];
          auto        parent              = original[sectionBegin];
          parent.m_uiFirstPrimitive       = coverageFirst;
          parent.m_uiPrimitiveCount       = static_cast<xiiUInt16>(built.triangle_count);
          parent.m_uiVertexCount          = static_cast<xiiUInt16>(built.vertex_count);
          parent.m_uiVertexRemapOffset    = m_MeshletVertexRemap.GetCount();
          parent.m_uiPrimitiveIndexOffset = m_MeshletPrimitiveIndices.GetCount();
          parent.m_Bounds                 = bounds;
          parent.m_fConeCutoff            = -1.0f;
          parent.m_uiGpuPadding[1]        = coverageEnd - coverageFirst;
          parent.m_Hierarchy              = xiiVec4(0.0f);
          m_MeshletVertexRemap.PushBackRange(xiiMakeArrayPtr(remaps.GetData() + built.vertex_offset, built.vertex_count));
          m_MeshletPrimitiveIndices.PushBackRange(xiiMakeArrayPtr(triangles.GetData() + built.triangle_offset, built.triangle_count * 3U));
          parents.PushBack(parent);
        }
        const auto parentPage = m_GeometryPages.GetCount();
        XII_SUCCEED_OR_RETURN(pack(parents, childError + error));
        auto& parent          = m_GeometryPages[parentPage];
        parent.m_uiFirstChild = level[first];
        parent.m_uiChildCount = childCount;
        for (xiiUInt32 child = 0U; child < childCount; ++child)
          m_GeometryPages[level[first + child]].m_uiParent = parentPage;
        next.PushBack(parentPage);
        first += childCount;
      }
      if (next.GetCount() == level.GetCount())
        break;
      level = std::move(next);
    }
    for (const auto pageIndex : level)
    {
      auto& page       = m_GeometryPages[pageIndex];
      page.m_bRequired = true;
      for (xiiUInt32 cluster = 0U; cluster < page.m_uiClusterCount; ++cluster)
        clusters[page.m_uiFirstCluster + cluster].m_Hierarchy.z = 1.0f;
    }
    sectionBegin = sectionEnd;
  }
  // Cluster metadata is part of the independently resident payload. Finalize it
  // after root flags and hierarchy links have been assigned, then compress once.
  xiiDynamicArray<xiiUInt8> compressed, payload, encoded;
  for (auto& page : m_GeometryPages)
  {
    payload             = m_GeometryPageData.GetArrayPtr().GetSubArray(page.m_uiOffset, page.m_uiSize);
    const auto metadata = clusters.GetArrayPtr().GetSubArray(page.m_uiFirstCluster, page.m_uiClusterCount).ToByteArray();
    payload.PushBackRange(metadata);
    XII_SUCCEED_OR_RETURN(xiiCompressionUtils::Compress(payload, xiiCompressionMethod::ZStd, encoded));
    if (encoded.GetCount() > xiiMath::MaxValue<xiiUInt32>() - compressed.GetCount())
      return XII_FAILURE;
    page.m_uiSize       = payload.GetCount();
    page.m_uiOffset     = compressed.GetCount();
    page.m_uiStoredSize = encoded.GetCount();
    compressed.PushBackRange(encoded);
  }
  m_GeometryPageData = std::move(compressed);
  m_MeshletMaterialIndices.Clear();
  for (const auto& cluster : clusters)
    m_MeshletMaterialIndices.PushBack(cluster.m_uiMaterialIndex);
  m_Meshlets        = std::move(clusters);
  m_uiPageDataBytes = m_GeometryPageData.GetCount();
  return XII_SUCCESS;
}

xiiResult xiiMeshBufferResourceDescriptor::ReadGeometryPage(xiiUInt32 index, xiiDynamicArray<xiiUInt8>& out_data) const
{
  if (index >= m_GeometryPages.GetCount())
    return XII_FAILURE;
  const auto& page       = m_GeometryPages[index];
  const auto  storedSize = page.m_uiStoredSize == 0U ? page.m_uiSize : page.m_uiStoredSize;
  // The compiled codec can add a small framing overhead to an incompressible page.
  if (page.m_uiSize == 0U || page.m_uiSize > 128U * 1024U || storedSize > 132U * 1024U)
    return XII_FAILURE;
  xiiDynamicArray<xiiUInt8> encoded;
  if (m_sGeometryPageFile.IsEmpty())
  {
    if (page.m_uiOffset > m_GeometryPageData.GetCount() || storedSize > m_GeometryPageData.GetCount() - page.m_uiOffset)
      return XII_FAILURE;
    encoded = m_GeometryPageData.GetArrayPtr().GetSubArray(page.m_uiOffset, storedSize);
  }
  else
  {
    xiiFileReader file;
    XII_SUCCEED_OR_RETURN(file.Open(m_sGeometryPageFile));
    xiiStreamReaderWithStats stream(&file);
    xiiAssetFileHeader       header;
    XII_SUCCEED_OR_RETURN(header.Read(stream));
    if (!header.IsFileUpToDate(m_uiGeometryFileHash, m_uiGeometryFileVersion) || m_uiGeometryFileOffset + page.m_uiOffset < stream.m_uiBytesRead)
      return XII_FAILURE;
    const auto skip = m_uiGeometryFileOffset + page.m_uiOffset - stream.m_uiBytesRead;
    if (stream.SkipBytes(skip) != skip)
      return XII_FAILURE;
    encoded.SetCountUninitialized(storedSize);
    if (stream.ReadBytes(encoded.GetData(), storedSize) != storedSize)
      return XII_FAILURE;
  }
  if (page.m_uiStoredSize == 0U)
    out_data = std::move(encoded);
  else
    XII_SUCCEED_OR_RETURN(xiiCompressionUtils::Decompress(encoded, xiiCompressionMethod::ZStd, page.m_uiSize, out_data));
  return out_data.GetCount() == page.m_uiSize ? XII_SUCCESS : XII_FAILURE;
}
