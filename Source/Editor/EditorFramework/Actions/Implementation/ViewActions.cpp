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
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Render/RenderWorldSettingsComponent.h>
#include <GraphicsCore/Pipeline/RenderPipelineProfileConfig.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class xiiViewportRenderSettingsDocument final : public xiiDocument
{
public:
  xiiViewportRenderSettingsDocument() :
    xiiDocument("<viewport render settings>", XII_DEFAULT_NEW(xiiDocumentObjectManager))
  {
  }

  xiiDocumentInfo* CreateDocumentInfo() override { return XII_DEFAULT_NEW(xiiDocumentInfo); }
};

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
  const xiiDocumentObject* FindSceneRenderSettings(const xiiDocumentObjectManager& manager)
  {
    xiiHybridArray<const xiiDocumentObject*, 32> pending;
    pending.PushBack(manager.GetRootObject());
    const xiiDocumentObject* pSettings = nullptr;
    while (!pending.IsEmpty())
    {
      const auto* pObject = pending.PeekBack();
      pending.PopBack();
      if (pObject->GetType()->IsDerivedFrom<xiiRenderWorldSettingsComponent>())
      {
        if (pSettings)
          return nullptr;
        pSettings = pObject;
      }
      for (const auto* pChild : pObject->GetChildren())
        pending.PushBack(pChild);
    }
    return pSettings;
  }

  xiiStatus CopyRenderOverrides(xiiObjectAccessorBase& target, const xiiDocumentObject* pTarget, const xiiDocumentObject* pSource)
  {
    if (!pSource || !pTarget || pSource->GetType() != pTarget->GetType())
      return xiiStatus("The scene render settings no longer match the viewport settings.");
    for (const auto* pProperty : pSource->GetType()->GetProperties())
    {
      const auto value = pSource->GetTypeAccessor().GetValue(pProperty->GetPropertyName());
      if (pProperty->GetFlags().IsSet(xiiPropertyFlags::Class))
      {
        if (!value.IsA<xiiUuid>())
          return xiiStatus("An embedded render settings object is missing.");
        const auto* pChild = pSource->GetDocumentObjectManager()->GetObject(value.Get<xiiUuid>());
        XII_SUCCEED_OR_RETURN(CopyRenderOverrides(target, target.GetChildObjectByName(pTarget, pProperty->GetPropertyName(), {}), pChild));
      }
      else if (value != xiiReflectionUtils::GetDefaultValue(pProperty))
      {
        XII_SUCCEED_OR_RETURN(target.SetValue(pTarget, pProperty, value));
      }
    }
    return XII_SUCCESS;
  }

  xiiStatus SaveProjectRenderOverrides(const xiiRenderConfiguration& overrides)
  {
    auto*      pCurator = xiiAssetCurator::GetSingleton();
    const auto uiActive = pCurator->GetActiveAssetProfileIndex();
    if (pCurator->GetNumAssetProfiles() <= uiActive)
      return xiiStatus("There is no active project profile.");
    auto* pProfile = xiiReflectionSerializer::Clone(pCurator->GetAssetProfile(uiActive));
    XII_SCOPE_EXIT(XII_DEFAULT_DELETE(pProfile));
    auto* pConfig = pProfile->GetTypeConfig<xiiRenderPipelineProfileConfig>();
    if (!pConfig)
      return xiiStatus("The active project profile has no render pipeline configuration.");
    pConfig->m_RenderConfiguration.ApplyOverrides(overrides);
    if (!pConfig->m_RenderConfiguration.IsValid())
      return xiiStatus("The combined project render configuration is invalid.");
    xiiHybridArray<const xiiPlatformProfile*, 8> profiles;
    for (xiiUInt32 i = 0; i < pCurator->GetNumAssetProfiles(); ++i)
      profiles.PushBack(i == uiActive ? pProfile : pCurator->GetAssetProfile(i));
    return pCurator->ApplyAssetProfiles(profiles, uiActive);
  }

  void EditViewportRenderSettings(xiiQtEngineViewWidget& view)
  {
    auto&                             configuration = view.m_pViewConfig->m_RenderConfiguration;
    auto                              original      = configuration;
    xiiViewportRenderSettingsDocument document;
    auto*                             pManager = document.GetObjectManager();
    const auto*                       pType    = xiiGetStaticRTTI<xiiRenderConfiguration>();
    auto*                             pObject  = pManager->CreateObject(pType);
    pManager->AddObject(pObject, pManager->GetRootObject(), "Children", -1);
    {
      xiiAbstractObjectGraph  graph;
      xiiRttiConverterContext context;
      xiiRttiConverterWriter  writer(&graph, &context, true, true);
      context.RegisterObject(pObject->GetGuid(), pType, &original);
      const auto*                      pNode = writer.AddObjectToGraph(pType, &original, "root");
      xiiDocumentObjectConverterReader reader(&graph, pManager, xiiDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
      reader.ApplyPropertiesToObject(pNode, pObject);
    }

    QDialog dialog(&view);
    dialog.setWindowTitle("Viewport Render Settings");
    auto* pLayout = new QVBoxLayout(&dialog);
    auto* pInfo   = new QLabel("Viewport overrides preview live. Reset a field to its default to inherit. Bloom strength 0 disables bloom.", &dialog);
    pInfo->setWordWrap(true);
    pLayout->addWidget(pInfo);
    auto* pGrid = new xiiQtPropertyGridWidget(&dialog, &document);
    document.GetSelectionManager()->SetSelection(pObject);
    pLayout->addWidget(pGrid);
    auto* pStatus = new QLabel(&dialog);
    pStatus->setWordWrap(true);
    pStatus->setTextFormat(Qt::PlainText);
    pLayout->addWidget(pStatus);
    auto* pButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    auto*      pSaveScene        = pButtons->addButton("Apply to Scene", QDialogButtonBox::ActionRole);
    auto*      pSaveProject      = pButtons->addButton("Save to Project Profile", QDialogButtonBox::ActionRole);
    auto*      pSceneDocument    = view.GetDocumentWindow()->GetDocument();
    const bool bHasSceneSettings = FindSceneRenderSettings(*pSceneDocument->GetObjectManager()) != nullptr;
    pSaveScene->setEnabled(bHasSceneSettings);
    pSaveScene->setToolTip("Apply explicit overrides to the scene's Render World Settings component. Save the scene to persist.");
    pLayout->addWidget(pButtons);
    QObject::connect(pButtons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(pButtons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    const auto finishSave = [&](const xiiStatus& status) {
      if (status.Failed())
      {
        QMessageBox::warning(&dialog, "Render Settings", QString::fromUtf8(status.GetMessageString().GetData()));
        return;
      }
      configuration = {};
      dialog.accept();
    };
    QObject::connect(pSaveScene, &QPushButton::clicked, &dialog, [&] {
      auto*       pAccessor = pSceneDocument->GetObjectAccessor();
      const auto* pSettings = FindSceneRenderSettings(*pSceneDocument->GetObjectManager());
      if (!pSettings)
      {
        finishSave(xiiStatus("The scene needs a single Render World Settings component."));
        return;
      }
      const auto* pTarget = pAccessor->GetChildObjectByName(pSettings, "RenderConfiguration", {});
      pAccessor->StartTransaction("Apply Viewport Render Settings");
      const auto status = CopyRenderOverrides(*pAccessor, pTarget, pObject);
      if (status.Succeeded())
        pAccessor->FinishTransaction();
      else
        pAccessor->CancelTransaction();
      finishSave(status);
    });
    QObject::connect(pSaveProject, &QPushButton::clicked, &dialog, [&] { finishSave(SaveProjectRenderOverrides(configuration)); });

    QTimer     statusTimer;
    const auto updateStatus = [&] {
      const auto error = view.GetRenderConfigurationError();
      if (!error.IsEmpty())
      {
        pStatus->setText(QString::fromUtf8(error.GetStartPointer(), error.GetElementCount()));
        return;
      }
      const auto&      effective = view.GetEffectiveRenderConfiguration();
      const auto       size      = view.GetRenderResolution();
      xiiStringBuilder mode;
      xiiReflectionUtils::EnumerationToString(effective.m_GlobalIllumination, mode, xiiReflectionUtils::EnumConversionMode::ValueNameOnly);
      if (effective.m_GlobalIllumination == xiiRenderGlobalIllumination::Inherit)
        mode = "Pipeline Default";
      xiiStringBuilder status;
      status.SetFormat("Effective GI {} | Internal resolution {} × {}", mode, size.width, size.height);
      pStatus->setText(QString::fromUtf8(status.GetData()));
    };
    QObject::connect(&statusTimer, &QTimer::timeout, &dialog, updateStatus);
    updateStatus();
    statusTimer.start(250);

    xiiEventSubscriptionID subscription = pManager->m_PropertyEvents.AddEventHandler([&](const xiiDocumentObjectPropertyEvent&) {
      xiiAbstractObjectGraph           graph;
      xiiDocumentObjectConverterWriter writer(&graph, pManager);
      const auto*                      pNode = writer.AddObjectToGraph(pObject, "root");
      xiiRttiConverterContext          context;
      xiiRttiConverterReader           reader(&graph, &context);
      xiiRenderConfiguration           preview;
      reader.ApplyPropertiesToObject(pNode, pType, &preview);
      const bool bValid = preview.IsValid();
      pButtons->button(QDialogButtonBox::Ok)->setEnabled(bValid);
      pSaveScene->setEnabled(bValid && bHasSceneSettings);
      pSaveProject->setEnabled(bValid);
      if (bValid)
        configuration = preview;
    });
    dialog.resize(520, 500);
    if (dialog.exec() != QDialog::Accepted)
      configuration = original;
    pManager->m_PropertyEvents.RemoveEventHandler(subscription);
  }

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
  auto& settings       = out_entries.ExpandAndGetRef();
  settings.m_sDisplay  = "Viewport Render Settings...";
  settings.m_UserValue = 5;
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
    case 5:
      EditViewportRenderSettings(*view);
      break;
  }
  TriggerUpdate();
}
