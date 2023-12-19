#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAsset.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAssetManager.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationGraphAssetManager, 1, xiiRTTIDefaultAllocator<xiiAnimationGraphAssetManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiAnimationGraphAssetManager::xiiAnimationGraphAssetManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiAnimationGraphAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Animation Graph";
  m_DocTypeDesc.m_sFileExtension    = "xiiAnimationGraphAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/AnimationGraph.svg";
  m_DocTypeDesc.m_sAssetCategory    = "Animation";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiAnimationGraphAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Keyframe_Graph");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiAnimGraphBin";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave;

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("Animation Graph", QPixmap(":/AssetIcons/AnimationGraph.svg"));
}

xiiAnimationGraphAssetManager::~xiiAnimationGraphAssetManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiAnimationGraphAssetManager::OnDocumentManagerEvent, this));
}

void xiiAnimationGraphAssetManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiAnimationGraphAssetDocument>())
      {
        new xiiQtAnimationGraphAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiAnimationGraphAssetManager::InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiAnimationGraphAssetDocument(sPath);
}

void xiiAnimationGraphAssetManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
