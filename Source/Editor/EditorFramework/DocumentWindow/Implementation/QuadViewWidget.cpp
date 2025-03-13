#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/Preferences/QuadViewPreferences.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>

xiiQtQuadViewWidget::xiiQtQuadViewWidget(xiiAssetDocument* pDocument, xiiQtEngineDocumentWindow* pWindow, ViewFactory viewFactory, const char* szViewToolBarMapping)
{
  setObjectName("xiiQtQuadViewWidget");

  m_pDocument           = pDocument;
  m_pWindow             = pWindow;
  m_ViewFactory         = viewFactory;
  m_sViewToolBarMapping = szViewToolBarMapping;

  m_pViewLayout = new QGridLayout(this);
  m_pViewLayout->setObjectName("QGridLayout1");
  m_pViewLayout->setContentsMargins(0, 0, 0, 0);
  m_pViewLayout->setSpacing(4);

  setLayout(m_pViewLayout);

  LoadViewConfigs();
}

xiiQtQuadViewWidget::~xiiQtQuadViewWidget()
{
  SaveViewConfigs();
}

void xiiQtQuadViewWidget::SaveViewConfig(const xiiEngineViewConfig& cfg, xiiEngineViewPreferences& pref) const
{
  pref.m_vCamPos         = cfg.m_Camera.GetPosition();
  pref.m_vCamDir         = cfg.m_Camera.GetDirForwards();
  pref.m_vCamUp          = cfg.m_Camera.GetDirUp();
  pref.m_PerspectiveMode = cfg.m_Perspective;
  pref.m_RenderMode      = cfg.m_RenderMode;
  pref.m_fFov            = cfg.m_Camera.GetFovOrDim();
}

void xiiQtQuadViewWidget::LoadViewConfig(xiiEngineViewConfig& cfg, xiiEngineViewPreferences& pref)
{
  cfg.m_Perspective = (xiiSceneViewPerspective::Enum)pref.m_PerspectiveMode;
  cfg.m_RenderMode  = (xiiViewRenderMode::Enum)pref.m_RenderMode;
  cfg.m_Camera.LookAt(xiiVec3(0), xiiVec3(1, 0, 0), xiiVec3(0, 0, 1));

  if (cfg.m_Perspective == xiiSceneViewPerspective::Perspective)
  {
    xiiEditorPreferencesUser* pPref = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
    cfg.ApplyPerspectiveSetting(pPref->m_fPerspectiveFieldOfView);
  }
  else
  {
    cfg.ApplyPerspectiveSetting(pref.m_fFov);
  }

  pref.m_vCamDir.NormalizeIfNotZero(xiiVec3(1, 0, 0)).IgnoreResult();
  pref.m_vCamUp.MakeOrthogonalTo(pref.m_vCamDir);
  pref.m_vCamUp.NormalizeIfNotZero(pref.m_vCamDir.GetOrthogonalVector().GetNormalized()).IgnoreResult();

  cfg.m_Camera.LookAt(pref.m_vCamPos, pref.m_vCamPos + pref.m_vCamDir, pref.m_vCamUp);
}

void xiiQtQuadViewWidget::SaveViewConfigs() const
{
  xiiQuadViewPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiQuadViewPreferencesUser>(m_pDocument);
  pPreferences->m_bQuadView                = m_ActiveMainViews.GetCount() == 4;

  SaveViewConfig(m_ViewConfigSingle, pPreferences->m_ViewSingle);
  SaveViewConfig(m_ViewConfigQuad[0], pPreferences->m_ViewQuad0);
  SaveViewConfig(m_ViewConfigQuad[1], pPreferences->m_ViewQuad1);
  SaveViewConfig(m_ViewConfigQuad[2], pPreferences->m_ViewQuad2);
  SaveViewConfig(m_ViewConfigQuad[3], pPreferences->m_ViewQuad3);
}

void xiiQtQuadViewWidget::LoadViewConfigs()
{
  xiiQuadViewPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiQuadViewPreferencesUser>(m_pDocument);

  LoadViewConfig(m_ViewConfigSingle, pPreferences->m_ViewSingle);
  LoadViewConfig(m_ViewConfigQuad[0], pPreferences->m_ViewQuad0);
  LoadViewConfig(m_ViewConfigQuad[1], pPreferences->m_ViewQuad1);
  LoadViewConfig(m_ViewConfigQuad[2], pPreferences->m_ViewQuad2);
  LoadViewConfig(m_ViewConfigQuad[3], pPreferences->m_ViewQuad3);

  CreateViews(pPreferences->m_bQuadView);
}

void xiiQtQuadViewWidget::CreateViews(bool bQuad)
{
  xiiQtScopedUpdatesDisabled _(this);
  for (auto pContainer : m_ActiveMainViews)
  {
    delete pContainer;
  }
  m_ActiveMainViews.Clear();

  if (bQuad)
  {
    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      xiiQtEngineViewWidget*    pViewWidget = m_ViewFactory(m_pWindow, &m_ViewConfigQuad[i]);
      xiiQtViewWidgetContainer* pContainer  = new xiiQtViewWidgetContainer(m_pWindow->GetContainerWindow()->GetDockManager(), m_pWindow, pViewWidget, m_sViewToolBarMapping);
      m_ActiveMainViews.PushBack(pContainer);
      m_pViewLayout->addWidget(pContainer, i / 2, i % 2);
    }
  }
  else
  {
    xiiQtEngineViewWidget*    pViewWidget = m_ViewFactory(m_pWindow, &m_ViewConfigSingle);
    xiiQtViewWidgetContainer* pContainer  = new xiiQtViewWidgetContainer(m_pWindow->GetContainerWindow()->GetDockManager(), m_pWindow, pViewWidget, m_sViewToolBarMapping);
    m_ActiveMainViews.PushBack(pContainer);
    m_pViewLayout->addWidget(pContainer, 0, 0);
  }
}

void xiiQtQuadViewWidget::ToggleViews(QWidget* pView)
{
  xiiQtEngineViewWidget* pViewport = qobject_cast<xiiQtEngineViewWidget*>(pView);
  XII_ASSERT_DEV(pViewport != nullptr, "xiiQtSceneDocumentWindow::ToggleViews must be called with a xiiQtSceneViewWidget as parameter!");
  bool bIsQuad = m_ActiveMainViews.GetCount() == 4;
  if (bIsQuad)
  {
    m_ViewConfigSingle                     = *pViewport->m_pViewConfig;
    m_ViewConfigSingle.m_pLinkedViewConfig = pViewport->m_pViewConfig;
    CreateViews(false);
  }
  else
  {
    if (pViewport->m_pViewConfig->m_pLinkedViewConfig != nullptr)
      *pViewport->m_pViewConfig->m_pLinkedViewConfig = *pViewport->m_pViewConfig;

    CreateViews(true);
  }
}
