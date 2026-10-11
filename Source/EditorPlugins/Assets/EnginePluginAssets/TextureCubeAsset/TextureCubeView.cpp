/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureCubeAsset/TextureCubeContext.h>
#include <EnginePluginAssets/TextureCubeAsset/TextureCubeView.h>

#include <Core/World/World.h>
#include <EditorEngineProcessFramework/Rendering/TexturePreviewPass.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

xiiTextureCubeViewContext::xiiTextureCubeViewContext(xiiTextureCubeContext* pContext) :
  xiiEngineProcessViewContext(pContext)
{
  m_pTextureContext = pContext;
}

xiiTextureCubeViewContext::~xiiTextureCubeViewContext() = default;

xiiViewHandle xiiTextureCubeViewContext::CreateView()
{
  xiiView* pView = nullptr;
  GetDocumentContext()->GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("TextureCube Editor - View", pView);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
  pView->SetRenderPipelineResource(xiiTexturePreviewPass::GetPipeline(true));
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void xiiTextureCubeViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  XII_LOCK(GetDocumentContext()->GetWorld()->GetWriteMarker());
  // Inspection uses fixed clipping planes and its own pipeline for every viewport render mode.
  xiiCameraMode::Enum cameraMode = pMsg->m_CameraMode;
  m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, 0.0001f, 50.0f);
  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);

  if (xiiView* view = GetView())
  {
    view->SetPipelineParameter("Texture", m_pTextureContext->GetTexture().GetResourceID());
    view->SetPipelineParameter("Channel", m_pTextureContext->GetChannelMode());
    view->SetPipelineParameter("Mip", m_pTextureContext->GetLodLevel());
    view->SetPipelineParameter("AlphaThreshold", m_pTextureContext->GetAlphaThreshold());
  }

  // Draw some stats
  auto hResource = m_pTextureContext->GetTexture();
  if (hResource.IsValid())
  {
    xiiResourceLock<xiiTextureCubeResource> pResource(hResource, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (!pResource || pResource->GetLoadingState() != xiiResourceState::Loaded)
      return;
    xiiGALResourceFormat::Enum format           = pResource->GetFormat();
    xiiUInt32                  uiWidthAndHeight = pResource->GetWidthAndHeight();

    xiiStringBuilder sText;
    if (!xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALResourceFormat>(), format, sText, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
    {
      sText = "Unknown format";
    }

    sText.PrependFormat("{0}x{1}x6 - ", uiWidthAndHeight, uiWidthAndHeight);

    if (const xiiView* pView = GetView())
      xiiDebugRenderer::DrawInfoText(*pView, xiiDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
