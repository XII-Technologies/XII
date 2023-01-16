#pragma once

#include <Core/Collection/CollectionResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class xiiCollectionAssetEntry : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCollectionAssetEntry, xiiReflectedClass);

public:
  xiiString m_sLookupName;
  xiiString m_sRedirectionAsset;
};

class xiiCollectionAssetData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCollectionAssetData, xiiReflectedClass);

public:
  xiiDynamicArray<xiiCollectionAssetEntry> m_Entries;
};

class xiiCollectionAssetDocument : public xiiSimpleAssetDocument<xiiCollectionAssetData>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCollectionAssetDocument, xiiSimpleAssetDocument<xiiCollectionAssetData>);

public:
  xiiCollectionAssetDocument(const char* szDocumentPath);

protected:
  virtual void UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;

  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
};
