#pragma once

#include <Core/Physics/SurfaceResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class xiiSurfaceAssetDocument : public xiiSimpleAssetDocument<xiiSurfaceResourceDescriptor>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSurfaceAssetDocument, xiiSimpleAssetDocument<xiiSurfaceResourceDescriptor>);

public:
  xiiSurfaceAssetDocument(const char* szDocumentPath);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
};
