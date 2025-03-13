#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/LUTAsset/LUTAsset.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetManager.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLUTAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiLUTAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiLUTAssetDocumentManager::xiiLUTAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiLUTAssetDocumentManager::OnDocumentManagerEvent, this));

  // LUT asset source files
  xiiAssetFileExtensionWhitelist::AddAssetFileExtension("LUT", "cube");

  m_DocTypeDesc.m_sDocumentTypeName      = "LUT";
  m_DocTypeDesc.m_sFileExtension         = "xiiLUTAsset";
  m_DocTypeDesc.m_sIcon                  = ":/AssetIcons/LUT.svg";
  m_DocTypeDesc.m_sAssetCategory         = "Rendering";
  m_DocTypeDesc.m_pDocumentType          = xiiGetStaticRTTI<xiiLUTAssetDocument>();
  m_DocTypeDesc.m_pManager               = this;
  m_DocTypeDesc.m_sResourceFileExtension = "xiiBinLUT";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::None;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_3D");

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("LUT", QPixmap(":/AssetIcons/LUT.svg"));

  // xiiQtImageCache::GetSingleton()->RegisterTypeImage("LUT", QPixmap(":/AssetIcons/Render_Target.svg"));
}

xiiLUTAssetDocumentManager::~xiiLUTAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiLUTAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiLUTAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiLUTAssetDocument>())
      {
        new xiiQtLUTAssetDocumentWindow(static_cast<xiiLUTAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiLUTAssetDocumentManager::InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  xiiLUTAssetDocument* pDoc = new xiiLUTAssetDocument(sPath);
  out_pDocument             = pDoc;
}

void xiiLUTAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
