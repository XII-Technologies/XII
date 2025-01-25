#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ImageDataAsset/ImageDataAsset.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetManager.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetWindow.moc.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiImageDataAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiImageDataAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiImageDataAssetDocumentManager::xiiImageDataAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiImageDataAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName      = "Image Data";
  m_DocTypeDesc.m_sFileExtension         = "xiiImageDataAsset";
  m_DocTypeDesc.m_sIcon                  = ":/AssetIcons/ImageData.svg";
  m_DocTypeDesc.m_sAssetCategory         = "Utilities";
  m_DocTypeDesc.m_pDocumentType          = xiiGetStaticRTTI<xiiImageDataAssetDocument>();
  m_DocTypeDesc.m_pManager               = this;
  m_DocTypeDesc.m_sResourceFileExtension = "xiiBinImageData";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoThumbnailOnTransform;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Data_2D");
}

xiiImageDataAssetDocumentManager::~xiiImageDataAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiImageDataAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiImageDataAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiImageDataAssetDocument>())
      {
        new xiiQtImageDataAssetDocumentWindow(static_cast<xiiImageDataAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiImageDataAssetDocumentManager::InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  xiiImageDataAssetDocument* pDoc = new xiiImageDataAssetDocument(sPath);
  out_pDocument                   = pDoc;
}

void xiiImageDataAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
