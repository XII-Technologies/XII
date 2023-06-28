#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetManager.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptAssetManager, 1, xiiRTTIDefaultAllocator<xiiVisualScriptAssetManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiVisualScriptAssetManager::xiiVisualScriptAssetManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiVisualScriptAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Visual Script";
  m_DocTypeDesc.m_sFileExtension    = "xiiVisualScriptAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/Visual_Script.png";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiVisualScriptAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Code_VisualScript");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiVisualScriptBin";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave;

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("Visual Script", QPixmap(":/AssetIcons/Visual_Script.png"));
}

xiiVisualScriptAssetManager::~xiiVisualScriptAssetManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiVisualScriptAssetManager::OnDocumentManagerEvent, this));
}

void xiiVisualScriptAssetManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiVisualScriptAssetDocument>())
      {
        xiiQtVisualScriptAssetDocumentWindow* pDocWnd = new xiiQtVisualScriptAssetDocumentWindow(e.m_pDocument, e.m_pOpenContext);
      }
    }
    break;

    default:
      break;
  }
}

void xiiVisualScriptAssetManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiVisualScriptAssetDocument(szPath);
}

void xiiVisualScriptAssetManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
