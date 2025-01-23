#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/CollectionAsset/CollectionAsset.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAssetManager.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCollectionAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiCollectionAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiCollectionAssetDocumentManager::xiiCollectionAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiCollectionAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Collection";
  m_DocTypeDesc.m_sFileExtension    = "xiiCollectionAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/Collection.svg";
  m_DocTypeDesc.m_sAssetCategory    = "Utilities";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiCollectionAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_AssetCollection");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiBinCollection";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave;

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("Collection", QPixmap(":/AssetIcons/Collection.svg"));
}

xiiCollectionAssetDocumentManager::~xiiCollectionAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiCollectionAssetDocumentManager::OnDocumentManagerEvent, this));
}


void xiiCollectionAssetDocumentManager::GetAssetTypesRequiringTransformForSceneExport(xiiSet<xiiTempHashedString>& inout_assetTypes)
{
  inout_assetTypes.Insert(xiiTempHashedString(m_DocTypeDesc.m_sDocumentTypeName));
}

void xiiCollectionAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiCollectionAssetDocument>())
      {
        new xiiQtCollectionAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiCollectionAssetDocumentManager::InternalCreateDocument(
  xiiStringView            sDocumentTypeName,
  xiiStringView            sPath,
  bool                     bCreateNewDocument,
  xiiDocument*&            out_pDocument,
  const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiCollectionAssetDocument(sPath);
}

void xiiCollectionAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
