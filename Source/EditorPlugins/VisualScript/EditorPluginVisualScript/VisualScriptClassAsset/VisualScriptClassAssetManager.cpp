#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptClassAsset/VisualScriptClassAsset.h>
#include <EditorPluginVisualScript/VisualScriptClassAsset/VisualScriptClassAssetManager.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptClassAssetManager, 1, xiiRTTIDefaultAllocator<xiiVisualScriptClassAssetManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiVisualScriptClassAssetManager::xiiVisualScriptClassAssetManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiVisualScriptClassAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "VisualScriptClass";
  m_DocTypeDesc.m_sFileExtension = "xiiVisualScriptClassAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/VisualScript.png";
  m_DocTypeDesc.m_pDocumentType = xiiGetStaticRTTI<xiiVisualScriptClassAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_ScriptClass");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiVisualScriptClassBin";
  m_DocTypeDesc.m_AssetDocumentFlags = xiiAssetDocumentFlags::AutoTransformOnSave;

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("VisualScriptClass", QPixmap(":/AssetIcons/VisualScript.png"));
}

xiiVisualScriptClassAssetManager::~xiiVisualScriptClassAssetManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiVisualScriptClassAssetManager::OnDocumentManagerEvent, this));
}

void xiiVisualScriptClassAssetManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiVisualScriptClassAssetDocument>())
      {
        xiiQtVisualScriptWindow* pDocWnd = new xiiQtVisualScriptWindow(e.m_pDocument);
      }
    }
    break;

    default:
      break;
  }
}

void xiiVisualScriptClassAssetManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiVisualScriptClassAssetDocument(szPath);
}

void xiiVisualScriptClassAssetManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
