#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetManager.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltCollisionMeshAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiJoltCollisionMeshAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiJoltCollisionMeshAssetDocumentManager::xiiJoltCollisionMeshAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiJoltCollisionMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Jolt_Colmesh_Triangle";
  m_DocTypeDesc.m_sFileExtension    = "xiiJoltCollisionMeshAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/Jolt_Collision_Mesh.png";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiJoltCollisionMeshAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Jolt_Colmesh_Triangle");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiJoltMesh";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::SupportsThumbnail;

  m_DocTypeDesc2.m_sDocumentTypeName = "Jolt_Colmesh_Convex";
  m_DocTypeDesc2.m_sFileExtension    = "xiiJoltConvexCollisionMeshAsset";
  m_DocTypeDesc2.m_sIcon             = ":/AssetIcons/Jolt_Collision_Mesh_Convex.png";
  m_DocTypeDesc2.m_pDocumentType     = xiiGetStaticRTTI<xiiJoltCollisionMeshAssetDocument>();
  m_DocTypeDesc2.m_pManager          = this;
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Jolt_Colmesh_Triangle"); // convex meshes can also be used as triangle meshes (concave)
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Jolt_Colmesh_Convex");

  m_DocTypeDesc2.m_sResourceFileExtension = "xiiJoltMesh";
  m_DocTypeDesc2.m_AssetDocumentFlags     = xiiAssetDocumentFlags::SupportsThumbnail;
}

xiiJoltCollisionMeshAssetDocumentManager::~xiiJoltCollisionMeshAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiJoltCollisionMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiJoltCollisionMeshAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiJoltCollisionMeshAssetDocument>())
      {
        xiiQtJoltCollisionMeshAssetDocumentWindow* pDocWnd = new xiiQtJoltCollisionMeshAssetDocumentWindow(static_cast<xiiAssetDocument*>(e.m_pDocument));
      }
    }
    break;
    default:
      break;
  }
}

void xiiJoltCollisionMeshAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  if (xiiStringUtils::IsEqual(szDocumentTypeName, "Jolt_Colmesh_Convex"))
  {
    out_pDocument = new xiiJoltCollisionMeshAssetDocument(szPath, true);
  }
  else
  {
    out_pDocument = new xiiJoltCollisionMeshAssetDocument(szPath, false);
  }
}

void xiiJoltCollisionMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
  inout_DocumentTypes.PushBack(&m_DocTypeDesc2);
}

xiiUInt64 xiiJoltCollisionMeshAssetDocumentManager::ComputeAssetProfileHashImpl(const xiiPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}
