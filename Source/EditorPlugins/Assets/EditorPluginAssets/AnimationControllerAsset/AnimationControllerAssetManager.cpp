#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerAsset.h>
#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerAssetManager.h>
#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationControllerAssetManager, 1, xiiRTTIDefaultAllocator<xiiAnimationControllerAssetManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiAnimationControllerAssetManager::xiiAnimationControllerAssetManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiAnimationControllerAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Animation Controller";
  m_DocTypeDesc.m_sFileExtension    = "xiiAnimationControllerAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/AnimationController.png";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiAnimationControllerAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Keyframe_Graph");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiAnimationControllerBin";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave;

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("Animation Controller", QPixmap(":/AssetIcons/AnimationController.png"));
}

xiiAnimationControllerAssetManager::~xiiAnimationControllerAssetManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiAnimationControllerAssetManager::OnDocumentManagerEvent, this));
}

void xiiAnimationControllerAssetManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiAnimationControllerAssetDocument>())
      {
        xiiQtAnimationControllerAssetDocumentWindow* pDocWnd = new xiiQtAnimationControllerAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;

    default:
      break;
  }
}

void xiiAnimationControllerAssetManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiAnimationControllerAssetDocument(szPath);
}

void xiiAnimationControllerAssetManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
