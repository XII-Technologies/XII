/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ModelImporter/Implementation/SceneImportUtils.h>
#include <ModelImporter/MeshImport.h>
#include <assimp/scene.h>

namespace xiiMeshImportInternal
{
  using xiiModelImportInternal::Fail;
  using xiiModelImportInternal::ConvertMatrix;

  xiiResult ReadMaterial(const aiScene& scene, const aiMaterial& material, xiiModelMeshMaterial& out_material,
                         xiiDynamicArray<xiiModelEmbeddedTexture>& inout_textures, xiiStringBuilder* error);
  xiiResult ReadSkin(const aiMesh& mesh, xiiStringView sNodeName, const xiiMat4& meshTransform,
                     const xiiModelMeshImportOptions& options, xiiMeshResourceDescriptor& inout_mesh,
                     xiiArrayPtr<xiiMeshPackedVertex> vertices, xiiStringBuilder* error);
} // namespace xiiMeshImportInternal
