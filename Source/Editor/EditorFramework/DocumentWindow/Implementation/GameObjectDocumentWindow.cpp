/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/Manipulators/ManipulatorAdapterRegistry.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

xiiQtGameObjectDocumentWindow::xiiQtGameObjectDocumentWindow(xiiGameObjectDocument* pDocument) :
  xiiQtEngineDocumentWindow(pDocument)
{
  pDocument->m_GameObjectEvents.AddEventHandler(xiiMakeDelegate(&xiiQtGameObjectDocumentWindow::GameObjectEventHandler, this));
  xiiSnapProvider::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtGameObjectDocumentWindow::SnapProviderEventHandler, this));
}

xiiQtGameObjectDocumentWindow::~xiiQtGameObjectDocumentWindow()
{
  GetGameObjectDocument()->m_GameObjectEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtGameObjectDocumentWindow::GameObjectEventHandler, this));
  xiiSnapProvider::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtGameObjectDocumentWindow::SnapProviderEventHandler, this));
}

xiiGameObjectDocument* xiiQtGameObjectDocumentWindow::GetGameObjectDocument() const
{
  return static_cast<xiiGameObjectDocument*>(GetDocument());
}

xiiWorldSettingsMsgToEngine xiiQtGameObjectDocumentWindow::GetWorldSettings() const
{
  xiiWorldSettingsMsgToEngine msg;
  auto                        pGameObjectDoc = GetGameObjectDocument();
  msg.m_bRenderOverlay                       = pGameObjectDoc->GetRenderSelectionOverlay();
  msg.m_bRenderShapeIcons                    = pGameObjectDoc->GetRenderShapeIcons();
  msg.m_bRenderSelectionBoxes                = pGameObjectDoc->GetRenderVisualizers();
  msg.m_bAddAmbientLight                     = pGameObjectDoc->GetAddAmbientLight();
  return msg;
}

xiiGridSettingsMsgToEngine xiiQtGameObjectDocumentWindow::GetGridSettings() const
{
  xiiGridSettingsMsgToEngine msg;

  if (auto pTool = GetGameObjectDocument()->GetActiveEditTool())
  {
    pTool->GetGridSettings(msg);
  }
  else
  {
    xiiManipulatorAdapterRegistry::GetSingleton()->QueryGridSettings(GetDocument(), msg);
  }

  return msg;
}

void xiiQtGameObjectDocumentWindow::ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg)
{
  xiiQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg);
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiQuerySelectionBBoxResultMsgToEditor>())
  {
    const xiiQuerySelectionBBoxResultMsgToEditor* msg = static_cast<const xiiQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (msg->m_uiViewID == 0xFFFFFFFF)
    {
      for (auto pView : m_ViewWidgets)
      {
        if (!pView)
          continue;

        if (msg->m_iPurpose == 0)
          HandleFocusOnSelection(msg, static_cast<xiiQtGameObjectViewWidget*>(pView));
      }
    }
    else
    {
      xiiQtGameObjectViewWidget* pSceneView = static_cast<xiiQtGameObjectViewWidget*>(GetViewWidgetByID(msg->m_uiViewID));

      if (!pSceneView)
        return;

      if (msg->m_iPurpose == 0)
        HandleFocusOnSelection(msg, pSceneView);
    }

    return;
  }
}

void xiiQtGameObjectDocumentWindow::GameObjectEventHandler(const xiiGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiGameObjectEvent::Type::TriggerFocusOnSelection_Hovered:
      FocusOnSelectionHoveredView();
      break;

    case xiiGameObjectEvent::Type::TriggerFocusOnSelection_All:
      FocusOnSelectionAllViews();
      break;

    default:
      break;
  }
}

void xiiQtGameObjectDocumentWindow::FocusOnSelectionAllViews()
{
  const auto& sel = GetDocument()->GetSelectionManager()->GetSelection();

  if (sel.IsEmpty())
    return;
  if (!sel.PeekBack()->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
    return;

  xiiQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = 0;
  GetDocument()->SendMessageToEngine(&msg);
}

void xiiQtGameObjectDocumentWindow::FocusOnSelectionHoveredView()
{
  const auto& sel = GetDocument()->GetSelectionManager()->GetSelection();

  if (sel.IsEmpty())
    return;
  if (!sel.PeekBack()->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
    return;

  auto pView = GetHoveredViewWidget();

  if (pView == nullptr)
    return;

  xiiQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = pView->GetViewID();
  msg.m_iPurpose = 0;
  GetDocument()->SendMessageToEngine(&msg);
}

void xiiQtGameObjectDocumentWindow::HandleFocusOnSelection(const xiiQuerySelectionBBoxResultMsgToEditor* pMsg, xiiQtGameObjectViewWidget* pSceneView)
{
  const xiiVec3 vPivotPoint = pMsg->m_vCenter;

  const xiiCamera& cam = pSceneView->m_pViewConfig->m_Camera;

  xiiVec3 vNewCameraPosition  = cam.GetCenterPosition();
  xiiVec3 vNewCameraDirection = cam.GetDirForwards();
  float   fNewFovOrDim        = cam.GetFovOrDim();

  if (pSceneView->width() == 0 || pSceneView->height() == 0)
    return;

  const float fApsectRation = (float)pSceneView->width() / (float)pSceneView->height();

  xiiBoundingBox bbox;

  // clamp the bbox of the selection to ranges that won't break down due to float precision
  {
    bbox        = xiiBoundingBox::MakeFromCenterAndHalfExtents(pMsg->m_vCenter, pMsg->m_vHalfExtents);
    bbox.m_vMin = bbox.m_vMin.CompMax(xiiVec3(-1000.0f));
    bbox.m_vMax = bbox.m_vMax.CompMin(xiiVec3(+1000.0f));
  }

  const xiiVec3 vCurrentOrbitPoint = pSceneView->m_pCameraMoveContext->GetOrbitPoint();
  const bool    bZoomIn            = vPivotPoint.IsEqual(vCurrentOrbitPoint, 0.1f);

  if (cam.GetCameraMode() == xiiCameraMode::PerspectiveFixedFovX || cam.GetCameraMode() == xiiCameraMode::PerspectiveFixedFovY)
  {
    const float maxExt       = pMsg->m_vHalfExtents.GetLength();
    const float fMinDistance = cam.GetNearPlane() * 1.1f + maxExt;

    {
      xiiPlane p = xiiPlane::MakeFromNormalAndPoint(vNewCameraDirection, vNewCameraPosition);

      // at some distance the floating point precision gets so crappy that the camera movement breaks
      // therefore we clamp it to a 'reasonable' distance here
      const float distBest = xiiMath::Min(xiiMath::Abs(p.GetDistanceTo(vPivotPoint)), 500.0f);

      vNewCameraPosition = vPivotPoint - vNewCameraDirection * xiiMath::Max(fMinDistance, distBest);
    }

    // only zoom in on the object, if the target position is already identical (action executed twice)
    if (!pMsg->m_vHalfExtents.IsZero(xiiMath::DefaultEpsilon<float>()) && bZoomIn)
    {
      const xiiAngle fovX = cam.GetFovX(fApsectRation);
      const xiiAngle fovY = cam.GetFovY(fApsectRation);

      const float fRadius = bbox.GetBoundingSphere().m_fRadius * 1.5f;

      const float dist1    = fRadius / xiiMath::Sin(fovX * 0.75f);
      const float dist2    = fRadius / xiiMath::Sin(fovY * 0.75f);
      const float distBest = xiiMath::Max(dist1, dist2);

      vNewCameraPosition = vPivotPoint - vNewCameraDirection * xiiMath::Max(fMinDistance, distBest);
    }
  }
  else
  {
    vNewCameraPosition = pMsg->m_vCenter;

    // only zoom in on the object, if the target position is already identical (action executed twice)
    if (bZoomIn)
    {

      const xiiVec3 right = cam.GetDirRight();
      const xiiVec3 up    = cam.GetDirUp();

      const float fSizeFactor = 2.0f;

      const float fRequiredWidth  = xiiMath::Abs(right.Dot(bbox.GetHalfExtents()) * 2.0f) * fSizeFactor;
      const float fRequiredHeight = xiiMath::Abs(up.Dot(bbox.GetHalfExtents()) * 2.0f) * fSizeFactor;

      float fDimWidth, fDimHeight;

      if (cam.GetCameraMode() == xiiCameraMode::OrthoFixedHeight)
      {
        fDimHeight = cam.GetFovOrDim();
        fDimWidth  = fDimHeight * fApsectRation;
      }
      else
      {
        fDimWidth  = cam.GetFovOrDim();
        fDimHeight = fDimWidth / fApsectRation;
      }

      const float fScaleWidth  = fRequiredWidth / fDimWidth;
      const float fScaleHeight = fRequiredHeight / fDimHeight;

      const float fScaleDim = xiiMath::Max(fScaleWidth, fScaleHeight);

      if (fScaleDim > 0.0f)
      {
        fNewFovOrDim *= fScaleDim;
      }
    }
  }

  pSceneView->m_pCameraMoveContext->SetOrbitDistance(vPivotPoint.Distance(vNewCameraPosition));
  pSceneView->InterpolateCameraTo(vNewCameraPosition, vNewCameraDirection, fNewFovOrDim);
}

void xiiQtGameObjectDocumentWindow::SnapProviderEventHandler(const xiiSnapProviderEvent& e)
{
  switch (e.m_Type)
  {
    case xiiSnapProviderEvent::Type::RotationSnapChanged:
      ShowTemporaryStatusBarMsg(xiiFmt(xiiStringUtf8(L"Snapping Angle: {0}°").GetData(), xiiSnapProvider::GetRotationSnapValue().GetDegree()));
      break;

    case xiiSnapProviderEvent::Type::ScaleSnapChanged:
      ShowTemporaryStatusBarMsg(xiiFmt("Snapping Value: {0}", xiiSnapProvider::GetScaleSnapValue()));
      break;

    case xiiSnapProviderEvent::Type::TranslationSnapChanged:
      ShowTemporaryStatusBarMsg(xiiFmt("Snapping Value: {0}", xiiSnapProvider::GetTranslationSnapValue()));
      break;
  }
}
