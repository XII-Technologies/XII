#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <EditorFramework/InputContexts/SelectionContext.h>
#include <Foundation/Utilities/GraphicsUtils.h>

xiiSelectionContext::xiiSelectionContext(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView, const xiiCamera* pCamera)
{
  m_pCamera = pCamera;

  SetOwner(pOwnerWindow, pOwnerView);

  m_hMarqueeGizmo.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::LineBox, xiiColor::CadetBlue, xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::OnTop);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hMarqueeGizmo);
}

xiiSelectionContext::~xiiSelectionContext()
{
  // if anyone is registered for object picking, tell them that nothing was picked,
  // so that they reset their state
  if (m_PickObjectOverride.IsValid())
  {
    m_PickObjectOverride(nullptr);
    ResetPickObjectOverride();
  }
}

void xiiSelectionContext::SetPickObjectOverride(xiiDelegate<void(const xiiDocumentObject*)> pickOverride)
{
  m_PickObjectOverride = pickOverride;
  GetOwnerView()->setCursor(Qt::CrossCursor);
}

void xiiSelectionContext::ResetPickObjectOverride()
{
  if (m_PickObjectOverride.IsValid())
  {
    m_PickObjectOverride.Invalidate();
    GetOwnerView()->unsetCursor();
  }
}

xiiEditorInput xiiSelectionContext::DoMousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::LeftButton)
  {
    const xiiObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

    if (res.m_PickedOther.IsValid())
    {
      auto pSO = GetOwnerWindow()->GetDocument()->FindSyncObject(res.m_PickedOther);

      if (pSO != nullptr)
      {
        if (pSO->GetDynamicRTTI()->IsDerivedFrom<xiiGizmoHandle>())
        {
          xiiGizmoHandle* pGizmoHandle = static_cast<xiiGizmoHandle*>(pSO);
          xiiGizmo*       pGizmo       = pGizmoHandle->GetOwnerGizmo();

          if (pGizmo)
          {
            pGizmo->ConfigureInteraction(pGizmoHandle, m_pCamera, res.m_vPickedPosition, m_vViewport);
            return pGizmo->MousePressEvent(e);
          }
        }
      }
    }

    m_Mode = Mode::Single;

    if (m_bPressedSpace && !m_PickObjectOverride.IsValid())
    {
      m_uiMarqueeID += 23;
      m_vMarqueeStartPos.Set(e->pos().x(), e->pos().y(), 0.01f);

      // no modifier -> add, CTRL -> remove
      m_Mode = e->modifiers().testFlag(Qt::ControlModifier) ? Mode::MarqueeRemove : Mode::MarqueeAdd;
      MakeActiveInputContext();

      if (m_Mode == Mode::MarqueeAdd)
        m_hMarqueeGizmo.SetColor(xiiColor::LightSkyBlue);
      else
        m_hMarqueeGizmo.SetColor(xiiColor::PaleVioletRed);

      return xiiEditorInput::WasExclusivelyHandled;
    }
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

xiiEditorInput xiiSelectionContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::MiddleButton)
  {
    if (e->modifiers() & Qt::KeyboardModifier::ControlModifier)
    {
      const xiiObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

      OpenDocumentForPickedObject(res);
    }
  }

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    if (m_Mode == Mode::Single)
    {
      const xiiObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

      const bool bToggle = (e->modifiers() & Qt::KeyboardModifier::ControlModifier) != 0;
      const bool bDirect = (e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0;
      SelectPickedObject(res, bToggle, bDirect);

      DoFocusLost(false);

      // we handled the mouse click event
      // but this is it, we don't stay active
      return xiiEditorInput::WasExclusivelyHandled;
    }

    if (m_Mode == Mode::MarqueeAdd || m_Mode == Mode::MarqueeRemove)
    {
      SendMarqueeMsg(e, (m_Mode == Mode::MarqueeAdd) ? 1 : 2);

      const bool bPressedSpace = m_bPressedSpace;
      DoFocusLost(false);
      m_bPressedSpace = bPressedSpace;
      return xiiEditorInput::WasExclusivelyHandled;
    }
  }

  return xiiEditorInput::MayBeHandledByOthers;
}


void xiiSelectionContext::OpenDocumentForPickedObject(const xiiObjectPickingResult& res) const
{
  if (!res.m_PickedComponent.IsValid())
    return;

  auto* pDocument = GetOwnerWindow()->GetDocument();

  const xiiDocumentObject* pPickedComponent = pDocument->GetObjectManager()->GetObject(res.m_PickedComponent);

  for (auto pDocMan : xiiDocumentManager::GetAllDocumentManagers())
  {
    if (xiiAssetDocumentManager* pAssetMan = xiiDynamicCast<xiiAssetDocumentManager*>(pDocMan))
    {
      if (pAssetMan->OpenPickedDocument(pPickedComponent, res.m_uiPartIndex).Succeeded())
      {
        return;
      }
    }
  }

  GetOwnerWindow()->ShowTemporaryStatusBarMsg("Could not open a document for the picked object");
}

void xiiSelectionContext::SelectPickedObject(const xiiObjectPickingResult& res, bool bToggle, bool bDirect) const
{
  if (res.m_PickedObject.IsValid())
  {
    auto*                    pDocument = GetOwnerWindow()->GetDocument();
    const xiiDocumentObject* pObject   = pDocument->GetObjectManager()->GetObject(res.m_PickedObject);
    if (!pObject)
      return;

    if (m_PickObjectOverride.IsValid())
    {
      m_PickObjectOverride(pObject);
    }
    else
    {
      if (bToggle)
        pDocument->GetSelectionManager()->ToggleObject(determineObjectToSelect(pObject, true, bDirect));
      else
        pDocument->GetSelectionManager()->SetSelection(determineObjectToSelect(pObject, false, bDirect));
    }
  }
}

void xiiSelectionContext::SendMarqueeMsg(QMouseEvent* e, xiiUInt8 uiWhatToDo)
{
  xiiVec2I32 curPos;
  curPos.Set(e->pos().x(), e->pos().y());

  xiiMat4 mView = m_pCamera->GetViewMatrix();
  xiiMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);

  xiiMat4 mViewProj    = mProj * mView;
  xiiMat4 mInvViewProj = mViewProj;
  if (mInvViewProj.Invert(0.0f).Failed())
  {
    // if this fails, the marquee will not be rendered correctly
    XII_ASSERT_DEBUG(false, "Failed to invert view projection matrix.");
  }

  const xiiVec3 vMousePos(e->pos().x(), e->pos().y(), 0.01f);

  const xiiVec3 vScreenSpacePos0(vMousePos.x, vMousePos.y, vMousePos.z);
  const xiiVec3 vScreenSpacePos1(m_vMarqueeStartPos.x, m_vMarqueeStartPos.y, m_vMarqueeStartPos.z);

  xiiVec3 vPosOnNearPlane0, vRayDir0;
  xiiVec3 vPosOnNearPlane1, vRayDir1;
  xiiGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, vScreenSpacePos0, vPosOnNearPlane0, &vRayDir0).IgnoreResult();
  xiiGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, vScreenSpacePos1, vPosOnNearPlane1, &vRayDir1).IgnoreResult();

  xiiTransform t;
  t.SetIdentity();
  t.m_vPosition = xiiMath::Lerp(vPosOnNearPlane0, vPosOnNearPlane1, 0.5f);
  t.m_qRotation = xiiQuat::MakeFromMat3(m_pCamera->GetViewMatrix().GetRotationalPart());

  // box coordinates in screen space
  xiiVec3 vBoxPosSS0 = t.m_qRotation * vPosOnNearPlane0;
  xiiVec3 vBoxPosSS1 = t.m_qRotation * vPosOnNearPlane1;

  t.m_qRotation = t.m_qRotation.GetInverse();

  t.m_vScale.x = xiiMath::Abs(vBoxPosSS0.x - vBoxPosSS1.x);
  t.m_vScale.y = xiiMath::Abs(vBoxPosSS0.y - vBoxPosSS1.y);
  t.m_vScale.z = 0.0f;

  m_hMarqueeGizmo.SetTransformation(t);
  m_hMarqueeGizmo.SetVisible(true);

  {
    xiiViewMarqueePickingMsgToEngine msg;
    msg.m_uiViewID           = GetOwnerView()->GetViewID();
    msg.m_uiPickPosX0        = (xiiUInt16)m_vMarqueeStartPos.x;
    msg.m_uiPickPosY0        = (xiiUInt16)m_vMarqueeStartPos.y;
    msg.m_uiPickPosX1        = (xiiUInt16)(e->pos().x());
    msg.m_uiPickPosY1        = (xiiUInt16)(e->pos().y());
    msg.m_uiWhatToDo         = uiWhatToDo;
    msg.m_uiActionIdentifier = m_uiMarqueeID;

    GetOwnerView()->GetDocumentWindow()->GetDocument()->SendMessageToEngine(&msg);
  }
}

xiiEditorInput xiiSelectionContext::DoMouseMoveEvent(QMouseEvent* e)
{
  if (IsActiveInputContext() && (m_Mode == Mode::MarqueeAdd || m_Mode == Mode::MarqueeRemove))
  {
    SendMarqueeMsg(e, 0xFF);

    return xiiEditorInput::WasExclusivelyHandled;
  }
  else
  {
    xiiViewHighlightMsgToEngine msg;

    {
      const xiiObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

      if (res.m_PickedComponent.IsValid())
        msg.m_HighlightObject = res.m_PickedComponent;
      else if (res.m_PickedOther.IsValid())
        msg.m_HighlightObject = res.m_PickedOther;
      else
        msg.m_HighlightObject = res.m_PickedObject;
    }

    GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

    // we only updated the highlight, so others may do additional stuff, if they like
    return xiiEditorInput::MayBeHandledByOthers;
  }
}

xiiEditorInput xiiSelectionContext::DoKeyPressEvent(QKeyEvent* e)
{
  /// \todo Handle the current cursor (icon) across all active input contexts

  if (e->key() == Qt::Key_Space)
  {
    m_bPressedSpace = true;
    return xiiEditorInput::MayBeHandledByOthers;
  }

  if (e->key() == Qt::Key_Delete)
  {
    GetOwnerWindow()->GetDocument()->DeleteSelectedObjects();
    return xiiEditorInput::WasExclusivelyHandled;
  }

  if (e->key() == Qt::Key_Escape)
  {
    if (m_PickObjectOverride.IsValid())
    {
      m_PickObjectOverride(nullptr);
      ResetPickObjectOverride();
    }
    else
    {
      if (m_Mode == Mode::MarqueeAdd || m_Mode == Mode::MarqueeRemove)
      {
        const bool bPressedSpace = m_bPressedSpace;
        FocusLost(true);
        m_bPressedSpace = bPressedSpace;
      }
      else
      {
        GetOwnerWindow()->GetDocument()->GetSelectionManager()->Clear();
      }
    }

    return xiiEditorInput::WasExclusivelyHandled;
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

xiiEditorInput xiiSelectionContext::DoKeyReleaseEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key_Space)
  {
    m_bPressedSpace = false;
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

static const bool IsInSelection(const xiiDeque<const xiiDocumentObject*>& selection, const xiiDocumentObject* pObject, const xiiDocumentObject*& out_pParentInSelection, const xiiDocumentObject*& out_pParentChild, const xiiDocumentObject* pRootObject)
{
  if (pObject == pRootObject)
    return false;

  if (selection.IndexOf(pObject) != xiiInvalidIndex)
  {
    out_pParentInSelection = pObject;
    return true;
  }

  const xiiDocumentObject* pParent = pObject->GetParent();

  if (IsInSelection(selection, pParent, out_pParentInSelection, out_pParentChild, pRootObject))
  {
    if (out_pParentChild == nullptr)
      out_pParentChild = pObject;

    return true;
  }

  return false;
}

static const xiiDocumentObject* GetPrefabParentOrSelf(const xiiDocumentObject* pObject)
{
  const xiiDocumentObject* pParent   = pObject;
  const xiiDocument*       pDocument = pObject->GetDocumentObjectManager()->GetDocument();
  const auto&              metaData  = *pDocument->m_DocumentObjectMetaData;

  while (pParent != nullptr)
  {
    {
      const xiiDocumentObjectMetaData* pMeta     = metaData.BeginReadMetaData(pParent->GetGuid());
      bool                             bIsPrefab = pMeta->m_CreateFromPrefab.IsValid();
      metaData.EndReadMetaData();

      if (bIsPrefab)
        return pParent;
    }
    pParent = pParent->GetParent();
  }

  return pObject;
}

const xiiDocumentObject* xiiSelectionContext::determineObjectToSelect(const xiiDocumentObject* pickedObject, bool bToggle, bool bDirect) const
{
  auto*                                    pDocument = GetOwnerWindow()->GetDocument();
  const xiiDeque<const xiiDocumentObject*> sel       = pDocument->GetSelectionManager()->GetSelection();

  const xiiDocumentObject* pRootObject = pDocument->GetObjectManager()->GetRootObject();

  const xiiDocumentObject* pParentInSelection = nullptr;
  const xiiDocumentObject* pParentChild       = nullptr;

  if (!IsInSelection(sel, pickedObject, pParentInSelection, pParentChild, pRootObject))
  {
    if (bDirect)
      return pickedObject;

    return GetPrefabParentOrSelf(pickedObject);
  }
  else
  {
    if (bToggle)
    {
      // always toggle the object that is already in the selection
      return pParentInSelection;
    }

    if (bDirect)
      return pickedObject;

    if (sel.GetCount() > 1)
    {
      // multi-selection, but no toggle, so we are about to set the selection
      // -> always use the top-level parent in this case
      return GetPrefabParentOrSelf(pickedObject);
    }

    if (pParentInSelection == pickedObject)
    {
      // object itself is in the selection
      return pickedObject;
    }

    if (pParentChild == nullptr)
    {
      return pParentInSelection;
    }

    return pParentChild;
  }
}

void xiiSelectionContext::DoFocusLost(bool bCancel)
{
  xiiEditorInputContext::DoFocusLost(bCancel);

  m_bPressedSpace = false;
  m_Mode          = Mode::None;
  m_hMarqueeGizmo.SetVisible(false);

  if (IsActiveInputContext())
    MakeActiveInputContext(false);
}
