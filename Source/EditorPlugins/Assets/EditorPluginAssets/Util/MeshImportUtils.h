/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <SharedPluginAssets/Common/MeshMaterialSlots.h>

class xiiMeshResourceDescriptor;
struct xiiModelMeshImport;

namespace xiiMeshImportUtils
{
  /// Reconcile shared native material slots, preserving authored assignments across source reorderings.
  XII_EDITORPLUGINASSETS_DLL void      SetMeshAssetMaterialSlots(xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_slots, const xiiModelMeshImport& imported);
  XII_EDITORPLUGINASSETS_DLL xiiResult CopyMeshAssetMaterialSlotToResource(xiiMeshResourceDescriptor& inout_descriptor, xiiArrayPtr<const xiiMaterialResourceSlot> materialSlots);
  /// Stage material reconciliation/import without mutating the owning mesh document. Background jobs request foreground import.
  XII_EDITORPLUGINASSETS_DLL xiiTransformStatus PrepareMaterialSlots(xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_slots, bool bImportMaterials, bool bAllowImport, xiiStringView sDocumentPath, xiiStringView sAbsoluteSourceFile, const xiiModelMeshImport& imported);
  /// Import unassigned materials only. Explicit user assignments are retained. Output is committed only on success.
  XII_EDITORPLUGINASSETS_DLL xiiStatus ImportMeshAssetMaterials(xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_slots, xiiStringView sDocumentPath, xiiStringView sAbsoluteSourceFile, const xiiModelMeshImport& imported);
} // namespace xiiMeshImportUtils
