#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetManager.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimatedMeshAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiAnimatedMeshAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiAnimatedMeshAssetDocumentManager::xiiAnimatedMeshAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Animated Mesh";
  m_DocTypeDesc.m_sFileExtension    = "xiiAnimatedMeshAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/Animated_Mesh.svg";
  m_DocTypeDesc.m_sAssetCategory    = "Rendering";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiAnimatedMeshAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Mesh_Static");
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Mesh_Skinned");

  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::SupportsThumbnail;
  m_DocTypeDesc.m_sResourceFileExtension = "xiiBinAnimatedMesh";
}

xiiAnimatedMeshAssetDocumentManager::~xiiAnimatedMeshAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiAnimatedMeshAssetDocument>())
      {
        new xiiQtAnimatedMeshAssetDocumentWindow(static_cast<xiiAnimatedMeshAssetDocument*>(e.m_pDocument)); // NOLINT
      }
    }
    break;

    default:
      break;
  }
}

void xiiAnimatedMeshAssetDocumentManager::InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiAnimatedMeshAssetDocument(sPath);
}

void xiiAnimatedMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
