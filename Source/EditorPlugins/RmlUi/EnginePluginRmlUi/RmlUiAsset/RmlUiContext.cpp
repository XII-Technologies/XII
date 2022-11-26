#include <EnginePluginRmlUi/EnginePluginRmlUiPCH.h>

#include <EnginePluginRmlUi/RmlUiAsset/RmlUiContext.h>
#include <EnginePluginRmlUi/RmlUiAsset/RmlUiView.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRmlUiDocumentContext, 1, xiiRTTIDefaultAllocator<xiiRmlUiDocumentContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "RmlUi"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiRmlUiDocumentContext::xiiRmlUiDocumentContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
}

xiiRmlUiDocumentContext::~xiiRmlUiDocumentContext() = default;

void xiiRmlUiDocumentContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  XII_LOCK(pWorld->GetWriteMarker());

  // Preview object
  {
    xiiGameObjectDesc obj;
    obj.m_sName.Assign("RmlUiPreview");
    obj.m_bDynamic = true;
    pWorld->CreateObject(obj, m_pMainObject);

    xiiRmlUiCanvas2DComponent* pComponent = nullptr;
    xiiRmlUiCanvas2DComponent::CreateComponent(m_pMainObject, pComponent);

    xiiStringBuilder sResourceGuid;
    xiiConversionUtils::ToString(GetDocumentGuid(), sResourceGuid);
    m_hMainResource = xiiResourceManager::LoadResource<xiiRmlUiResource>(sResourceGuid);

    pComponent->SetRmlResource(m_hMainResource);
  }
}

xiiEngineProcessViewContext* xiiRmlUiDocumentContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiRmlUiViewContext, this);
}

void xiiRmlUiDocumentContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}

bool xiiRmlUiDocumentContext::UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext)
{
  XII_LOCK(m_pMainObject->GetWorld()->GetWriteMarker());

  m_pMainObject->UpdateLocalBounds();
  xiiBoundingBoxSphere bounds = m_pMainObject->GetGlobalBounds();

  xiiRmlUiViewContext* pMeshViewContext = static_cast<xiiRmlUiViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}
