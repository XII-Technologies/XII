#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorFramework/InputContexts/SelectionContext.h>

xiiQtOrbitCamViewWidget::xiiQtOrbitCamViewWidget(xiiQtEngineDocumentWindow* pOwnerWindow, xiiEngineViewConfig* pViewConfig, bool bPicking) :
  xiiQtEngineViewWidget(nullptr, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_pOrbitCameraContext = XII_DEFAULT_NEW(xiiOrbitCameraContext, pOwnerWindow, this);
  m_pOrbitCameraContext->SetCamera(&m_pViewConfig->m_Camera);

  if (bPicking)
  {
    m_pSelectionContext = XII_DEFAULT_NEW(xiiSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
    m_InputContexts.PushBack(m_pSelectionContext.Borrow());
  }

  m_InputContexts.PushBack(m_pOrbitCameraContext.Borrow());
}

xiiQtOrbitCamViewWidget::~xiiQtOrbitCamViewWidget() = default;


void xiiQtOrbitCamViewWidget::ConfigureFixed(const xiiVec3& vCenterPos, const xiiVec3& vHalfBoxSize, const xiiVec3& vCamPosition)
{
  m_pOrbitCameraContext->SetDefaultCameraFixed(vCamPosition);
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize);
  m_pOrbitCameraContext->MoveCameraToDefaultPosition();
  m_bSetDefaultCamPos = false;
}

void xiiQtOrbitCamViewWidget::ConfigureRelative(const xiiVec3& vCenterPos, const xiiVec3& vHalfBoxSize, const xiiVec3& vCamDirection, float fCamDistanceScale)
{
  m_pOrbitCameraContext->SetDefaultCameraRelative(vCamDirection, fCamDistanceScale);
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize);
  m_pOrbitCameraContext->MoveCameraToDefaultPosition();
  m_bSetDefaultCamPos = true;
}

void xiiQtOrbitCamViewWidget::SetOrbitVolume(const xiiVec3& vCenterPos, const xiiVec3& vHalfBoxSize)
{
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize);

  if (m_bSetDefaultCamPos)
  {
    if (vHalfBoxSize != xiiVec3(0.1f))
    {
      // 0.1f is a hard-coded value for the bounding box, in case nothing is available yet
      // not pretty, but somehow we need to know when the first 'proper' bounds are available

      m_bSetDefaultCamPos = false;
      m_pOrbitCameraContext->MoveCameraToDefaultPosition();
    }
  }
}

xiiOrbitCameraContext* xiiQtOrbitCamViewWidget::GetOrbitCamera()
{
  return m_pOrbitCameraContext.Borrow();
}

void xiiQtOrbitCamViewWidget::SyncToEngine()
{
  if (m_pSelectionContext)
  {
    m_pSelectionContext->SetWindowConfig(xiiVec2I32(width(), height()));
  }

  xiiQtEngineViewWidget::SyncToEngine();
}
