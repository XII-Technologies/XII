/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <ModelImporter/MeshImport.h>

XII_CREATE_SIMPLE_TEST(EditorViews, MeshImportGeometry)
{
  xiiStringBuilder folder(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/MeshImportGeometry");
  if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(folder)))
    return;
  xiiStringBuilder source(folder, "/Grid.obj"), text;
  // More than 65535 unique vertices exercises the complete source -> native 32-bit index path.
  constexpr xiiUInt32 size = 257;
  for (xiiUInt32 y = 0; y < size; ++y)
    for (xiiUInt32 x = 0; x < size; ++x)
      text.AppendFormat("v {} {} 0\n", x, y);
  text.Append("vn 0 0 1\n");
  for (xiiUInt32 y = 0; y + 1 < size; ++y)
  {
    for (xiiUInt32 x = 0; x + 1 < size; ++x)
    {
      const auto a = y * size + x + 1;
      const auto b = a + 1;
      const auto c = a + size;
      const auto d = c + 1;
      text.AppendFormat("f {}//1 {}//1 {}//1\nf {}//1 {}//1 {}//1\n", a, b, c, b, d, c);
    }
  }
  {
    xiiOSFile file;
    if (!XII_TEST_RESULT(file.Open(source, xiiFileOpenMode::Write)) || !XII_TEST_RESULT(file.Write(text.GetData(), text.GetElementCount())))
      return;
  }
  text.Clear();
  xiiModelMeshImportOptions options;
  xiiModelMeshImport        imported;
  xiiStringBuilder          error;
  if (!XII_TEST_RESULT(xiiModelMeshImport::ReadFile(source, options, imported, &error)))
  {
    XII_TEST_FAILURE("Grid import: {}", error);
    return;
  }
  const auto& buffer            = imported.m_Mesh.GetMeshBufferDescriptor();
  const auto  originalTriangles = 2 * (size - 1) * (size - 1);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Large native geometry and complete triangle coverage")
  {
    XII_TEST_INT(buffer.GetVertexCount(), size * size);
    XII_TEST_INT(buffer.GetPrimitiveCount(), originalTriangles);
    XII_TEST_BOOL(buffer.m_IndexType == xiiGALValueType::UInt32);
    XII_TEST_BOOL(!buffer.m_Meshlets.IsEmpty());
    const auto indices  = reinterpret_cast<const xiiUInt32*>(buffer.GetIndexData().GetPtr());
    const auto vertices = reinterpret_cast<const xiiMeshPackedVertex*>(buffer.GetVertexData().GetPtr());
    bool       valid    = true;
    double     area     = 0;
    for (xiiUInt32 i = 0; i < buffer.GetIndexCount(); i += 3)
    {
      if (indices[i] >= buffer.GetVertexCount() || indices[i + 1] >= buffer.GetVertexCount() || indices[i + 2] >= buffer.GetVertexCount())
      {
        valid = false;
        break;
      }
      const auto& a     = vertices[indices[i]];
      const auto& b     = vertices[indices[i + 1]];
      const auto& c     = vertices[indices[i + 2]];
      const auto  cross = (b.m_vPosition - a.m_vPosition).CrossRH(c.m_vPosition - a.m_vPosition);
      valid &= cross.z > 0 && a.m_vNormal.IsValid() && a.m_vTangent.IsValid();
      area += cross.z * 0.5;
    }
    XII_TEST_BOOL(valid);
    XII_TEST_DOUBLE(area, static_cast<double>((size - 1) * (size - 1)), 0.0001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Static import ignores obsolete LOD and simplification settings")
  {
    options.m_uiGeneratedLODs           = 4;
    options.m_fSimplification           = 0.8f;
    options.m_fMaxSimplificationError   = 1.0f;
    options.m_bAggressiveSimplification = true;
    xiiModelMeshImport reimported;
    if (!XII_TEST_RESULT(xiiModelMeshImport::ReadFile(source, options, reimported, &error)))
      return;
    const auto& full = reimported.m_Mesh.GetMeshBufferDescriptor();
    XII_TEST_INT(reimported.m_Mesh.GetSubMeshes().GetCount(), 1);
    XII_TEST_BOOL(reimported.m_Mesh.GetLODs().IsEmpty());
    XII_TEST_INT(full.GetVertexCount(), buffer.GetVertexCount());
    XII_TEST_INT(full.GetPrimitiveCount(), originalTriangles);
    XII_TEST_BOOL(full.m_GeometryPageData == buffer.m_GeometryPageData);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native hierarchy cuts cover the surface with valid page-local remaps")
  {
    xiiContiguousMemoryStreamStorage bytes;
    xiiMemoryStreamWriter            writer(&bytes);
    if (!XII_TEST_RESULT(imported.m_Mesh.Save(writer)))
      return;
    xiiMemoryStreamReader     reader(&bytes);
    xiiMeshResourceDescriptor decoded;
    if (!XII_TEST_RESULT(decoded.Load(reader)))
      return;
    const auto& geometry = decoded.GetMeshBufferDescriptor();
    const auto& pages    = geometry.m_GeometryPages;
    xiiUInt32   parents  = 0;
    for (const auto& page : pages)
      parents += page.m_uiChildCount != 0U ? 1U : 0U;
    if (!XII_TEST_BOOL(parents > 0))
      return;

    xiiDynamicArray<xiiUInt32> pending;
    xiiDynamicArray<xiiUInt8>  payload;
    xiiDynamicArray<xiiUInt8>  coverage;
    const xiiUInt32            stride          = geometry.GetVertexStride();
    xiiUInt32                  coarseTriangles = 0;
    for (xiiUInt32 cut = 0; cut < 3; ++cut)
    {
      coverage.Clear();
      coverage.SetCount(originalTriangles, 0);
      for (xiiUInt32 p = 0; p < pages.GetCount(); ++p)
        if (pages[p].m_bRequired)
          pending.PushBack(p);
      xiiUInt32 triangles = 0;
      double    area      = 0;
      bool      valid     = true;
      while (!pending.IsEmpty())
      {
        const auto index = pending.PeekBack();
        pending.PopBack();
        const auto& page = pages[index];
        // Root-only, partially resident and fully resident cuts. A missing
        // sibling keeps the complete parent in the partially resident cut.
        const bool complete = cut == 2 || (cut == 1 && page.m_uiChildCount != 0 && page.m_Children[0] % 2 == 0);
        if (complete && page.m_uiChildCount != 0)
        {
          for (xiiUInt32 child = 0; child < page.m_uiChildCount; ++child)
            pending.PushBack(page.m_Children[child]);
          continue;
        }
        if (!XII_TEST_RESULT(geometry.ReadGeometryPage(index, payload)))
          return;
        const auto metadata = payload.GetCount() - page.m_uiClusterCount * sizeof(xiiMeshlet);
        for (xiiUInt32 c = 0; c < page.m_uiClusterCount; ++c)
        {
          xiiMeshlet cluster;
          xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&cluster), payload.GetData() + metadata + c * sizeof(xiiMeshlet), sizeof(xiiMeshlet));
          XII_TEST_INT(cluster.m_Page.x, index);
          XII_TEST_INT(cluster.m_uiMaterialIndex, 0);
          // Every parent cluster shares the page's full source range.
          if (page.m_uiChildCount == 0 || c == 0)
          {
            const auto first = cluster.m_uiFirstPrimitive;
            const auto count = cluster.m_uiGpuPadding[1];
            if (!XII_TEST_BOOL(first <= originalTriangles && count <= originalTriangles - first))
              return;
            for (xiiUInt32 p = first; p < first + count; ++p)
              ++coverage[p];
          }
          triangles += cluster.m_uiPrimitiveCount;
          for (xiiUInt32 p = 0; p < cluster.m_uiPrimitiveCount; ++p)
          {
            xiiVec3 position[3];
            for (xiiUInt32 corner = 0; corner < 3; ++corner)
            {
              const auto local = payload[cluster.m_Page.z + p * 3U + corner];
              if (!XII_TEST_BOOL(local < cluster.m_uiVertexCount))
                return;
              xiiUInt32 vertex;
              xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&vertex), payload.GetData() + cluster.m_Page.y + local * sizeof(vertex), sizeof(vertex));
              if (!XII_TEST_BOOL(vertex < cluster.m_Page.w))
                return;
              xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&position[corner]), payload.GetData() + vertex * stride, sizeof(xiiVec3));
            }
            const auto cross = (position[1] - position[0]).CrossRH(position[2] - position[0]);
            valid &= cross.z >= 0;
            area += cross.z * 0.5;
          }
        }
      }
      for (auto covered : coverage)
        valid &= covered == 1;
      XII_TEST_BOOL(valid);
      XII_TEST_DOUBLE(area, static_cast<double>((size - 1) * (size - 1)), 0.0001);
      if (cut == 0)
        coarseTriangles = triangles;
      if (cut == 2)
      {
        XII_TEST_INT(triangles, originalTriangles);
        XII_TEST_BOOL(coarseTriangles < triangles);
      }
    }
  }
}

XII_CREATE_SIMPLE_TEST(EditorViews, ProceduralGeometryHierarchy)
{
  xiiGeometry source;
  source.AddStackedSphere(1.0f, 128, 64);
  xiiMeshResourceDescriptor mesh;
  auto&                     buffer = mesh.GetMeshBufferDescriptor();
  buffer.AllocateStreamsFromGeometry(source, xiiGALPrimitiveTopology::TriangleList, false);
  mesh.AddSubMesh(buffer.GetPrimitiveCount(), 0U, 0U);
  if (!XII_TEST_RESULT(mesh.BuildMeshlets()))
    return;
  XII_TEST_BOOL(mesh.GetLODs().IsEmpty());
  XII_TEST_BOOL(!buffer.m_GeometryPages.IsEmpty());
  xiiUInt32 parents = 0U;
  for (const auto& page : buffer.m_GeometryPages)
    parents += page.m_uiChildCount != 0U ? 1U : 0U;
  XII_TEST_BOOL(parents > 0U);
}
