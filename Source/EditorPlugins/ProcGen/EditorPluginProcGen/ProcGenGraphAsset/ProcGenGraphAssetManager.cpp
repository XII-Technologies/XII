#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetManager.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGenGraphAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiProcGenGraphAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiProcGenGraphAssetDocumentManager::xiiProcGenGraphAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiProcGenGraphAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "ProcGen Graph";
  m_DocTypeDesc.m_sFileExtension    = "xiiProcGenGraphAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/ProcGen_Graph.png";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiProcGenGraphAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_ProcGen_Graph");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiProcGenGraph";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave;

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("ProcGen Graph", QPixmap(":/AssetIcons/ProcGen_Graph.png"));
}

xiiProcGenGraphAssetDocumentManager::~xiiProcGenGraphAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiProcGenGraphAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiProcGenGraphAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiProcGenGraphAssetDocument>())
      {
        auto pDocWnd = new xiiProcGenGraphAssetDocumentWindow(static_cast<xiiProcGenGraphAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void xiiProcGenGraphAssetDocumentManager::InternalCreateDocument(
  const char*              szDocumentTypeName,
  const char*              szPath,
  bool                     bCreateNewDocument,
  xiiDocument*&            out_pDocument,
  const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiProcGenGraphAssetDocument(szPath);
}

void xiiProcGenGraphAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
