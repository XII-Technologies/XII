#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureCubeAsset/TextureCubeContext.h>
#include <EnginePluginAssets/TextureCubeAsset/TextureCubeView.h>

#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/RendererReflection.h>

xiiTextureCubeViewContext::xiiTextureCubeViewContext(xiiTextureCubeContext* pContext) :
  xiiEngineProcessViewContext(pContext)
{
  m_pTextureContext = pContext;
}

xiiTextureCubeViewContext::~xiiTextureCubeViewContext() = default;

xiiViewHandle xiiTextureCubeViewContext::CreateView()
{
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("Texture Cube Editor - View", pView);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDebugRenderPipeline());
  pView->SetRenderPassProperty("DepthPrePass", "Active", false);
  pView->SetRenderPassProperty("AOPass", "Active", false);

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void xiiTextureCubeViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  // Do not apply render mode here otherwise we would switch to a different pipeline.
  // Also use hard-coded clipping planes so the quad is not culled to early.

  xiiCameraMode::Enum cameraMode = (xiiCameraMode::Enum)pMsg->m_iCameraMode;
  m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, 0.0001f, 50.0f);
  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);

  // Draw some stats
  auto hResource = m_pTextureContext->GetTexture();
  if (hResource.IsValid())
  {
    xiiResourceLock<xiiTextureCubeResource> pResource(hResource, xiiResourceAcquireMode::AllowLoadingFallback);
    xiiGALTextureFormat::Enum               format           = pResource->GetFormat();
    xiiUInt32                               uiWidthAndHeight = pResource->GetWidthAndHeight();

    xiiStringBuilder sText;
    if (!xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALTextureFormat>(), format, sText, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
    {
      sText = "Unknown format";
    }

    sText.PrependFormat("{0}x{1}x6 - ", uiWidthAndHeight, uiWidthAndHeight);

    xiiDebugRenderer::DrawInfoText(m_hView, xiiDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
