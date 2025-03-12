#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/CustomDataAsset/CustomDataAsset.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAssetManager.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCustomDataAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiCustomDataAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiCustomDataAssetDocumentManager::xiiCustomDataAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiCustomDataAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "CustomData";
  m_DocTypeDesc.m_sFileExtension    = "xiiCustomDataAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/CustomData.svg";
  m_DocTypeDesc.m_sAssetCategory    = "Logic";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiCustomDataAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_CustomData"); // \todo should only be compatible with same type

  m_DocTypeDesc.m_sResourceFileExtension = "xiiBinCustomData";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave;

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("CustomData", QPixmap(":/AssetIcons/CustomData.svg"));
}

xiiCustomDataAssetDocumentManager::~xiiCustomDataAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiCustomDataAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiCustomDataAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiCustomDataAssetDocument>())
      {
        new xiiQtCustomDataAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiCustomDataAssetDocumentManager::InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiCustomDataAssetDocument(sPath);
}

void xiiCustomDataAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
