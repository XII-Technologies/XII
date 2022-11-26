#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetManager.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRmlUiAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiRmlUiAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRmlUiAssetDocumentManager::xiiRmlUiAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiRmlUiAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "RmlUi";
  m_DocTypeDesc.m_sFileExtension    = "xiiRmlUiAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/RmlUi.png";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiRmlUiAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Rml_UI");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiRmlUi";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::SupportsThumbnail;
}

xiiRmlUiAssetDocumentManager::~xiiRmlUiAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiRmlUiAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiRmlUiAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiRmlUiAssetDocument>())
      {
        xiiQtRmlUiAssetDocumentWindow* pDocWnd = new xiiQtRmlUiAssetDocumentWindow(static_cast<xiiRmlUiAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void xiiRmlUiAssetDocumentManager::InternalCreateDocument(
  const char*              szDocumentTypeName,
  const char*              szPath,
  bool                     bCreateNewDocument,
  xiiDocument*&            out_pDocument,
  const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiRmlUiAssetDocument(szPath);
}

void xiiRmlUiAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
