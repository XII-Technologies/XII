#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineAsset.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineAssetManager.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineAssetManager, 1, xiiRTTIDefaultAllocator<xiiStateMachineAssetManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiStateMachineAssetManager::xiiStateMachineAssetManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiStateMachineAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "StateMachine";
  m_DocTypeDesc.m_sFileExtension    = "xiiStateMachineAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/StateMachine.svg";
  m_DocTypeDesc.m_sAssetCategory    = "Logic";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiStateMachineAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_StateMachine");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiBinStateMachine";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave;

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("StateMachine", xiiSvgThumbnailToPixmap(":/AssetIcons/StateMachine.svg"));
}

xiiStateMachineAssetManager::~xiiStateMachineAssetManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiStateMachineAssetManager::OnDocumentManagerEvent, this));
}

void xiiStateMachineAssetManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiStateMachineAssetDocument>())
      {
        new xiiQtStateMachineAssetDocumentWindow(e.m_pDocument); // Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiStateMachineAssetManager::InternalCreateDocument(xiiStringView sDocumentTypeName, xiiStringView sPath, bool bCreateNewDocument, xiiDocument*& out_pDocument, const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiStateMachineAssetDocument(sPath);
}

void xiiStateMachineAssetManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
