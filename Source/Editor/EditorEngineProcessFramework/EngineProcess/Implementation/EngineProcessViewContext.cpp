/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <GraphicsCore/Lighting/PostProcessRenderSettings.h>


#include <Core/GameApplication/GameApplicationBase.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <EditorEngineProcessFramework/Rendering/InfiniteGridPass.h>
#include <EditorEngineProcessFramework/Rendering/TexturePreviewPass.h>
#include <EditorEngineProcessFramework/Rendering/ViewportNavigationGizmo.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <Texture/Image/Image.h>

namespace
{
  xiiCVarInt cvar_ViewportFrameSlots("Rendering.Editor.ViewportFrameSlots", 3, xiiCVarFlags::Default,
                                     "Number of persistent offscreen targets per new editor view (2 or 3).");
}

xiiEngineProcessViewContext::xiiEngineProcessViewContext(xiiEngineProcessDocumentContext* pContext) :
  m_pDocumentContext(pContext), m_uiViewID(0xFFFFFFFFU)
{
  if (auto* pApplication = xiiGameApplicationBase::GetGameApplicationBaseInstance())
  {
    m_pPipelineProfile = &pApplication->GetPlatformProfile();
  }
}

xiiResult xiiEngineProcessViewContext::ResolveRenderPipeline(xiiEditorRenderPipelineUsage usage, xiiRenderPipelineResourceHandle& out_pipeline)
{
  m_PipelineUsage           = usage;
  m_bPipelinePolicySelected = true;
  if (m_sPipelineReference.IsEmpty())
  {
    switch (usage)
    {
      case xiiEditorRenderPipelineUsage::EditorScene:
      case xiiEditorRenderPipelineUsage::ScenePreview:
        out_pipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(xiiRenderPipelineProfileConfig::s_szEditorPipeline);
        return XII_SUCCESS;
      case xiiEditorRenderPipelineUsage::TexturePreview:
      case xiiEditorRenderPipelineUsage::CubeTexturePreview:
        out_pipeline = xiiTexturePreviewPass::GetPipelineResource(usage == xiiEditorRenderPipelineUsage::CubeTexturePreview);
        return XII_SUCCESS;
      case xiiEditorRenderPipelineUsage::Main:
        break;
    }
  }
  xiiStringBuilder error;
  const auto       result = xiiRenderPipelineProfileConfig::ResolveResource(m_sPipelineReference, m_pPipelineProfile, out_pipeline, &error);
  if (result.Failed())
    xiiLog::Error("Cannot select the editor view pipeline: {}", error);
  return result;
}

void xiiEngineProcessViewContext::UpdateRenderPipeline(xiiView& view)
{
  if (!m_bPipelinePolicySelected)
    return;
  xiiRenderPipelineResourceHandle pipeline;
  ResolveRenderPipeline(m_PipelineUsage, pipeline).IgnoreResult();
  view.SetRenderPipelineResource(pipeline);
}

xiiEngineProcessViewContext::~xiiEngineProcessViewContext()
{
  if (xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext())
  {
    if (xiiWorld* pWorld = pDocumentContext->GetWorld())
    {
      XII_LOCK(pWorld->GetWriteMarker());

      if (xiiRenderWorldModule* pRenderWorldModule = pWorld->GetModule<xiiRenderWorldModule>())
      {
        pRenderWorldModule->DestroyView(m_hView);

        m_hView.Invalidate();
      }
    }
  }
}

void xiiEngineProcessViewContext::SetViewID(xiiUInt32 uiId)
{
  XII_ASSERT_DEBUG(m_uiViewID == 0xFFFFFFFF, "View ID may only be set once.");

  m_uiViewID = uiId;
}

void xiiEngineProcessViewContext::HandleViewMessage(const xiiEditorEngineViewMsg* pMsg)
{
  if (const xiiViewRedrawMsgToEngine* pViewRedrawMsg = xiiDynamicCast<const xiiViewRedrawMsgToEngine*>(pMsg))
  {
    // Camera and input state continue to update while presentation applies backpressure.
    const bool bReady             = UpdateRenderTarget(pViewRedrawMsg->m_uiWindowWidth, pViewRedrawMsg->m_uiWindowHeight, pViewRedrawMsg->m_bUseSharedTexture).Succeeded();
    m_bShowNavigationGizmo        = pViewRedrawMsg->m_bShowNavigationGizmo;
    m_bNavigationHovered          = pViewRedrawMsg->m_bNavigationHovered;
    m_iNavigationHoveredAxis      = pViewRedrawMsg->m_iNavigationHoveredAxis;
    m_fNavigationDevicePixelRatio = xiiMath::IsFinite(pViewRedrawMsg->m_fDevicePixelRatio) ? xiiMath::Max(0.5f, pViewRedrawMsg->m_fDevicePixelRatio) : 1.0f;
    if (auto* world = GetDocumentContext()->GetWorld())
    {
      XII_LOCK(world->GetWriteMarker());
      if (auto* view = GetView())
      {
        view->SetShowFrameStats(pViewRedrawMsg->m_bShowFrameStats);
        view->SetRenderConfiguration(pViewRedrawMsg->m_RenderConfiguration).IgnoreResult();
      }
    }
    // Preview cameras may add render overrides for their own view.
    SetCamera(pViewRedrawMsg);
    if (bReady)
      Redraw(true);
    else
      SuspendRendering();
  }
  else if (const xiiViewScreenshotMsgToEngine* pViewScreenshotMsg = xiiDynamicCast<const xiiViewScreenshotMsgToEngine*>(pMsg))
  {
    m_sScreenshot = pViewScreenshotMsg->m_sOutputFile;
  }
  else if (const auto* consumed = xiiDynamicCast<const xiiViewFrameConsumedMsgToEngine*>(pMsg))
  {
    ReturnFrameSlot(*consumed);
  }
}

void xiiEngineProcessViewContext::ReturnFrameSlot(const xiiViewFrameConsumedMsgToEngine& message)
{
  XII_PROFILE_SCOPE("Viewport: Return frame slot");

  if (message.m_uiSlotIndex >= m_FrameSlots.GetCount())
  {
    return;
  }
  auto& slot = m_FrameSlots[message.m_uiSlotIndex];
  if (slot.m_State != FrameSlotState::Published || slot.m_uiFrameID != message.m_uiFrameID)
  {
    return;
  }

  slot.m_State = FrameSlotState::Returning;
  if (!slot.m_pTexture->GetExternalMemoryKind().IsSet(xiiGALExternalMemoryKind::Exportable))
  {
    return;
  }

  auto pDevice = xiiGALDevice::GetDefaultDevice();
  if (message.m_bSharedTextureAccepted)
  {
    auto pCommand = pDevice->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    if (pCommand)
    {
      pCommand->Begin();
      const auto result = slot.m_pTexture->AcquireExternalMemory(pCommand);
      pCommand->End();
      if (result.Succeeded())
      {
        slot.m_uiFenceValue = pDevice->GetCommandQueue()->Submit(pCommand);
        return;
      }
    }
  }

  // This allocation will never be written again. An imported allocation has its
  // own native reference, and GAL defers the producer's native destruction.
  slot.m_pTarget.Clear();
  slot.m_pTexture.Clear();
  m_bSharingFailed = true;
  xiiLog::Warning("Could not return shared viewport ownership. Using offscreen readback presentation.");
}

void xiiEngineProcessViewContext::SendViewMessage(xiiEditorEngineViewMsg* pViewMsg)
{
  pViewMsg->m_DocumentGuid = GetDocumentContext()->GetDocumentGuid();
  pViewMsg->m_uiViewID     = m_uiViewID;

  GetDocumentContext()->SendProcessMessage(pViewMsg);
}

xiiResult xiiEngineProcessViewContext::UpdateRenderTarget(xiiUInt16 uiWidth, xiiUInt16 uiHeight, bool bShared)
{
  XII_PROFILE_SCOPE("Viewport: Select frame slot");

  if (m_uiRenderSlot < m_FrameSlots.GetCount())
  {
    m_FrameSlots[m_uiRenderSlot].m_State = FrameSlotState::Available;
    m_uiRenderSlot                       = XII_MAX_VIEWPORT_FRAME_SLOTS;
  }
  if (uiWidth == 0 || uiHeight == 0)
  {
    return XII_FAILURE;
  }
  if (m_FrameSlots.IsEmpty())
  {
    m_FrameSlots.SetCount(static_cast<xiiUInt32>(xiiMath::Clamp(cvar_ViewportFrameSlots.GetValue(), 2, static_cast<xiiInt32>(XII_MAX_VIEWPORT_FRAME_SLOTS))));
  }

  auto        pDevice  = xiiGALDevice::GetDefaultDevice();
  const auto& features = pDevice->GetFeatures();
  bShared              = bShared && !m_bSharingFailed &&
    features.m_ExternalMemory == xiiGALDeviceFeatureState::Enabled &&
    features.m_ExternalSemaphore == xiiGALDeviceFeatureState::Enabled;
  const xiiUInt64  uiCompleted = pDevice->GetCommandQueue()->GetCompletedFenceValue();
  const xiiSizeU32 size(uiWidth, uiHeight);
  xiiUInt8         uiSelected = XII_MAX_VIEWPORT_FRAME_SLOTS;
  for (xiiUInt8 uiIndex = 0; uiIndex < m_FrameSlots.GetCount(); ++uiIndex)
  {
    auto& slot = m_FrameSlots[uiIndex];
    if (slot.m_State == FrameSlotState::Returning && slot.m_uiFenceValue <= uiCompleted)
    {
      slot.m_State = FrameSlotState::Available;
    }
    if (slot.m_State != FrameSlotState::Available)
    {
      continue;
    }
    if (slot.m_pTexture && slot.m_pTexture->GetDescription().m_Size == size &&
        slot.m_pTexture->GetExternalMemoryKind().IsSet(xiiGALExternalMemoryKind::Exportable) == bShared)
    {
      uiSelected = uiIndex;
      break;
    }
    if (uiSelected == XII_MAX_VIEWPORT_FRAME_SLOTS)
    {
      uiSelected = uiIndex;
    }
  }
  if (uiSelected == XII_MAX_VIEWPORT_FRAME_SLOTS)
  {
    XII_PROFILE_SCOPE("Viewport: All frame slots busy");
    return XII_FAILURE;
  }

  auto& slot = m_FrameSlots[uiSelected];
  if (!slot.m_pTexture || slot.m_pTexture->GetDescription().m_Size != size ||
      slot.m_pTexture->GetExternalMemoryKind().IsSet(xiiGALExternalMemoryKind::Exportable) != bShared)
  {
    XII_SUCCEED_OR_RETURN(CreateFrameSlot(slot, size, bShared));
  }

  m_bViewportSetup = true;
  SetupRenderTarget({}, slot.m_pTarget, uiWidth, uiHeight);
  m_bViewportSetup = false;
  slot.m_State     = FrameSlotState::Rendering;
  m_uiRenderSlot   = uiSelected;
  return XII_SUCCESS;
}

xiiResult xiiEngineProcessViewContext::CreateFrameSlot(FrameSlot& ref_slot, xiiSizeU32 size, bool bShared)
{
  auto                             pDevice = xiiGALDevice::GetDefaultDevice();
  xiiGALTextureCreationDescription description;
  description.m_Type                   = xiiGALResourceDimension::Texture2D;
  description.m_Size                   = size;
  description.m_Format                 = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  description.m_BindFlags              = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  description.m_ExternalMemory.m_Flags = xiiGALExternalMemoryFlags::BinarySemaphore;
  auto pTexture                        = pDevice->CreateTexture(description, nullptr, bShared ? xiiGALExternalMemoryKind::Exportable : xiiGALExternalMemoryKind::None);
  if (!pTexture && bShared)
  {
    m_bSharingFailed = true;
    xiiLog::Warning("Vulkan image export unavailable; using offscreen readback presentation.");
    pTexture = pDevice->CreateTexture(description);
  }
  if (!pTexture)
  {
    return XII_FAILURE;
  }
  auto pTarget = pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget);
  if (!pTarget)
  {
    return XII_FAILURE;
  }
  ref_slot.m_pTarget     = std::move(pTarget);
  ref_slot.m_pTexture    = std::move(pTexture);
  ref_slot.m_uiTextureID = ++m_uiTextureID;
  return XII_SUCCESS;
}

void xiiEngineProcessViewContext::PresentFrame()
{
  XII_PROFILE_SCOPE("Viewport: Publish frame");
  if (m_uiRenderSlot >= m_FrameSlots.GetCount())
  {
    return;
  }

  const xiiUInt8 uiSlotIndex = m_uiRenderSlot;
  m_uiRenderSlot             = XII_MAX_VIEWPORT_FRAME_SLOTS;
  auto& slot                 = m_FrameSlots[uiSlotIndex];
  auto  pDevice              = xiiGALDevice::GetDefaultDevice();
  // The ViewTarget's terminal use is on the graphics queue. Preserve that
  // submission's lifetime even if the graph only partially executed.
  slot.m_State                 = FrameSlotState::Returning;
  slot.m_uiFenceValue          = pDevice->GetCommandQueue()->GetNextFenceValue() - 1;
  const auto              size = slot.m_pTexture->GetDescription().m_Size;
  xiiViewFrameMsgToEditor frame;
  if (const auto* pWorld = GetDocumentContext()->GetWorld())
  {
    XII_LOCK(pWorld->GetReadMarker());
    const auto* pView = GetView();
    if (!pView || pView->GetLastRenderResult().Failed())
    {
      return;
    }
    frame.m_uiRenderWidth                = pView->GetRenderResolutionWidth();
    frame.m_uiRenderHeight               = pView->GetRenderResolutionHeight();
    frame.m_EffectiveRenderConfiguration = pView->GetEffectiveRenderConfiguration();
    frame.m_sRenderConfigurationError    = pView->GetRenderPipelineInstance().GetConfigurationError();
  }
  else
  {
    return;
  }
  if (!m_sScreenshot.IsEmpty())
  {
    xiiImage image;
    if (xiiCaptureThumbnail(slot.m_pTexture, size, image).Failed() || image.SaveTo(m_sScreenshot).Failed())
      xiiLog::Error("Could not capture viewport screenshot '{}'.", m_sScreenshot);
    m_sScreenshot.Clear();
  }
  frame.m_uiFrameID   = ++m_uiFrameID;
  frame.m_uiTextureID = slot.m_uiTextureID;
  frame.m_uiSlotIndex = uiSlotIndex;
  frame.m_uiWidth     = static_cast<xiiUInt16>(size.width);
  frame.m_uiHeight    = static_cast<xiiUInt16>(size.height);
  if (slot.m_pTexture->GetExternalMemoryKind().IsSet(xiiGALExternalMemoryKind::Exportable))
  {
    auto command = pDevice->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    if (!command)
      return;
    command->Begin();
    const auto result = slot.m_pTexture->ReleaseExternalMemory(command);
    command->End();
    if (result.Failed())
      return;
    slot.m_uiFenceValue = pDevice->GetCommandQueue()->Submit(command);
    frame.m_Memory      = slot.m_pTexture->GetExternalMemoryDescription();
  }
  else
  {
    xiiImage image;
    if (xiiCaptureThumbnail(slot.m_pTexture, size, image).Failed())
      return;
    frame.m_Pixels.SetCountUninitialized(size.width * size.height * 4);
    xiiMemoryUtils::Copy(frame.m_Pixels.GetData(), image.GetPixelPointer<xiiUInt8>(), frame.m_Pixels.GetCount());
  }
  slot.m_uiFrameID = frame.m_uiFrameID;
  slot.m_State     = FrameSlotState::Published;
  SendViewMessage(&frame);
}

xiiView* xiiEngineProcessViewContext::GetView() const
{
  auto* world = GetDocumentContext()->GetWorld();
  if (!world)
    return nullptr;
  const auto* module = static_cast<const xiiWorld*>(world)->GetModule<xiiRenderWorldModule>();
  xiiView*    view   = nullptr;
  if (module)
    module->TryGetView(m_hView, view);
  return view;
}

void xiiEngineProcessViewContext::SetupRenderTarget(xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiGALTextureView* pRenderTargetView, xiiUInt16 uiWidth, xiiUInt16 uiHeight)
{
  XII_ASSERT_DEV((pSwapChain != nullptr) != (pRenderTargetView != nullptr), "Supply exactly one swapchain or offscreen target.");
  auto* world = GetDocumentContext()->GetWorld();
  if (!world)
    return;
  XII_LOCK(world->GetWriteMarker());
  if (m_hView.IsInvalidated())
  {
    m_hView = CreateView();
    if (auto* view = GetView())
      view->SetRenderEnabled(false);
  }
  if (auto* view = GetView())
  {
    if (pRenderTargetView != nullptr && !m_bViewportSetup && m_PipelineUsage == xiiEditorRenderPipelineUsage::ScenePreview)
    {
      // A short thumbnail capture cannot adapt exposure reliably against its
      // mostly black background. Use stable studio exposure for asset inspection.
      auto configuration = view->GetRenderConfiguration();
      if (configuration.SetOverride<xiiPostProcessRenderSettings>("Display/Exposure/Mode", xiiInt64(xiiExposureMode::Manual)).Succeeded())
        view->SetRenderConfiguration(configuration).IgnoreResult();
    }
    view->SetSwapChain(pSwapChain.Borrow());
    view->SetRenderTargetView(pRenderTargetView);
    const auto size = pSwapChain ? pSwapChain->GetCurrentSize() : xiiSizeU32(uiWidth, uiHeight);
    view->SetViewport(xiiRectFloat(0.0f, 0.0f, static_cast<float>(size.width), static_cast<float>(size.height)));
  }
}

void xiiEngineProcessViewContext::Redraw(bool bRenderEditorGizmos)
{
  if (xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext())
  {
    if (xiiWorld* pWorld = pDocumentContext->GetWorld())
    {
      XII_LOCK(pWorld->GetWriteMarker());

      if (xiiRenderWorldModule* pRenderWorldModule = pWorld->GetModule<xiiRenderWorldModule>())
      {
        xiiView* pView = nullptr;
        if (!pRenderWorldModule->TryGetView(m_hView, pView))
          return;

        // Remote standalone views still follow their own swapchain extent.
        if (const auto* swapChain = pView->GetSwapChain())
        {
          const auto size = swapChain->GetCurrentSize();
          pView->SetViewport(xiiRectFloat(0.0f, 0.0f, static_cast<float>(size.width), static_cast<float>(size.height)));
        }

        // Thumbnail views have no camera messages. Retry resource-dependent
        // pipeline composition here as well once the scene resource has loaded.
        UpdateRenderPipeline(*pView);
        pView->SetRenderEnabled(true);
        if (m_bShowNavigationGizmo && pView->GetCamera() && pView->GetCamera()->GetCameraMode() != xiiCameraMode::Stereo)
        {
          const auto&                      viewport = pView->GetViewport();
          const xiiViewportNavigationGizmo geometry(*pView->GetCamera(), xiiVec2(viewport.width, viewport.height) / m_fNavigationDevicePixelRatio);
          geometry.Draw(xiiDebugRendererContext(*pView), m_fNavigationDevicePixelRatio, m_iNavigationHoveredAxis, m_bNavigationHovered);
        }
        const xiiTag& editorTag = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

        if (bRenderEditorGizmos)
        {
          pView->m_ExcludeTags.Remove(editorTag);
          if (pDocumentContext->DrawGizmos(*pView).Failed())
            xiiLog::Warning("Could not draw one or more Editor handles for this view.");
        }
        else
        {
          pView->m_ExcludeTags.Set(editorTag);
          pView->RemovePipelineParameter("Editor.Grid");
          pView->RemovePipelineParameter("Editor.Gizmos");
          pView->RemovePipelineParameter("Editor.GizmoMeshes");
        }
      }
    }
  }
}

void xiiEngineProcessViewContext::SuspendRendering()
{
  if (auto* world = GetDocumentContext()->GetWorld())
  {
    XII_LOCK(world->GetWriteMarker());
    if (auto* view = GetView())
      view->SetRenderEnabled(false);
  }
}

xiiResult xiiEngineProcessViewContext::DrawGrid(float fSpacing, const xiiTransform& transform, bool bGlobal)
{
  auto* world = GetDocumentContext()->GetWorld();
  if (!world)
    return XII_FAILURE;
  XII_LOCK(world->GetWriteMarker());
  auto* view = GetView();
  if (!view)
    return XII_FAILURE;
  return xiiInfiniteGridPass::ConfigureView(*view, fSpacing, transform, bGlobal);
}

xiiResult xiiEngineProcessViewContext::PickGizmoAtPixel(xiiUInt32 x, xiiUInt32 y, xiiViewPickingResultMsgToEditor& out_result) const
{
  out_result.m_ObjectGuid    = xiiUuid();
  out_result.m_ComponentGuid = xiiUuid();
  out_result.m_OtherGuid     = xiiUuid();
  out_result.m_uiPartIndex   = 0;
  out_result.m_vPickedNormal.SetZero();
  out_result.m_vPickedPosition.SetZero();
  out_result.m_vPickingRayStartPosition.SetZero();

  auto* world = GetDocumentContext()->GetWorld();
  if (!world)
    return XII_FAILURE;
  XII_LOCK(world->GetWriteMarker());
  const auto* view = GetView();
  if (!view || !view->IsValid())
    return XII_FAILURE;
  const auto& viewport = view->GetViewport();
  if (!xiiMath::IsFinite(viewport.x) || !xiiMath::IsFinite(viewport.y) || !xiiMath::IsFinite(viewport.width) || !xiiMath::IsFinite(viewport.height) || viewport.width <= 0 || viewport.height <= 0)
    return XII_FAILURE;

  xiiEditorGizmoPickResult hit;
  XII_SUCCEED_OR_RETURN(GetDocumentContext()->PickGizmo(*view, (static_cast<float>(x) + 0.5f - viewport.x) / viewport.width, (static_cast<float>(y) + 0.5f - viewport.y) / viewport.height, hit));
  out_result.m_OtherGuid                = hit.m_HandleGuid;
  out_result.m_vPickedPosition          = hit.m_vPosition;
  out_result.m_vPickingRayStartPosition = hit.m_vRayStartPosition;
  return XII_SUCCESS;
}

bool xiiEngineProcessViewContext::FocusCameraOnObject(xiiCamera& inout_camera, const xiiBoundingBoxSphere& objectBounds, float fFov, const xiiVec3& vViewDir)
{
  if (!objectBounds.IsValid())
    return false;

  bool    bChanged        = false;
  xiiVec3 vCameraPosition = inout_camera.GetCenterPosition();
  xiiVec3 vCenterPosition = objectBounds.GetSphere().m_vCenter;

  const float fDistance     = xiiMath::Max(0.1f, objectBounds.GetSphere().m_fRadius) / xiiMath::Sin(xiiAngle::MakeFromDegree(fFov / 2));
  xiiVec3     vNewCameraPos = vCenterPosition - vViewDir.GetNormalized() * fDistance;
  if (!vNewCameraPos.IsEqual(vCameraPosition, 0.01f))
  {
    vCameraPosition = vNewCameraPos;
    bChanged        = true;
  }

  if (bChanged)
  {
    if (!vNewCameraPos.IsValid())
      return false;

    const float radius = xiiMath::Max(0.001f, objectBounds.GetSphere().m_fRadius);
    inout_camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, fFov, xiiMath::Max(0.0001f, radius * 0.001f), xiiMath::Max(1.0f, fDistance + radius * 4.0f));
    inout_camera.LookAt(vNewCameraPos, vCenterPosition, xiiVec3(0.0f, 0.0f, 1.0f));
  }

  return bChanged;
}

void xiiEngineProcessViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  if (xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext())
  {
    if (xiiWorld* pWorld = pDocumentContext->GetWorld())
    {
      XII_LOCK(pWorld->GetWriteMarker());

      if (xiiRenderWorldModule* pRenderWorldModule = pWorld->GetModule<xiiRenderWorldModule>())
      {
        xiiView* pView = nullptr;
        if (!pRenderWorldModule->TryGetView(m_hView, pView))
          return;

        UpdateRenderPipeline(*pView);

        pView->SetViewRenderMode(pMsg->m_RenderMode);
        if (xiiMath::IsFinite(pMsg->m_fDevicePixelRatio) && pMsg->m_fDevicePixelRatio > 0 && pMsg->m_fDevicePixelRatio <= 8)
          pView->SetPipelineParameter("Editor.DevicePixelRatio", pMsg->m_fDevicePixelRatio);

        if (m_Camera.GetCameraMode() != xiiCameraMode::Stereo)
        {
          xiiCameraComponentManager* pCameraManager   = pWorld->GetModule<xiiCameraComponentManager>();
          xiiCameraComponent*        pCameraComponent = pCameraManager ? pCameraManager->GetCameraByUsageHint(pView->GetCameraUsageHint()) : nullptr;

          // Camera mode should be controlled by a matching camera component if one exists.
          if (pCameraComponent == nullptr || !pCameraComponent->IsActive())
          {
            m_Camera.SetCameraMode(pMsg->m_CameraMode, pMsg->m_fFovOrDim, pMsg->m_fNearPlane, pMsg->m_fFarPlane);
          }
          if (pMsg->m_bUseCameraTransformOnDevice)
            m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);
        }
      }
    }
  }
}
