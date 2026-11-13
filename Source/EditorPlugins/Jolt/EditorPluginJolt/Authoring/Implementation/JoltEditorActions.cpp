/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorPluginJolt/Authoring/JoltEditorActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EnginePluginJolt/JoltAuthoring.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <JoltPlugin/Joints/JoltJointComponent.h>
#include <JoltPlugin/System/JoltCollisionLayerConfig.h>
#include <JoltPlugin/System/JoltDebugDraw.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltEditorAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  void CollectJoints(const xiiDocumentObject* object, xiiSet<const xiiDocumentObject*>& joints)
  {
    const auto* type = object->GetTypeAccessor().GetType();
    if (type->IsDerivedFrom<xiiJoltJointComponent>())
      joints.Insert(object);
    for (const auto* child : object->GetChildren())
      CollectJoints(child, joints);
  }

  xiiStatus AlignJointFrames(xiiGameObjectDocument& document)
  {
    const auto* scene = xiiDynamicCast<const xiiSceneDocument*>(document.GetMainDocument());
    if (document.IsReadOnly() || (scene && scene->GetGameMode() != GameMode::Off))
      return xiiStatus("Joint frame authoring is available only outside simulation in writable scenes.");
    xiiSet<const xiiDocumentObject*> joints;
    for (const auto* selected : document.GetSelectionManager()->GetSelection())
      CollectJoints(selected, joints);
    if (joints.IsEmpty())
      return xiiStatus("Select a Jolt joint or an object containing joints.");
    auto* accessor = document.GetObjectAccessor();
    accessor->StartTransaction("Align Jolt Joint Frames");
    for (const auto* joint : joints)
    {
      xiiTransform global;
      const auto   fail = [&]() {
        accessor->CancelTransaction();
        return xiiStatus("Assign distinct, valid actor references before aligning the joint frames.");
      };
      if (document.ComputeObjectTransformation(joint, global).Failed())
        return fail();
      global.m_vScale  = xiiVec3(1.0f);
      const auto* type = joint->GetTypeAccessor().GetType();
      const auto  a    = accessor->Get<xiiString>(joint, type->FindPropertyByName("ActorA"));
      const auto  b    = accessor->Get<xiiString>(joint, type->FindPropertyByName("ActorB"));
      if (b.IsEmpty() || a.IsEqual_NoCase(b))
        return fail();
      for (bool first : {true, false})
      {
        const auto& reference = first ? a : b;
        auto        frame     = global;
        if (!reference.IsEmpty())
        {
          if (!xiiConversionUtils::IsStringUuid(reference))
            return fail();
          const auto*  actor = document.GetObjectManager()->GetObject(xiiConversionUtils::ConvertStringToUuid(reference));
          xiiTransform pose;
          if (!actor || document.ComputeObjectTransformation(actor, pose).Failed() || !pose.m_vScale.IsValid() ||
              pose.m_vScale.x == 0 || pose.m_vScale.y == 0 || pose.m_vScale.z == 0)
            return fail();
          frame          = xiiTransform::MakeLocalTransform(pose, global);
          frame.m_vScale = xiiVec3(1.0f);
        }
        if (!frame.m_vPosition.IsValid() || !frame.m_qRotation.IsValid() ||
            accessor->SetValue(joint, type->FindPropertyByName(first ? "LocalFrameA" : "LocalFrameB"), frame).Failed())
          return fail();
      }
    }
    accessor->FinishTransaction();
    return XII_SUCCESS;
  }

  xiiMap<const xiiAssetDocument*, xiiUniquePtr<xiiJoltAuthoringSyncObject>> s_Settings;
  xiiHybridArray<xiiActionDescriptorHandle, 20>                             s_Actions;
  xiiEvent<const xiiAssetDocument*>                                         s_SettingsChanged;
  xiiUInt32                                                                 s_uiLayerRevision = 0;

  xiiJoltAuthoringSyncObject& Settings(const xiiAssetDocument* document)
  {
    auto& settings = s_Settings[document];
    if (!settings)
    {
      settings                    = XII_DEFAULT_NEW(xiiJoltAuthoringSyncObject);
      settings->m_uiLayerRevision = s_uiLayerRevision;
      document->AddSyncObject(settings.Borrow());
    }
    return *settings;
  }
  void RefreshLayerNames()
  {
    auto& values = xiiDynamicEnum::GetDynamicEnum("JoltCollisionLayer");
    values.Clear();
    values.SetEditCommand("Jolt Collision Layers", {});
    xiiJoltCollisionLayerConfig config;
    if (xiiToolsProject::IsProjectOpen())
      config.Load().LogFailure();
    for (xiiUInt32 i = 0; i < xiiJoltCollisionLayerConfig::LayerCount; ++i)
    {
      xiiStringBuilder name;
      name.SetFormat("{} ({})", config.GetName(static_cast<xiiUInt8>(i)).IsEmpty() ? "Unnamed" : config.GetName(static_cast<xiiUInt8>(i)).GetData(), i);
      values.SetValueAndName(static_cast<xiiInt32>(i), name);
    }
  }
  void ProjectEvent(const xiiToolsProjectEvent& event)
  {
    if (event.m_Type == xiiToolsProjectEvent::Type::ProjectOpened || event.m_Type == xiiToolsProjectEvent::Type::ProjectConfigChanged || event.m_Type == xiiToolsProjectEvent::Type::ProjectClosed)
    {
      RefreshLayerNames();
      ++s_uiLayerRevision;
      for (auto& entry : s_Settings)
      {
        entry.Value()->m_uiLayerRevision = s_uiLayerRevision;
        entry.Value()->SetModified();
      }
    }
  }
  void DocumentEvent(const xiiDocumentManager::Event& event)
  {
    if (event.m_Type == xiiDocumentManager::Event::Type::DocumentClosing)
      if (const auto* document = xiiDynamicCast<const xiiAssetDocument*>(event.m_pDocument))
        s_Settings.Remove(document);
  }

  void EditLayers(QWidget* parent)
  {
    if (!xiiToolsProject::IsProjectOpen())
      return;
    xiiJoltCollisionLayerConfig config;
    const auto                  loaded = config.Load();
    if (loaded.Failed())
    {
      QMessageBox::warning(parent, "Jolt Collision Layers", QString::fromUtf8(loaded.GetMessageString().GetData()));
      return;
    }
    QDialog dialog(parent);
    dialog.setWindowTitle("Jolt Collision Layers");
    dialog.resize(980, 680);
    auto* layout = new QVBoxLayout(&dialog);
    auto* help   = new QLabel("Layer IDs are stable. Name layers to include them in the matrix. Checked pairs collide.\nChanges apply to new physics worlds; stop and restart simulation after saving.", &dialog);
    help->setWordWrap(true);
    layout->addWidget(help);
    auto* splitter = new QSplitter(&dialog);
    layout->addWidget(splitter);
    auto* names = new QTableWidget(256, 2, splitter);
    names->setHorizontalHeaderLabels({"ID", "Name"});
    names->verticalHeader()->hide();
    names->horizontalHeader()->setStretchLastSection(true);
    auto* matrix = new QTableWidget(splitter);
    matrix->setEditTriggers(QAbstractItemView::NoEditTriggers);
    matrix->setSelectionMode(QAbstractItemView::NoSelection);
    xiiHybridArray<xiiUInt8, 32> ids;
    const auto                   rebuild = [&]() {
      QSignalBlocker block(matrix);
      ids.Clear();
      QStringList labels;
      for (xiiUInt32 i = 0; i < 256; ++i)
        if (!config.GetName(static_cast<xiiUInt8>(i)).IsEmpty())
        {
          ids.PushBack(static_cast<xiiUInt8>(i));
          labels.push_back(QString::fromUtf8(config.GetName(static_cast<xiiUInt8>(i)).GetData()) + QString(" (%1)").arg(i));
        }
      matrix->clear();
      matrix->setRowCount(static_cast<int>(ids.GetCount()));
      matrix->setColumnCount(static_cast<int>(ids.GetCount()));
      matrix->setHorizontalHeaderLabels(labels);
      matrix->setVerticalHeaderLabels(labels);
      for (int a = 0; a < static_cast<int>(ids.GetCount()); ++a)
        for (int b = 0; b < static_cast<int>(ids.GetCount()); ++b)
        {
          auto* item = new QTableWidgetItem;
          item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
          item->setCheckState(config.IsCollisionEnabled(ids[a], ids[b]) ? Qt::Checked : Qt::Unchecked);
          item->setToolTip(labels[a] + " / " + labels[b]);
          matrix->setItem(a, b, item);
        }
      matrix->resizeColumnsToContents();
    };
    for (int i = 0; i < 256; ++i)
    {
      auto* id = new QTableWidgetItem(QString::number(i));
      id->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      names->setItem(i, 0, id);
      names->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(config.GetName(static_cast<xiiUInt8>(i)).GetData())));
    }
    rebuild();
    QObject::connect(names, &QTableWidget::itemChanged, &dialog, [&](QTableWidgetItem* item) {
      if (item->column() != 1)
        return;
      const auto bytes  = item->text().toUtf8();
      const auto status = config.SetName(static_cast<xiiUInt8>(item->row()), bytes.constData());
      if (status.Failed())
      {
        QMessageBox::warning(&dialog, "Invalid Layer Name", QString::fromUtf8(status.GetMessageString().GetData()));
        QSignalBlocker block(names);
        item->setText(QString::fromUtf8(config.GetName(static_cast<xiiUInt8>(item->row())).GetData()));
      }
      else
        rebuild();
    });
    QObject::connect(matrix, &QTableWidget::itemChanged, &dialog, [&](QTableWidgetItem* item) {
      const int  a = item->row(), b = item->column();
      const bool enabled = item->checkState() == Qt::Checked;
      config.SetCollisionEnabled(ids[a], ids[b], enabled);
      QSignalBlocker block(matrix);
      matrix->item(b, a)->setCheckState(item->checkState());
    });
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
      const auto saved = config.Save();
      if (saved.Failed())
        QMessageBox::warning(&dialog, "Jolt Collision Layers", QString::fromUtf8(saved.GetMessageString().GetData()));
      else
      {
        xiiToolsProject::BroadcastConfigChanged();
        dialog.accept();
      }
    });
    dialog.exec();
  }
} // namespace

xiiJoltEditorAction::xiiJoltEditorAction(const xiiActionContext& context, const char* szName, xiiInt32 iMode) :
  xiiButtonAction(context, szName, iMode >= -1, ":/EditorFramework/Icons/Visualizers.svg"), m_iMode(iMode)
{
  s_SettingsChanged.AddEventHandler(xiiMakeDelegate(&xiiJoltEditorAction::Refresh, this));
  Refresh();
}
xiiJoltEditorAction::~xiiJoltEditorAction() { s_SettingsChanged.RemoveEventHandler(xiiMakeDelegate(&xiiJoltEditorAction::Refresh, this)); }
void xiiJoltEditorAction::Refresh(const xiiAssetDocument* pDocument)
{
  XII_IGNORE_UNUSED(pDocument);
  if (m_iMode == -2)
  {
    SetEnabled(xiiToolsProject::IsProjectOpen());
    return;
  }
  const auto* document = xiiDynamicCast<const xiiGameObjectDocument*>(m_Context.m_pDocument);
  SetEnabled(document != nullptr);
  if (document)
  {
    if (m_iMode == -3)
      return;
    const auto& settings = Settings(document);
    SetChecked(m_iMode == -1 ? settings.m_bAllShapes : (settings.m_uiModes & static_cast<xiiUInt32>(m_iMode)) != 0);
  }
}
void xiiJoltEditorAction::Execute(const xiiVariant& value)
{
  if (m_iMode == -2)
  {
    EditLayers(m_Context.m_pWindow);
    return;
  }
  const auto* document = xiiDynamicCast<const xiiGameObjectDocument*>(m_Context.m_pDocument);
  if (!document)
    return;
  if (m_iMode == -3)
  {
    auto*      active = const_cast<xiiGameObjectDocument*>(document)->GetRedirectedGameObjectDoc();
    const auto result = AlignJointFrames(*active);
    if (result.Failed())
      QMessageBox::warning(m_Context.m_pWindow, "Align Jolt Joint Frames", QString::fromUtf8(result.GetMessageString().GetData()));
    return;
  }
  auto&      settings = Settings(document);
  const bool enabled  = value.IsValid() ? value.ConvertTo<bool>() : !IsChecked();
  if (m_iMode == -1)
    settings.m_bAllShapes = enabled;
  else if (enabled)
    settings.m_uiModes |= static_cast<xiiUInt32>(m_iMode);
  else
    settings.m_uiModes &= ~static_cast<xiiUInt32>(m_iMode);
  settings.SetModified();
  s_SettingsChanged.Broadcast(document);
}
void xiiRegisterJoltEditorActions()
{
  const auto menu = XII_REGISTER_MENU("Jolt Physics");
  s_Actions.PushBack(menu);
#define XII_JOLT_AUTHORING_ACTION(Name, Mode)                                                                         \
  {                                                                                                                   \
    const auto handle = XII_REGISTER_ACTION_1(Name, xiiActionScope::Document, "Jolt", "", xiiJoltEditorAction, Mode); \
    s_Actions.PushBack(handle);                                                                                       \
  }
  XII_JOLT_AUTHORING_ACTION("Authored Collision Previews", -1);
  XII_JOLT_AUTHORING_ACTION("Native Collision Geometry", xiiJoltDebugMode::Shapes);
  XII_JOLT_AUTHORING_ACTION("Rigid Body States", xiiJoltDebugMode::BodyStates);
  XII_JOLT_AUTHORING_ACTION("Physics Bounds", xiiJoltDebugMode::Bounds);
  XII_JOLT_AUTHORING_ACTION("Centers Of Mass", xiiJoltDebugMode::CentersOfMass);
  XII_JOLT_AUTHORING_ACTION("Physics Velocities", xiiJoltDebugMode::Velocities);
  XII_JOLT_AUTHORING_ACTION("Native Constraints", xiiJoltDebugMode::Constraints);
  XII_JOLT_AUTHORING_ACTION("Constraint Limits", xiiJoltDebugMode::ConstraintLimits);
  XII_JOLT_AUTHORING_ACTION("Contact Points", xiiJoltDebugMode::Contacts);
  XII_JOLT_AUTHORING_ACTION("Contact Normals", xiiJoltDebugMode::ContactNormals);
  XII_JOLT_AUTHORING_ACTION("Character Controllers", xiiJoltDebugMode::Characters);
  XII_JOLT_AUTHORING_ACTION("Physics Statistics", xiiJoltDebugMode::Statistics);
  XII_JOLT_AUTHORING_ACTION("Jolt Collision Layers", -2);
  XII_JOLT_AUTHORING_ACTION("Align Joint Frames To Object", -3);
#undef XII_JOLT_AUTHORING_ACTION
  for (const char* name : {"EditorPluginScene_DocumentMenuBar", "EditorPluginScene_Scene2MenuBar"})
    if (auto* map = xiiActionMapManager::GetActionMap(name))
    {
      map->MapAction(menu, "G.View", 30.0f);
      for (xiiUInt32 i = 1; i < s_Actions.GetCount(); ++i)
        map->MapAction(s_Actions[i], "G.View/Jolt Physics", static_cast<float>(i));
    }
  xiiDocumentManager::s_Events.AddEventHandler(DocumentEvent);
  xiiToolsProject::s_Events.AddEventHandler(ProjectEvent);
  RefreshLayerNames();
}
void xiiUnregisterJoltEditorActions()
{
  xiiToolsProject::s_Events.RemoveEventHandler(ProjectEvent);
  xiiDocumentManager::s_Events.RemoveEventHandler(DocumentEvent);
  for (auto handle : s_Actions)
    xiiActionManager::UnregisterAction(handle);
  s_Actions.Clear();
  s_Settings.Clear();
  xiiDynamicEnum::GetDynamicEnum("JoltCollisionLayer").Clear();
  xiiDynamicEnum::GetDynamicEnum("JoltCollisionLayer").SetEditCommand({}, {});
}

XII_STATICLINK_FILE(EditorPluginJolt, EditorPluginJolt_Authoring_Implementation_JoltEditorActions);
