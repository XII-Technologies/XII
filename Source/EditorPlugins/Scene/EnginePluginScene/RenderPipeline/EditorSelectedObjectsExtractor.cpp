#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <EnginePluginScene/RenderPipeline/EditorSelectedObjectsExtractor.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/Configuration/RendererProfileConfigs.h>
#include <GraphicsCore/Components/CameraComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsCore/Textures/RenderToTexture2DResource.h>
#include <GraphicsFoundation/Device/SwapChain.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorSelectedObjectsExtractor, 1, xiiRTTIDefaultAllocator<xiiEditorSelectedObjectsExtractor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiEditorSelectedObjectsExtractor::xiiEditorSelectedObjectsExtractor()
{
  m_pSceneContext = nullptr;
}

xiiEditorSelectedObjectsExtractor::~xiiEditorSelectedObjectsExtractor()
{
  xiiRenderWorld::DeleteView(m_hRenderTargetView);
}

const xiiDeque<xiiGameObjectHandle>* xiiEditorSelectedObjectsExtractor::GetSelection()
{
  if (m_pSceneContext == nullptr)
    return nullptr;

  return &m_pSceneContext->GetSelectionWithChildren();
}

void xiiEditorSelectedObjectsExtractor::Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData)
{
  const bool bShowCameraOverlays = view.GetCameraUsageHint() == xiiCameraUsageHint::EditorView;

  if (bShowCameraOverlays && m_pSceneContext && m_pSceneContext->GetRenderSelectionBoxes())
  {
    const xiiDeque<xiiGameObjectHandle>* pSelection = GetSelection();
    if (pSelection == nullptr)
      return;

    const xiiCameraComponent* pCameraComponent = nullptr;

    CreateRenderTargetTexture(view);

    XII_LOCK(view.GetWorld()->GetReadMarker());

    for (const auto& hObj : *pSelection)
    {
      const xiiGameObject* pObject = nullptr;
      if (!view.GetWorld()->TryGetObject(hObj, pObject))
        continue;

      if (FilterByViewTags(view, pObject))
        continue;

      if (pObject->TryGetComponentOfBaseType(pCameraComponent))
      {
        UpdateRenderTargetCamera(pCameraComponent);

        const float fAspect = 9.0f / 16.0f;

        // TODO: use aspect ratio of camera render target, if available
        xiiDebugRenderer::Draw2DRectangle(view.GetHandle(), xiiRectFloat(20, 20, 256, 256 * fAspect), 0, xiiColor::White, m_hRenderTarget);

        // TODO: if the camera renders to a texture anyway, use its view + render target instead

        xiiRenderWorld::AddViewToRender(m_hRenderTargetView);

        break;
      }
    }
  }

  xiiSelectedObjectsExtractorBase::Extract(view, visibleObjects, ref_extractedRenderData);
}

xiiResult xiiEditorSelectedObjectsExtractor::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return XII_SUCCESS;
}

xiiResult xiiEditorSelectedObjectsExtractor::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  return XII_SUCCESS;
}

void xiiEditorSelectedObjectsExtractor::CreateRenderTargetTexture(const xiiView& view)
{
  if (m_hRenderTarget.IsValid())
    return;

  m_hRenderTarget = xiiResourceManager::GetExistingResource<xiiRenderToTexture2DResource>("EditorCameraRT");

  if (!m_hRenderTarget.IsValid())
  {
    const float     fAspect = 9.0f / 16.0f;
    const xiiUInt32 uiWidth = 256;

    xiiRenderToTexture2DResourceDescriptor d;
    d.m_Format   = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    d.m_uiWidth  = uiWidth;
    d.m_uiHeight = (xiiUInt32)(uiWidth * fAspect);

    m_hRenderTarget = xiiResourceManager::GetOrCreateResource<xiiRenderToTexture2DResource>("EditorCameraRT", std::move(d));
  }

  CreateRenderTargetView(view);
}

void xiiEditorSelectedObjectsExtractor::CreateRenderTargetView(const xiiView& view)
{
  XII_ASSERT_DEV(m_hRenderTargetView.IsInvalidated(), "Render target view is already created");

  xiiResourceLock<xiiRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, xiiResourceAcquireMode::BlockTillLoaded);

  xiiStringBuilder sName("EditorCameraRT");

  xiiView* pRenderTargetView = nullptr;
  m_hRenderTargetView        = xiiRenderWorld::CreateView(sName, pRenderTargetView);

  const auto* pConfig         = xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<xiiRenderPipelineProfileConfig>();
  auto        hRenderPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(pConfig->m_sEditorRenderPipeline);
  pRenderTargetView->SetRenderPipelineResource(hRenderPipeline);

  // TODO: get rid of const cast ?
  pRenderTargetView->SetWorld(const_cast<xiiWorld*>(view.GetWorld()));
  pRenderTargetView->SetCamera(&m_RenderTargetCamera);

  m_RenderTargetCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 45, 0.1f, 100.0f);

  xiiRenderTargets renderTargets;
  renderTargets.m_pRTs[0] = pRenderTarget->GetGALTexture()->GetDefaultView(xiiGALTextureViewType::RenderTarget);
  pRenderTargetView->SetRenderTargets(renderTargets);

  const float resX = (float)pRenderTarget->GetWidth();
  const float resY = (float)pRenderTarget->GetHeight();

  pRenderTargetView->SetViewport(xiiRectFloat(0, 0, resX, resY));
}

void xiiEditorSelectedObjectsExtractor::UpdateRenderTargetCamera(const xiiCameraComponent* pCameraComponent)
{
  float fFarPlane = xiiMath::Max(pCameraComponent->GetNearPlane() + 0.00001f, pCameraComponent->GetFarPlane());
  switch (pCameraComponent->GetCameraMode())
  {
    case xiiCameraMode::OrthoFixedHeight:
    case xiiCameraMode::OrthoFixedWidth:
      m_RenderTargetCamera.SetCameraMode(pCameraComponent->GetCameraMode(), pCameraComponent->GetOrthoDimension(), pCameraComponent->GetNearPlane(), fFarPlane);
      break;
    case xiiCameraMode::PerspectiveFixedFovX:
    case xiiCameraMode::PerspectiveFixedFovY:
      m_RenderTargetCamera.SetCameraMode(pCameraComponent->GetCameraMode(), pCameraComponent->GetFieldOfView(), pCameraComponent->GetNearPlane(), fFarPlane);
      break;
    case xiiCameraMode::Stereo:
      m_RenderTargetCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 45, pCameraComponent->GetNearPlane(), fFarPlane);
      break;
    default:
      break;
  }

  xiiView* pRenderTargetView = nullptr;
  if (!xiiRenderWorld::TryGetView(m_hRenderTargetView, pRenderTargetView))
    return;

  pRenderTargetView->m_IncludeTags = pCameraComponent->m_IncludeTags;
  pRenderTargetView->m_ExcludeTags = pCameraComponent->m_ExcludeTags;
  pRenderTargetView->m_ExcludeTags.SetByName("Editor");

  if (pCameraComponent->GetRenderPipeline().IsValid())
  {
    pRenderTargetView->SetRenderPipelineResource(pCameraComponent->GetRenderPipeline());
  }
  else
  {
    const auto* pConfig         = xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<xiiRenderPipelineProfileConfig>();
    auto        hRenderPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(pConfig->m_sEditorRenderPipeline);
    pRenderTargetView->SetRenderPipelineResource(hRenderPipeline);
  }

  const xiiVec3 pos = pCameraComponent->GetOwner()->GetGlobalPosition();
  const xiiVec3 dir = pCameraComponent->GetOwner()->GetGlobalDirForwards();
  const xiiVec3 up  = pCameraComponent->GetOwner()->GetGlobalDirUp();

  m_RenderTargetCamera.LookAt(pos, pos + dir, up);
  m_RenderTargetCamera.SetExposure(pCameraComponent->GetExposure());
}
