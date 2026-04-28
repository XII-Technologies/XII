/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAsset.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAssetManager.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBlackboardTemplateAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiBlackboardTemplateAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiBlackboardTemplateAssetDocumentManager::xiiBlackboardTemplateAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiBlackboardTemplateAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "BlackboardTemplate";
  m_DocTypeDesc.m_sFileExtension    = "xiiBlackboardTemplateAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/BlackboardTemplate.svg";
  m_DocTypeDesc.m_sAssetCategory    = "Logic";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiBlackboardTemplateAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_BlackboardTemplate");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiBinBlackboardTemplate";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave;

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("BlackboardTemplate", QPixmap(":/AssetIcons/BlackboardTemplate.svg"));
}

xiiBlackboardTemplateAssetDocumentManager::~xiiBlackboardTemplateAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiBlackboardTemplateAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiBlackboardTemplateAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiBlackboardTemplateAssetDocument>())
      {
        new xiiQtBlackboardTemplateAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiBlackboardTemplateAssetDocumentManager::InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiBlackboardTemplateAssetDocument(sPath);
}

void xiiBlackboardTemplateAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
