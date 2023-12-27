#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetManager.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineAssetManager, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineAssetManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRenderPipelineAssetManager::xiiRenderPipelineAssetManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiRenderPipelineAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "RenderPipeline";
  m_DocTypeDesc.m_sFileExtension    = "xiiRenderPipelineAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/RenderPipeline.svg";
  m_DocTypeDesc.m_sAssetCategory    = "Rendering";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiRenderPipelineAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_RenderPipeline");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiRenderPipelineBin";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave;

  xiiQtImageCache::GetSingleton()->RegisterTypeImage("RenderPipeline", QPixmap(":/AssetIcons/RenderPipeline.svg"));
}

xiiRenderPipelineAssetManager::~xiiRenderPipelineAssetManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiRenderPipelineAssetManager::OnDocumentManagerEvent, this));
}

void xiiRenderPipelineAssetManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiRenderPipelineAssetDocument>())
      {
        new xiiQtRenderPipelineAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void xiiRenderPipelineAssetManager::InternalCreateDocument(
  xiiStringView            sDocumentTypeName,
  xiiStringView            sPath,
  bool                     bCreateNewDocument,
  xiiDocument*&            out_pDocument,
  const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiRenderPipelineAssetDocument(sPath);
}

void xiiRenderPipelineAssetManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
