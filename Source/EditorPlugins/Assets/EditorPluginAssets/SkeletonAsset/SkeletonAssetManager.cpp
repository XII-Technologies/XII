#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetManager.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkeletonAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiSkeletonAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSkeletonAssetDocumentManager::xiiSkeletonAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiSkeletonAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Skeleton";
  m_DocTypeDesc.m_sFileExtension    = "xiiSkeletonAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/Skeleton.png";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiSkeletonAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Mesh_Skeleton");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiSkeleton";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::SupportsThumbnail | xiiAssetDocumentFlags::AutoTransformOnSave;
}

xiiSkeletonAssetDocumentManager::~xiiSkeletonAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiSkeletonAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiSkeletonAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiSkeletonAssetDocument>())
      {
        xiiQtSkeletonAssetDocumentWindow* pDocWnd = new xiiQtSkeletonAssetDocumentWindow(static_cast<xiiSkeletonAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void xiiSkeletonAssetDocumentManager::InternalCreateDocument(
  const char*              szDocumentTypeName,
  const char*              szPath,
  bool                     bCreateNewDocument,
  xiiDocument*&            out_pDocument,
  const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiSkeletonAssetDocument(szPath);
}

void xiiSkeletonAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
