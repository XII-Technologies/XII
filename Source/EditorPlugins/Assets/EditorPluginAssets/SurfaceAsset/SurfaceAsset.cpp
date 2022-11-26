#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SurfaceAsset/SurfaceAsset.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSurfaceAssetDocument, 2, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSurfaceAssetDocument::xiiSurfaceAssetDocument(const char* szDocumentPath) :
  xiiSimpleAssetDocument<xiiSurfaceResourceDescriptor>(szDocumentPath, xiiAssetDocEngineConnection::None)
{
}

xiiTransformStatus xiiSurfaceAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  const xiiSurfaceResourceDescriptor* pProp = GetProperties();

  pProp->Save(stream);

  return xiiStatus(XII_SUCCESS);
}
