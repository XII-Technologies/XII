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
  m_pOrbitCameraContext->SetOrbitVolume(xiiVec3(0, 0, 1), xiiVec3(10.0f), xiiVec3(-5, 1, 2), true);

  if (bPicking)
  {
    m_pSelectionContext = XII_DEFAULT_NEW(xiiSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
    m_InputContexts.PushBack(m_pSelectionContext.Borrow());
  }

  m_InputContexts.PushBack(m_pOrbitCameraContext.Borrow());
}

xiiQtOrbitCamViewWidget::~xiiQtOrbitCamViewWidget() = default;


void xiiQtOrbitCamViewWidget::ConfigureOrbitCameraVolume(const xiiVec3& vCenterPos, const xiiVec3& vHalfBoxSize, const xiiVec3& vDefaultCameraPosition)
{
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize, vDefaultCameraPosition, true);
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
