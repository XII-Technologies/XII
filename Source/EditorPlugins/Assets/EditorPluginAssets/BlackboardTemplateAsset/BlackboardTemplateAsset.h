#pragma once

#include <Core/Collection/CollectionResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>

struct xiiBlackboardTemplateAssetObject : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBlackboardTemplateAssetObject, xiiReflectedClass);

  xiiDynamicArray<xiiString>          m_BaseTemplates;
  xiiDynamicArray<xiiBlackboardEntry> m_Entries;
};

class xiiBlackboardTemplateAssetDocument : public xiiSimpleAssetDocument<xiiBlackboardTemplateAssetObject>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBlackboardTemplateAssetDocument, xiiSimpleAssetDocument<xiiBlackboardTemplateAssetObject>);

public:
  xiiBlackboardTemplateAssetDocument(xiiStringView sDocumentPath);

  xiiStatus WriteAsset(xiiStreamWriter& inout_stream, const xiiPlatformProfile* pAssetProfile) const;

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& inout_stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  xiiStatus RetrieveState(const xiiBlackboardTemplateAssetObject* pProp, xiiBlackboardTemplateResourceDescriptor& inout_Desc) const;
};
