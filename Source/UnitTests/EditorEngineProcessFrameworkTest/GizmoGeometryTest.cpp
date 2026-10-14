/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoGeometry.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

XII_CREATE_SIMPLE_TEST(EditorViews, GizmoGeometry)
{
  xiiGizmoGeometry geometry;
  float            fTime    = -1;
  const xiiMat4    identity = xiiMat4::MakeIdentity();
  XII_TEST_BOOL(geometry.PickRay(identity, xiiVec3(0, 0, 3), xiiVec3(0, 0, -1), 0, fTime).Failed());
  XII_TEST_FLOAT(fTime, -1, 0);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "All procedural shapes have valid triangulated geometry")
  {
    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(xiiEngineGizmoHandleType::FromFile); ++i)
    {
      XII_TEST_BOOL(geometry.Build(static_cast<xiiEngineGizmoHandleType>(i)).Succeeded());
      XII_TEST_BOOL(geometry.GetBounds().IsValid());
      const auto& mesh = geometry.GetGeometry();
      for (const auto& vertex : mesh.GetVertices())
        XII_TEST_BOOL(vertex.m_vPosition.IsValid());
      for (const auto& triangle : mesh.GetPolygons())
      {
        XII_TEST_INT(triangle.m_Vertices.GetCount(), 3);
        for (xiiUInt32 uiIndex : triangle.m_Vertices)
          XII_TEST_BOOL(uiIndex < mesh.GetVertices().GetCount());
      }
      for (const auto& line : mesh.GetLines())
      {
        XII_TEST_BOOL(line.m_uiStartVertex < mesh.GetVertices().GetCount());
        XII_TEST_BOOL(line.m_uiEndVertex < mesh.GetVertices().GetCount());
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Triangle hits preserve ray time across rotation and non-uniform scaling")
  {
    XII_TEST_BOOL(geometry.Build(xiiEngineGizmoHandleType::Box).Succeeded());
    xiiTransform transform = xiiTransform::MakeIdentity();
    transform.m_vPosition.Set(3, 4, 5);
    transform.m_vScale.Set(2, 3, 4);
    transform.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisZ(), xiiAngle::MakeFromDegree(37));
    const xiiMat4 matrix  = transform.GetAsMat4();
    XII_TEST_BOOL(geometry.PickRay(matrix, matrix.TransformPosition(xiiVec3(0, 0, 3)), matrix.TransformDirection(xiiVec3(0, 0, -2)), 0, fTime).Succeeded());
    XII_TEST_FLOAT(fTime, 1.25f, 0.0001f);
    XII_TEST_BOOL(geometry.PickRay(matrix, transform.m_vPosition, matrix.TransformDirection(xiiVec3::MakeAxisX()), 0, fTime).Succeeded());
    XII_TEST_FLOAT(fTime, 0.5f, 0.0001f);
    fTime = -1;
    XII_TEST_BOOL(geometry.PickRay(identity, xiiVec3(3, 0, 0), xiiVec3::MakeAxisX(), 0, fTime).Failed());
    XII_TEST_FLOAT(fTime, -1, 0);
    XII_TEST_BOOL(geometry.PickRay(identity, xiiVec3::MakeZero(), xiiVec3::MakeZero(), 0, fTime).Failed());
    XII_TEST_BOOL(geometry.PickRay(xiiMat4::MakeScaling(xiiVec3::MakeZero()), xiiVec3::MakeZero(), xiiVec3::MakeAxisX(), 0, fTime).Failed());
    XII_TEST_BOOL(geometry.PickRay(identity, xiiVec3::MakeZero(), xiiVec3::MakeAxisX(), -1, fTime).Failed());
    XII_TEST_FLOAT(fTime, -1, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Line picking uses world-space tolerance and bounded ray/segment parameters")
  {
    XII_TEST_BOOL(geometry.Build(xiiEngineGizmoHandleType::LineRect).Succeeded());
    XII_TEST_BOOL(geometry.PickRay(identity, xiiVec3(1.05f, 0, 3), xiiVec3(0, 0, -1), 0.1f, fTime).Succeeded());
    XII_TEST_FLOAT(fTime, 3, 0.0001f);
    XII_TEST_BOOL(geometry.PickRay(identity, xiiVec3(1.05f, 0, 3), xiiVec3(0, 0, -1), 0.01f, fTime).Failed());
    XII_TEST_BOOL(geometry.PickRay(identity, xiiVec3(1, 0, 0), xiiVec3(0, 1, 0), 0, fTime).Succeeded());
    XII_TEST_FLOAT(fTime, 0, 0.0001f);
    XII_TEST_BOOL(geometry.PickRay(identity, xiiVec3(1, 2, 0), xiiVec3(0, 1, 0), 0, fTime).Failed());
    const xiiMat4 scaled = xiiMat4::MakeScaling(xiiVec3(2, 3, 4));
    XII_TEST_BOOL(geometry.PickRay(scaled, xiiVec3(2.05f, 0, 3), xiiVec3(0, 0, -1), 0.1f, fTime).Succeeded());
    XII_TEST_FLOAT(fTime, 3, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "OBJ geometry uses the shared loader and fails safely")
  {
    const char* szGroup = "GizmoGeometryTest";
    if (!XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(xiiTestFramework::GetInstance()->GetAbsOutputPath(), szGroup, "gizmotest", xiiDataDirUsage::AllowWrites).Succeeded()))
      return;
    XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup(szGroup));
    xiiStringBuilder path(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/GizmoGeometry.obj");
    XII_SCOPE_EXIT(xiiOSFile::DeleteFile(path).IgnoreResult());
    auto WriteMesh = [&](xiiStringView content) {
      xiiOSFile file;
      if (!XII_TEST_BOOL(file.Open(path, xiiFileOpenMode::Write).Succeeded()))
        return;
      XII_TEST_BOOL(file.Write(content.GetStartPointer(), content.GetElementCount()).Succeeded());
    };
    WriteMesh("v -1 -1 0\nv 1 -1 0\nv 1 1 0\nv -1 1 0\nf 1 2 3 4\n");
    XII_TEST_BOOL(geometry.Build(xiiEngineGizmoHandleType::FromFile, ":gizmotest/GizmoGeometry.obj").Succeeded());
    XII_TEST_INT(geometry.GetGeometry().GetPolygons().GetCount(), 2);
    XII_TEST_BOOL(geometry.PickRay(identity, xiiVec3(0, 0, -3), xiiVec3::MakeAxisZ(), 0, fTime).Succeeded());
    XII_TEST_FLOAT(fTime, 3, 0.0001f);
    for (const char* szInvalidMesh : {"v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 99\n", "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 0 2 3\n", "v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nf 1/1 2/1 3/99\n", "v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\nf 1//1 2//1 3//99\n"})
    {
      WriteMesh(szInvalidMesh);
      XII_TEST_BOOL(geometry.Build(xiiEngineGizmoHandleType::FromFile, ":gizmotest/GizmoGeometry.obj").Failed());
      XII_TEST_BOOL(!geometry.GetBounds().IsValid());
      XII_TEST_BOOL(geometry.GetGeometry().GetVertices().IsEmpty());
    }
    XII_TEST_BOOL(geometry.Build(xiiEngineGizmoHandleType::FromFile).Failed());
    XII_TEST_BOOL(geometry.Build(static_cast<xiiEngineGizmoHandleType>(255)).Failed());
  }
}
