#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SurfaceAsset/SurfaceAsset.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAssetManager.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSurfaceAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiSurfaceAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSurfaceAssetDocumentManager::xiiSurfaceAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiSurfaceAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Surface";
  m_DocTypeDesc.m_sFileExtension    = "xiiSurfaceAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/Surface.svg";
  m_DocTypeDesc.m_sAssetCategory    = "Utilities";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiSurfaceAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Surface");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiSurface";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave;

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("Surface", QPixmap(":/AssetIcons/Surface.svg"));
}

xiiSurfaceAssetDocumentManager::~xiiSurfaceAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiSurfaceAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiSurfaceAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiSurfaceAssetDocument>())
      {
        new xiiQtSurfaceAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiSurfaceAssetDocumentManager::InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiSurfaceAssetDocument(sPath);
}

void xiiSurfaceAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
