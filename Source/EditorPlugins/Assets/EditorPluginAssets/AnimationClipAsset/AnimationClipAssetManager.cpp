#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetManager.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationClipAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiAnimationClipAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiAnimationClipAssetDocumentManager::xiiAnimationClipAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiAnimationClipAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Animation Clip";
  m_DocTypeDesc.m_sFileExtension    = "xiiAnimationClipAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/Animation_Clip.svg";
  m_DocTypeDesc.m_sAssetCategory    = "Animation";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiAnimationClipAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Keyframe_Animation");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiAnimationClip";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::SupportsThumbnail;

  // xiiQtImageCache::GetSingleton()->RegisterTypeImage("Animation Clip", QPixmap(":/AssetIcons/Animation_Clip.svg"));
}

xiiAnimationClipAssetDocumentManager::~xiiAnimationClipAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiAnimationClipAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiAnimationClipAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiAnimationClipAssetDocument>())
      {
        new xiiQtAnimationClipAssetDocumentWindow(static_cast<xiiAnimationClipAssetDocument*>(e.m_pDocument)); // NOLINT
      }
    }
    break;

    default:
      break;
  }
}

void xiiAnimationClipAssetDocumentManager::InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiAnimationClipAssetDocument(sPath);
}

void xiiAnimationClipAssetDocumentManager::InternalGetSupportedDocumentTypes(
  xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
