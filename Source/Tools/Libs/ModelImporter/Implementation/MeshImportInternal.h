/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Strings/StringBuilder.h>
#include <ModelImporter/MeshImport.h>
#include <assimp/scene.h>

namespace xiiMeshImportInternal
{
  inline xiiResult Fail(xiiStringBuilder* error, xiiStringView message)
  {
    if (error != nullptr)
      *error = message;
    return XII_FAILURE;
  }

  inline xiiMat4 ConvertMatrix(const aiMatrix4x4& matrix)
  {
    const float values[] = {matrix.a1, matrix.a2, matrix.a3, matrix.a4,
                            matrix.b1, matrix.b2, matrix.b3, matrix.b4, matrix.c1, matrix.c2, matrix.c3, matrix.c4,
                            matrix.d1, matrix.d2, matrix.d3, matrix.d4};
    return xiiMat4::MakeFromRowMajorArray(values);
  }

  xiiResult ReadMaterial(const aiScene& scene, const aiMaterial& material, xiiModelMeshMaterial& out_material,
                         xiiDynamicArray<xiiModelEmbeddedTexture>& inout_textures, xiiStringBuilder* error);
  xiiResult ReadSkin(const aiMesh& mesh, xiiStringView sNodeName, const xiiMat4& meshTransform,
                     const xiiModelMeshImportOptions& options, xiiMeshResourceDescriptor& inout_mesh,
                     xiiArrayPtr<xiiMeshPackedVertex> vertices, xiiStringBuilder* error);
} // namespace xiiMeshImportInternal
