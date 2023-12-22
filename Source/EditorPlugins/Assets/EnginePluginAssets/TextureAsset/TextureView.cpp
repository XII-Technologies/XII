#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureAsset/TextureContext.h>
#include <EnginePluginAssets/TextureAsset/TextureView.h>

#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

xiiTextureViewContext::xiiTextureViewContext(xiiTextureContext* pContext) :
  xiiEngineProcessViewContext(pContext)
{
  m_pTextureContext = pContext;
}

xiiTextureViewContext::~xiiTextureViewContext() = default;

xiiViewHandle xiiTextureViewContext::CreateView()
{
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("Texture Editor - View", pView);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDebugRenderPipeline());
  pView->SetRenderPassProperty("DepthPrePass", "Active", false);
  pView->SetRenderPassProperty("AOPass", "Active", false);

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void xiiTextureViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  // Do not apply render mode here otherwise we would switch to a different pipeline.
  // Also use hard-coded clipping planes so the quad is not culled too early.

  xiiCameraMode::Enum cameraMode = (xiiCameraMode::Enum)pMsg->m_iCameraMode;
  m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, 0.0001f, 50.0f);
  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);

  // Draw some stats
  auto hResource = m_pTextureContext->GetTexture();
  if (hResource.IsValid())
  {
    xiiResourceLock<xiiTexture2DResource> pResource(hResource, xiiResourceAcquireMode::AllowLoadingFallback);
    xiiGALTextureFormat::Enum             format   = pResource->GetFormat();
    xiiUInt32                             uiWidth  = pResource->GetWidth();
    xiiUInt32                             uiHeight = pResource->GetHeight();

    xiiStringBuilder sText;
    if (!xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALTextureFormat>(), format, sText, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
    {
      sText = "Unknown format";
    }

    sText.PrependFormat("{0}x{1} - ", uiWidth, uiHeight);

    xiiDebugRenderer::DrawInfoText(m_hView, xiiDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
