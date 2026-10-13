/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <Core/Interfaces/SoundInterface.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EnginePluginScene/SceneView/SceneView.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Pipeline/View.h>
#include <SharedPluginScene/Scene/SceneCameraSettings.h>

xiiSceneViewContext::xiiSceneViewContext(xiiSceneContext* pSceneContext) :
  xiiEngineProcessViewContext(pSceneContext), m_CameraPreview(*pSceneContext->GetWorld())
{
  m_pSceneContext = pSceneContext;

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

    XII_LOCK(GetDocumentContext()->GetWorld()->GetWriteMarker());
    if (xiiView* pView = GetView())
    {
      pView->SetPipelineParameter("Editor.Picking", pMsg2->m_bUpdatePickingData);
      pView->SetPipelineParameter("Editor.PickSelected", pMsg2->m_bEnablePickingSelected);
      pView->SetPipelineParameter("Editor.PickTransparent", pMsg2->m_bEnablePickTransparent);
    }

    if (pMsg2->m_CameraMode == xiiCameraMode::PerspectiveFixedFovX || pMsg2->m_CameraMode == xiiCameraMode::PerspectiveFixedFovY)
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

bool xiiSceneViewContext::UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds)
{
  XII_LOCK(m_pSceneContext->GetWorld()->GetWriteMarker());
  if (xiiView* pView = GetView())
  {
    pView->SetViewRenderMode(xiiViewRenderMode::Default);
    pView->SetPipelineParameter("Editor.SelectionOverlay", false);
    pView->SetPipelineParameter("Editor.ShapeIcons", false);
    pView->SetPipelineParameter("Editor.PickSelected", true);
    pView->SetPipelineParameter("Editor.Picking", false);
  }

  const xiiCameraComponentManager* pCamMan = m_pSceneContext->GetWorld()->GetComponentManager<xiiCameraComponentManager>();
  if (pCamMan)
  {
    for (auto it = pCamMan->GetComponents(); it.IsValid(); ++it)
    {
      const xiiCameraComponent* pCamComp = it;

      if (pCamComp->GetUsageHint() == xiiCameraUsageHint::Thumbnail)
      {
        if (xiiConfigureSceneCamera(*pCamComp, m_Camera).Failed())
        {
          xiiLog::Warning("Invalid thumbnail camera component; trying the next camera.");
          continue;
        }

        m_CullingCamera = m_Camera;
        return true;
      }
    }
  }

  bool bResult    = !FocusCameraOnObject(m_Camera, bounds, 70.0f, -xiiVec3(5, -2, 3));
  m_CullingCamera = m_Camera;
  return bResult;
}

void xiiSceneViewContext::Redraw(bool bRenderEditorGizmos)
{
  XII_LOCK(GetDocumentContext()->GetWorld()->GetWriteMarker());
  UpdateCameraPreview(bRenderEditorGizmos);
  if (xiiView* pView = GetView())
  {
    if (bRenderEditorGizmos && DrawGrid(m_pSceneContext->GetGridDensity(), m_pSceneContext->GetGridTransform(), m_pSceneContext->IsGridInGlobalSpace()).Failed())
      xiiLog::Warning("Could not build the scene grid.");
    xiiVariantArray selection;
    selection.Reserve(m_pSceneContext->GetSelectionWithChildren().GetCount());
    for (const auto object : m_pSceneContext->GetSelectionWithChildren())
      selection.PushBack(static_cast<xiiUInt64>(object.GetInternalID().m_Data));
    selection.Sort([](const xiiVariant& a, const xiiVariant& b) { return a.Get<xiiUInt64>() < b.Get<xiiUInt64>(); });
    pView->SetPipelineParameter("Editor.SelectedObjects", selection);
    const xiiTag& tagNoOrtho = xiiTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode");

    if (pView->GetCamera()->IsOrthographic())
    {
      pView->m_ExcludeTags.Set(tagNoOrtho);
    }
    else
    {
      pView->m_ExcludeTags.Remove(tagNoOrtho);
    }
  }

  xiiEngineProcessViewContext::Redraw(bRenderEditorGizmos);
}

void xiiSceneViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  xiiEngineProcessViewContext::SetCamera(pMsg);
  XII_LOCK(GetDocumentContext()->GetWorld()->GetWriteMarker());
  UpdateCullingCamera();
  if (xiiView* pView = GetView())
  {
    pView->SetPipelineParameter("Editor.SelectionOverlay", m_pSceneContext->GetRenderSelectionOverlay());
    pView->SetPipelineParameter("Editor.ShapeIcons", m_pSceneContext->GetRenderShapeIcons());
  }
}
