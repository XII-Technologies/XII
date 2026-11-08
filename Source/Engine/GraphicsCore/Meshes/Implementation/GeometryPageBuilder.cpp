/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Containers/HashTable.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <meshoptimizer/meshoptimizer.h>

xiiResult xiiGeometryPage::Serialize(xiiStreamWriter& stream) const
{
  stream << m_uiOffset << m_uiSize << m_bRequired;
  return XII_SUCCESS;
}

xiiResult xiiGeometryPage::Deserialize(xiiStreamReader& stream)
{
  stream >> m_uiOffset >> m_uiSize >> m_bRequired;
  return XII_SUCCESS;
}

xiiResult xiiMeshBufferResourceDescriptor::BuildGeometryPages()
{
  m_GeometryPages.Clear();
  m_GeometryPageData.Clear();
  // Tiny meshes occupy less than a page and benefit from direct residency.
  if (m_VertexData.GetCount() + xiiUInt64(m_IndexData.GetCount()) < 128U * 1024U)
    return XII_SUCCESS;
  if (!m_bStreamableGeometry || m_ResourceUsage != xiiGALResourceUsage::Immutable || m_Meshlets.IsEmpty())
    return XII_SUCCESS;

  const auto                 stride   = GetVertexStride();
  const xiiMeshVertexStream* position = nullptr;
  for (const auto& stream : m_VertexStreams)
    if (stream.m_Semantic == xiiMeshVertexSemantic::Position && stream.m_Format == xiiMeshVertexStreamFormat::Float3)
      position = &stream;
  if (!position || stride == 0U || position->m_uiOffset + sizeof(xiiVec3) > stride ||
      xiiUInt64(GetVertexCount()) * stride > m_VertexData.GetCount())
    return XII_FAILURE;

  const auto                                              original = std::move(m_Meshlets);
  xiiDynamicArray<xiiMeshlet, xiiAlignedAllocatorWrapper> clusters;
  xiiDynamicArray<xiiUInt8>                               vertices, primitives;
  xiiDynamicArray<xiiUInt32>                              remaps, pageClusters;
  xiiHashTable<xiiUInt32, xiiUInt32>                      vertexMap;
  auto                                                    flush = [&](bool required) {
    if (pageClusters.IsEmpty())
      return;
    const xiiUInt32 vertexBytes = vertices.GetCount();
    const xiiUInt32 remapBytes  = remaps.GetCount() * sizeof(xiiUInt32);
    const xiiUInt32 size        = xiiMath::RoundUp(vertexBytes + remapBytes + primitives.GetCount(), 4U);
    const xiiUInt32 offset      = m_GeometryPageData.GetCount();
    m_GeometryPageData.SetCount(offset + size, 0U);
    xiiMemoryUtils::Copy(m_GeometryPageData.GetData() + offset, vertices.GetData(), vertexBytes);
    xiiMemoryUtils::Copy(m_GeometryPageData.GetData() + offset + vertexBytes, reinterpret_cast<const xiiUInt8*>(remaps.GetData()), remapBytes);
    xiiMemoryUtils::Copy(m_GeometryPageData.GetData() + offset + vertexBytes + remapBytes, primitives.GetData(), primitives.GetCount());
    for (const auto index : pageClusters)
    {
      auto& cluster    = clusters[index];
      cluster.m_Page.x = m_GeometryPages.GetCount();
      cluster.m_Page.y = vertexBytes + cluster.m_Page.y * sizeof(xiiUInt32);
      cluster.m_Page.z += vertexBytes + remapBytes;
      cluster.m_Page.w = vertexMap.GetCount();
    }
    m_GeometryPages.PushBack({offset, size, required});
    vertices.Clear();
    remaps.Clear();
    primitives.Clear();
    pageClusters.Clear();
    vertexMap.Clear();
  };
  auto append = [&](xiiMeshlet cluster) {
    cluster.m_Page.y = remaps.GetCount();
    cluster.m_Page.z = primitives.GetCount();
    for (xiiUInt32 v = 0U; v < cluster.m_uiVertexCount; ++v)
    {
      const auto source = m_MeshletVertexRemap[cluster.m_uiVertexRemapOffset + v];
      xiiUInt32  local;
      if (!vertexMap.TryGetValue(source, local))
      {
        local = vertexMap.GetCount();
        vertexMap.Insert(source, local);
        vertices.PushBackRange(m_VertexData.GetArrayPtr().GetSubArray(source * stride, stride));
      }
      remaps.PushBack(local);
    }
    primitives.PushBackRange(m_MeshletPrimitiveIndices.GetArrayPtr().GetSubArray(cluster.m_uiPrimitiveIndexOffset, cluster.m_uiPrimitiveCount * 3U));
    pageClusters.PushBack(clusters.GetCount());
    clusters.PushBack(cluster);
  };

  for (xiiUInt32 first = 0U; first < original.GetCount();)
  {
    const xiiUInt32 begin    = first;
    xiiUInt32       estimate = 0U;
    while (first < original.GetCount() && original[first].m_uiLodIndex == original[begin].m_uiLodIndex &&
           original[first].m_uiSectionIndex == original[begin].m_uiSectionIndex)
    {
      const auto& cluster = original[first];
      const auto  bytes   = cluster.m_uiVertexCount * (stride + 4U) + cluster.m_uiPrimitiveCount * 3U;
      if (first > begin && estimate + bytes > 64U * 1024U)
        break;
      estimate += bytes;
      append(cluster);
      ++first;
    }
    const xiiUInt32 clusterBegin = clusters.GetCount() - (first - begin);
    const xiiUInt32 detailPage   = m_GeometryPages.GetCount();
    flush(false);
    for (xiiUInt32 i = begin; i < first; ++i)
    {
      auto                       proxy = original[i];
      xiiDynamicArray<xiiVec3>   positions;
      xiiDynamicArray<xiiUInt32> indices, simplified;
      positions.SetCountUninitialized(proxy.m_uiVertexCount);
      for (xiiUInt32 v = 0U; v < proxy.m_uiVertexCount; ++v)
      {
        const auto source = m_MeshletVertexRemap[proxy.m_uiVertexRemapOffset + v];
        xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&positions[v]), m_VertexData.GetData() + source * stride + position->m_uiOffset, sizeof(xiiVec3));
      }
      for (xiiUInt32 p = 0U; p < proxy.m_uiPrimitiveCount * 3U; ++p)
        indices.PushBack(m_MeshletPrimitiveIndices[proxy.m_uiPrimitiveIndexOffset + p]);
      simplified.SetCountUninitialized(indices.GetCount());
      float      error = 0.0f;
      const auto count = meshopt_simplify(simplified.GetData(), indices.GetData(), indices.GetCount(), &positions[0].x,
                                          positions.GetCount(), sizeof(xiiVec3), xiiMath::Max(3U, indices.GetCount() / 12U * 3U), 1.0f,
                                          meshopt_SimplifyErrorAbsolute | meshopt_SimplifyLockBorder, &error);
      if (count == 0U || count % 3U != 0U || !xiiMath::IsFinite(error))
        return XII_FAILURE;
      const auto originalRemap = proxy.m_uiVertexRemapOffset;
      xiiUInt32  localRemap[xiiMeshlet::s_uiMaxVertices];
      xiiMemoryUtils::PatternFill(localRemap, 0xFF, XII_ARRAY_SIZE(localRemap));
      proxy.m_uiVertexRemapOffset = m_MeshletVertexRemap.GetCount();
      proxy.m_uiVertexCount       = 0U;
      for (size_t p = 0U; p < count; ++p)
      {
        auto& index = simplified[static_cast<xiiUInt32>(p)];
        if (localRemap[index] == xiiInvalidIndex)
        {
          localRemap[index]       = proxy.m_uiVertexCount++;
          const auto sourceVertex = m_MeshletVertexRemap[originalRemap + index];
          m_MeshletVertexRemap.PushBack(sourceVertex);
        }
        index = localRemap[index];
      }
      proxy.m_uiPrimitiveIndexOffset = m_MeshletPrimitiveIndices.GetCount();
      proxy.m_uiPrimitiveCount       = static_cast<xiiUInt16>(count / 3U);
      for (size_t p = 0U; p < count; ++p)
        m_MeshletPrimitiveIndices.PushBack(static_cast<xiiUInt8>(simplified[static_cast<xiiUInt32>(p)]));
      // Both representations share conservative original bounds and disable proxy cone culling.
      proxy.m_fConeCutoff                              = -1.0f;
      proxy.m_Hierarchy                                = xiiVec4(error, static_cast<float>(detailPage + 1U), 1.0f, static_cast<float>(original[i].m_uiPrimitiveCount));
      clusters[clusterBegin + i - begin].m_Hierarchy   = proxy.m_Hierarchy;
      clusters[clusterBegin + i - begin].m_Hierarchy.z = 0.0f;
      append(proxy);
    }
    flush(true);
  }
  m_MeshletMaterialIndices.Clear();
  for (const auto& cluster : clusters)
    m_MeshletMaterialIndices.PushBack(cluster.m_uiMaterialIndex);
  m_Meshlets = std::move(clusters);
  return XII_SUCCESS;
}
