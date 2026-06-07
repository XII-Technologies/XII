/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetObjects.h>

struct xiiImageDataAssetEvent
{
  enum class Type
  {
    Transformed,
  };

  Type m_Type = Type::Transformed;
};

class xiiImageDataAssetDocument : public xiiSimpleAssetDocument<xiiImageDataAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiImageDataAssetDocument, xiiSimpleAssetDocument<xiiImageDataAssetProperties>);

public:
  xiiImageDataAssetDocument(xiiStringView sDocumentPath);

  const xiiEvent<const xiiImageDataAssetEvent&>& Events() const { return m_Events; }

protected:
  xiiEvent<const xiiImageDataAssetEvent&> m_Events;

  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override { return XII_SUCCESS; }
  virtual xiiTransformStatus InternalTransformAsset(xiiStringView sTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  xiiStatus RunTextureConverter(xiiStringView sTargetFile, const xiiAssetFileHeader& AssetHeader, bool bUpdateThumbnail);
};
