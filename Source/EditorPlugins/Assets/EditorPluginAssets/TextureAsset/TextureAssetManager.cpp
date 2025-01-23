#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureAssetProfileConfig, 1, xiiRTTIDefaultAllocator<xiiTextureAssetProfileConfig>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MaxResolution", m_uiMaxResolution)->AddAttributes(new xiiDefaultValueAttribute(16 * 1024)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiTextureAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTextureAssetDocumentManager::xiiTextureAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiTextureAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  xiiAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Texture_2D", "dds");
  xiiAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Texture_2D", "color");

  // texture asset source files
  xiiAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "dds");
  xiiAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "tga");

  m_DocTypeDesc.m_sDocumentTypeName      = "Texture 2D";
  m_DocTypeDesc.m_sFileExtension         = "xiiTextureAsset";
  m_DocTypeDesc.m_sIcon                  = ":/AssetIcons/Texture_2D.svg";
  m_DocTypeDesc.m_sAssetCategory         = "Rendering";
  m_DocTypeDesc.m_pDocumentType          = xiiGetStaticRTTI<xiiTextureAssetDocument>();
  m_DocTypeDesc.m_pManager               = this;
  m_DocTypeDesc.m_sResourceFileExtension = "xiiBinTexture2D";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoThumbnailOnTransform;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_2D");

  m_DocTypeDesc2.m_sDocumentTypeName      = "Render Target";
  m_DocTypeDesc2.m_sFileExtension         = "xiiRenderTargetAsset";
  m_DocTypeDesc2.m_sIcon                  = ":/AssetIcons/Render_Target.svg";
  m_DocTypeDesc2.m_sAssetCategory         = "Rendering";
  m_DocTypeDesc2.m_pDocumentType          = xiiGetStaticRTTI<xiiTextureAssetDocument>();
  m_DocTypeDesc2.m_pManager               = this;
  m_DocTypeDesc2.m_sResourceFileExtension = "xiiBinRenderTarget";
  m_DocTypeDesc2.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave;
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_2D"); // render targets can also be used as 2D textures
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_Target");

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("Render Target", QPixmap(":/AssetIcons/Render_Target.svg"));
}

xiiTextureAssetDocumentManager::~xiiTextureAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiTextureAssetDocumentManager::OnDocumentManagerEvent, this));
}

xiiUInt64 xiiTextureAssetDocumentManager::ComputeAssetProfileHashImpl(const xiiPlatformProfile* pAssetProfile) const
{
  return pAssetProfile->GetTypeConfig<xiiTextureAssetProfileConfig>()->m_uiMaxResolution;
}

void xiiTextureAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiTextureAssetDocument>())
      {
        new xiiQtTextureAssetDocumentWindow(static_cast<xiiTextureAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiTextureAssetDocumentManager::InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  xiiTextureAssetDocument* pDoc = new xiiTextureAssetDocument(sPath);
  out_pDocument                 = pDoc;

  if (sDocumentTypeName.IsEqual("Render Target"))
  {
    pDoc->m_bIsRenderTarget = true;
  }
}

void xiiTextureAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
  inout_DocumentTypes.PushBack(&m_DocTypeDesc2);
}

xiiString xiiTextureAssetDocumentManager::GetRelativeOutputFileName(const xiiAssetDocumentTypeDescriptor* pTypeDescriptor, xiiStringView sDataDirectory, xiiStringView sDocumentPath, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile) const
{
  if (sOutputTag.IsEqual("LOWRES"))
  {
    xiiStringBuilder sRelativePath(sDocumentPath);
    sRelativePath.MakeRelativeTo(sDataDirectory).IgnoreResult();
    sRelativePath.RemoveFileExtension();
    sRelativePath.Append("-lowres");
    xiiAssetDocumentManager::GenerateOutputFilename(sRelativePath, pAssetProfile, "xiiTexture2D", true);
    return sRelativePath;
  }

  return SUPER::GetRelativeOutputFileName(pTypeDescriptor, sDataDirectory, sDocumentPath, sOutputTag, pAssetProfile);
}
