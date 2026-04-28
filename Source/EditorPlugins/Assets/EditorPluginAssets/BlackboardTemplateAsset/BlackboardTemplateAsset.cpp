/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAsset.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBlackboardTemplateAssetObject, 1, xiiRTTIDefaultAllocator<xiiBlackboardTemplateAssetObject>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("BaseTemplates", m_BaseTemplates)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate", xiiDependencyFlags::Transform)),
    XII_ARRAY_MEMBER_PROPERTY("Entries", m_Entries),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBlackboardTemplateAssetDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBlackboardTemplateAssetDocument::xiiBlackboardTemplateAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiBlackboardTemplateAssetObject>(sDocumentPath, xiiAssetDocEngineConnection::None)
{
}

xiiStatus xiiBlackboardTemplateAssetDocument::WriteAsset(xiiStreamWriter& inout_stream, const xiiPlatformProfile* pAssetProfile) const
{
  xiiBlackboardTemplateResourceDescriptor desc;
  XII_SUCCEED_OR_RETURN(RetrieveState(GetProperties(), desc));
  XII_SUCCEED_OR_RETURN(desc.Serialize(inout_stream));

  return XII_SUCCESS;
}

xiiStatus xiiBlackboardTemplateAssetDocument::RetrieveState(const xiiBlackboardTemplateAssetObject* pProp, xiiBlackboardTemplateResourceDescriptor& inout_Desc) const
{
  for (const xiiString& sTempl : pProp->m_BaseTemplates)
  {
    if (sTempl.IsEmpty())
      continue;

    auto pOther = xiiAssetCurator::GetSingleton()->FindSubAsset(sTempl);
    if (!pOther.isValid())
    {
      return xiiStatus(xiiFmt("Base template '{}' not found.", sTempl));
    }

    xiiDocument* pDoc;
    XII_SUCCEED_OR_RETURN(pOther->m_pAssetInfo->GetManager()->OpenDocument(pOther->m_Data.m_sSubAssetsDocumentTypeName, pOther->m_pAssetInfo->m_Path, pDoc, xiiDocumentFlags::None, nullptr));

    if (xiiBlackboardTemplateAssetDocument* pTmpDoc = xiiDynamicCast<xiiBlackboardTemplateAssetDocument*>(pDoc))
    {
      XII_SUCCEED_OR_RETURN(RetrieveState(pTmpDoc->GetProperties(), inout_Desc));
    }

    pOther->m_pAssetInfo->GetManager()->CloseDocument(pDoc);
  }

  for (const auto& e : pProp->m_Entries)
  {
    for (auto& e2 : inout_Desc.m_Entries)
    {
      if (e2.m_sName == e.m_sName)
      {
        e2 = e;
        goto next;
      }
    }

    inout_Desc.m_Entries.PushBack(e);

  next:;
  }

  return XII_SUCCESS;
}

xiiTransformStatus xiiBlackboardTemplateAssetDocument::InternalTransformAsset(xiiStreamWriter& inout_stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  return WriteAsset(inout_stream, pAssetProfile);
}
