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
#include <GraphicsCore/Pipeline/DynamicResolutionSettings.h>
#include <GraphicsCore/Pipeline/RenderPipelineProfileConfig.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
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
  void ReadRenderConfiguration(const xiiDocumentObject& object, xiiRenderConfiguration& configuration)
  {
    xiiAbstractObjectGraph           graph;
    xiiDocumentObjectConverterWriter writer(&graph, object.GetDocumentObjectManager());
    const auto*                      root = writer.AddObjectToGraph(&object, "root");
    xiiRttiConverterContext          context;
    xiiRttiConverterReader           reader(&graph, &context);
    reader.ApplyPropertiesToObject(root, xiiGetStaticRTTI<xiiRenderConfiguration>(), const_cast<xiiRenderConfiguration*>(&configuration));
  }

  void WriteRenderConfiguration(xiiObjectAccessorBase& accessor, const xiiDocumentObject& object, const xiiRenderConfiguration& configuration)
  {
    xiiAbstractObjectGraph           before, after;
    xiiDocumentObjectConverterWriter documentWriter(&before, object.GetDocumentObjectManager());
    documentWriter.AddObjectToGraph(&object, "root");
    xiiRttiConverterContext context;
    xiiRttiConverterWriter  writer(&after, &context, true, true);
    context.RegisterObject(object.GetGuid(), xiiGetStaticRTTI<xiiRenderConfiguration>(), const_cast<xiiRenderConfiguration*>(&configuration));
    writer.AddObjectToGraph(xiiGetStaticRTTI<xiiRenderConfiguration>(), &configuration, "root");
    xiiDeque<xiiAbstractGraphDiffOperation> diff;
    after.CreateDiffWithBaseGraph(before, diff);
    accessor.StartTransaction("Edit Render Settings");
    xiiDocumentObjectConverterReader::ApplyDiffToObject(&accessor, &object, diff);
    accessor.FinishTransaction();
  }

  const char* GetScopeLabel(xiiRenderSettingScope::Enum scope)
  {
    switch (scope)
    {
      case xiiRenderSettingScope::Project:
        return "Project / Platform";
      case xiiRenderSettingScope::Pipeline:
        return "Pipeline";
      case xiiRenderSettingScope::Scene:
        return "Scene";
      case xiiRenderSettingScope::Camera:
        return "Camera";
      case xiiRenderSettingScope::Viewport:
        return "Viewport";
      default:
        return "Default";
    }
  }

  bool EditRenderSettings(QWidget& parent, const QString& title, xiiRenderConfiguration& configuration,
                          const xiiRenderConfiguration& effective, xiiQtEngineViewWidget* liveView = nullptr)
  {
    const auto original         = configuration;
    const auto initialEffective = effective;
    auto       staged           = initialEffective;
    if (staged.m_Subsystems.IsEmpty())
    {
      xiiRTTI::ForEachDerivedType<xiiRenderSubsystemSettings>([&](const xiiRTTI* type) { staged.EditSubsystem(type); },
                                                              xiiRTTI::ForEachOptions::ExcludeNotConcrete);
    }
    staged.ApplyOverrides(original);
    for (auto* subsystem : staged.m_Subsystems)
    {
      const auto* authored = original.FindSubsystem(subsystem->GetDynamicRTTI());
      subsystem->m_Overrides.Clear();
      if (authored)
        subsystem->m_Overrides = authored->m_Overrides;
    }

    xiiViewportRenderSettingsDocument document;
    auto*                             manager  = document.GetObjectManager();
    auto*                             accessor = document.GetObjectAccessor();
    auto*                             object   = manager->CreateObject(xiiGetStaticRTTI<xiiRenderConfiguration>());
    manager->AddObject(object, manager->GetRootObject(), "Children", -1);
    xiiAbstractObjectGraph  graph;
    xiiRttiConverterContext context;
    xiiRttiConverterWriter  writer(&graph, &context, true, true);
    context.RegisterObject(object->GetGuid(), object->GetType(), &staged);
    const auto*                      root = writer.AddObjectToGraph(object->GetType(), &staged, "root");
    xiiDocumentObjectConverterReader reader(&graph, manager, xiiDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
    reader.ApplyPropertiesToObject(root, object);

    QDialog dialog(&parent);
    dialog.setWindowTitle(title);
    auto* layout = new QVBoxLayout(&dialog);
    auto* search = new QLineEdit(&dialog);
    search->setPlaceholderText("Search rendering settings");
    layout->addWidget(search);
    auto* tree = new QTreeWidget(&dialog);
    tree->setHeaderLabels({"Subsystem / Property", "Value", "Override", "Effective source"});
    tree->setAlternatingRowColors(true);
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    layout->addWidget(tree);
    auto* status = new QLabel(&dialog);
    status->setWordWrap(true);
    layout->addWidget(status);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    auto* grid = new xiiQtPropertyGridWidget(&dialog, &document);
    grid->hide();

    struct Row
    {
      QTreeWidgetItem*           m_pItem;
      QCheckBox*                 m_pOverride;
      xiiQtPropertyWidget*       m_pEditor;
      QLabel*                    m_pInherited;
      const xiiDocumentObject*   m_pGroup;
      const xiiDocumentObject*   m_pOwner;
      const xiiAbstractProperty* m_pProperty;
      xiiString                  m_sPath;
    };
    xiiDynamicArray<Row> rows;
    auto                 branch = [&](QTreeWidgetItem* parentItem, const QString& label) {
      const int count = parentItem ? parentItem->childCount() : tree->topLevelItemCount();
      for (int i = 0; i < count; ++i)
      {
        auto* child = parentItem ? parentItem->child(i) : tree->topLevelItem(i);
        if (child->text(0) == label)
          return child;
      }
      auto* item = parentItem ? new QTreeWidgetItem(parentItem, {label}) : new QTreeWidgetItem(tree, {label});
      item->setFirstColumnSpanned(true);
      return item;
    };
    for (const auto* group : object->GetChildren())
    {
      const auto*      type     = group->GetType();
      QTreeWidgetItem* category = nullptr;
      if (const auto* metadata = type->GetAttributeByType<xiiCategoryAttribute>())
      {
        for (const auto& segment : xiiMakeQString(metadata->GetCategory()).split('/', Qt::SkipEmptyParts))
          category = branch(category, segment);
      }
      else
        category = branch(nullptr, xiiMakeQString(type->GetTypeName()));
      xiiDynamicArray<xiiRenderSettingProperty> properties;
      xiiRenderSubsystemSettings::GetProperties(type, properties);
      for (const auto& property : properties)
      {
        const auto               segments   = xiiMakeQString(property.m_sPath).split('/');
        const xiiDocumentObject* owner      = group;
        auto*                    parentItem = category;
        for (qsizetype i = 0; owner && i + 1 < segments.size(); ++i)
        {
          const auto name = segments[i].toUtf8();
          owner           = accessor->GetChildObjectByName(owner, name.constData(), {});
          parentItem      = branch(parentItem, segments[i]);
        }
        if (!owner)
          continue;
        auto* item   = new QTreeWidgetItem(parentItem, {segments.back()});
        auto* editor = xiiQtPropertyGridWidget::CreatePropertyWidget(property.m_pProperty);
        editor->Init(grid, accessor, owner->GetType(), property.m_pProperty);
        xiiHybridArray<xiiPropertySelection, 8> selection;
        selection.PushBack({owner, {}});
        editor->SetSelection(selection);
        auto* cell       = new QWidget(tree);
        auto* cellLayout = new QHBoxLayout(cell);
        cellLayout->setContentsMargins(0, 0, 0, 0);
        auto* inherited = new QLabel(cell);
        inherited->setTextInteractionFlags(Qt::TextSelectableByMouse);
        cellLayout->addWidget(editor);
        cellLayout->addWidget(inherited);
        tree->setItemWidget(item, 1, cell);
        for (const auto* attribute : property.m_pProperty->GetAttributes())
        {
          const auto* binding = xiiDynamicCast<const xiiRenderSettingsBindingAttribute*>(attribute);
          if (!binding)
            continue;
          const xiiBitflags<xiiRenderSettingsChange> changes(static_cast<xiiRenderSettingsChange::Enum>(binding->GetChanges()));
          if (changes.IsSet(xiiRenderSettingsChange::Output))
            item->setToolTip(0, "Requires a compatible output target.");
          else if (changes.IsSet(xiiRenderSettingsChange::Pipeline))
            item->setToolTip(0, "Selects a compiled pipeline variant when the pipeline provides one.");
          else if (changes.IsSet(xiiRenderSettingsChange::Resources))
            item->setToolTip(0, "Recreates affected render resources.");
          else if (changes.IsSet(xiiRenderSettingsChange::History))
            item->setToolTip(0, "Resets affected temporal history.");
        }
        auto* toggle = new QCheckBox(tree);
        toggle->setAccessibleName(QString("Override %1").arg(segments.back()));
        tree->setItemWidget(item, 2, toggle);
        rows.PushBack({item, toggle, editor, inherited, group, owner, property.m_pProperty, property.m_sPath});
        const auto path = property.m_sPath;
        QObject::connect(toggle, &QCheckBox::toggled, &dialog, [&, group, path](bool enabled) {
          xiiDynamicArray<xiiVariant> masks;
          const auto*                 mask = group->GetType()->FindPropertyByName("Overrides");
          if (accessor->GetValues(group, mask, masks).Failed())
            return;
          const xiiVariant value = path;
          const auto       index = masks.IndexOf(value);
          accessor->StartTransaction(enabled ? "Override Render Property" : "Inherit Render Property");
          const auto result = enabled ? accessor->InsertValue(group, mask, value, -1) :
            index != xiiInvalidIndex  ? accessor->RemoveValue(group, mask, index) :
                                        xiiStatus(XII_SUCCESS);
          if (result.Succeeded())
            accessor->FinishTransaction();
          else
            accessor->CancelTransaction();
        });
      }
    }

    auto updateFeedback = [&]() {
      const auto& reported = liveView ? liveView->GetEffectiveRenderConfiguration() : initialEffective;
      for (const auto& row : rows)
      {
        const auto* type    = row.m_pGroup->GetType();
        const auto  scope   = reported.GetSource(type, row.m_sPath);
        const auto* applied = reported.FindSubsystem(type);
        if (!applied)
          applied = staged.FindSubsystem(type);
        xiiVariant value;
        if (applied && xiiRenderSubsystemSettings::ReadProperty(*applied, row.m_sPath, value).Succeeded())
        {
          xiiStringBuilder display;
          if (!row.m_pProperty->GetFlags().IsSet(xiiPropertyFlags::IsEnum) ||
              !xiiReflectionUtils::EnumerationToString(row.m_pProperty->GetSpecificType(), value.ConvertTo<xiiInt64>(), display, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
            display = value.ConvertTo<xiiString>();
          row.m_pInherited->setText(xiiMakeQString(display));
          row.m_pItem->setToolTip(3, xiiMakeQString(display));
        }
        row.m_pItem->setText(3, row.m_pOverride->isChecked() ? "Overridden here" : GetScopeLabel(scope));
      }
      if (!buttons->button(QDialogButtonBox::Ok)->isEnabled())
        status->setText("One or more settings are outside the supported range or conflict with another value.");
      else if (liveView && !liveView->GetRenderConfigurationError().IsEmpty())
        status->setText(xiiMakeQString(liveView->GetRenderConfigurationError()));
      else if (liveView)
      {
        const auto size = liveView->GetRenderResolution();
        status->setText(QString("Live preview · %1 × %2 internal resolution").arg(size.width).arg(size.height));
      }
      else
        status->setText("Enable Override to edit a property. Clear Override to inherit it.");
    };

    xiiRenderConfiguration candidate;
    auto                   refresh = [&]() {
      xiiRenderConfiguration draft;
      ReadRenderConfiguration(*object, draft);
      for (const auto& row : rows)
      {
        const auto*          subsystem  = draft.FindSubsystem(row.m_pGroup->GetType());
        const bool           overridden = subsystem && subsystem->IsOverridden(row.m_sPath);
        const QSignalBlocker block(row.m_pOverride);
        row.m_pOverride->setChecked(overridden);
        row.m_pEditor->setVisible(overridden);
        row.m_pInherited->setVisible(!overridden);
        xiiHybridArray<xiiPropertySelection, 8> selection;
        selection.PushBack({row.m_pOwner, {}});
        row.m_pEditor->SetSelection(selection);
      }
      // Preserve unsupported authored subsystems and persist only explicitly authored values.
      xiiRenderConfiguration overrides;
      overrides.ApplyOverrides(draft);
      for (xiiUInt32 i = overrides.m_Subsystems.GetCount(); i-- > 0;)
      {
        auto* subsystem = overrides.m_Subsystems[i];
        if (subsystem->m_Overrides.IsEmpty())
        {
          subsystem->GetDynamicRTTI()->GetAllocator()->Deallocate(subsystem);
          overrides.m_Subsystems.RemoveAtAndCopy(i);
        }
      }
      const bool valid = draft.IsValid() && overrides.IsValid();
      buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
      if (valid)
      {
        candidate = std::move(overrides);
        if (liveView)
          liveView->m_pViewConfig->m_RenderConfiguration = candidate;
      }
      updateFeedback();
    };
    QObject::connect(search, &QLineEdit::textChanged, &dialog, [tree](const QString& text) {
      const auto filter = [&](auto&& visit, QTreeWidgetItem* item, bool parentMatches) -> bool {
        const bool matches      = parentMatches || item->text(0).contains(text, Qt::CaseInsensitive);
        bool       childMatches = false;
        for (int i = 0; i < item->childCount(); ++i)
          childMatches |= visit(visit, item->child(i), matches);
        item->setHidden(!matches && !childMatches);
        if (!text.isEmpty() && childMatches)
          item->setExpanded(true);
        return matches || childMatches;
      };
      for (int i = 0; i < tree->topLevelItemCount(); ++i)
        filter(filter, tree->topLevelItem(i), false);
    });

    auto* undo = new QShortcut(QKeySequence::Undo, &dialog);
    auto* redo = new QShortcut(QKeySequence::Redo, &dialog);
    QObject::connect(undo, &QShortcut::activated, &dialog, [&] { if (document.GetCommandHistory()->CanUndo()) document.GetCommandHistory()->Undo().IgnoreResult(); });
    QObject::connect(redo, &QShortcut::activated, &dialog, [&] { if (document.GetCommandHistory()->CanRedo()) document.GetCommandHistory()->Redo().IgnoreResult(); });
    QTimer update;
    update.setSingleShot(true);
    QObject::connect(&update, &QTimer::timeout, &dialog, refresh);
    auto   subscription = manager->m_PropertyEvents.AddEventHandler([&](const xiiDocumentObjectPropertyEvent&) { update.start(0); });
    QTimer feedback;
    QObject::connect(&feedback, &QTimer::timeout, &dialog, updateFeedback);
    if (liveView)
      feedback.start(250);
    tree->expandToDepth(0);
    refresh();
    dialog.resize(930, 640);
    const bool accepted = dialog.exec() == QDialog::Accepted;
    manager->m_PropertyEvents.RemoveEventHandler(subscription);
    for (const auto& row : rows)
      row.m_pEditor->PrepareToDie();
    configuration = accepted ? candidate : original;
    return accepted;
  }

  void EditViewportRenderSettings(xiiQtEngineViewWidget& view)
  {
    auto& configuration = view.m_pViewConfig->m_RenderConfiguration;
    EditRenderSettings(view, "Viewport Render Settings", configuration, view.GetEffectiveRenderConfiguration(), &view);
  }

  class xiiQtRenderConfigurationPropertyWidget final : public xiiQtPropertyWidget
  {
    void OnInit() override
    {
      auto* layout = new QHBoxLayout(this);
      layout->setContentsMargins(0, 0, 0, 0);
      m_pEdit = new QPushButton("Edit Render Settings…", this);
      layout->addWidget(m_pEdit);
      connect(m_pEdit, &QPushButton::clicked, this, [this] {
        if (m_Items.GetCount() != 1)
          return;
        const auto* object = m_pObjectAccessor->GetChildObjectByName(m_Items[0].m_pObject, m_pProp->GetPropertyName(), {});
        if (!object)
          return;
        xiiRenderConfiguration configuration, effective;
        ReadRenderConfiguration(*object, configuration);
        for (auto* widget : QApplication::allWidgets())
        {
          const auto* view = qobject_cast<xiiQtEngineViewWidget*>(widget);
          if (view && view->GetDocumentWindow()->GetDocument() == m_pGrid->GetDocument() && !view->GetEffectiveRenderConfiguration().m_Subsystems.IsEmpty())
          {
            effective = view->GetEffectiveRenderConfiguration();
            break;
          }
        }
        const auto before = configuration;
        if (EditRenderSettings(*this, "Render Settings", configuration, effective) && configuration != before)
          WriteRenderConfiguration(*m_pObjectAccessor, *object, configuration);
      });
    }
    void         DoPrepareToDie() override {}
    QPushButton* m_pEdit = nullptr;
  };

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
    {
      // Keep the existing per-view preference and apply this action at view scope.
      view->m_pViewConfig->m_RenderConfiguration.SetOverride<xiiDynamicResolutionSettings>("RenderScale", fScale).AssertSuccess();
    }
  }

  float GetReportedRenderScale(const xiiQtEngineViewWidget& view)
  {
    const auto* settings = view.GetEffectiveRenderConfiguration().FindSubsystem<xiiDynamicResolutionSettings>();
    if (!settings)
      settings = view.m_pViewConfig->m_RenderConfiguration.FindSubsystem<xiiDynamicResolutionSettings>();
    return settings ? settings->m_fRenderScale : 1.0f;
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
                     GetReportedRenderScale(*view) * 100.0f, size.width, size.height, view->m_pViewConfig->m_bShowFrameStats, view->m_pViewConfig->m_bVSync);
      }
  }
  xiiConsoleFunction<void(xiiUInt32, float)> s_ViewScale("View_RenderScale", "(viewID, scale): scale 0.1-1.0; View_List() lists explicit targets.", SetViewScale);
  xiiConsoleFunction<void(xiiUInt32, bool)>  s_ViewStats("View_ShowFPS", "(viewID, enabled): changes this viewport's persisted overlay preference.", SetViewStats);
  xiiConsoleFunction<void(xiiUInt32, bool)>  s_ViewVSync("View_VSync", "(viewID, enabled): changes this viewport window's presentation setting.", SetViewVSync);
  xiiConsoleFunction<void()>                 s_ViewList("View_List", "(): logs open viewport IDs and their rendering settings.", ListRenderingViews);
} // namespace

void xiiViewActions::RegisterRenderSettingsWidget()
{
  xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiRenderConfiguration>(),
                                                        [](const xiiRTTI*) -> xiiQtPropertyWidget* { return new xiiQtRenderConfigurationPropertyWidget(); });
}

void xiiViewActions::UnregisterRenderSettingsWidget()
{
  xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiRenderConfiguration>());
}

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
  label.SetFormat("Render scale: {}%% ({} x {})...", xiiArgF(GetReportedRenderScale(*view) * 100.0f, 0), size.width, size.height);
  scale.m_sDisplay     = label;
  scale.m_UserValue    = 1;
  auto& vsync          = out_entries.ExpandAndGetRef();
  vsync.m_sDisplay     = "VSync";
  vsync.m_CheckState   = config.m_bVSync ? Item::CheckMark::Checked : Item::CheckMark::Unchecked;
  vsync.m_UserValue    = 2;
  auto& limit          = out_entries.ExpandAndGetRef();
  limit.m_sDisplay     = "Application frame limit...";
  limit.m_UserValue    = 3;
  auto& quality        = out_entries.ExpandAndGetRef();
  quality.m_sDisplay   = "Editor scene pipeline quality...";
  quality.m_UserValue  = 4;
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
      const double percent = QInputDialog::getDouble(view, "Render scale", "Internal scene resolution (%)", GetReportedRenderScale(*view) * 100.0, 10, 100, 0, &accepted);
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
