/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>

xiiUInt32 xiiQtEngineViewWidget::s_uiNextViewID = 0;

xiiQtEngineViewWidget::InteractionContext xiiQtEngineViewWidget::s_InteractionContext;

void xiiObjectPickingResult::Reset()
{
  m_PickedComponent = xiiUuid();
  m_PickedObject    = xiiUuid();
  m_PickedOther     = xiiUuid();
  m_uiPartIndex     = 0;
  m_vPickedPosition.SetZero();
  m_vPickedNormal.SetZero();
  m_vPickingRayStart.SetZero();
}

/// Small helper class which exposes the native surface that the renderer can render into.
class xiiQtNativeSurfaceWidget : public QWidget
{
public:
  xiiQtNativeSurfaceWidget(QWidget* pParent = nullptr) :
    QWidget(pParent)
  {
    // setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(false);
    setMouseTracking(true);
    setMinimumSize(64, 64); // prevent the window from becoming zero sized, otherwise the rendering code may crash

    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_NoSystemBackground);
  }

  virtual void          paintEvent(QPaintEvent* pEvent) override {}
  virtual QPaintEngine* paintEngine() const override { return nullptr; }
};

////////////////////////////////////////////////////////////////////////
// xiiQtEngineViewWidget public functions
////////////////////////////////////////////////////////////////////////

xiiSizeU32 xiiQtEngineViewWidget::s_FixedResolution(0, 0);

xiiQtEngineViewWidget::xiiQtEngineViewWidget(QWidget* pParent, xiiQtEngineDocumentWindow* pDocumentWindow, xiiEngineViewConfig* pViewConfig) :
  QWidget(pParent), m_pDocumentWindow(pDocumentWindow), m_pViewConfig(pViewConfig)
{
  setAutoFillBackground(false);
  setMouseTracking(true);
  setMinimumSize(64, 64);
  setFocusPolicy(Qt::FocusPolicy::StrongFocus);

  m_pMainLayout = new QHBoxLayout(this);
  m_pMainLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pMainLayout);

  RecreateEngineViewport();

  m_bUpdatePickingData      = false;
  m_bInDragAndDropOperation = false;

  m_uiViewID = s_uiNextViewID;
  ++s_uiNextViewID;

  m_fCameraLerp           = 1.0f;
  m_fCameraTargetFovOrDim = 70.0f;

  xiiEditorEngineProcessConnection::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtEngineViewWidget::EngineViewProcessEventHandler, this));

  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
  {
    ShowRestartButton(true);
  }
}

xiiQtEngineViewWidget::~xiiQtEngineViewWidget()
{
  xiiEditorEngineProcessConnection::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtEngineViewWidget::EngineViewProcessEventHandler, this));

  {
    // Ensure the engine process swap chain is destroyed before the window.
    xiiViewDestroyedMsgToEngine msg;
    msg.m_uiViewID = GetViewID();
    // If we fail to send the message the engine process is down and we don't need to clean up.
    if (m_pDocumentWindow->GetDocument()->SendMessageToEngine(&msg))
    {
      // Wait for engine process response
      auto Callback = [&](xiiProcessMessage* pMsg) -> bool {
        auto pResponse = static_cast<xiiViewDestroyedResponseMsgToEditor*>(pMsg);
        return pResponse->m_DocumentGuid == m_pDocumentWindow->GetDocument()->GetGuid() && pResponse->m_uiViewID == msg.m_uiViewID;
      };
      xiiProcessCommunicationChannel::WaitForMessageCallback cb = Callback;

      if (xiiEditorEngineProcessConnection::GetSingleton()->WaitForMessage(xiiGetStaticRTTI<xiiViewDestroyedResponseMsgToEditor>(), xiiTime::MakeFromSeconds(5), &cb).Failed())
      {
        xiiLog::Error("Timeout while waiting for engine process to destroy view.");
      }
    }
  }

  m_pDocumentWindow->RemoveViewWidget(this);
}

void xiiQtEngineViewWidget::SyncToEngine()
{
  xiiViewRedrawMsgToEngine cam;
  cam.m_uiRenderMode = m_pViewConfig->m_RenderMode;

  float fFov = m_pViewConfig->m_Camera.GetFovOrDim();
  if (m_pViewConfig->m_Camera.IsPerspective())
  {
    xiiEditorPreferencesUser* pEditorPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
    fFov                                         = pEditorPreferences->m_fPerspectiveFieldOfView;
  }

  cam.m_uiViewID                    = GetViewID();
  cam.m_fNearPlane                  = m_pViewConfig->m_Camera.GetNearPlane();
  cam.m_fFarPlane                   = m_pViewConfig->m_Camera.GetFarPlane();
  cam.m_iCameraMode                 = (xiiInt8)m_pViewConfig->m_Camera.GetCameraMode();
  cam.m_bUseCameraTransformOnDevice = m_pViewConfig->m_bUseCameraTransformOnDevice;
  cam.m_fFovOrDim                   = fFov;
  cam.m_vDirForwards                = m_pViewConfig->m_Camera.GetCenterDirForwards();
  cam.m_vDirUp                      = m_pViewConfig->m_Camera.GetCenterDirUp();
  cam.m_vDirRight                   = m_pViewConfig->m_Camera.GetCenterDirRight();
  cam.m_vPosition                   = m_pViewConfig->m_Camera.GetCenterPosition();
  cam.m_ViewMatrix                  = m_pViewConfig->m_Camera.GetViewMatrix();
  m_pViewConfig->m_Camera.GetProjectionMatrix((float)m_pViewportWidget->width() / (float)m_pViewportWidget->height(), cam.m_ProjMatrix);

  cam.m_uiHWND                 = (xiiUInt64)(m_pViewportWidget->winId());
  cam.m_uiWindowWidth          = m_pViewportWidget->width() * this->devicePixelRatio();
  cam.m_uiWindowHeight         = m_pViewportWidget->height() * this->devicePixelRatio();
  cam.m_bUpdatePickingData     = m_bUpdatePickingData;
  cam.m_bEnablePickingSelected = IsPickingAgainstSelectionAllowed() && (!xiiEditorInputContext::IsAnyInputContextActive() || xiiEditorInputContext::GetActiveInputContext()->IsPickingSelectedAllowed());
  cam.m_bEnablePickTransparent = m_bPickTransparent;

  if (s_FixedResolution.HasNonZeroArea())
  {
    cam.m_uiWindowWidth  = s_FixedResolution.width;
    cam.m_uiWindowHeight = s_FixedResolution.height;
  }

  m_pDocumentWindow->GetEditorEngineConnection()->SendMessage(&cam);
}

void xiiQtEngineViewWidget::GetCameraMatrices(xiiMat4& out_mViewMatrix, xiiMat4& out_mProjectionMatrix) const
{
  out_mViewMatrix = m_pViewConfig->m_Camera.GetViewMatrix();
  m_pViewConfig->m_Camera.GetProjectionMatrix((float)m_pViewportWidget->width() / (float)m_pViewportWidget->height(), out_mProjectionMatrix);
}

void xiiQtEngineViewWidget::UpdateCameraInterpolation()
{
  if (m_fCameraLerp >= 1.0f)
    return;

  const xiiTime tNow  = xiiTime::Now();
  const xiiTime tDiff = tNow - m_LastCameraUpdate;
  m_LastCameraUpdate  = tNow;

  m_fCameraLerp += tDiff.GetSeconds() * 3.0f;

  if (m_fCameraLerp >= 1.0f)
    m_fCameraLerp = 1.0f;

  xiiCamera& cam = m_pViewConfig->m_Camera;

  const float fLerpValue = xiiMath::Sin(xiiAngle::MakeFromDegree(90.0f * m_fCameraLerp));

  xiiQuat qRot, qRotFinal;
  qRot      = xiiQuat::MakeShortestRotation(m_vCameraStartDirection, m_vCameraTargetDirection);
  qRotFinal = xiiQuat::MakeSlerp(xiiQuat::MakeIdentity(), qRot, fLerpValue);

  const xiiVec3 vNewDirection = qRotFinal * m_vCameraStartDirection;
  const xiiVec3 vNewPosition  = xiiMath::Lerp(m_vCameraStartPosition, m_vCameraTargetPosition, fLerpValue);
  const float   fNewFovOrDim  = xiiMath::Lerp(m_fCameraStartFovOrDim, m_fCameraTargetFovOrDim, fLerpValue);

  /// \todo Hard coded up vector
  cam.LookAt(vNewPosition, vNewPosition + vNewDirection, m_vCameraUp);
  cam.SetCameraMode(cam.GetCameraMode(), fNewFovOrDim, cam.GetNearPlane(), cam.GetFarPlane());
}

void xiiQtEngineViewWidget::InterpolateCameraTo(const xiiVec3& vPosition, const xiiVec3& vDirection, float fFovOrDim, const xiiVec3* pNewUpDirection /*= nullptr*/, bool bImmediate /*= false*/)
{
  m_vCameraStartPosition  = m_pViewConfig->m_Camera.GetPosition();
  m_vCameraTargetPosition = vPosition;

  m_vCameraStartDirection  = m_pViewConfig->m_Camera.GetCenterDirForwards();
  m_vCameraTargetDirection = vDirection;

  if (pNewUpDirection)
    m_vCameraUp = *pNewUpDirection;
  else
    m_vCameraUp = m_pViewConfig->m_Camera.GetCenterDirUp();

  m_vCameraStartDirection.Normalize();
  m_vCameraTargetDirection.Normalize();
  m_vCameraUp.Normalize();


  m_fCameraStartFovOrDim = m_pViewConfig->m_Camera.GetFovOrDim();

  if (fFovOrDim > 0.0f)
    m_fCameraTargetFovOrDim = fFovOrDim;

  XII_ASSERT_DEV(m_fCameraTargetFovOrDim > 0, "Invalid FOV or ortho dimension");

  if (m_vCameraStartPosition == m_vCameraTargetPosition && m_vCameraStartDirection == m_vCameraTargetDirection && m_fCameraStartFovOrDim == m_fCameraTargetFovOrDim)
    return;

  m_LastCameraUpdate = xiiTime::Now();
  m_fCameraLerp      = 0.0f;

  if (bImmediate)
  {
    // make sure the next camera update interpolates all the way
    m_LastCameraUpdate -= xiiTime::MakeFromSeconds(10);
    m_fCameraLerp = 0.9f;
  }
}

void xiiQtEngineViewWidget::SetEnablePicking(bool bEnable)
{
  m_bUpdatePickingData = bEnable;
}

void xiiQtEngineViewWidget::SetPickTransparent(bool bEnable)
{
  if (m_bPickTransparent == bEnable)
    return;

  m_bPickTransparent = bEnable;
  m_LastPickingResult.Reset();
}

void xiiQtEngineViewWidget::OpenContextMenu(QPoint globalPos)
{
  s_InteractionContext.m_pLastHoveredViewWidget = this;
  s_InteractionContext.m_pLastPickingResult     = &m_LastPickingResult;

  OnOpenContextMenu(globalPos);
}

const xiiObjectPickingResult& xiiQtEngineViewWidget::PickObject(xiiUInt16 uiScreenPosX, xiiUInt16 uiScreenPosY) const
{
  if (!xiiEditorEngineProcessConnection::GetSingleton()->IsEngineSetup())
  {
    m_LastPickingResult.Reset();
  }
  else
  {
    xiiViewPickingMsgToEngine msg;
    msg.m_uiViewID   = GetViewID();
    msg.m_uiPickPosX = uiScreenPosX * devicePixelRatio();
    msg.m_uiPickPosY = uiScreenPosY * devicePixelRatio();

    GetDocumentWindow()->GetDocument()->SendMessageToEngine(&msg);
  }

  return m_LastPickingResult;
}

xiiResult xiiQtEngineViewWidget::PickPlane(xiiUInt16 uiScreenPosX, xiiUInt16 uiScreenPosY, const xiiPlane& plane, xiiVec3& out_vPosition) const
{
  const auto& cam = m_pViewConfig->m_Camera;

  xiiMat4 mView = cam.GetViewMatrix();
  xiiMat4 mProj;
  cam.GetProjectionMatrix((float)m_pViewportWidget->width() / (float)m_pViewportWidget->height(), mProj);
  xiiMat4 mViewProj    = mProj * mView;
  xiiMat4 mInvViewProj = mViewProj.GetInverse();

  xiiVec3 vScreenPos(uiScreenPosX, uiScreenPosY, 0);
  xiiVec3 vResPos, vResRay;

  if (xiiGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_pViewportWidget->width(), m_pViewportWidget->height(), vScreenPos, vResPos, &vResRay).Failed())
    return XII_FAILURE;

  if (plane.GetRayIntersection(vResPos, vResRay, nullptr, &out_vPosition))
    return XII_SUCCESS;

  return XII_FAILURE;
}

void xiiQtEngineViewWidget::HandleViewMessage(const xiiEditorEngineViewMsg* pMsg)
{
  if (const xiiViewPickingResultMsgToEditor* pFullMsg = xiiDynamicCast<const xiiViewPickingResultMsgToEditor*>(pMsg))
  {
    m_LastPickingResult.m_PickedObject     = pFullMsg->m_ObjectGuid;
    m_LastPickingResult.m_PickedComponent  = pFullMsg->m_ComponentGuid;
    m_LastPickingResult.m_PickedOther      = pFullMsg->m_OtherGuid;
    m_LastPickingResult.m_uiPartIndex      = pFullMsg->m_uiPartIndex;
    m_LastPickingResult.m_vPickedPosition  = pFullMsg->m_vPickedPosition;
    m_LastPickingResult.m_vPickedNormal    = pFullMsg->m_vPickedNormal;
    m_LastPickingResult.m_vPickingRayStart = pFullMsg->m_vPickingRayStartPosition;

    return;
  }
  else if (const xiiViewMarqueePickingResultMsgToEditor* pFullMsg = xiiDynamicCast<const xiiViewMarqueePickingResultMsgToEditor*>(pMsg))
  {
    HandleMarqueePickingResult(pFullMsg);
    return;
  }
}

xiiPlane xiiQtEngineViewWidget::GetFallbackPickingPlane(xiiVec3 vPointOnPlane) const
{
  if (m_pViewConfig->m_Camera.IsPerspective())
  {
    return xiiPlane::MakeFromNormalAndPoint(xiiVec3(0, 0, 1), vPointOnPlane);
  }
  else
  {
    return xiiPlane::MakeFromNormalAndPoint(-m_pViewConfig->m_Camera.GetCenterDirForwards(), vPointOnPlane);
  }
}

void xiiQtEngineViewWidget::TakeScreenshot(xiiStringView sOutputPath) const
{
  xiiViewScreenshotMsgToEngine msg;
  msg.m_uiViewID    = GetViewID();
  msg.m_sOutputFile = sOutputPath;
  m_pDocumentWindow->GetDocument()->SendMessageToEngine(&msg);
}

////////////////////////////////////////////////////////////////////////
// xiiQtEngineViewWidget qt overrides
////////////////////////////////////////////////////////////////////////

bool xiiQtEngineViewWidget::eventFilter(QObject* object, QEvent* event)
{
  if (event->type() == QEvent::Type::ShortcutOverride)
  {
    if (xiiEditorInputContext::IsAnyInputContextActive())
    {
      // if the active input context does not like other shortcuts,
      // accept this event and thus block further shortcut processing
      // instead Qt will then send a keypress event
      if (xiiEditorInputContext::GetActiveInputContext()->GetShortcutsDisabled())
        event->accept();
    }
  }

  return false;
}

void xiiQtEngineViewWidget::paintEvent(QPaintEvent* event)
{
  // event->accept();
}

void xiiQtEngineViewWidget::resizeEvent(QResizeEvent* event)
{
  m_pDocumentWindow->TriggerRedraw();
}

void xiiQtEngineViewWidget::keyPressEvent(QKeyEvent* e)
{
  if (e->isAutoRepeat())
    return;

  // if a context is active, it gets exclusive access to the input data
  if (xiiEditorInputContext::IsAnyInputContextActive())
  {
    if (xiiEditorInputContext::GetActiveInputContext()->KeyPressEvent(e) == xiiEditorInput::WasExclusivelyHandled)
      return;
  }

  if (xiiEditorInputContext::IsAnyInputContextActive())
    return;

  // Override context
  {
    xiiEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->KeyPressEvent(e) == xiiEditorInput::WasExclusivelyHandled || xiiEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->KeyPressEvent(e) == xiiEditorInput::WasExclusivelyHandled || xiiEditorInputContext::IsAnyInputContextActive())
      return;
  }

  QWidget::keyPressEvent(e);
}

void xiiQtEngineViewWidget::keyReleaseEvent(QKeyEvent* e)
{
  if (e->isAutoRepeat())
    return;

  // if a context is active, it gets exclusive access to the input data
  if (xiiEditorInputContext::IsAnyInputContextActive())
  {
    if (xiiEditorInputContext::GetActiveInputContext()->KeyReleaseEvent(e) == xiiEditorInput::WasExclusivelyHandled)
      return;
  }

  if (xiiEditorInputContext::IsAnyInputContextActive())
    return;

  // Override context
  {
    xiiEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->KeyReleaseEvent(e) == xiiEditorInput::WasExclusivelyHandled || xiiEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->KeyReleaseEvent(e) == xiiEditorInput::WasExclusivelyHandled || xiiEditorInputContext::IsAnyInputContextActive())
      return;
  }

  QWidget::keyReleaseEvent(e);
}

void xiiQtEngineViewWidget::mousePressEvent(QMouseEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (xiiEditorInputContext::IsAnyInputContextActive())
  {
    if (xiiEditorInputContext::GetActiveInputContext()->MousePressEvent(e) == xiiEditorInput::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (xiiEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // Override context
  {
    xiiEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->MousePressEvent(e) == xiiEditorInput::WasExclusivelyHandled || xiiEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MousePressEvent(e) == xiiEditorInput::WasExclusivelyHandled || xiiEditorInputContext::IsAnyInputContextActive())
    {
      e->accept();
      return;
    }
  }

  QWidget::mousePressEvent(e);
}

void xiiQtEngineViewWidget::mouseReleaseEvent(QMouseEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (xiiEditorInputContext::IsAnyInputContextActive())
  {
    if (xiiEditorInputContext::GetActiveInputContext()->MouseReleaseEvent(e) == xiiEditorInput::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (xiiEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // Override context
  {
    xiiEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->MouseReleaseEvent(e) == xiiEditorInput::WasExclusivelyHandled || xiiEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MouseReleaseEvent(e) == xiiEditorInput::WasExclusivelyHandled || xiiEditorInputContext::IsAnyInputContextActive())
    {
      e->accept();
      return;
    }
  }

  QWidget::mouseReleaseEvent(e);
}

void xiiQtEngineViewWidget::mouseMoveEvent(QMouseEvent* e)
{
  s_InteractionContext.m_pLastHoveredViewWidget = this;
  s_InteractionContext.m_pLastPickingResult     = &m_LastPickingResult;

  // kick off the picking
  PickObject(e->pos().x(), e->pos().y());

  // if a context is active, it gets exclusive access to the input data
  if (xiiEditorInputContext::IsAnyInputContextActive())
  {
    if (xiiEditorInputContext::GetActiveInputContext()->MouseMoveEvent(e) == xiiEditorInput::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (xiiEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // Override context
  {
    xiiEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->MouseMoveEvent(e) == xiiEditorInput::WasExclusivelyHandled || xiiEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MouseMoveEvent(e) == xiiEditorInput::WasExclusivelyHandled || xiiEditorInputContext::IsAnyInputContextActive())
    {
      e->accept();
      return;
    }
  }

  QWidget::mouseMoveEvent(e);
}

void xiiQtEngineViewWidget::wheelEvent(QWheelEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (xiiEditorInputContext::IsAnyInputContextActive())
  {
    if (xiiEditorInputContext::GetActiveInputContext()->WheelEvent(e) == xiiEditorInput::WasExclusivelyHandled)
      return;
  }

  if (xiiEditorInputContext::IsAnyInputContextActive())
    return;

  // Override context
  {
    xiiEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->WheelEvent(e) == xiiEditorInput::WasExclusivelyHandled || xiiEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->WheelEvent(e) == xiiEditorInput::WasExclusivelyHandled || xiiEditorInputContext::IsAnyInputContextActive())
      return;
  }

  QWidget::wheelEvent(e);
}

void xiiQtEngineViewWidget::focusOutEvent(QFocusEvent* e)
{
  if (xiiEditorInputContext::IsAnyInputContextActive())
  {
    xiiEditorInputContext::GetActiveInputContext()->FocusLost(false);
    xiiEditorInputContext::SetActiveInputContext(nullptr);
  }

  QWidget::focusOutEvent(e);
}


void xiiQtEngineViewWidget::dragEnterEvent(QDragEnterEvent* e)
{
  m_bInDragAndDropOperation = true;
}


void xiiQtEngineViewWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
  m_bInDragAndDropOperation = false;
}


void xiiQtEngineViewWidget::dropEvent(QDropEvent* e)
{
  m_bInDragAndDropOperation = false;
}

////////////////////////////////////////////////////////////////////////
// xiiQtEngineViewWidget protected functions
////////////////////////////////////////////////////////////////////////

void xiiQtEngineViewWidget::EngineViewProcessEventHandler(const xiiEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case xiiEditorEngineProcessConnection::Event::Type::ProcessCrashed:
    {
      ShowRestartButton(true);
    }
    break;

    case xiiEditorEngineProcessConnection::Event::Type::ProcessStarted:
    {
      RecreateEngineViewport();
      ShowRestartButton(false);
    }
    break;

    case xiiEditorEngineProcessConnection::Event::Type::ProcessShutdown:
      break;

    case xiiEditorEngineProcessConnection::Event::Type::ProcessMessage:
      break;

    case xiiEditorEngineProcessConnection::Event::Type::Invalid:
      XII_ASSERT_DEV(false, "Invalid message should never happen");
      break;

    case xiiEditorEngineProcessConnection::Event::Type::ProcessRestarted:
      break;
  }
}

void xiiQtEngineViewWidget::ShowRestartButton(bool bShow)
{
  xiiQtScopedUpdatesDisabled _(this);

  if (m_pRestartButton == nullptr && bShow == true)
  {
    m_pRestartButton = new QPushButton(this);
    m_pRestartButton->setText("Restart Engine View Process");
    m_pRestartButton->setVisible(xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());
    m_pRestartButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_pRestartButton->connect(m_pRestartButton, &QPushButton::clicked, this, &xiiQtEngineViewWidget::SlotRestartEngineProcess);

    m_pMainLayout->addWidget(m_pRestartButton);
  }

  if (m_pRestartButton)
  {
    m_pRestartButton->setVisible(bShow);

    if (bShow)
      m_pRestartButton->update();
  }

  m_pViewportWidget->setVisible(!bShow);
}

void xiiQtEngineViewWidget::RecreateEngineViewport()
{
  if (m_pViewportWidget)
  {
    m_pViewportWidget->removeEventFilter(this);
    m_pViewportWidget->hide();
    m_pViewportWidget->setParent(nullptr);
    m_pViewportWidget->deleteLater();
  }

  m_pViewportWidget = new xiiQtNativeSurfaceWidget(this);
  m_pViewportWidget->installEventFilter(this);
  m_pViewportWidget->setFocusProxy(this);
  if (s_FixedResolution.HasNonZeroArea())
  {
    qreal pixelRatio = devicePixelRatio();
    // When using DPI scaling, this could actually not be possible to achieve so we use the ceiling of the logical size. This is fine, as the editor tests crop the resulting image if not of the proper size.
    m_pViewportWidget->setFixedSize(static_cast<xiiInt32>(xiiMath::Ceil(s_FixedResolution.width / pixelRatio)), static_cast<xiiInt32>(xiiMath::Ceil(s_FixedResolution.height / pixelRatio)));
  }
  else
  {
    m_pMainLayout->addWidget(m_pViewportWidget);
  }
}

////////////////////////////////////////////////////////////////////////
// xiiQtEngineViewWidget private slots
////////////////////////////////////////////////////////////////////////

void xiiQtEngineViewWidget::SlotRestartEngineProcess()
{
  xiiEditorEngineProcessConnection::GetSingleton()->RestartProcess().IgnoreResult();
}

////////////////////////////////////////////////////////////////////////
// xiiQtViewWidgetContainer
////////////////////////////////////////////////////////////////////////

xiiQtViewWidgetContainer::xiiQtViewWidgetContainer(ads::CDockManager* pDockManager, QWidget* pParent, xiiQtEngineViewWidget* pViewWidget, xiiStringView sToolBarMapping) :
  ads::CDockWidget(pDockManager, "3D View", pParent)
{
  setObjectName("xiiQtViewWidgetContainer");

  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetClosable, false);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetFloatable, false);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetMovable, false);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetFocusable, true);

  // need contrast with the rest of the widgets around it
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);

  QWidget* pDummy = new QWidget();
  pDummy->setObjectName("Dummy");

  m_pLayout = new QVBoxLayout(pDummy);
  m_pLayout->setObjectName("QVBoxLayout1");
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(0);
  pDummy->setLayout(m_pLayout);

  m_pViewWidget = pViewWidget;
  m_pViewWidget->setParent(pDummy);

  if (!sToolBarMapping.IsEmpty())
  {
    // Add Tool Bar
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;

    context.m_sMapping  = sToolBarMapping;
    context.m_pDocument = pViewWidget->GetDocumentWindow()->GetDocument();
    context.m_pWindow   = m_pViewWidget;

    pToolBar->SetActionContext(context);
    m_pLayout->addWidget(pToolBar, 0);
  }

  m_pLayout->addWidget(m_pViewWidget, 1);

  setWidget(pDummy);
}

xiiQtViewWidgetContainer::~xiiQtViewWidgetContainer() = default;
