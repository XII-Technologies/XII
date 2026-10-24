/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <ModelImporter/MeshImport.h>
#include <ModelImporter/SkeletonImport.h>

namespace
{
  xiiResult WriteSource(xiiStringView path, xiiStringView text)
  {
    xiiOSFile file;
    XII_SUCCEED_OR_RETURN(file.Open(path, xiiFileOpenMode::Write));
    return file.Write(text.GetStartPointer(), text.GetElementCount());
  }

  xiiMeshPackedVertex Vertex(const xiiMeshBufferResourceDescriptor& mesh, xiiUInt32 index)
  {
    xiiMeshPackedVertex vertex;
    xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&vertex), mesh.GetVertexData().GetPtr() + index * sizeof(vertex), sizeof(vertex));
    return vertex;
  }

  xiiUInt32 Index(const xiiMeshBufferResourceDescriptor& mesh, xiiUInt32 index)
  {
    if (mesh.m_IndexType == xiiGALValueType::UInt16)
    {
      xiiUInt16 value;
      xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&value), mesh.GetIndexData().GetPtr() + index * sizeof(value), sizeof(value));
      return value;
    }
    xiiUInt32 value;
    xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&value), mesh.GetIndexData().GetPtr() + index * sizeof(value), sizeof(value));
    return value;
  }

  const char* s_szSource = R"JSON({
    "asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0,1,2]}],
    "nodes":[{"name":"Surface","mesh":0},{"name":"Mirrored","mesh":0,"translation":[4,0,0],"scale":[-2,3,1]},
      {"name":"BoneRoot","children":[3]},{"name":"BoneChild","translation":[0,1,0]}],
    "meshes":[{"primitives":[{"attributes":{"POSITION":0,"NORMAL":1,"TANGENT":2,"TEXCOORD_0":3,"TEXCOORD_1":4,"COLOR_0":5},"indices":8,"material":0}]}],
    "skins":[{"joints":[2,3],"inverseBindMatrices":9}],
    "materials":[{"name":"Paint","pbrMetallicRoughness":{"baseColorFactor":[0.2,0.4,0.6,0.8],"metallicFactor":0.35,"roughnessFactor":0.2,
      "baseColorTexture":{"index":0,"texCoord":1},"metallicRoughnessTexture":{"index":1}},
      "normalTexture":{"index":1,"scale":0.4},"occlusionTexture":{"index":1,"strength":0.6},"emissiveFactor":[0.1,0.2,0.3],"alphaMode":"MASK","alphaCutoff":0.3,"doubleSided":true}],
    "textures":[{"source":0,"sampler":0},{"source":1,"sampler":1}],
    "samplers":[{"magFilter":9728,"minFilter":9729,"wrapS":33071,"wrapT":33648},{"magFilter":9729,"minFilter":9987}],
    "images":[{"uri":"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+jRZkAAAAASUVORK5CYII="},{"uri":"Maps/Surface.png"}],
    "buffers":[{"uri":"Geometry.bin","byteLength":524}],
    "bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":36},
      {"buffer":0,"byteOffset":72,"byteLength":48},{"buffer":0,"byteOffset":120,"byteLength":24},
      {"buffer":0,"byteOffset":144,"byteLength":24},{"buffer":0,"byteOffset":168,"byteLength":48},
      {"buffer":0,"byteOffset":216,"byteLength":24},{"buffer":0,"byteOffset":240,"byteLength":48},
      {"buffer":0,"byteOffset":288,"byteLength":6},{"buffer":0,"byteOffset":296,"byteLength":128}],
    "accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3","min":[0,0,0],"max":[1,1,0]},
      {"bufferView":1,"componentType":5126,"count":3,"type":"VEC3"},{"bufferView":2,"componentType":5126,"count":3,"type":"VEC4"},
      {"bufferView":3,"componentType":5126,"count":3,"type":"VEC2"},{"bufferView":4,"componentType":5126,"count":3,"type":"VEC2"},
      {"bufferView":5,"componentType":5126,"count":3,"type":"VEC4"},{"bufferView":6,"componentType":5123,"count":3,"type":"VEC4"},
      {"bufferView":7,"componentType":5126,"count":3,"type":"VEC4"},{"bufferView":8,"componentType":5123,"count":3,"type":"SCALAR"},
      {"bufferView":9,"componentType":5126,"count":2,"type":"MAT4"}]
  })JSON";

  xiiResult WriteGeometry(xiiStringView folder)
  {
    xiiOSFile file;
    XII_SUCCEED_OR_RETURN(file.Open(xiiStringBuilder(folder, "/Geometry.bin"), xiiFileOpenMode::Write));
    const float     positions[]         = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    const float     normals[]           = {0, 0, 1, 0, 0, 1, 0, 0, 1};
    const float     tangents[]          = {1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1};
    const float     uv0[]               = {0, 0, 1, 0, 0, 1};
    const float     uv1[]               = {0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f};
    const float     colors[]            = {0.5f, 0.25f, 1, 1, 0.5f, 0.25f, 1, 1, 0.5f, 0.25f, 1, 1};
    const xiiUInt16 joints[]            = {0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0};
    const float     weights[]           = {0.5f, 0.5f, 0, 0, 0.25f, 0.75f, 0, 0, 0, 0, 0, 0};
    const xiiUInt16 indicesAndPadding[] = {0, 1, 2, 0};
    const float     inverseBind[]       = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
                                           1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, -1, 0, 1};
    XII_SUCCEED_OR_RETURN(file.Write(positions, sizeof(positions)));
    XII_SUCCEED_OR_RETURN(file.Write(normals, sizeof(normals)));
    XII_SUCCEED_OR_RETURN(file.Write(tangents, sizeof(tangents)));
    XII_SUCCEED_OR_RETURN(file.Write(uv0, sizeof(uv0)));
    XII_SUCCEED_OR_RETURN(file.Write(uv1, sizeof(uv1)));
    XII_SUCCEED_OR_RETURN(file.Write(colors, sizeof(colors)));
    XII_SUCCEED_OR_RETURN(file.Write(joints, sizeof(joints)));
    XII_SUCCEED_OR_RETURN(file.Write(weights, sizeof(weights)));
    XII_SUCCEED_OR_RETURN(file.Write(indicesAndPadding, sizeof(indicesAndPadding)));
    XII_SUCCEED_OR_RETURN(file.Write(inverseBind, sizeof(inverseBind)));
    // Buffer is padded to its declared size; glTF requires four-byte alignment of buffer views.
    const xiiUInt8 padding[100] = {};
    return file.Write(padding, sizeof(padding));
  }
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, MeshImport)
{
  xiiStringBuilder folder(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/MeshImport-骨");
  if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(folder)) || !XII_TEST_RESULT(WriteGeometry(folder)))
    return;
  xiiStringBuilder source(folder, "/Surface.gltf");
  if (!XII_TEST_RESULT(WriteSource(source, s_szSource)))
    return;
  xiiModelMeshImportOptions options;
  options.m_bRecalculateTangents = false;
  options.m_bFlipTextureV        = false;
  xiiModelMeshImport imported;
  xiiStringBuilder   error;
  if (!XII_TEST_RESULT(xiiModelMeshImport::ReadFile(source, options, imported, &error)))
  {
    XII_TEST_FAILURE("Mesh import: {}", error);
    return;
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Instances, shared materials and native meshlets")
  {
    const auto& mesh   = imported.m_Mesh;
    const auto& buffer = mesh.GetMeshBufferDescriptor();
    XII_TEST_BOOL(error.IsEmpty());
    XII_TEST_INT(buffer.GetVertexCount(), 6);
    XII_TEST_INT(buffer.GetIndexCount(), 6);
    XII_TEST_INT(mesh.GetSubMeshes().GetCount(), 2);
    XII_TEST_INT(mesh.GetMaterials().GetCount(), 1);
    XII_TEST_INT(imported.m_Materials.GetCount(), 1);
    XII_TEST_INT(mesh.GetSubMeshes()[0].m_uiMaterialIndex, 0);
    XII_TEST_INT(mesh.GetSubMeshes()[1].m_uiMaterialIndex, 0);
    for (xiiUInt32 section = 0; section < 2; ++section)
    {
      const auto a = Vertex(buffer, Index(buffer, section * 3));
      const auto b = Vertex(buffer, Index(buffer, section * 3 + 1));
      const auto c = Vertex(buffer, Index(buffer, section * 3 + 2));
      XII_TEST_BOOL((b.m_vPosition - a.m_vPosition).CrossRH(c.m_vPosition - a.m_vPosition).Dot(a.m_vNormal) > 0);
      XII_TEST_FLOAT(a.m_vNormal.GetLength(), 1, 0.00001f);
      XII_TEST_FLOAT(a.m_vNormal.Dot(a.m_vTangent.GetAsVec3()), 0, 0.00001f);
      XII_TEST_VEC3(a.m_vPosition, section == 0 ? xiiVec3::MakeZero() : xiiVec3(4, 0, 0), 0.0001f);
      XII_TEST_FLOAT(a.m_vTexCoord1.x, 0.2f, 0.0001f);
      XII_TEST_FLOAT(a.m_vTexCoord1.y, 0.7f, 0.0001f); // Assimp's canonical glTF V coordinate.
      XII_TEST_INT(a.m_Color0.r, 128);
      XII_TEST_INT(a.m_Color0.g, 64);
      XII_TEST_INT(a.m_Color0.b, 255);
      XII_TEST_FLOAT(a.m_vTangent.w, section == 0 ? 1.0f : -1.0f, 0.0001f);
    }
    XII_TEST_INT(buffer.m_Meshlets.GetCount(), 2);
    for (xiiUInt32 i = 0; i < buffer.m_Meshlets.GetCount(); ++i)
    {
      XII_TEST_INT(buffer.m_Meshlets[i].m_uiSectionIndex, i);
      XII_TEST_INT(buffer.m_Meshlets[i].m_uiMaterialIndex, 0);
    }
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    mesh.Save(writer);
    xiiMemoryStreamReader     reader(&storage);
    xiiMeshResourceDescriptor restored;
    XII_TEST_RESULT(restored.Load(reader));
    XII_TEST_INT(restored.GetSubMeshes().GetCount(), 2);
    XII_TEST_BOOL(xiiArrayPtr<const xiiUInt8>(restored.GetMeshBufferDescriptor().GetVertexData()) == buffer.GetVertexData());
    XII_TEST_BOOL(xiiArrayPtr<const xiiUInt8>(restored.GetMeshBufferDescriptor().GetIndexData()) == buffer.GetIndexData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Owned PBR properties, texture references and embedded bytes")
  {
    const auto& material = imported.m_Materials[0];
    XII_TEST_STRING(material.m_sName.GetData(), "Paint");
    XII_TEST_FLOAT(material.m_fMetallic, 0.35f, 0.0001f);
    XII_TEST_FLOAT(material.m_fRoughness, 0.2f, 0.0001f);
    XII_TEST_FLOAT(material.m_BaseColor.b, 0.6f, 0.0001f);
    XII_TEST_FLOAT(material.m_BaseColor.a * material.m_fOpacity, 0.8f, 0.0001f);
    XII_TEST_FLOAT(material.m_EmissiveColor.g, 0.2f, 0.0001f);
    XII_TEST_FLOAT(material.m_fAlphaCutoff, 0.3f, 0.0001f);
    XII_TEST_BOOL(material.m_bTwoSided && material.m_AlphaMode == xiiModelMeshMaterial::AlphaMode::Mask);
    bool base = false, normal = false, orm = false, ao = false;
    for (const auto& texture : material.m_Textures)
    {
      switch (texture.m_Usage)
      {
        case xiiModelTextureUsage::BaseColor:
          base = true;
          XII_TEST_INT(texture.m_uiUVChannel, 1);
          XII_TEST_BOOL(texture.m_Sampler.m_AddressU == xiiGALTextureAddressMode::Clamp);
          XII_TEST_BOOL(texture.m_Sampler.m_AddressV == xiiGALTextureAddressMode::Mirror);
          XII_TEST_BOOL(texture.m_Sampler.m_MagFilter == xiiGALFilterType::Point);
          XII_TEST_FLOAT(texture.m_Sampler.m_fMaxLOD, 0, 0);
          break;
        case xiiModelTextureUsage::Normal:
          normal = true;
          XII_TEST_FLOAT(texture.m_fStrength, 0.4f, 0.0001f);
          break;
        case xiiModelTextureUsage::MetallicRoughness:
          orm = true;
          break;
        case xiiModelTextureUsage::Occlusion:
          ao = true;
          XII_TEST_FLOAT(texture.m_fStrength, 0.6f, 0.0001f);
          break;
        default:
          break;
      }
    }
    XII_TEST_BOOL(base && normal && orm && ao);
    if (XII_TEST_INT(imported.m_EmbeddedTextures.GetCount(), 1))
    {
      const auto& texture = imported.m_EmbeddedTextures[0];
      XII_TEST_STRING(texture.m_sFileExtension.GetData(), "png");
      XII_TEST_BOOL(texture.m_Data.GetCount() > 8);
      XII_TEST_INT(texture.m_Data[0], 137);
      XII_TEST_INT(texture.m_Data[1], 'P');
      XII_TEST_INT(texture.m_uiWidth, 0);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Deterministic reimport, root transform, UV flip and color conversion")
  {
    xiiModelMeshImport repeated;
    XII_TEST_RESULT(xiiModelMeshImport::ReadFile(source, options, repeated, &error));
    XII_TEST_BOOL(xiiArrayPtr<const xiiUInt8>(repeated.m_Mesh.GetMeshBufferDescriptor().GetVertexData()) == xiiArrayPtr<const xiiUInt8>(imported.m_Mesh.GetMeshBufferDescriptor().GetVertexData()));
    XII_TEST_BOOL(xiiArrayPtr<const xiiUInt8>(repeated.m_Mesh.GetMeshBufferDescriptor().GetIndexData()) == xiiArrayPtr<const xiiUInt8>(imported.m_Mesh.GetMeshBufferDescriptor().GetIndexData()));
    options.m_RootTransform.SetTranslationVector(xiiVec3(1, 2, 3));
    options.m_bFlipTextureV         = true;
    options.m_VertexColorConversion = xiiModelVertexColorConversion::SRGBToLinear;
    XII_TEST_RESULT(xiiModelMeshImport::ReadFile(source, options, repeated, &error));
    const auto vertex = Vertex(repeated.m_Mesh.GetMeshBufferDescriptor(), 0);
    XII_TEST_VEC3(vertex.m_vPosition, xiiVec3(1, 2, 3), 0.0001f);
    XII_TEST_FLOAT(vertex.m_vTexCoord1.y, 0.3f, 0.0001f);
    XII_TEST_INT(vertex.m_Color0.r, 55);
    options.m_RootTransform.Element(0, 2) = 0.5f; // Affine shear needs inverse-transpose normals.
    XII_TEST_RESULT(xiiModelMeshImport::ReadFile(source, options, repeated, &error));
    const auto& sheared        = repeated.m_Mesh.GetMeshBufferDescriptor();
    const auto  a              = Vertex(sheared, Index(sheared, 0));
    const auto  b              = Vertex(sheared, Index(sheared, 1));
    const auto  c              = Vertex(sheared, Index(sheared, 2));
    const auto  expectedNormal = (b.m_vPosition - a.m_vPosition).CrossRH(c.m_vPosition - a.m_vPosition).GetNormalized();
    XII_TEST_VEC3(a.m_vNormal, expectedNormal, 0.0001f);
    XII_TEST_FLOAT(a.m_vNormal.Dot(a.m_vTangent.GetAsVec3()), 0, 0.0001f);
    options = {};
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Skin palette, exact normalized byte weights and native skeleton agreement")
  {
    xiiStringBuilder skin(s_szSource);
    skin.ReplaceAll("\"nodes\":[0,1,2]", "\"nodes\":[0,2]");
    skin.ReplaceAll("\"name\":\"Surface\",\"mesh\":0", "\"name\":\"Surface\",\"mesh\":0,\"skin\":0");
    skin.ReplaceAll("\"COLOR_0\":5", "\"COLOR_0\":5,\"JOINTS_0\":6,\"WEIGHTS_0\":7");
    XII_TEST_RESULT(WriteSource(source, skin));
    options.m_bImportSkinning = true;
    options.m_RootTransform.SetTranslationVector(xiiVec3(3, 4, 5));
    if (XII_TEST_RESULT(xiiModelMeshImport::ReadFile(source, options, imported, &error)))
    {
      XII_TEST_INT(imported.m_Mesh.m_Bones.GetCount(), 3); // Includes the rigid fallback for the unweighted vertex.
      XII_TEST_BOOL(imported.m_Mesh.m_UsageFlags.IsSet(xiiMeshResourceUsageFlags::Skinned));
      xiiModelSkeletonImport skeleton;
      XII_TEST_RESULT(xiiModelSkeletonImport::ReadFile(source, skeleton, &error));
      for (auto it = imported.m_Mesh.m_Bones.GetIterator(); it.IsValid(); ++it)
      {
        const auto joint = skeleton.m_Skeleton.FindJointByName(xiiTempHashedString(it.Key().GetString()));
        if (XII_TEST_BOOL(joint < skeleton.m_Skeleton.m_Joints.GetCount()))
          XII_TEST_BOOL((options.m_RootTransform * skeleton.m_Skeleton.m_Joints[joint].m_ModelRestPose * it.Value().m_GlobalInverseRestPoseMatrix).IsEqual(xiiMat4::MakeIdentity(), 0.0001f));
      }
      const auto& buffer = imported.m_Mesh.GetMeshBufferDescriptor();
      for (xiiUInt32 i = 0; i < buffer.GetVertexCount(); ++i)
      {
        const auto vertex = Vertex(buffer, i);
        xiiUInt32  sum    = 0;
        for (xiiUInt32 c = 0; c < 4; ++c)
        {
          sum += vertex.m_BoneWeights0.GetData()[c];
          XII_TEST_BOOL(vertex.m_vBoneIndices0.GetData()[c] < imported.m_Mesh.m_Bones.GetCount());
        }
        XII_TEST_INT(sum, 255);
      }
      const auto first = Vertex(buffer, 0);
      XII_TEST_INT(first.m_BoneWeights0.r, 128);
      XII_TEST_INT(first.m_BoneWeights0.g, 127);
      options.m_bNormalizeWeights = false;
      XII_TEST_RESULT(xiiModelMeshImport::ReadFile(source, options, imported, &error));
      const auto raw = Vertex(imported.m_Mesh.GetMeshBufferDescriptor(), 0);
      XII_TEST_INT(raw.m_BoneWeights0.r, 127);
      XII_TEST_INT(raw.m_BoneWeights0.g, 127);
    }
    skin.ReplaceAll("\"indices\":8", "\"targets\":[{\"POSITION\":0}],\"indices\":8");
    XII_TEST_RESULT(WriteSource(source, skin));
    XII_TEST_BOOL(xiiModelMeshImport::ReadFile(source, options, imported, &error).Failed());
    XII_TEST_BOOL(!error.IsEmpty() && imported.m_Mesh.m_Bones.IsEmpty());
    options = {};
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Failure clears all prior output")
  {
    auto expectFailure = [&]() {
      XII_TEST_BOOL(xiiModelMeshImport::ReadFile(source, options, imported, &error).Failed());
      XII_TEST_BOOL(!error.IsEmpty());
      XII_TEST_INT(imported.m_Mesh.GetMeshBufferDescriptor().GetVertexCount(), 0);
      XII_TEST_BOOL(imported.m_Materials.IsEmpty() && imported.m_EmbeddedTextures.IsEmpty() && imported.m_Mesh.m_Bones.IsEmpty());
    };
    XII_TEST_RESULT(WriteSource(source, s_szSource));
    options.m_fSimplification = 1;
    expectFailure();
    options = {};
    options.m_RootTransform.SetScalingFactors(xiiVec3(0, 1, 1)).IgnoreResult();
    expectFailure();
    options = {};
    xiiStringBuilder invalid(s_szSource);
    invalid.ReplaceAll("[-2,3,1]", "[0,3,1]"); // Failure after the first instance has already been imported.
    XII_TEST_RESULT(WriteSource(source, invalid));
    expectFailure();
    invalid = s_szSource;
    invalid.ReplaceAll("\"TEXCOORD_1\":4", "\"TEXCOORD_2\":4");
    XII_TEST_RESULT(WriteSource(source, invalid));
    expectFailure(); // glTF requires contiguous texture-coordinate accessor indices.
    invalid = s_szSource;
    invalid.ReplaceAll("\"TEXCOORD_1\":4", "\"TEXCOORD_1\":4,\"TEXCOORD_2\":4");
    XII_TEST_RESULT(WriteSource(source, invalid));
    const auto optionalUVResult = xiiModelMeshImport::ReadFile(source, options, imported, &error);
    XII_TEST_RESULT_MSG(optionalUVResult, "%s", error.GetData());
    XII_TEST_BOOL(!imported.m_Mesh.GetMeshBufferDescriptor().GetVertexData().IsEmpty());
    XII_TEST_RESULT(WriteSource(source, "not a model"));
    expectFailure();
    XII_TEST_BOOL(xiiModelMeshImport::ReadFile("Relative.gltf", options, imported, &error).Failed());
    source.ChangeFileName("Missing");
    expectFailure();
  }
}
