#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetManager.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetWindow.moc.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiMeshAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiMeshAssetDocumentManager::xiiMeshAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  xiiAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Mesh_Static", "xiiMesh");

  m_DocTypeDesc.m_sDocumentTypeName = "Mesh";
  m_DocTypeDesc.m_sFileExtension    = "xiiMeshAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/Mesh.svg";
  m_DocTypeDesc.m_sAssetCategory    = "Rendering";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiMeshAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Mesh_Static");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiBinMesh";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::SupportsThumbnail;
}

xiiMeshAssetDocumentManager::~xiiMeshAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}

xiiResult xiiMeshAssetDocumentManager::OpenPickedDocument(const xiiDocumentObject* pPickedComponent, xiiUInt32 uiPartIndex)
{
  // check that we actually picked a mesh component
  if (!pPickedComponent->GetTypeAccessor().GetType()->IsDerivedFrom<xiiMeshComponent>())
    return XII_FAILURE;

  // first try the materials array on the component itself, and see if we have a material override to pick
  if ((xiiInt32)uiPartIndex < pPickedComponent->GetTypeAccessor().GetCount("Materials"))
  {
    // access the material at the given index
    // this might be empty, though, in which case we still need to check the mesh asset
    const xiiVariant varMatGuid = pPickedComponent->GetTypeAccessor().GetValue("Materials", uiPartIndex);

    // if it were anything else than a string that would be weird
    XII_ASSERT_DEV(varMatGuid.IsA<xiiString>(), "Material override property is not a string type");

    if (varMatGuid.IsA<xiiString>())
    {
      if (TryOpenAssetDocument(varMatGuid.Get<xiiString>()).Succeeded())
        return XII_SUCCESS;
    }
  }

  // couldn't open it through the override, so we now need to inspect the mesh asset
  const xiiVariant varMeshGuid = pPickedComponent->GetTypeAccessor().GetValue("Mesh");

  XII_ASSERT_DEV(varMeshGuid.IsA<xiiString>(), "Mesh property is not a string type");

  if (!varMeshGuid.IsA<xiiString>())
    return XII_FAILURE;

  // we don't support non-guid mesh asset references, because I'm too lazy
  if (!xiiConversionUtils::IsStringUuid(varMeshGuid.Get<xiiString>()))
    return XII_FAILURE;

  const xiiUuid meshGuid = xiiConversionUtils::ConvertStringToUuid(varMeshGuid.Get<xiiString>());

  auto pSubAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(meshGuid);

  // unknown mesh asset
  if (!pSubAsset)
    return XII_FAILURE;

  // now we need to open the mesh and we cannot wait for it (usually that is queued for GUI reasons)
  // though we do not want a window
  xiiMeshAssetDocument* pMeshDoc =
    static_cast<xiiMeshAssetDocument*>(xiiQtEditorApp::GetSingleton()->OpenDocument(pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath(), xiiDocumentFlags::None));

  if (!pMeshDoc)
    return XII_FAILURE;

  xiiResult result = XII_FAILURE;

  // if we are outside the stored index, tough luck
  if (uiPartIndex < pMeshDoc->GetProperties()->m_Slots.GetCount())
  {
    result = TryOpenAssetDocument(pMeshDoc->GetProperties()->m_Slots[uiPartIndex].m_sResource);
  }

  // make sure to close the document again, if we were the ones to open it
  // otherwise keep it open
  if (!pMeshDoc->HasWindowBeenRequested())
    pMeshDoc->GetDocumentManager()->CloseDocument(pMeshDoc);

  return result;
}

void xiiMeshAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiMeshAssetDocument>())
      {
        new xiiQtMeshAssetDocumentWindow(static_cast<xiiMeshAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiMeshAssetDocumentManager::InternalCreateDocument(
  xiiStringView            sDocumentTypeName,
  xiiStringView            sPath,
  bool                     bCreateNewDocument,
  xiiDocument*&            out_pDocument,
  const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiMeshAssetDocument(sPath);
}

void xiiMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
