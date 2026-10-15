/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <meshoptimizer/meshoptimizer.h>

XII_DEFINE_AS_POD_TYPE(meshopt_Meshlet);

xiiResult xiiMeshBufferResourceDescriptor::BuildMeshlets(xiiUInt32 uiMaxVertices, xiiUInt32 uiMaxPrimitives)
{
  const xiiMeshletBuildRange range{0, m_uiIndexCount / 3};
  return BuildMeshlets(xiiMakeArrayPtr(&range, 1), uiMaxVertices, uiMaxPrimitives);
}

xiiResult xiiMeshBufferResourceDescriptor::BuildMeshlets(xiiArrayPtr<const xiiMeshletBuildRange> ranges, xiiUInt32 uiMaxVertices, xiiUInt32 uiMaxPrimitives)
{
  auto clear = [&]() {
    m_Meshlets.Clear();
    m_MeshletVertexRemap.Clear();
    m_MeshletPrimitiveIndices.Clear();
    m_MeshletMaterialIndices.Clear();
    m_DrawCommands.Clear();
  };
  clear();
  if (ranges.IsEmpty())
    return XII_SUCCESS;

  if (m_Topology != xiiGALPrimitiveTopology::TriangleList || m_uiIndexCount % 3 != 0 || (m_IndexType != xiiGALValueType::UInt16 && m_IndexType != xiiGALValueType::UInt32))
    return XII_FAILURE;

  const xiiUInt32 indexSize = m_IndexType == xiiGALValueType::UInt16 ? 2 : 4;
  if (static_cast<xiiUInt64>(m_uiIndexCount) * indexSize > m_IndexData.GetCount())
    return XII_FAILURE;

  xiiUInt32 largestRange = 0;
  for (const auto& range : ranges)
  {
    if (range.m_uiFirstPrimitive > m_uiIndexCount / 3 || range.m_uiPrimitiveCount > m_uiIndexCount / 3 - range.m_uiFirstPrimitive)
      return XII_FAILURE;
    largestRange = xiiMath::Max(largestRange, range.m_uiPrimitiveCount);
  }
  if (largestRange == 0)
    return XII_SUCCESS;

  const xiiMeshVertexStream* position = nullptr;
  for (const auto& stream : m_VertexStreams)
  {
    if (stream.m_Semantic == xiiMeshVertexSemantic::Position)
      position = &stream;
  }
  if (position == nullptr || position->m_Format != xiiMeshVertexStreamFormat::Float3 || m_uiVertexCount == 0 || position->m_uiStride < position->m_uiOffset + sizeof(xiiVec3) || static_cast<xiiUInt64>(m_uiVertexCount - 1) * position->m_uiStride + position->m_uiOffset + sizeof(xiiVec3) > m_VertexData.GetCount())
    return XII_FAILURE;

  xiiDynamicArray<xiiVec3> positions;
  positions.SetCountUninitialized(m_uiVertexCount);
  for (xiiUInt32 i = 0; i < m_uiVertexCount; ++i)
  {
    xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&positions[i]), m_VertexData.GetData() + i * position->m_uiStride + position->m_uiOffset, sizeof(xiiVec3));
    if (!positions[i].IsValid())
      return XII_FAILURE;
  }

  uiMaxVertices         = xiiMath::Clamp<xiiUInt32>(uiMaxVertices, 3, xiiMeshlet::s_uiMaxVertices);
  uiMaxPrimitives       = xiiMath::Clamp<xiiUInt32>(uiMaxPrimitives, 4, xiiMeshlet::s_uiMaxPrimitives) & ~3U;
  const size_t capacity = meshopt_buildMeshletsBound(largestRange * 3, uiMaxVertices, uiMaxPrimitives);
  if (capacity > xiiMath::MaxValue<xiiUInt32>() / uiMaxVertices || capacity > xiiMath::MaxValue<xiiUInt32>() / (uiMaxPrimitives * 3))
    return XII_FAILURE;

  xiiDynamicArray<xiiUInt32>       indices, optimizedVertices;
  xiiDynamicArray<xiiUInt8>        optimizedTriangles;
  xiiDynamicArray<meshopt_Meshlet> optimizedMeshlets;
  indices.Reserve(largestRange * 3);
  optimizedMeshlets.SetCountUninitialized(static_cast<xiiUInt32>(capacity));
  optimizedVertices.SetCountUninitialized(static_cast<xiiUInt32>(capacity * uiMaxVertices));
  optimizedTriangles.SetCountUninitialized(static_cast<xiiUInt32>(capacity * uiMaxPrimitives * 3));

  for (const auto& range : ranges)
  {
    if (range.m_uiPrimitiveCount == 0)
      continue;
    indices.SetCountUninitialized(range.m_uiPrimitiveCount * 3);
    for (xiiUInt32 i = 0; i < indices.GetCount(); ++i)
    {
      const auto* source = m_IndexData.GetData() + (range.m_uiFirstPrimitive * 3 + i) * indexSize;
      if (indexSize == 2)
      {
        xiiUInt16 value;
        xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&value), source, sizeof(value));
        indices[i] = value;
      }
      else
        xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&indices[i]), source, sizeof(xiiUInt32));
      if (indices[i] >= m_uiVertexCount)
      {
        clear();
        return XII_FAILURE;
      }
    }

    const size_t count          = meshopt_buildMeshlets(optimizedMeshlets.GetData(), optimizedVertices.GetData(), optimizedTriangles.GetData(), indices.GetData(), indices.GetCount(), &positions[0].x, positions.GetCount(), sizeof(xiiVec3), uiMaxVertices, uiMaxPrimitives, 0.5f);
    xiiUInt32    firstPrimitive = range.m_uiFirstPrimitive;
    for (size_t i = 0; i < count; ++i)
    {
      const auto& source = optimizedMeshlets[static_cast<xiiUInt32>(i)];
      if (source.vertex_count > xiiMath::MaxValue<xiiUInt32>() - m_MeshletVertexRemap.GetCount() || source.triangle_count * 3 > xiiMath::MaxValue<xiiUInt32>() - m_MeshletPrimitiveIndices.GetCount() || m_Meshlets.GetCount() == xiiMath::MaxValue<xiiUInt32>())
      {
        clear();
        return XII_FAILURE;
      }
      const auto bounds          = meshopt_computeMeshletBounds(optimizedVertices.GetData() + source.vertex_offset, optimizedTriangles.GetData() + source.triangle_offset, source.triangle_count, &positions[0].x, positions.GetCount(), sizeof(xiiVec3));
      auto&      meshlet         = m_Meshlets.ExpandAndGetRef();
      meshlet.m_uiFirstPrimitive = firstPrimitive;
      firstPrimitive += source.triangle_count;
      meshlet.m_uiPrimitiveCount       = static_cast<xiiUInt16>(source.triangle_count);
      meshlet.m_uiVertexCount          = static_cast<xiiUInt16>(source.vertex_count);
      meshlet.m_uiVertexRemapOffset    = m_MeshletVertexRemap.GetCount();
      meshlet.m_uiPrimitiveIndexOffset = m_MeshletPrimitiveIndices.GetCount();
      meshlet.m_uiMaterialIndex        = range.m_uiMaterialIndex;
      meshlet.m_uiSectionIndex         = range.m_uiSectionIndex;
      meshlet.m_uiLodIndex             = range.m_uiLodIndex;
      meshlet.m_Bounds                 = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3(bounds.center[0], bounds.center[1], bounds.center[2]), bounds.radius);
      meshlet.m_vConeAxis              = xiiVec3(bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2]);
      // meshoptimizer's cutoff of one denotes a degenerate/wide cone that must always
      // pass. Preserve XII's disabled sentinel rather than normalizing its zero axis.
      meshlet.m_fConeCutoff = bounds.cone_cutoff >= 1.0f || meshlet.m_vConeAxis.IsZero() ? -1.0f : xiiMath::Sqrt(xiiMath::Max(0.0f, 1.0f - bounds.cone_cutoff * bounds.cone_cutoff));
      m_MeshletVertexRemap.PushBackRange(xiiArrayPtr<const xiiUInt32>(optimizedVertices.GetData() + source.vertex_offset, source.vertex_count));
      m_MeshletPrimitiveIndices.PushBackRange(xiiArrayPtr<const xiiUInt8>(optimizedTriangles.GetData() + source.triangle_offset, source.triangle_count * 3));
      m_MeshletMaterialIndices.PushBack(range.m_uiMaterialIndex);
    }
  }
  if (!m_Meshlets.IsEmpty())
  {
    auto& command                 = m_DrawCommands.ExpandAndGetRef();
    command.m_uiThreadGroupCountX = m_Meshlets.GetCount();
  }
  return XII_SUCCESS;
}
