#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetManager.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiKrautTreeAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiKrautTreeAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiKrautTreeAssetDocumentManager::xiiKrautTreeAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiKrautTreeAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Kraut Tree";
  m_DocTypeDesc.m_sFileExtension    = "xiiKrautTreeAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/Kraut_Tree.png";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiKrautTreeAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Kraut_Tree");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiKrautTree";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::SupportsThumbnail;
}

xiiKrautTreeAssetDocumentManager::~xiiKrautTreeAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiKrautTreeAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiKrautTreeAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiKrautTreeAssetDocument>())
      {
        xiiQtKrautTreeAssetDocumentWindow* pDocWnd = new xiiQtKrautTreeAssetDocumentWindow(static_cast<xiiKrautTreeAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void xiiKrautTreeAssetDocumentManager::InternalCreateDocument(
  const char*              szDocumentTypeName,
  const char*              szPath,
  bool                     bCreateNewDocument,
  xiiDocument*&            out_pDocument,
  const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiKrautTreeAssetDocument(szPath);
}

void xiiKrautTreeAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
