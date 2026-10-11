/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Meshes/MeshResource.h>

namespace
{
  xiiUInt32 ReadIndex(const xiiMeshBufferResourceDescriptor& buffer, xiiUInt32 index)
  {
    if (buffer.m_IndexType == xiiGALValueType::UInt16)
      return reinterpret_cast<const xiiUInt16*>(buffer.GetIndexData().GetPtr())[index];
    return reinterpret_cast<const xiiUInt32*>(buffer.GetIndexData().GetPtr())[index];
  }

  void VerifySection(const xiiMeshBufferResourceDescriptor& buffer, const xiiMeshSection& section, xiiUInt32 sectionIndex, xiiUInt32 lodIndex)
  {
    xiiDynamicArray<xiiUInt8> seen;
    seen.SetCount(section.m_uiPrimitiveCount, 0);
    xiiUInt32 triangleCount = 0;
    for (xiiUInt32 i = section.m_uiFirstMeshlet; i < section.m_uiFirstMeshlet + section.m_uiMeshletCount; ++i)
    {
      const auto& meshlet = buffer.m_Meshlets[i];
      XII_TEST_INT(meshlet.m_uiMaterialIndex, section.m_uiMaterialIndex);
      XII_TEST_INT(buffer.m_MeshletMaterialIndices[i], section.m_uiMaterialIndex);
      XII_TEST_INT(meshlet.m_uiSectionIndex, sectionIndex);
      XII_TEST_INT(meshlet.m_uiLodIndex, lodIndex);
      XII_TEST_BOOL(meshlet.m_uiFirstPrimitive >= section.m_uiFirstPrimitive);
      XII_TEST_BOOL(meshlet.m_uiFirstPrimitive + meshlet.m_uiPrimitiveCount <= section.m_uiFirstPrimitive + section.m_uiPrimitiveCount);
      triangleCount += meshlet.m_uiPrimitiveCount;
      for (xiiUInt32 p = 0; p < meshlet.m_uiPrimitiveCount; ++p)
      {
        xiiUInt32 triangle[3];
        for (xiiUInt32 c = 0; c < 3; ++c)
          triangle[c] = buffer.m_MeshletVertexRemap[meshlet.m_uiVertexRemapOffset + buffer.m_MeshletPrimitiveIndices[meshlet.m_uiPrimitiveIndexOffset + p * 3 + c]];
        bool found = false;
        for (xiiUInt32 original = 0; original < section.m_uiPrimitiveCount && !found; ++original)
        {
          if (seen[original])
            continue;
          const xiiUInt32 first = (section.m_uiFirstPrimitive + original) * 3;
          for (xiiUInt32 rotation = 0; rotation < 3 && !found; ++rotation)
          {
            if (triangle[0] == ReadIndex(buffer, first + rotation) && triangle[1] == ReadIndex(buffer, first + (rotation + 1) % 3) &&
                triangle[2] == ReadIndex(buffer, first + (rotation + 2) % 3))
            {
              seen[original] = 1;
              found          = true;
            }
          }
        }
        XII_TEST_BOOL(found);
      }
    }
    XII_TEST_INT(triangleCount, section.m_uiPrimitiveCount);
    for (auto value : seen)
      XII_TEST_INT(value, 1);
  }
} // namespace

XII_CREATE_SIMPLE_TEST(Meshes, MeshletSections)
{
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(2), true);
  xiiMeshResourceDescriptor mesh;
  auto&                     buffer = mesh.GetMeshBufferDescriptor();
  buffer.AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
  const auto originalVertices = buffer.m_VertexData;
  const auto originalIndices  = buffer.m_IndexData;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Materials and interleaved LODs have independent, contiguous clusters")
  {
    mesh.AddSection(5, 0, 2, 0);
    mesh.AddSection(12, 0, 4, 1);
    mesh.AddSection(7, 5, 9, 0);
    mesh.AddSection(0, 12, 3, 1);
    // Identical geometry and material may be reused in another LOD.
    mesh.AddSection(5, 0, 2, 2);
    if (!XII_TEST_RESULT(mesh.BuildMeshlets()))
      return;
    XII_TEST_INT(buffer.m_Meshlets.GetCount(), 4);
    XII_TEST_BOOL(mesh.m_UsageFlags.IsSet(xiiMeshResourceUsageFlags::MeshShaderReady));
    const xiiUInt32 lodIds[] = {0, 1, 0, 1, 2};
    for (xiiUInt32 i = 0; i < mesh.GetSubMeshes().GetCount(); ++i)
      VerifySection(buffer, mesh.GetSubMeshes()[i], i, lodIds[i]);
    xiiUInt32 cursor = 0;
    for (xiiUInt32 l = 0; l < mesh.GetLODs().GetCount(); ++l)
    {
      const auto& lod = mesh.GetLODs()[l];
      XII_TEST_INT(lod.m_uiFirstMeshlet, cursor);
      xiiUInt32 count = 0;
      for (const auto& section : lod.m_Sections)
      {
        XII_TEST_INT(section.m_uiFirstMeshlet, cursor + count);
        count += section.m_uiMeshletCount;
      }
      XII_TEST_INT(lod.m_uiMeshletCount, count);
      cursor += count;
    }
    XII_TEST_INT(cursor, buffer.m_Meshlets.GetCount());
    XII_TEST_BOOL(buffer.m_VertexData == originalVertices);
    XII_TEST_BOOL(buffer.m_IndexData == originalIndices);
    const auto firstBuild = buffer.m_MeshletVertexRemap;
    XII_TEST_RESULT(mesh.BuildMeshlets());
    XII_TEST_BOOL(buffer.m_MeshletVertexRemap == firstBuild);

    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    mesh.Save(writer);
    xiiMemoryStreamReader     reader(&storage);
    xiiMeshResourceDescriptor restored;
    XII_TEST_RESULT(restored.Load(reader));
    for (xiiUInt32 i = 0; i < restored.GetSubMeshes().GetCount(); ++i)
      VerifySection(restored.GetMeshBufferDescriptor(), restored.GetSubMeshes()[i], i, lodIds[i]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "UInt32 indices and explicit build ranges")
  {
    xiiDynamicArray<xiiUInt32> indices;
    for (xiiUInt32 i = 0; i < buffer.GetIndexCount(); ++i)
      indices.PushBack(ReadIndex(buffer, i));
    buffer.SetIndexData(xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(indices.GetData()), indices.GetCount() * sizeof(xiiUInt32)), indices.GetCount(), xiiGALValueType::UInt32);
    XII_TEST_RESULT(mesh.BuildMeshlets(3, 4));
    for (xiiUInt32 i = 0; i < mesh.GetSubMeshes().GetCount(); ++i)
      VerifySection(buffer, mesh.GetSubMeshes()[i], i, i == 1 || i == 3 ? 1 : (i == 4 ? 2 : 0));
    for (const auto& cluster : buffer.m_Meshlets)
      XII_TEST_BOOL(cluster.m_uiVertexCount <= 3 && cluster.m_uiPrimitiveCount <= 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Failures clear all generated state and preserve source geometry")
  {
    xiiMeshletBuildRange invalid{12, 1};
    XII_TEST_BOOL(buffer.BuildMeshlets(xiiMakeArrayPtr(&invalid, 1)).Failed());
    auto verifyEmpty = [&]() {
      XII_TEST_BOOL(buffer.m_Meshlets.IsEmpty() && buffer.m_MeshletVertexRemap.IsEmpty() &&
                    buffer.m_MeshletPrimitiveIndices.IsEmpty() && buffer.m_MeshletMaterialIndices.IsEmpty() && buffer.m_DrawCommands.IsEmpty());
    };
    verifyEmpty();
    invalid = {xiiMath::MaxValue<xiiUInt32>(), xiiMath::MaxValue<xiiUInt32>()};
    XII_TEST_BOOL(buffer.BuildMeshlets(xiiMakeArrayPtr(&invalid, 1)).Failed());
    verifyEmpty();
    const xiiMeshletBuildRange ranges[] = {{0, 5}, {5, 7}};
    auto*                      indices  = reinterpret_cast<xiiUInt32*>(buffer.m_IndexData.GetData());
    const auto                 saved    = indices[35];
    indices[35]                         = buffer.GetVertexCount();
    XII_TEST_BOOL(buffer.BuildMeshlets(xiiMakeArrayPtr(ranges)).Failed());
    verifyEmpty(); // Also clears clusters already produced for the first valid range.
    indices[35]                                                                            = saved;
    const auto savedVertices                                                               = buffer.m_VertexData;
    reinterpret_cast<xiiMeshPackedVertex*>(buffer.m_VertexData.GetData())[0].m_vPosition.x = xiiMath::Infinity<float>();
    XII_TEST_BOOL(mesh.BuildMeshlets().Failed());
    XII_TEST_BOOL(!mesh.m_UsageFlags.IsSet(xiiMeshResourceUsageFlags::MeshShaderReady));
    for (const auto& section : mesh.GetSubMeshes())
      XII_TEST_INT(section.m_uiMeshletCount, 0);
    verifyEmpty();
    buffer.m_VertexData                  = savedVertices;
    const auto streams                   = buffer.m_VertexStreams;
    buffer.m_VertexStreams[0].m_uiStride = 1;
    XII_TEST_BOOL(mesh.BuildMeshlets().Failed());
    verifyEmpty();
    buffer.m_VertexStreams = streams;
    buffer.m_VertexStreams.Clear();
    XII_TEST_BOOL(mesh.BuildMeshlets().Failed());
    verifyEmpty();
    buffer.m_VertexStreams = streams;
    buffer.m_IndexData.PopBack();
    XII_TEST_BOOL(mesh.BuildMeshlets().Failed());
    verifyEmpty();
    buffer.m_IndexData.PushBack(0);
    XII_TEST_RESULT(mesh.BuildMeshlets());
    XII_TEST_RESULT(buffer.BuildMeshlets(xiiArrayPtr<const xiiMeshletBuildRange>()));
    verifyEmpty();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Empty and non-triangle mesh resources remain supported")
  {
    xiiMeshResourceDescriptor empty;
    XII_TEST_RESULT(empty.BuildMeshlets());
    XII_TEST_BOOL(!empty.m_UsageFlags.IsSet(xiiMeshResourceUsageFlags::MeshShaderReady));
    geometry.AddLine(0, 1);
    xiiMeshResourceDescriptor lines;
    lines.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::LineList, false);
    lines.AddSection(1, 0, 0);
    XII_TEST_RESULT(lines.BuildMeshlets());
    XII_TEST_BOOL(!lines.m_UsageFlags.IsSet(xiiMeshResourceUsageFlags::MeshShaderReady));
    XII_TEST_BOOL(lines.GetMeshBufferDescriptor().m_Meshlets.IsEmpty());
    XII_TEST_INT(lines.GetMeshBufferDescriptor().GetPrimitiveCount(), 1);
    XII_TEST_BOOL(lines.GetMeshBufferDescriptor().BuildMeshlets().Failed());
  }
}
