#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetManager.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiColorGradientAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiColorGradientAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiColorGradientAssetDocumentManager::xiiColorGradientAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiColorGradientAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "ColorGradient";
  m_DocTypeDesc.m_sFileExtension    = "xiiColorGradientAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/ColorGradient.svg";
  m_DocTypeDesc.m_sAssetCategory    = "Animation";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiColorGradientAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Data_Gradient");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiColorGradient";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave | xiiAssetDocumentFlags::SupportsThumbnail;
}

xiiColorGradientAssetDocumentManager::~xiiColorGradientAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiColorGradientAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiColorGradientAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiColorGradientAssetDocument>())
      {
        new xiiQtColorGradientAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiColorGradientAssetDocumentManager::InternalCreateDocument(
  xiiStringView            sDocumentTypeName,
  xiiStringView            sPath,
  bool                     bCreateNewDocument,
  xiiDocument*&            out_pDocument,
  const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiColorGradientAssetDocument(sPath);
}

void xiiColorGradientAssetDocumentManager::InternalGetSupportedDocumentTypes(
  xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
