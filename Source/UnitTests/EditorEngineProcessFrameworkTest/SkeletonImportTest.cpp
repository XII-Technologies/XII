/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringBuilder.h>
#include <ModelImporter/SkeletonImport.h>

XII_CREATE_SIMPLE_TEST(EditorViews, SkeletonImport)
{
  xiiStringBuilder folder(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/SkeletonImport-骨");
  if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(folder)))
    return;
  xiiStringBuilder       source(folder, "/Character.gltf");
  const xiiStringBuilder buffer(folder, "/Geometry.bin");
  {
    xiiOSFile file;
    if (!XII_TEST_RESULT(file.Open(buffer, xiiFileOpenMode::Write)))
      return;
    const float     positions[] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    const xiiUInt16 indices[]   = {0, 1, 2};
    XII_TEST_RESULT(file.Write(positions, sizeof(positions)));
    XII_TEST_RESULT(file.Write(indices, sizeof(indices)));
  }
  const char* json  = R"JSON({
    "asset":{"version":"2.0"}, "scene":0, "scenes":[{"nodes":[0]}],
    "nodes":[
      {"name":"Root","translation":[1,2,3],"scale":[-2,3,4],"children":[1,3]},
      {"name":"Hand_L","translation":[0,2,0],"children":[2]},
      {"name":"Finger","translation":[0,1,0]},
      {"name":"ColliderInstance","translation":[0,0,2],"mesh":0}
    ],
    "meshes":[{"name":"UCX_Hand_L_01","primitives":[{"attributes":{"POSITION":0},"indices":1}]}],
    "buffers":[{"uri":"Geometry.bin","byteLength":42}],
    "bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":6}],
    "accessors":[
      {"bufferView":0,"componentType":5126,"count":3,"type":"VEC3","min":[0,0,0],"max":[1,1,0]},
      {"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"}
    ]
  })JSON";
  auto        write = [&](xiiStringView text) {
    xiiOSFile file;
    if (!XII_TEST_RESULT(file.Open(source, xiiFileOpenMode::Write)))
      return;
    XII_TEST_RESULT(file.Write(text.GetStartPointer(), text.GetElementCount()));
  };
  write(json);

  xiiModelSkeletonImport imported;
  xiiStringBuilder       error;
  if (!XII_TEST_RESULT(xiiModelSkeletonImport::ReadFile(source, imported, &error)))
  {
    XII_TEST_FAILURE("Skeleton import: {}", error);
    return;
  }
  XII_TEST_BOOL(error.IsEmpty());
  const auto& skeleton = imported.m_Skeleton;
  XII_TEST_INT(skeleton.m_Joints.GetCount(), 4);
  const auto root   = skeleton.FindJointByName(xiiTempHashedString("Root"));
  const auto hand   = skeleton.FindJointByName(xiiTempHashedString("Hand_L"));
  const auto finger = skeleton.FindJointByName(xiiTempHashedString("Finger"));
  if (!XII_TEST_BOOL(root < skeleton.m_Joints.GetCount() && hand < skeleton.m_Joints.GetCount() && finger < skeleton.m_Joints.GetCount()))
    return;
  XII_TEST_INT(skeleton.m_Joints[hand].m_uiParentIndex, root);
  XII_TEST_INT(skeleton.m_Joints[finger].m_uiParentIndex, hand);
  const auto expectedRoot = xiiTransform::Make(xiiVec3(1, 2, 3), xiiQuat::MakeIdentity(), xiiVec3(-2, 3, 4)).GetAsMat4();
  XII_TEST_BOOL(skeleton.m_Joints[root].m_ModelRestPose.IsEqual(expectedRoot, 0.0001f));
  XII_TEST_VEC3(skeleton.m_Joints[finger].m_ModelRestPose.GetTranslationVector(), xiiVec3(1, 11, 3), 0.0001f);
  for (xiiUInt32 i = 0; i < skeleton.m_Joints.GetCount(); ++i)
  {
    const auto& joint = skeleton.m_Joints[i];
    XII_TEST_BOOL(joint.m_uiParentIndex == xiiMath::MaxValue<xiiUInt16>() || joint.m_uiParentIndex < i);
    XII_TEST_BOOL((joint.m_ModelRestPose * joint.m_InverseBindPose).IsEqual(xiiMat4::MakeIdentity(), 0.0001f));
    XII_TEST_BOOL(skeleton.m_Bounds.GetBox().Contains(joint.m_ModelRestPose.GetTranslationVector()));
  }
  if (XII_TEST_INT(imported.m_Colliders.GetCount(), 1))
  {
    const auto& collider = imported.m_Colliders[0];
    XII_TEST_INT(collider.m_uiJointIndex, hand);
    XII_TEST_STRING(collider.m_sIdentifier.GetData(), "UCX_Hand_L_01");
    XII_TEST_INT(collider.m_Vertices.GetCount(), 3);
    XII_TEST_INT(collider.m_TriangleIndices.GetCount(), 3);
    XII_TEST_VEC3(collider.m_Vertices[0], xiiVec3(0, -2, 2), 0.0001f);
    for (auto index : collider.m_TriangleIndices)
      XII_TEST_BOOL(index < collider.m_Vertices.GetCount());
  }
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  XII_TEST_RESULT(skeleton.Serialize(writer));
  xiiMemoryStreamReader         reader(&storage);
  xiiSkeletonResourceDescriptor restored;
  XII_TEST_RESULT(restored.Deserialize(reader));
  XII_TEST_INT(restored.m_uiRuntimeHash, skeleton.m_uiRuntimeHash);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reimport and invalid input discard stale data")
  {
    xiiStringBuilder changed(json);
    changed.ReplaceAll("[1,2,3]", "[5,2,3]");
    write(changed);
    XII_TEST_RESULT(xiiModelSkeletonImport::ReadFile(source, imported, &error));
    XII_TEST_VEC3(imported.m_Skeleton.m_Joints[0].m_LocalRestPose.m_vPosition, xiiVec3(5, 2, 3), 0.0001f);
    changed = json;
    changed.ReplaceAll("[-2,3,4]", "[0,3,4]");
    write(changed);
    XII_TEST_BOOL(xiiModelSkeletonImport::ReadFile(source, imported, &error).Failed());
    XII_TEST_BOOL(!error.IsEmpty() && imported.m_Skeleton.m_Joints.IsEmpty() && imported.m_Colliders.IsEmpty());
    changed = json;
    changed.ReplaceAll("\"Finger\"", "\"Hand_L\"");
    write(changed);
    XII_TEST_BOOL(xiiModelSkeletonImport::ReadFile(source, imported, &error).Failed());
    XII_TEST_BOOL(!error.IsEmpty() && imported.m_Skeleton.m_Joints.IsEmpty());
    changed = json;
    changed.ReplaceAll("\"translation\":[0,1,0]", "\"matrix\":[1,0,0,0,0.5,1,0,0,0,0,1,0,0,1,0,1]");
    write(changed);
    XII_TEST_BOOL(xiiModelSkeletonImport::ReadFile(source, imported, &error).Failed());
    XII_TEST_BOOL(!error.IsEmpty() && imported.m_Skeleton.m_Joints.IsEmpty());
    XII_TEST_BOOL(xiiModelSkeletonImport::ReadFile("Relative.gltf", imported, &error).Failed());
    XII_TEST_BOOL(!error.IsEmpty());
    source.ChangeFileName("Missing");
    XII_TEST_BOOL(xiiModelSkeletonImport::ReadFile(source, imported, &error).Failed());
    XII_TEST_BOOL(!error.IsEmpty() && imported.m_Skeleton.m_Joints.IsEmpty() && imported.m_Colliders.IsEmpty());
  }
}
