#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/RenderTargetActivatorComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiRenderTargetActivatorComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("RenderTarget", GetRenderTargetFile, SetRenderTargetFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_Target")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiRenderTargetActivatorComponent::xiiRenderTargetActivatorComponent()  = default;
xiiRenderTargetActivatorComponent::~xiiRenderTargetActivatorComponent() = default;

void xiiRenderTargetActivatorComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  xiiStreamWriter& s = stream.GetStream();

  s << m_hRenderTarget;
}

void xiiRenderTargetActivatorComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = stream.GetStream();

  s >> m_hRenderTarget;
}

xiiResult xiiRenderTargetActivatorComponent::GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg)
{
  if (m_hRenderTarget.IsValid())
  {
    bounds = xiiBoundingSphere(xiiVec3::ZeroVector(), 0.1f);
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiRenderTargetActivatorComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  // only add render target views from main views
  // otherwise every shadow casting light source would activate a render target
  if (msg.m_pView->GetCameraUsageHint() != xiiCameraUsageHint::MainView && msg.m_pView->GetCameraUsageHint() != xiiCameraUsageHint::EditorView)
    return;

  if (!m_hRenderTarget.IsValid())
    return;

  xiiResourceLock<xiiRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, xiiResourceAcquireMode::BlockTillLoaded);

  for (auto hView : pRenderTarget->GetAllRenderViews())
  {
    xiiRenderWorld::AddViewToRender(hView);
  }
}

void xiiRenderTargetActivatorComponent::SetRenderTarget(const xiiRenderToTexture2DResourceHandle& hResource)
{
  m_hRenderTarget = hResource;

  TriggerLocalBoundsUpdate();
}

void xiiRenderTargetActivatorComponent::SetRenderTargetFile(const char* szFile)
{
  xiiRenderToTexture2DResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiRenderToTexture2DResource>(szFile);
  }

  SetRenderTarget(hResource);
}

const char* xiiRenderTargetActivatorComponent::GetRenderTargetFile() const
{
  if (!m_hRenderTarget.IsValid())
    return "";

  return m_hRenderTarget.GetResourceID();
}



XII_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RenderTargetActivatorComponent);
