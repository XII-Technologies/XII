#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/RenderTargetActivatorComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiRenderTargetActivatorComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("RenderTarget", GetRenderTarget, SetRenderTarget)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_Target", xiiDependencyFlags::Package)),
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

void xiiRenderTargetActivatorComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_hRenderTarget;
}

void xiiRenderTargetActivatorComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_hRenderTarget;
}

xiiResult xiiRenderTargetActivatorComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  if (m_hRenderTarget.IsValid())
  {
    ref_bounds = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), 0.1f);
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiRenderTargetActivatorComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  // only add render target views from main views otherwise every shadow casting light source would activate a render target
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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_RenderTargetActivatorComponent);
