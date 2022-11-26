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
  xiiImageDataAssetDocument(const char* szDocumentPath);

  const xiiEvent<const xiiImageDataAssetEvent&>& Events() const { return m_Events; }

protected:
  xiiEvent<const xiiImageDataAssetEvent&> m_Events;

  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override { return xiiStatus(XII_SUCCESS); }
  virtual xiiTransformStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  xiiStatus RunTexConv(const char* szTargetFile, const xiiAssetFileHeader& AssetHeader, bool bUpdateThumbnail);
};
