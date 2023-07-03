#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetManager.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPropertyAnimAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiPropertyAnimAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiPropertyAnimAssetDocumentManager::xiiPropertyAnimAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiPropertyAnimAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "PropertyAnim";
  m_DocTypeDesc.m_sFileExtension    = "xiiPropertyAnimAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/PropertyAnim.png";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiPropertyAnimAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Property_Animation");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiPropertyAnim";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave;

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("PropertyAnim", QPixmap(":/AssetIcons/PropertyAnim.png"));
}

xiiPropertyAnimAssetDocumentManager::~xiiPropertyAnimAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiPropertyAnimAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiPropertyAnimAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiPropertyAnimAssetDocument>())
      {
        xiiQtPropertyAnimAssetDocumentWindow* pDocWnd =
          new xiiQtPropertyAnimAssetDocumentWindow(static_cast<xiiPropertyAnimAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void xiiPropertyAnimAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiPropertyAnimAssetDocument(szPath);
}

void xiiPropertyAnimAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
