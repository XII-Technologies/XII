/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

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
    xiiUInt32 meshletTriangles = 0;
    for (const auto& meshlet : buffer.m_Meshlets)
      meshletTriangles += meshlet.m_uiPrimitiveCount;
    XII_TEST_INT(meshletTriangles, originalTriangles);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Simplification compacts unused vertices and preserves a valid surface")
  {
    options.m_fSimplification         = 0.8f;
    options.m_fMaxSimplificationError = 1.0f;
    for (bool aggressive : {false, true})
    {
      options.m_bAggressiveSimplification = aggressive;
      xiiModelMeshImport simplified;
      if (!XII_TEST_RESULT(xiiModelMeshImport::ReadFile(source, options, simplified, &error)))
      {
        XII_TEST_FAILURE("Grid simplification: {}", error);
        continue;
      }
      const auto& reduced = simplified.m_Mesh.GetMeshBufferDescriptor();
      XII_TEST_BOOL(reduced.GetPrimitiveCount() > 0 && reduced.GetPrimitiveCount() < originalTriangles / 2);
      XII_TEST_BOOL(reduced.GetVertexCount() < buffer.GetVertexCount());
      XII_TEST_BOOL(reduced.m_IndexType == xiiGALValueType::UInt16);
      XII_TEST_BOOL(!reduced.m_Meshlets.IsEmpty());
      XII_TEST_INT(simplified.m_Mesh.GetSubMeshes().GetCount(), 1);
      XII_TEST_INT(simplified.m_Mesh.GetSubMeshes()[0].m_uiPrimitiveCount, reduced.GetPrimitiveCount());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Generated LODs use native sections and meshlet ranges")
  {
    options                   = {};
    options.m_uiGeneratedLODs = 2;
    xiiModelMeshImport lodMesh;
    if (!XII_TEST_RESULT(xiiModelMeshImport::ReadFile(source, options, lodMesh, &error)))
      return;
    const auto lods = lodMesh.m_Mesh.GetLODs();
    if (!XII_TEST_INT(lods.GetCount(), 3))
      return;
    for (xiiUInt32 i = 0; i < lods.GetCount(); ++i)
    {
      XII_TEST_INT(lods[i].m_Sections.GetCount(), 1);
      XII_TEST_BOOL(lods[i].m_uiMeshletCount > 0);
      if (i > 0)
      {
        XII_TEST_BOOL(lods[i].m_fScreenSize < lods[i - 1].m_fScreenSize);
        XII_TEST_BOOL(lods[i].m_Sections[0].m_uiPrimitiveCount < lods[i - 1].m_Sections[0].m_uiPrimitiveCount);
      }
      for (xiiUInt32 m = lods[i].m_uiFirstMeshlet; m < lods[i].m_uiFirstMeshlet + lods[i].m_uiMeshletCount; ++m)
        XII_TEST_INT(lodMesh.m_Mesh.GetMeshBufferDescriptor().m_Meshlets[m].m_uiLodIndex, i);
    }
  }
}
