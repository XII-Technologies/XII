/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/Declarations.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>
#include <Texture/Converter/TextureConverterEnums.h>

struct xiiModelMaterialTexture;
struct xiiModelMeshImport;

namespace xiiMeshImportUtils
{
  /// Refresh stable extracted images even when material slots already have authored overrides.
  /// Background callers only compare content and return NeedsImport when a write is required.
  xiiTransformStatus PrepareEmbeddedTextures(const xiiModelMeshImport& imported, xiiStringView sDocumentPath, bool bAllowImport);
  /// Reuse the document identity even before asynchronous curator discovery completes.
  /// Existing documents are never modified or closed by this call.
  xiiStatus ResolveAssetReference(xiiStringView sAbsolutePath, xiiString& out_sAsset);
  /// Resolve source-relative or embedded data, then create a texture asset with the exact imported sampler.
  xiiStatus ImportTexture(const xiiModelMaterialTexture& texture, const xiiModelMeshImport& imported, xiiStringView sSourceDirectory, xiiStringView sTargetDirectory, xiiTextureConverterUsage::Enum usage, bool bAlpha, xiiString& out_sAsset);
  /// Pack separate linear red-channel factors into native green roughness / blue metallic.
  /// Original source files remain texture document inputs, preserving dependency tracking and reload.
  xiiStatus ImportMetallicRoughnessTexture(const xiiModelMaterialTexture* pRoughness, const xiiModelMaterialTexture* pMetallic, const xiiModelMeshImport& imported, xiiStringView sSourceDirectory, xiiStringView sTargetDirectory, xiiString& out_sAsset);
} // namespace xiiMeshImportUtils
