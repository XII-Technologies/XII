/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureAsset/TextureContext.h>
#include <EnginePluginAssets/TextureAsset/TextureView.h>

#include <Core/World/World.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

xiiTextureViewContext::xiiTextureViewContext(xiiTextureContext* pContext) :
  xiiEngineProcessViewContext(pContext)
{
  m_pTextureContext = pContext;
}

xiiTextureViewContext::~xiiTextureViewContext() = default;

xiiViewHandle xiiTextureViewContext::CreateView()
{
  xiiRenderPipelineResourceHandle pipeline;
  if (ResolveRenderPipeline(xiiRenderPipelineUsage::TexturePreview, pipeline).Failed())
    return {};
  xiiView* pView = nullptr;
  GetDocumentContext()->GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Texture Editor - View", pView);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
  pView->SetRenderPipelineResource(pipeline);
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void xiiTextureViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  XII_LOCK(GetDocumentContext()->GetWorld()->GetWriteMarker());
  // Inspection uses fixed clipping planes and its own pipeline for every viewport render mode.
  xiiCameraMode::Enum cameraMode = pMsg->m_CameraMode;
  m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, 0.0001f, 50.0f);
  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);

  if (xiiView* view = GetView())
  {
    UpdateRenderPipeline(*view);
    view->SetPipelineParameter("Texture", m_pTextureContext->GetTexture().GetResourceID());
    view->SetPipelineParameter("Channel", m_pTextureContext->GetChannelMode());
    view->SetPipelineParameter("Mip", m_pTextureContext->GetLodLevel());
    view->SetPipelineParameter("AlphaThreshold", m_pTextureContext->GetAlphaThreshold());
  }

  // Draw some stats
  auto hResource = m_pTextureContext->GetTexture();
  if (hResource.IsValid())
  {
    xiiResourceLock<xiiTexture2DResource> pResource(hResource, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (!pResource || pResource->GetLoadingState() != xiiResourceState::Loaded)
      return;
    const xiiGALResourceFormat::Enum format    = pResource->GetFormat();
    const int                        iMipLevel = m_pTextureContext->GetLodLevel();
    const xiiUInt32                  uiWidth   = pResource->GetWidth();
    const xiiUInt32                  uiHeight  = pResource->GetHeight();

    xiiStringBuilder sText;
    if (!xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALResourceFormat>(), format, sText, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
    {
      sText = "Unknown format";
    }

    sText.PrependFormat("{}x{} - ", uiWidth, uiHeight);
    sText.Append("\nPreview Mip Level: ");

    if (iMipLevel < 0)
    {
      sText.Append("Auto");
    }
    else
    {
      const xiiUInt32 uiMipWidth  = xiiMath::Max(1u, uiWidth >> xiiMath::Clamp(iMipLevel, 0, 31));
      const xiiUInt32 uiMipHeight = xiiMath::Max(1u, uiHeight >> xiiMath::Clamp(iMipLevel, 0, 31));

      sText.AppendFormat("{} ({}x{})", iMipLevel, uiMipWidth, uiMipHeight);
    }

    if (const xiiView* pView = GetView())
      xiiDebugRenderer::DrawInfoText(*pView, xiiDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
