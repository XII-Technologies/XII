#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetManager.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetWindow.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEffectAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiParticleEffectAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiParticleEffectAssetDocumentManager::xiiParticleEffectAssetDocumentManager()
{
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiParticleEffectAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Particle Effect";
  m_DocTypeDesc.m_sFileExtension    = "xiiParticleEffectAsset";
  m_DocTypeDesc.m_sIcon             = ":/AssetIcons/Particle_Effect.png";
  m_DocTypeDesc.m_pDocumentType     = xiiGetStaticRTTI<xiiParticleEffectAssetDocument>();
  m_DocTypeDesc.m_pManager          = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Particle_Effect");

  m_DocTypeDesc.m_sResourceFileExtension = "xiiParticleEffect";
  m_DocTypeDesc.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave | xiiAssetDocumentFlags::SupportsThumbnail;
}

xiiParticleEffectAssetDocumentManager::~xiiParticleEffectAssetDocumentManager()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiParticleEffectAssetDocumentManager::OnDocumentManagerEvent, this));
}

void xiiParticleEffectAssetDocumentManager::OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiParticleEffectAssetDocument>())
      {
        xiiQtParticleEffectAssetDocumentWindow* pDocWnd =
          new xiiQtParticleEffectAssetDocumentWindow(static_cast<xiiParticleEffectAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void xiiParticleEffectAssetDocumentManager::InternalCreateDocument(
  const char*              szDocumentTypeName,
  const char*              szPath,
  bool                     bCreateNewDocument,
  xiiDocument*&            out_pDocument,
  const xiiDocumentObject* pOpenContext)
{
  out_pDocument = new xiiParticleEffectAssetDocument(szPath);
}

void xiiParticleEffectAssetDocumentManager::InternalGetSupportedDocumentTypes(
  xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
