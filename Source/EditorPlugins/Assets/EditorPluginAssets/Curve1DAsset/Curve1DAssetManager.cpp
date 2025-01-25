#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetManager.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCurve1DAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiCurve1DAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiCurve1DAssetDocumentManager::xiiCurve1DAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiCurve1DAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Curve1D";
  m_DocTypeDesc.m_sFileExtension    = "xiiCurve1DAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/Curve1D.svg";
  m_DocTypeDesc.m_sAssetCategory    = "Utilities";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiCurve1DAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Data_Curve");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiBinCurve1D";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave | xiiAssetDocumentFlags::SupportsThumbnail;
}

xiiCurve1DAssetDocumentManager::~xiiCurve1DAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiCurve1DAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiCurve1DAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiCurve1DAssetDocument>())
      {
        new xiiQtCurve1DAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiCurve1DAssetDocumentManager::InternalCreateDocument(
  xiiStringView            sDocumentTypeName,
  xiiStringView            sPath,
  bool                     bCreateNewDocument,
  xiiDocument*&            out_pDocument,
  const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiCurve1DAssetDocument(sPath);
}

void xiiCurve1DAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
