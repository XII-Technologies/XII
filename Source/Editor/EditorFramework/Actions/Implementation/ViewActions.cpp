#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

xiiActionDescriptorHandle xiiViewActions::s_hRenderMode;
xiiActionDescriptorHandle xiiViewActions::s_hPerspective;
xiiActionDescriptorHandle xiiViewActions::s_hActivateRemoteProcess;
xiiActionDescriptorHandle xiiViewActions::s_hLinkDeviceCamera;


void xiiViewActions::RegisterActions()
{
  s_hRenderMode            = XII_REGISTER_DYNAMIC_MENU("View.RenderMode", xiiRenderModeAction, ":/EditorFramework/Icons/RenderMode.png");
  s_hPerspective           = XII_REGISTER_DYNAMIC_MENU("View.RenderPerspective", xiiPerspectiveAction, ":/EditorFramework/Icons/Perspective.png");
  s_hActivateRemoteProcess = XII_REGISTER_ACTION_1("View.ActivateRemoteProcess", xiiActionScope::Window, "View", "", xiiViewAction, xiiViewAction::ButtonType::ActivateRemoteProcess);
  s_hLinkDeviceCamera      = XII_REGISTER_ACTION_1("View.LinkDeviceCamera", xiiActionScope::Window, "View", "", xiiViewAction, xiiViewAction::ButtonType::LinkDeviceCamera);
}

void xiiViewActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hRenderMode);
  xiiActionManager::UnregisterAction(s_hPerspective);
  xiiActionManager::UnregisterAction(s_hActivateRemoteProcess);
  xiiActionManager::UnregisterAction(s_hLinkDeviceCamera);
}

void xiiViewActions::MapActions(const char* szMapping, const char* szPath, xiiUInt32 flags)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  if (flags & Flags::PerspectiveMode)
    pMap->MapAction(s_hPerspective, szPath, 1.0f);

  if (flags & Flags::RenderMode)
    pMap->MapAction(s_hRenderMode, szPath, 2.0f);

  if (flags & Flags::ActivateRemoteProcess)
  {
    pMap->MapAction(s_hActivateRemoteProcess, szPath, 4.0f);
    pMap->MapAction(s_hLinkDeviceCamera, szPath, 5.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// xiiRenderModeAction
////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderModeAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRenderModeAction::xiiRenderModeAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
  xiiEnumerationMenuAction(context, szName, szIconPath)
{
  xiiQtEngineViewWidget* pView = qobject_cast<xiiQtEngineViewWidget*>(context.m_pWindow);
  XII_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'xiiQtEngineViewWidget'!");
  InitEnumerationType(xiiGetStaticRTTI<xiiViewRenderMode>());
}

xiiInt64 xiiRenderModeAction::GetValue() const
{
  xiiQtEngineViewWidget* pView = qobject_cast<xiiQtEngineViewWidget*>(m_Context.m_pWindow);
  return (xiiInt64)pView->m_pViewConfig->m_RenderMode;
}

void xiiRenderModeAction::Execute(const xiiVariant& value)
{
  xiiQtEngineViewWidget* pView       = qobject_cast<xiiQtEngineViewWidget*>(m_Context.m_pWindow);
  pView->m_pViewConfig->m_RenderMode = (xiiViewRenderMode::Enum)value.ConvertTo<xiiInt64>();
  TriggerUpdate();
}

////////////////////////////////////////////////////////////////////////
// xiiPerspectiveAction
////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPerspectiveAction, 1, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiPerspectiveAction::xiiPerspectiveAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
  xiiEnumerationMenuAction(context, szName, szIconPath)
{
  xiiQtEngineViewWidget* pView = qobject_cast<xiiQtEngineViewWidget*>(context.m_pWindow);
  XII_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'xiiQtEngineViewWidget'!");
  InitEnumerationType(xiiGetStaticRTTI<xiiSceneViewPerspective>());
}

xiiInt64 xiiPerspectiveAction::GetValue() const
{
  xiiQtEngineViewWidget* pView = qobject_cast<xiiQtEngineViewWidget*>(m_Context.m_pWindow);
  return (xiiInt64)pView->m_pViewConfig->m_Perspective;
}

void xiiPerspectiveAction::Execute(const xiiVariant& value)
{
  xiiQtEngineViewWidget* pView    = qobject_cast<xiiQtEngineViewWidget*>(m_Context.m_pWindow);
  auto                   newValue = (xiiSceneViewPerspective::Enum)value.ConvertTo<xiiInt64>();

  if (pView->m_pViewConfig->m_Perspective != newValue)
  {
    pView->m_pViewConfig->m_Perspective = newValue;
    pView->m_pViewConfig->ApplyPerspectiveSetting();
    TriggerUpdate();
  }
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewAction, 1, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiViewAction::xiiViewAction(const xiiActionContext& context, const char* szName, ButtonType button) :
  xiiButtonAction(context, szName, false, "")
{
  m_ButtonType                 = button;
  xiiQtEngineViewWidget* pView = qobject_cast<xiiQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case xiiViewAction::ButtonType::ActivateRemoteProcess:
      SetIconPath(":/EditorFramework/Icons/SwitchToRemoteProcess16.png");
      break;
    case xiiViewAction::ButtonType::LinkDeviceCamera:
      SetIconPath(":/EditorFramework/Icons/LinkDeviceCamera16.png");
      SetCheckable(true);
      SetChecked(pView->m_pViewConfig->m_bUseCameraTransformOnDevice);
      break;
  }
}

xiiViewAction::~xiiViewAction() {}

void xiiViewAction::Execute(const xiiVariant& value)
{
  xiiQtEngineViewWidget* pView = qobject_cast<xiiQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case xiiViewAction::ButtonType::ActivateRemoteProcess:
    {
      xiiEditorEngineProcessConnection::GetSingleton()->ActivateRemoteProcess(xiiDynamicCast<xiiAssetDocument*>(m_Context.m_pDocument), pView->GetViewID());
    }
    break;

    case xiiViewAction::ButtonType::LinkDeviceCamera:
    {
      pView->m_pViewConfig->m_bUseCameraTransformOnDevice = !pView->m_pViewConfig->m_bUseCameraTransformOnDevice;
      SetChecked(pView->m_pViewConfig->m_bUseCameraTransformOnDevice);
      xiiEditorEngineProcessConnection::GetSingleton()->ActivateRemoteProcess(xiiDynamicCast<xiiAssetDocument*>(m_Context.m_pDocument), pView->GetViewID());
    }
    break;
  }
}
