/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <Core/Interfaces/SoundInterface.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EnginePluginScene/SceneView/SceneView.h>
#include <GraphicsCore/Components/CameraComponent.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

xiiSceneViewContext::xiiSceneViewContext(xiiSceneContext* pSceneContext) :
  xiiEngineProcessViewContext(pSceneContext)
{
  m_pSceneContext      = pSceneContext;
  m_bUpdatePickingData = true;

  // Start with something valid.
  m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(xiiVec3(1, 1, 1), xiiVec3::MakeZero(), xiiVec3(0.0f, 0.0f, 1.0f));

  m_CullingCamera = m_Camera;
}

xiiSceneViewContext::~xiiSceneViewContext() = default;

void xiiSceneViewContext::HandleViewMessage(const xiiEditorEngineViewMsg* pMsg)
{
  xiiEngineProcessViewContext::HandleViewMessage(pMsg);

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiViewRedrawMsgToEngine>())
  {
    const xiiViewRedrawMsgToEngine* pMsg2 = static_cast<const xiiViewRedrawMsgToEngine*>(pMsg);

    xiiView* pView = nullptr;
    if (xiiRenderWorld::TryGetView(m_hView, pView))
    {
      pView->SetRenderPassProperty("EditorPickingPass", "Active", pMsg2->m_bUpdatePickingData);
      pView->SetRenderPassProperty("EditorPickingPass", "PickSelected", pMsg2->m_bEnablePickingSelected);
      pView->SetRenderPassProperty("EditorPickingPass", "PickTransparent", pMsg2->m_bEnablePickTransparent);
    }

    if (pMsg2->m_iCameraMode == xiiCameraMode::PerspectiveFixedFovX || pMsg2->m_iCameraMode == xiiCameraMode::PerspectiveFixedFovY)
    {
      if (!m_pSceneContext->IsPlayTheGameActive())
      {
        if (xiiSoundInterface* pSoundInterface = xiiSingletonRegistry::GetSingletonInstance<xiiSoundInterface>())
        {
          pSoundInterface->SetListener(-1, pMsg2->m_vPosition, pMsg2->m_vDirForwards, pMsg2->m_vDirUp, xiiVec3::MakeZero());
        }
      }
    }
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiViewPickingMsgToEngine>())
  {
    const xiiViewPickingMsgToEngine* pMsg2 = static_cast<const xiiViewPickingMsgToEngine*>(pMsg);

    PickObjectAt(pMsg2->m_uiPickPosX, pMsg2->m_uiPickPosY);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiViewMarqueePickingMsgToEngine>())
  {
    const xiiViewMarqueePickingMsgToEngine* pMsg2 = static_cast<const xiiViewMarqueePickingMsgToEngine*>(pMsg);

    MarqueePickObjects(pMsg2);
  }
}

void xiiSceneViewContext::SetupRenderTarget(xiiSharedPtr<xiiGALSwapChain> pSwapChain, const xiiRenderTargets* pRenderTargets, xiiUInt16 uiWidth, xiiUInt16 uiHeight)
{
  xiiEngineProcessViewContext::SetupRenderTarget(pSwapChain, pRenderTargets, uiWidth, uiHeight);

  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hView, pView))
  {
    xiiTagSet&                      excludeTags = pView->m_ExcludeTags;
    const xiiArrayPtr<const xiiTag> addTags     = m_pSceneContext->GetInvisibleLayerTags();
    for (const xiiTag& addTag : addTags)
    {
      excludeTags.Set(addTag);
    }
  }
}

bool xiiSceneViewContext::UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds)
{
  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hView, pView))
  {
    pView->SetViewRenderMode(xiiViewRenderMode::Default);
    pView->SetRenderPassProperty("EditorSelectionPass", "Active", false);
    pView->SetExtractorProperty("EditorShapeIconsExtractor", "Active", false);
    pView->SetExtractorProperty("EditorGridExtractor", "Active", false);
    pView->SetRenderPassProperty("EditorPickingPass", "PickSelected", true);
  }

  XII_LOCK(m_pSceneContext->GetWorld()->GetWriteMarker());
  const xiiCameraComponentManager* pCamMan = m_pSceneContext->GetWorld()->GetComponentManager<xiiCameraComponentManager>();
  if (pCamMan)
  {
    for (auto it = pCamMan->GetComponents(); it.IsValid(); ++it)
    {
      const xiiCameraComponent* pCamComp = it;

      if (pCamComp->GetUsageHint() == xiiCameraUsageHint::Thumbnail)
      {
        m_Camera.LookAt(pCamComp->GetOwner()->GetGlobalPosition(), pCamComp->GetOwner()->GetGlobalPosition() + pCamComp->GetOwner()->GetGlobalDirForwards(), pCamComp->GetOwner()->GetGlobalDirUp());

        m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 70.0f, 0.1f, 100.0f);

        m_CullingCamera = m_Camera;
        return true;
      }
    }
  }

  bool bResult    = !FocusCameraOnObject(m_Camera, bounds, 70.0f, -xiiVec3(5, -2, 3));
  m_CullingCamera = m_Camera;
  return bResult;
}

void xiiSceneViewContext::SetInvisibleLayerTags(const xiiArrayPtr<xiiTag> removeTags, const xiiArrayPtr<xiiTag> addTags)
{
  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hView, pView))
  {
    xiiTagSet& excludeTags = pView->m_ExcludeTags;
    for (const xiiTag& removeTag : removeTags)
    {
      excludeTags.Remove(removeTag);
    }
    for (const xiiTag& addTag : addTags)
    {
      excludeTags.Set(addTag);
    }
  }
}

void xiiSceneViewContext::Redraw(bool bRenderEditorGizmos)
{
  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hView, pView))
  {
    const xiiTag& tagNoOrtho = xiiTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode");

    if (pView->GetCamera()->IsOrthographic())
    {
      pView->m_ExcludeTags.Set(tagNoOrtho);
    }
    else
    {
      pView->m_ExcludeTags.Remove(tagNoOrtho);
    }

    XII_LOCK(pView->GetWorld()->GetWriteMarker());
    if (auto pGizmoManager = pView->GetWorld()->GetComponentManager<xiiGizmoComponentManager>())
    {
      pGizmoManager->m_uiHighlightID = GetDocumentContext()->m_Context.m_uiHighlightID;
    }
  }

  xiiEngineProcessViewContext::Redraw(bRenderEditorGizmos);
}

void xiiSceneViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  xiiEngineProcessViewContext::SetCamera(pMsg);

  xiiView* pView = nullptr;
  xiiRenderWorld::TryGetView(m_hView, pView);

  bool bDebugCulling = false;
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  bDebugCulling = xiiRenderPipeline::cvar_SpatialCullingVis;
#endif

  if (bDebugCulling && pView != nullptr)
  {
    if (const xiiCameraComponentManager* pCameraManager = pView->GetWorld()->GetComponentManager<xiiCameraComponentManager>())
    {
      if (const xiiCameraComponent* pCameraComponent = pCameraManager->GetCameraByUsageHint(xiiCameraUsageHint::Culling))
      {
        const xiiGameObject* pOwner    = pCameraComponent->GetOwner();
        xiiVec3              vPosition = pOwner->GetGlobalPosition();
        xiiVec3              vForward  = pOwner->GetGlobalDirForwards();
        xiiVec3              vUp       = pOwner->GetGlobalDirUp();

        m_CullingCamera.LookAt(vPosition, vPosition + vForward, vUp);

        auto  cameraMode = pCameraComponent->GetCameraMode();
        float fFovOrDim  = pCameraComponent->GetFieldOfView();
        if (cameraMode == xiiCameraMode::OrthoFixedWidth || cameraMode == xiiCameraMode::OrthoFixedHeight)
        {
          fFovOrDim = pCameraComponent->GetOrthoDimension();
        }

        const float fNearPlane = pCameraComponent->GetNearPlane();
        const float fFarPlane  = pCameraComponent->GetFarPlane();
        m_CullingCamera.SetCameraMode(cameraMode, fFovOrDim, fNearPlane, xiiMath::Max(fNearPlane + 0.00001f, fFarPlane));
      }
    }
  }
  else
  {
    m_CullingCamera = m_Camera;
  }

  if (pView != nullptr)
  {
    pView->SetRenderPassProperty("EditorSelectionPass", "Active", m_pSceneContext->GetRenderSelectionOverlay());
    pView->SetExtractorProperty("EditorShapeIconsExtractor", "Active", m_pSceneContext->GetRenderShapeIcons());
  }
}

xiiViewHandle xiiSceneViewContext::CreateView()
{
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  xiiVariant sceneContextVariant(m_pSceneContext);
  pView->SetExtractorProperty("EditorSelectedObjectsExtractor", "SceneContext", sceneContextVariant);
  pView->SetExtractorProperty("EditorShapeIconsExtractor", "SceneContext", sceneContextVariant);
  pView->SetExtractorProperty("EditorGridExtractor", "SceneContext", sceneContextVariant);

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  pView->SetCullingCamera(&m_CullingCamera);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);

  const xiiTag& tagHidden = xiiTagRegistry::GetGlobalRegistry().RegisterTag("EditorHidden");

  pView->m_ExcludeTags.Set(tagHidden);
  return pView->GetHandle();
}

void xiiSceneViewContext::PickObjectAt(xiiUInt16 x, xiiUInt16 y)
{
  // remote processes do not support picking, just ignore this
  if (xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  xiiViewPickingResultMsgToEditor res;
  XII_SCOPE_EXIT(SendViewMessage(&res));

  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hView, pView) == false)
    return;

  pView->SetRenderPassProperty("EditorPickingPass", "PickingPosition", xiiVec2(x, y));

  if (pView->IsRenderPassReadBackPropertyExisting("EditorPickingPass", "PickedPosition") == false)
    return;

  xiiVariant varPickedPos = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedPosition");
  if (varPickedPos.IsA<xiiVec3>() == false)
    return;

  const xiiUInt32 uiPickingID    = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedID").ConvertTo<xiiUInt32>();
  res.m_vPickedNormal            = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedNormal").ConvertTo<xiiVec3>();
  res.m_vPickingRayStartPosition = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedRayStartPosition").ConvertTo<xiiVec3>();
  res.m_vPickedPosition          = varPickedPos.ConvertTo<xiiVec3>();

  XII_ASSERT_DEBUG(!res.m_vPickedPosition.IsNaN(), "");

  const xiiUInt32 uiComponentID = (uiPickingID & 0x00FFFFFF);
  const xiiUInt32 uiPartIndex   = (uiPickingID >> 24) & 0x7F; // highest bit indicates whether the object is dynamic, ignore this

  xiiArrayPtr<xiiWorldRttiConverterContext*> contexts = m_pSceneContext->GetAllContexts();
  for (xiiWorldRttiConverterContext* pContext : contexts)
  {
    res.m_ComponentGuid = pContext->m_ComponentPickingMap.GetGuid(uiComponentID);
    if (res.m_ComponentGuid.IsValid() == false)
      continue;

    xiiComponentHandle hComponent = pContext->m_ComponentMap.GetHandle(res.m_ComponentGuid);

    xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();

    // check whether the component is still valid
    xiiComponent* pComponent = nullptr;
    if (pDocumentContext->GetWorld()->TryGetComponent<xiiComponent>(hComponent, pComponent))
    {
      // if yes, fill out the parent game object guid
      res.m_ObjectGuid  = pContext->m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle());
      res.m_uiPartIndex = uiPartIndex;
    }
    else
    {
      res.m_ComponentGuid = xiiUuid();
    }
    break;
  }

  // Always take the other picking ID from the scene itself as gizmos are handled by the window and only the scene itself has one.
  res.m_OtherGuid = m_pSceneContext->m_Context.m_OtherPickingMap.GetGuid(uiComponentID);
}

void xiiSceneViewContext::MarqueePickObjects(const xiiViewMarqueePickingMsgToEngine* pMsg)
{
  // remote processes do not support picking, just ignore this
  if (xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  xiiViewMarqueePickingResultMsgToEditor res;
  res.m_uiWhatToDo         = pMsg->m_uiWhatToDo;
  res.m_uiActionIdentifier = 0;

  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hView, pView))
  {
    pView->SetRenderPassProperty("EditorPickingPass", "MarqueePickPos0", xiiVec2(pMsg->m_uiPickPosX0, pMsg->m_uiPickPosY0));
    pView->SetRenderPassProperty("EditorPickingPass", "MarqueePickPos1", xiiVec2(pMsg->m_uiPickPosX1, pMsg->m_uiPickPosY1));
    pView->SetRenderPassProperty("EditorPickingPass", "MarqueeActionID", pMsg->m_uiActionIdentifier);

    if (pMsg->m_uiWhatToDo == 0xFF)
      return;

    if (!pView->IsRenderPassReadBackPropertyExisting("EditorPickingPass", "MarqueeActionID") || pView->GetRenderPassReadBackProperty("EditorPickingPass", "MarqueeActionID").ConvertTo<xiiUInt32>() != pMsg->m_uiActionIdentifier)
      return;

    res.m_uiActionIdentifier = pMsg->m_uiActionIdentifier;

    xiiVariant varMarquee = pView->GetRenderPassReadBackProperty("EditorPickingPass", "MarqueeResult");

    if (varMarquee.IsA<xiiVariantArray>())
    {
      xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();

      const xiiVariantArray resArray = varMarquee.Get<xiiVariantArray>();

      for (xiiUInt32 i = 0; i < resArray.GetCount(); ++i)
      {
        const xiiVariant& singleRes = resArray[i];

        const xiiUInt32 uiPickingID   = singleRes.ConvertTo<xiiUInt32>();
        const xiiUInt32 uiComponentID = (uiPickingID & 0x00FFFFFF);

        const xiiUuid componentGuid = m_pSceneContext->GetActiveContext().m_ComponentPickingMap.GetGuid(uiComponentID);

        if (componentGuid.IsValid())
        {
          xiiComponentHandle hComponent = m_pSceneContext->GetActiveContext().m_ComponentMap.GetHandle(componentGuid);

          // check whether the component is still valid
          xiiComponent* pComponent = nullptr;
          if (pDocumentContext->GetWorld()->TryGetComponent<xiiComponent>(hComponent, pComponent))
          {
            // if yes, fill out the parent game object guid
            res.m_ObjectGuids.PushBack(m_pSceneContext->GetActiveContext().m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle()));
          }
        }
      }
    }
  }

  if (res.m_uiActionIdentifier == 0)
    return;

  SendViewMessage(&res);
}
