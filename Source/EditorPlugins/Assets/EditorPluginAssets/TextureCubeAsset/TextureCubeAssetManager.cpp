#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include "ToolsFoundation/Assets/AssetFileExtensionWhitelist.h"
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAsset.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetManager.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureCubeAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiTextureCubeAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiTextureCubeAssetDocumentManager::xiiTextureCubeAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiTextureCubeAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  xiiAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Texture_Cube", "dds");

  m_DocTypeDesc.m_sDocumentTypeName = "Texture Cube";
  m_DocTypeDesc.m_sFileExtension    = "xiiTextureCubeAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/Texture_Cube.png";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiTextureCubeAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_Cube");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiTextureCube";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoThumbnailOnTransform;
}

xiiTextureCubeAssetDocumentManager::~xiiTextureCubeAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiTextureCubeAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiTextureCubeAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiTextureCubeAssetDocument>())
      {
        xiiQtTextureCubeAssetDocumentWindow* pDocWnd = new xiiQtTextureCubeAssetDocumentWindow(static_cast<xiiTextureCubeAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void xiiTextureCubeAssetDocumentManager::InternalCreateDocument(
  const char*              szDocumentTypeName,
  const char*              szPath,
  bool                     bCreateNewDocument,
  xiiDocument*&            out_pDocument,
  const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiTextureCubeAssetDocument(szPath);
}

void xiiTextureCubeAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

xiiUInt64 xiiTextureCubeAssetDocumentManager::ComputeAssetProfileHashImpl(const xiiPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}
