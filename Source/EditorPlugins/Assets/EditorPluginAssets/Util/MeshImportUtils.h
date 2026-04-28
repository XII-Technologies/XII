/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <EditorPluginAssets/Util/AssetUtils.h>
#include <GraphicsCore/Meshes/MeshBufferUtils.h>

class xiiMeshResourceDescriptor;

namespace xiiModelImporter2
{
  class Importer;
  enum class TextureSemantic : xiiInt8;
} // namespace xiiModelImporter2

namespace xiiMeshImportUtils
{
  XII_EDITORPLUGINASSETS_DLL xiiString ImportOrResolveTexture(const char* szImportSourceFolder, const char* szImportTargetFolder, const char* szTexturePath, xiiModelImporter2::TextureSemantic hint, bool bTextureClamp);

  // XII_EDITORPLUGINASSETS_DLL void ImportMaterial(xiiMaterialAssetDocument* materialDocument, const xiiModelImporter::Material* material, const char* szImportSourceFolder, const char* szImportTargetFolder);

  // XII_EDITORPLUGINASSETS_DLL void ImportMeshMaterials(const xiiModelImporter::Scene& scene, const xiiModelImporter::Mesh& mesh, xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_MaterialSlots, const char* szImportSourceFolder, const char* szImportTargetFolder);

  // XII_EDITORPLUGINASSETS_DLL void ImportMeshAssetMaterials(const char* szAssetDocument, const char* szMeshFile, bool bUseSubFolderForImportedMaterials, const xiiModelImporter::Scene& scene, const xiiModelImporter::Mesh& mesh, xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_MaterialSlots);

  // XII_EDITORPLUGINASSETS_DLL const xiiString GetResourceSlotProperty(const xiiHybridArray<xiiMaterialResourceSlot, 8>& materialSlots, xiiUInt32 uiSlot);

  // XII_EDITORPLUGINASSETS_DLL void AddMeshToDescriptor(xiiMeshResourceDescriptor& meshDescriptor, const xiiModelImporter::Scene& scene, const xiiModelImporter::Mesh& mesh, const xiiHybridArray<xiiMaterialResourceSlot, 8>& materialSlots);

  // XII_EDITORPLUGINASSETS_DLL void UpdateMaterialSlots(xiiStringView sDocumentPath, const xiiModelImporter::Scene& scene, const xiiModelImporter::Mesh& mesh, bool bImportMaterials, bool bUseSubFolderForImportedMaterials, const char* szMeshFile, xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_MaterialSlots);

  // XII_EDITORPLUGINASSETS_DLL void PrepareMeshForImport(xiiModelImporter::Mesh& mesh, bool bRecalculateNormals, xiiProgressRange& range);

  // XII_EDITORPLUGINASSETS_DLL xiiStatus GenerateMeshBuffer(const xiiModelImporter::Mesh& mesh, xiiMeshResourceDescriptor& meshDescriptor, const xiiMat3& mTransformation, bool bInvertNormals, xiiMeshNormalPrecision::Enum normalPrecision, xiiMeshTexCoordPrecision::Enum texCoordPrecision, bool bSkinnedMesh);

  // XII_EDITORPLUGINASSETS_DLL xiiStatus TryImportMesh(xiiSharedPtr<xiiModelImporter::Scene>& out_pScene, xiiModelImporter::Mesh*& out_pMesh, const char* szMeshFile, const char* szSubMeshName, const xiiMat3& mMeshTransform, bool bRecalculateNormals, bool bInvertNormals, xiiMeshNormalPrecision::Enum normalPrecision, xiiMeshTexCoordPrecision::Enum texCoordPrecision, xiiProgressRange& range, xiiMeshResourceDescriptor& meshDescriptor, bool bSkinnedMesh);

  XII_EDITORPLUGINASSETS_DLL void SetMeshAssetMaterialSlots(xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_materialSlots, const xiiModelImporter2::Importer* pImporter);
  XII_EDITORPLUGINASSETS_DLL void CopyMeshAssetMaterialSlotToResource(xiiMeshResourceDescriptor& ref_desc, const xiiHybridArray<xiiMaterialResourceSlot, 8>& materialSlots);
  XII_EDITORPLUGINASSETS_DLL void ImportMeshAssetMaterials(xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_materialSlots, xiiStringView sDocumentDirectory, const xiiModelImporter2::Importer* pImporter);
} // namespace xiiMeshImportUtils
