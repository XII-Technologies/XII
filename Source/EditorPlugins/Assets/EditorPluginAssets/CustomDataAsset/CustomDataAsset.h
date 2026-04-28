/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Utils/CustomData.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class xiiCustomDataAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCustomDataAssetProperties, xiiReflectedClass);

public:
  xiiCustomData* m_pType = nullptr;
};

class xiiCustomDataAssetDocument : public xiiSimpleAssetDocument<xiiCustomDataAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCustomDataAssetDocument, xiiSimpleAssetDocument<xiiCustomDataAssetProperties>);

public:
  xiiCustomDataAssetDocument(xiiStringView sDocumentPath);

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  virtual void UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;
};
