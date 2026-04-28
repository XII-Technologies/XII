/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SurfaceAsset/SurfaceAsset.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSurfaceAssetDocument, 2, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSurfaceAssetDocument::xiiSurfaceAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiSurfaceResourceDescriptor>(sDocumentPath, xiiAssetDocEngineConnection::None)
{
}

xiiTransformStatus xiiSurfaceAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  const xiiSurfaceResourceDescriptor* pProp = GetProperties();

  pProp->Save(stream);

  return XII_SUCCESS;
}
