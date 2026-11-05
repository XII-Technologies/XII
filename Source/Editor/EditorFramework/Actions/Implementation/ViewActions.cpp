/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Console/ConsoleFunction.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/Panels/CVarPanel/CVarPanel.moc.h>
#include <GraphicsCore/Pipeline/RenderPipelineProfileConfig.h>
#include <QApplication>
#include <QInputDialog>

xiiActionDescriptorHandle xiiViewActions::s_hRenderingControls;
xiiActionDescriptorHandle xiiViewActions::s_hRenderMode;
xiiActionDescriptorHandle xiiViewActions::s_hPerspective;
xiiActionDescriptorHandle xiiViewActions::s_hNavigationGizmo;
xiiActionDescriptorHandle xiiViewActions::s_hOptions;
xiiActionDescriptorHandle xiiViewActions::s_hActivateRemoteProcess;
xiiActionDescriptorHandle xiiViewActions::s_hLinkDeviceCamera;

void xiiViewActions::RegisterActions()
{
  s_hRenderingControls     = XII_REGISTER_DYNAMIC_MENU("View.Rendering", xiiRenderingControlsAction, ":/EditorFramework/Icons/RenderMode.svg");
  s_hOptions               = XII_REGISTER_MENU("View.Options");
  s_hNavigationGizmo       = XII_REGISTER_ACTION_1("View.NavigationGizmo", xiiActionScope::Window, "View", "", xiiViewAction, xiiViewAction::ButtonType::NavigationGizmo);
  s_hRenderMode            = XII_REGISTER_DYNAMIC_MENU("View.RenderMode", xiiRenderModeAction, ":/EditorFramework/Icons/RenderMode.svg");
  s_hPerspective           = XII_REGISTER_DYNAMIC_MENU("View.RenderPerspective", xiiPerspectiveAction, ":/EditorFramework/Icons/Perspective.svg");
  s_hActivateRemoteProcess = XII_REGISTER_ACTION_1("View.ActivateRemoteProcess", xiiActionScope::Window, "View", "", xiiViewAction, xiiViewAction::ButtonType::ActivateRemoteProcess);
  s_hLinkDeviceCamera      = XII_REGISTER_ACTION_1("View.LinkDeviceCamera", xiiActionScope::Window, "View", "", xiiViewAction, xiiViewAction::ButtonType::LinkDeviceCamera);
}

void xiiViewActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hRenderingControls);
  xiiActionManager::UnregisterAction(s_hOptions);
  xiiActionManager::UnregisterAction(s_hRenderMode);
  xiiActionManager::UnregisterAction(s_hPerspective);
  xiiActionManager::UnregisterAction(s_hNavigationGizmo);
  xiiActionManager::UnregisterAction(s_hActivateRemoteProcess);
  xiiActionManager::UnregisterAction(s_hLinkDeviceCamera);
}

void xiiViewActions::MapToolbarActions(xiiStringView sMapping, xiiUInt32 uiFlags)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  if (uiFlags & Flags::PerspectiveMode)
    pMap->MapAction(s_hPerspective, "", 1.0f);

  if (uiFlags & Flags::RenderMode)
  {
    pMap->MapAction(s_hRenderMode, "", 2.0f);
  }

  if (uiFlags & (Flags::RenderMode | Flags::ActivateRemoteProcess))
    pMap->MapAction(s_hOptions, "", 3.0f);
  if (uiFlags & Flags::RenderMode)
  {
    pMap->MapAction(s_hNavigationGizmo, "View.Options", 1.0f);
    pMap->MapAction(s_hRenderingControls, "View.Options", 1.5f);
  }

  if (uiFlags & Flags::ActivateRemoteProcess)
  {
    pMap->MapAction(s_hActivateRemoteProcess, "View.Options", 2.0f);
    pMap->MapAction(s_hLinkDeviceCamera, "View.Options", 3.0f);
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

void xiiRenderModeAction::GetEntries(xiiDynamicArray<Item>& out_entries)
{
  xiiEnumerationMenuAction::GetEntries(out_entries);
  for (xiiUInt32 i = out_entries.GetCount(); i > 0; --i)
    if (const auto mode = static_cast<xiiViewRenderMode::Enum>(out_entries[i - 1].m_UserValue.ConvertTo<xiiInt64>());
        !xiiViewRenderMode::IsSupportedVisualizationMode(mode) ||
        (xiiViewRenderMode::IsRayTracingVisualizationMode(mode) && !xiiEditorEngineProcessConnection::GetSingleton()->SupportsRayTracingModes()))
      out_entries.RemoveAtAndCopy(i - 1);
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

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPerspectiveAction, 1, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

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

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiViewAction::xiiViewAction(const xiiActionContext& context, const char* szName, ButtonType button) :
  xiiButtonAction(context, szName, false, "")
{
  m_ButtonType                 = button;
  xiiQtEngineViewWidget* pView = qobject_cast<xiiQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case xiiViewAction::ButtonType::ActivateRemoteProcess:
      SetIconPath(":/EditorFramework/Icons/SwitchToRemoteProcess.svg");
      break;
    case xiiViewAction::ButtonType::LinkDeviceCamera:
      SetIconPath(":/EditorFramework/Icons/LinkDeviceCamera.svg");
      SetCheckable(true);
      SetChecked(pView->m_pViewConfig->m_bUseCameraTransformOnDevice);
      break;
    case xiiViewAction::ButtonType::NavigationGizmo:
      SetIconPath(":/EditorFramework/Icons/ViewportNavigation.svg");
      SetCheckable(true);
      SetChecked(pView->m_pViewConfig->m_bShowNavigationGizmo);
      SetVisible(pView->HasNavigationGizmo());
      break;
  }
}

xiiViewAction::~xiiViewAction() = default;

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

    case xiiViewAction::ButtonType::NavigationGizmo:
    {
      pView->SetNavigationGizmoEnabled(!pView->m_pViewConfig->m_bShowNavigationGizmo);
      SetChecked(pView->m_pViewConfig->m_bShowNavigationGizmo);
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

namespace
{
  xiiQtEngineViewWidget* FindRenderingView(xiiUInt32 uiViewID)
  {
    for (QWidget* widget : QApplication::allWidgets())
      if (auto* view = qobject_cast<xiiQtEngineViewWidget*>(widget); view && view->GetViewID() == uiViewID)
        return view;
    xiiLog::Error("No editor viewport with ID {}. Use View_List() to list open viewports.", uiViewID);
    return nullptr;
  }

  void SetViewScale(xiiUInt32 uiViewID, float fScale)
  {
    if (!xiiMath::IsFinite(fScale) || fScale < 0.1f || fScale > 1.0f)
    {
      xiiLog::Error("Render scale must be finite and between 0.1 and 1.0.");
      return;
    }
    if (auto* view = FindRenderingView(uiViewID))
      view->m_pViewConfig->m_fRenderScale = fScale;
  }
  void SetViewStats(xiiUInt32 uiViewID, bool bEnabled)
  {
    if (auto* view = FindRenderingView(uiViewID))
      view->m_pViewConfig->m_bShowFrameStats = bEnabled;
  }
  void SetViewVSync(xiiUInt32 uiViewID, bool bEnabled)
  {
    if (auto* view = FindRenderingView(uiViewID))
      view->m_pViewConfig->m_bVSync = bEnabled;
  }
  void ListRenderingViews()
  {
    for (QWidget* widget : QApplication::allWidgets())
      if (auto* view = qobject_cast<xiiQtEngineViewWidget*>(widget))
      {
        const auto size = view->GetRenderResolution();
        xiiLog::Info("View {}: {} | scale {}%%, {} x {}, Show FPS {}, VSync {}", view->GetViewID(), view->GetDocumentWindow()->GetDocument()->GetDocumentPath(),
                     view->m_pViewConfig->m_fRenderScale * 100.0f, size.width, size.height, view->m_pViewConfig->m_bShowFrameStats, view->m_pViewConfig->m_bVSync);
      }
  }
  xiiConsoleFunction<void(xiiUInt32, float)> s_ViewScale("View_RenderScale", "(viewID, scale): scale 0.1-1.0; View_List() lists explicit targets.", SetViewScale);
  xiiConsoleFunction<void(xiiUInt32, bool)>  s_ViewStats("View_ShowFPS", "(viewID, enabled): changes this viewport's persisted overlay preference.", SetViewStats);
  xiiConsoleFunction<void(xiiUInt32, bool)>  s_ViewVSync("View_VSync", "(viewID, enabled): changes this viewport window's presentation setting.", SetViewVSync);
  xiiConsoleFunction<void()>                 s_ViewList("View_List", "(): logs open viewport IDs and their rendering settings.", ListRenderingViews);
} // namespace

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderingControlsAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRenderingControlsAction::xiiRenderingControlsAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
  xiiDynamicMenuAction(context, szName, szIconPath)
{
}

void xiiRenderingControlsAction::GetEntries(xiiDynamicArray<Item>& out_entries)
{
  out_entries.Clear();
  const auto* view       = qobject_cast<xiiQtEngineViewWidget*>(m_Context.m_pWindow);
  const auto& config     = *view->m_pViewConfig;
  auto&       fps        = out_entries.ExpandAndGetRef();
  fps.m_sDisplay         = "Show FPS";
  fps.m_CheckState       = config.m_bShowFrameStats ? Item::CheckMark::Checked : Item::CheckMark::Unchecked;
  fps.m_UserValue        = 0;
  auto&            scale = out_entries.ExpandAndGetRef();
  const auto       size  = view->GetRenderResolution();
  xiiStringBuilder label;
  label.SetFormat("Render scale: {}%% ({} x {})...", xiiArgF(config.m_fRenderScale * 100.0f, 0), size.width, size.height);
  scale.m_sDisplay    = label;
  scale.m_UserValue   = 1;
  auto& vsync         = out_entries.ExpandAndGetRef();
  vsync.m_sDisplay    = "VSync";
  vsync.m_CheckState  = config.m_bVSync ? Item::CheckMark::Checked : Item::CheckMark::Unchecked;
  vsync.m_UserValue   = 2;
  auto& limit         = out_entries.ExpandAndGetRef();
  limit.m_sDisplay    = "Application frame limit...";
  limit.m_UserValue   = 3;
  auto& quality       = out_entries.ExpandAndGetRef();
  quality.m_sDisplay  = "Editor scene pipeline quality...";
  quality.m_UserValue = 4;
}

void xiiRenderingControlsAction::Execute(const xiiVariant& value)
{
  auto* view     = qobject_cast<xiiQtEngineViewWidget*>(m_Context.m_pWindow);
  bool  accepted = false;
  switch (value.ConvertTo<xiiInt32>())
  {
    case 0:
      SetViewStats(view->GetViewID(), !view->m_pViewConfig->m_bShowFrameStats);
      break;
    case 1:
    {
      const double percent = QInputDialog::getDouble(view, "Render scale", "Internal scene resolution (%)", view->m_pViewConfig->m_fRenderScale * 100.0, 10, 100, 0, &accepted);
      if (accepted)
        SetViewScale(view->GetViewID(), static_cast<float>(percent / 100.0));
      break;
    }
    case 2:
      SetViewVSync(view->GetViewID(), !view->m_pViewConfig->m_bVSync);
      break;
    case 3:
    {
      const auto* panel   = xiiQtCVarPanel::GetSingleton();
      const auto* current = panel ? panel->FindEngineCVar("App.FrameLimit") : nullptr;
      if (!current)
      {
        xiiLog::Warning("The engine has not reported App.FrameLimit yet. Retry once the engine process is ready.");
        return;
      }
      const int limit = QInputDialog::getInt(view, "Application frame limit", "Engine application FPS (0 = unlimited; editor redraw preferences also apply)", xiiMath::Clamp(current->m_iValue, 0, 1000), 0, 1000, 1, &accepted);
      if (accepted)
      {
        xiiChangeCVarMsgToEngine message;
        message.m_sCVarName = "App.FrameLimit";
        message.m_NewValue  = limit;
        xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&message);
      }
      break;
    }
    case 4:
    {
      const auto asset = xiiAssetCurator::GetSingleton()->FindSubAsset(xiiRenderPipelineProfileConfig::s_szEditorPipeline);
      if (asset)
        xiiQtEditorApp::GetSingleton()->OpenDocumentQueued(asset->m_pAssetInfo->m_Path.GetAbsolutePath());
      else
        xiiLog::Warning("The Editor scene pipeline asset is unavailable in this project.");
      break;
    }
  }
  TriggerUpdate();
}
