/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMujoco/EditorPluginMujocoPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginMujoco/Authoring/MuJoCoSceneInspector.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Math/Math.h>
#include <MujocoPlugin/Components/MuJoCoActuatorComponent.h>
#include <MujocoPlugin/Components/MuJoCoBodyComponent.h>
#include <MujocoPlugin/Components/MuJoCoJointComponent.h>
#include <MujocoPlugin/Components/MuJoCoModelComponent.h>
#include <MujocoPlugin/Components/MuJoCoSensorComponent.h>
#include <MujocoPlugin/System/MuJoCoDebugDraw.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QStandardItemModel>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

namespace
{
  using Command           = xiiMuJoCoInspectorCommand;
  constexpr int TypeRole  = Qt::UserRole + 1;
  constexpr int IndexRole = Qt::UserRole + 2;

  xiiVariant Read(const xiiVariantDictionary& data, const char* key)
  {
    if (const auto* value = data.GetValue(key))
      return *value;
    return {};
  }

  template <typename T>
  T Read(const xiiVariantDictionary& data, const char* key, T fallback = {})
  {
    const auto value = Read(data, key);
    return value.CanConvertTo<T>() ? value.ConvertTo<T>() : fallback;
  }

  QString Describe(const xiiVariant& value)
  {
    if (value.IsA<xiiVariantArray>())
    {
      QStringList entries;
      for (const auto& item : value.Get<xiiVariantArray>())
        entries.push_back(Describe(item));
      return entries.join(", ");
    }
    if (value.IsA<double>())
      return QString::number(value.Get<double>(), 'g', 17);
    if (value.IsA<xiiVec3>())
    {
      const auto v = value.Get<xiiVec3>();
      return QString("%1, %2, %3").arg(v.x, 0, 'g', 7).arg(v.y, 0, 'g', 7).arg(v.z, 0, 'g', 7);
    }
    if (value.CanConvertTo<xiiString>())
      return QString::fromUtf8(value.ConvertTo<xiiString>().GetData());
    return {};
  }
} // namespace

xiiQtMuJoCoSceneInspector::xiiQtMuJoCoSceneInspector(xiiSceneDocument& document, xiiQtEngineDocumentWindow& window) :
  QWidget(&window), m_Document(document), m_Window(window)
{
  auto* layout    = new QVBoxLayout(this);
  auto* instances = new QHBoxLayout;
  m_pModels       = new QComboBox(this);
  m_pModels->setAccessibleName(tr("MuJoCo model instance"));
  instances->addWidget(m_pModels, 1);
  auto* asset = new QPushButton(tr("Open Asset"), this);
  instances->addWidget(asset);
  auto* visualization = new QPushButton(tr("Visualization"), this);
  instances->addWidget(visualization);
  layout->addLayout(instances);
  auto* menu = new QMenu(visualization);
  visualization->setMenu(menu);
  using Mode = xiiMuJoCoDebugMode;
  struct Option
  {
    const char* m_szLabel;
    Mode::Enum  m_Mode;
  };
  for (const auto& option : {Option{"Collision geometry", Mode::CollisionGeometry}, Option{"Visual geometry", Mode::VisualGeometry},
                             Option{"Body frames", Mode::BodyFrames}, Option{"Hierarchy", Mode::Hierarchy}, Option{"Joint axes", Mode::Joints},
                             Option{"Joint limits", Mode::JointLimits}, Option{"Centers of mass", Mode::CentersOfMass}, Option{"Actuators", Mode::Actuators},
                             Option{"Sensors", Mode::Sensors}, Option{"Contacts", Mode::Contacts}, Option{"Contact normals", Mode::ContactNormals},
                             Option{"Contact forces", Mode::ContactForces}, Option{"Statistics", Mode::Statistics}})
  {
    auto* action = menu->addAction(tr(option.m_szLabel));
    action->setCheckable(true);
    action->setChecked(option.m_Mode == Mode::CollisionGeometry);
    connect(action, &QAction::toggled, this, [this, mode = option.m_Mode](bool enabled) {
      if (enabled)
        m_Sync.m_uiModes |= mode;
      else
        m_Sync.m_uiModes &= ~static_cast<xiiUInt32>(mode);
      m_Sync.SetModified();
    });
  }
  m_Sync.m_uiModes = Mode::CollisionGeometry;

  auto* controls = new QHBoxLayout;
  m_pStartPause  = new QPushButton(tr("Simulate"), this);
  m_pStep        = new QPushButton(tr("Step MuJoCo"), this);
  m_pReset       = new QPushButton(tr("Reset MuJoCo"), this);
  controls->addWidget(m_pStartPause);
  controls->addWidget(m_pStep);
  controls->addWidget(m_pReset);
  layout->addLayout(controls);
  m_pStatus = new QLabel(this);
  m_pStatus->setWordWrap(true);
  m_pStatus->setTextFormat(Qt::PlainText);
  m_pStatistics = new QLabel(this);
  m_pStatistics->setWordWrap(true);
  layout->addWidget(m_pStatus);
  layout->addWidget(m_pStatistics);

  m_pTree      = new QTreeView(this);
  m_pTreeModel = new QStandardItemModel(m_pTree);
  m_pTreeModel->setHorizontalHeaderLabels({tr("Compiled articulation"), tr("Element")});
  m_pTree->setModel(m_pTreeModel);
  m_pTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_pTree->setUniformRowHeights(true);
  layout->addWidget(m_pTree, 2);
  auto* navigation    = new QHBoxLayout;
  m_pMore             = new QPushButton(tr("Load More"), this);
  m_pLocate           = new QPushButton(tr("Locate"), this);
  m_pContacts         = new QCheckBox(tr("Contacts"), this);
  m_pPreviousContacts = new QPushButton(tr("Previous"), this);
  m_pNextContacts     = new QPushButton(tr("Next"), this);
  navigation->addWidget(m_pMore);
  navigation->addWidget(m_pLocate);
  m_pBind = new QPushButton(tr("Bind Selection"), this);
  navigation->addWidget(m_pBind);
  navigation->addWidget(m_pContacts);
  navigation->addWidget(m_pPreviousContacts);
  navigation->addWidget(m_pNextContacts);
  layout->addLayout(navigation);
  m_pDetails = new QPlainTextEdit(this);
  m_pDetails->setReadOnly(true);
  layout->addWidget(m_pDetails, 1);

  auto* edit  = new QHBoxLayout;
  m_pEditMode = new QComboBox(this);
  m_pValues   = new QLineEdit(this);
  m_pValues->setPlaceholderText(tr("Enter comma-separated runtime values"));
  m_pApply      = new QPushButton(tr("Apply"), this);
  m_pResetJoint = new QPushButton(tr("Reset Joint"), this);
  edit->addWidget(m_pEditMode);
  edit->addWidget(m_pValues, 1);
  edit->addWidget(m_pApply);
  edit->addWidget(m_pResetJoint);
  layout->addLayout(edit);
  m_pSlider = new QSlider(Qt::Horizontal, this);
  m_pSlider->setRange(0, 1000);
  m_pSlider->hide();
  layout->addWidget(m_pSlider);

  connect(asset, &QPushButton::clicked, this, &xiiQtMuJoCoSceneInspector::OpenModelAsset);
  connect(m_pLocate, &QPushButton::clicked, this, &xiiQtMuJoCoSceneInspector::Locate);
  connect(m_pBind, &QPushButton::clicked, this, &xiiQtMuJoCoSceneInspector::BindSelectedElement);
  connect(m_pMore, &QPushButton::clicked, this, [this] { Request(Command::Metadata, m_uiNextElement); });
  connect(m_pContacts, &QCheckBox::toggled, this, [this] { m_uiContactOffset = 0; });
  connect(m_pNextContacts, &QPushButton::clicked, this, [this] { m_uiContactOffset += 32; });
  connect(m_pPreviousContacts, &QPushButton::clicked, this, [this] { m_uiContactOffset = m_uiContactOffset >= 32 ? m_uiContactOffset - 32 : 0; });
  connect(m_pStartPause, &QPushButton::clicked, this, [this] {
    if (m_Document.GetGameMode() == GameMode::Off)
      m_Document.StartSimulateWorld();
    else if (m_Document.GetGameMode() == GameMode::Simulate)
      m_Document.SetPauseSimulation(!m_Document.GetPauseSimulation());
    UpdateSettings();
  });
  connect(m_pStep, &QPushButton::clicked, this, [this] { Request(Command::SingleStep); });
  connect(m_pReset, &QPushButton::clicked, this, [this] { Request(Command::ResetSimulation); });
  connect(m_pResetJoint, &QPushButton::clicked, this, [this] { Request(Command::ResetJoint); });
  connect(m_pApply, &QPushButton::clicked, this, [this] {
    ApplyValues(static_cast<Command>(m_pEditMode->currentData().toInt()));
  });
  connect(m_pEditMode, &QComboBox::currentIndexChanged, this, [this] { m_pValues->clear(); });
  connect(m_pSlider, &QSlider::valueChanged, this, [this](int value) {
    if (m_pSlider->isSliderDown())
      m_pValues->setText(QString::number(m_fSliderMin + (m_fSliderMax - m_fSliderMin) * value / 1000.0, 'g', 17));
  });
  connect(m_pSlider, &QSlider::sliderReleased, this, [this] { ApplyValues(Command::SetActuatorControls); });
  connect(m_pModels, &QComboBox::currentIndexChanged, this, [this] {
    m_Sync.m_uiModelToken = m_pModels->currentData().toULongLong();
    ResetSelection();
  });
  connect(m_pTree->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& index) {
    const auto first       = index.siblingAtColumn(0);
    m_Sync.m_iElementType  = first.isValid() ? first.data(TypeRole).toInt() : -1;
    m_Sync.m_iElementIndex = first.isValid() ? first.data(IndexRole).toInt() : -1;
    m_pValues->clear();
    m_bCanEdit = m_bHasPosition = false;
    m_pApply->setEnabled(false);
    m_pResetJoint->setEnabled(false);
    m_pLocate->setEnabled(false);
    m_pBind->setEnabled(false);
    m_sSelectedName.Clear();
    m_pSlider->hide();
    m_Sync.SetModified();
  });
  m_Document.AddSyncObject(&m_Sync);
  m_Document.m_ProcessMessageEvent.AddEventHandler(xiiMakeDelegate(&xiiQtMuJoCoSceneInspector::HandleMessage, this));
  xiiQtDocumentWindow::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtMuJoCoSceneInspector::WindowEvent, this));
  auto* timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &xiiQtMuJoCoSceneInspector::Tick);
  timer->start(200);
  ResetSelection();
  Tick();
}

xiiQtMuJoCoSceneInspector::~xiiQtMuJoCoSceneInspector()
{
  xiiQtDocumentWindow::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtMuJoCoSceneInspector::WindowEvent, this));
  m_Document.m_ProcessMessageEvent.RemoveEventHandler(xiiMakeDelegate(&xiiQtMuJoCoSceneInspector::HandleMessage, this));
}

void xiiQtMuJoCoSceneInspector::WindowEvent(const xiiQtDocumentWindowEvent& event)
{
  if (event.m_pWindow == &m_Window && event.m_Type == xiiQtDocumentWindowEvent::BeforeRedraw)
  {
    UpdateSettings();
    // Scene redraw sends its view messages before the base window sends sync objects.
    // Flush changed settings first so the next view sees the current transaction state.
    if (m_Sync.GetModified() && m_Document.GetEngineStatus() == xiiAssetDocument::EngineStatus::Loaded &&
        !xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
      m_Document.SyncObjectsToEngine();
  }
}

void xiiQtMuJoCoSceneInspector::UpdateSettings()
{
  const bool visible   = isVisible() && m_Window.IsVisibleInContainer();
  const bool authoring = m_Document.GetGameMode() == GameMode::Off;
  const bool defer     = authoring && m_Document.GetActiveSubDocument()->GetCommandHistory()->IsInTransaction();
  const bool contacts  = visible && m_pContacts->isChecked();
  if (m_Sync.m_bPreview && !visible)
  {
    m_bWaiting = false;
    ++m_Sync.m_uiRequest;
    m_Sync.m_uiCommand = static_cast<xiiUInt8>(Command::Inspect);
    m_Sync.m_Values.Clear();
  }
  if (m_Sync.m_bPreview != visible || m_Sync.m_bAuthoring != authoring || m_Sync.m_bDeferComposition != defer || m_Sync.m_bContacts != contacts)
  {
    m_Sync.m_bPreview          = visible;
    m_Sync.m_bAuthoring        = authoring;
    m_Sync.m_bDeferComposition = defer;
    m_Sync.m_bContacts         = contacts;
    m_Sync.SetModified();
  }
}

void xiiQtMuJoCoSceneInspector::Tick()
{
  UpdateSettings();
  if (!m_Sync.m_bPreview || m_Document.GetEngineStatus() != xiiAssetDocument::EngineStatus::Loaded)
    return;
  const auto mode = m_Document.GetGameMode();
  m_pStartPause->setEnabled(mode != GameMode::Play);
  m_pStartPause->setText(mode == GameMode::Off ? tr("Simulate") : m_Document.GetPauseSimulation() ? tr("Resume") :
                                                                                                    tr("Pause"));
  if (m_bWaiting && xiiTime::Now() - m_LastRequest < xiiTime::MakeFromSeconds(2))
    return;
  if (m_bWaiting && static_cast<Command>(m_Sync.m_uiCommand) == Command::Metadata)
  {
    const auto offset = m_Sync.m_uiOffset;
    m_bWaiting        = false;
    Request(Command::Metadata, offset);
    return;
  }
  m_bWaiting = false;
  if (m_bHierarchyRequested && m_Sync.m_uiGeneration != 0 && m_Sync.m_uiModelToken != 0)
  {
    m_bHierarchyRequested = false;
    Request(Command::Metadata);
  }
  else
    Request(Command::Inspect, m_uiContactOffset);
}

void xiiQtMuJoCoSceneInspector::FollowSceneSelection()
{
  auto*       document = m_Document.GetRedirectedGameObjectDoc();
  const auto* selected = document ? document->GetSelectionManager()->GetCurrentObject() : nullptr;
  const auto  guid     = selected ? selected->GetGuid() : xiiUuid{};
  if (guid == m_LastSceneSelection || m_Models.IsEmpty())
    return;
  m_LastSceneSelection = guid;
  for (auto* object = selected; object; object = object->GetParent())
    for (xiiUInt32 i = 0; i < m_Models.GetCount(); ++i)
    {
      const auto& row = m_Models[i].Get<xiiVariantDictionary>();
      if (object->GetGuid() == Read<xiiUuid>(row, "ObjectGuid") || object->GetGuid() == Read<xiiUuid>(row, "Guid"))
      {
        m_pModels->setCurrentIndex(static_cast<int>(i));
        return;
      }
    }
}

void xiiQtMuJoCoSceneInspector::Request(Command command, xiiUInt64 offset)
{
  UpdateSettings();
  if (!m_Sync.m_bPreview || m_Document.GetEngineStatus() != xiiAssetDocument::EngineStatus::Loaded)
    return;
  // A poll may be superseded. An explicit command remains immutable until acknowledged.
  const auto pending = static_cast<Command>(m_Sync.m_uiCommand);
  if (m_bWaiting && (pending != Command::Inspect || command == Command::Inspect))
    return;
  if (command != Command::Inspect && command != Command::Metadata)
  {
    m_Sync.m_uiCommandModel = m_Sync.m_uiModelToken;
    m_Sync.m_iCommandType   = m_Sync.m_iElementType;
    m_Sync.m_iCommandIndex  = m_Sync.m_iElementIndex;
  }
  else
  {
    m_Sync.m_Values.Clear();
  }
  m_Sync.m_uiCommand = static_cast<xiiUInt8>(command);
  m_Sync.m_uiOffset  = offset;
  ++m_Sync.m_uiRequest;
  m_Sync.SetModified();
  m_bWaiting    = true;
  m_LastRequest = xiiTime::Now();
}

void xiiQtMuJoCoSceneInspector::ResetSelection()
{
  m_pTreeModel->removeRows(0, m_pTreeModel->rowCount());
  m_Bodies.clear();
  m_Sync.m_iElementIndex = m_Sync.m_iElementType = -1;
  m_uiNextElement                                = 0;
  m_bHierarchyRequested                          = true;
  m_bCanEdit = m_bHasPosition = false;
  m_pMore->setEnabled(false);
  m_pApply->setEnabled(false);
  m_pResetJoint->setEnabled(false);
  m_pLocate->setEnabled(false);
  m_BoundObject = {};
  m_pBind->setEnabled(false);
  m_sSelectedName.Clear();
  m_pStep->setEnabled(false);
  m_pReset->setEnabled(false);
  m_pSlider->hide();
  m_pValues->clear();
  m_pDetails->clear();
}

void xiiQtMuJoCoSceneInspector::AppendElements(const xiiVariantArray& entries)
{
  static const char* names[] = {"Body", "Joint", "Actuator", "Sensor", "Geometry", "Site"};
  for (const auto& entry : entries)
  {
    const auto& row   = entry.Get<xiiVariantDictionary>();
    const int   type  = Read<int>(row, "Type", -1);
    const int   index = Read<int>(row, "Index", -1);
    if (type < 0 || type >= XII_ARRAY_SIZE(names) || index < 0)
      continue;
    auto* name = new QStandardItem(Describe(Read(row, "Name")));
    auto* kind = new QStandardItem(tr(names[type]));
    name->setData(type, TypeRole);
    name->setData(index, IndexRole);
    const auto parent     = Read<int>(row, "Parent", -1);
    auto*      parentItem = m_Bodies.value(parent, m_pTreeModel->invisibleRootItem());
    parentItem->appendRow({name, kind});
    if (type == 0)
      m_Bodies.insert(index, name);
  }
}

void xiiQtMuJoCoSceneInspector::HandleMessage(const xiiEditorEngineDocumentMsg* message)
{
  const auto* reply = xiiDynamicCast<const xiiMuJoCoInspectorResponse*>(message);
  if (!reply || reply->m_Session != m_Sync.GetGuid() || reply->m_uiRequest != m_Sync.m_uiRequest)
    return;
  m_bWaiting = false;
  if (!m_Sync.m_bPreview)
    return;
  const bool generationChanged = reply->m_uiGeneration != m_Sync.m_uiGeneration || reply->m_EngineEpoch != m_Sync.m_EngineEpoch;
  m_Sync.m_EngineEpoch         = reply->m_EngineEpoch;
  m_Sync.m_uiGeneration        = reply->m_uiGeneration;
  if (generationChanged)
    ResetSelection();
  const auto& replyData = reply->m_Data;
  const auto  modelRows = Read<xiiVariantArray>(replyData, "Models");
  if (modelRows != m_Models)
  {
    QSignalBlocker block(m_pModels);
    m_Models       = modelRows;
    const auto old = m_Sync.m_uiModelToken;
    m_pModels->clear();
    for (const auto& value : m_Models)
    {
      const auto& row   = value.Get<xiiVariantDictionary>();
      const auto  state = Read<bool>(row, "Missing") ? tr("invalid resource") : Read<bool>(row, "Pending") ? tr("loading") :
        Read<bool>(row, "Installed")                                                                       ? tr("installed") :
                                                                                                             tr("awaiting composition");
      auto        name  = Describe(Read(row, "Name"));
      if (name.isEmpty())
        name = Describe(Read(row, "Resource"));
      m_pModels->addItem(name + " (" + state + ")", QVariant::fromValue<qulonglong>(Read<xiiUInt64>(row, "Token")));
      m_pModels->setItemData(m_pModels->count() - 1, Describe(Read(row, "Resource")) + "\n" + Describe(Read(row, "Namespace")), Qt::ToolTipRole);
    }
    const int index = m_pModels->findData(QVariant::fromValue<qulonglong>(old));
    m_pModels->setCurrentIndex(index >= 0 ? index : (m_pModels->count() ? 0 : -1));
    m_Sync.m_uiModelToken = m_pModels->currentData().toULongLong();
    if (old != m_Sync.m_uiModelToken)
      ResetSelection();
  }
  FollowSceneSelection();
  const auto error  = Describe(Read(replyData, "Error"));
  auto       status = error.isEmpty() ? Describe(Read(replyData, "Status")) : error;
  if (m_Models.IsEmpty() && (status.isEmpty() || status == tr("No model instances registered.")))
    status = tr("Add a MuJoCo Model component and assign a transformed asset.");
  if (Read<xiiUInt32>(replyData, "ModelCount") > m_Models.GetCount())
    status += tr(" Showing the first %1 model instances.").arg(m_Models.GetCount());
  m_pStatus->setText(status);
  const bool runtime = m_Document.GetGameMode() == GameMode::Simulate && m_Sync.m_uiGeneration != 0 && Read<bool>(replyData, "Ready");
  const bool paused  = Read<bool>(replyData, "Paused") && m_Document.GetPauseSimulation();
  m_pReset->setEnabled(runtime);
  m_pStep->setEnabled(runtime && paused);
  const auto stats = Read<xiiVariantDictionary>(replyData, "Statistics");
  if (stats.IsEmpty())
    m_pStatistics->clear();
  else
  {
    QStringList summary;
    for (const auto* key : {"Time", "Step", "Timestep", "Bodies", "Joints", "Nq", "Nv", "Actuators", "Sensors", "Contacts", "Constraints", "Iterations", "DurationMs"})
      summary << QString::fromLatin1(key) + " " + Describe(Read(stats, key));
    m_pStatistics->setText(summary.join("  |  "));
  }
  if (!generationChanged && Read(replyData, "Elements").IsValid() && Read<xiiUInt64>(replyData, "Model") == m_Sync.m_uiModelToken)
  {
    AppendElements(Read<xiiVariantArray>(replyData, "Elements"));
    m_uiNextElement = Read<xiiUInt64>(replyData, "Next");
    m_pMore->setEnabled(Read<bool>(replyData, "More"));
  }
  const auto selected = Read<xiiVariantDictionary>(replyData, "Selected");
  if (!generationChanged && !selected.IsEmpty() && Read<xiiUInt64>(selected, "Model") == m_Sync.m_uiModelToken &&
      Read<int>(selected, "Type", -1) == m_Sync.m_iElementType && Read<int>(selected, "Index", -1) == m_Sync.m_iElementIndex)
    ShowSelected(selected, runtime);
  const auto contactCount = Read<xiiUInt32>(replyData, "ContactCount");
  m_pPreviousContacts->setEnabled(m_pContacts->isChecked() && m_uiContactOffset > 0);
  m_pNextContacts->setEnabled(m_pContacts->isChecked() && contactCount > m_uiContactOffset + 32);
  if (m_pContacts->isChecked())
  {
    QStringList lines;
    const auto  modelName = [this](xiiUInt64 token) {
      for (const auto& instance : m_Models)
      {
        const auto& row = instance.Get<xiiVariantDictionary>();
        if (Read<xiiUInt64>(row, "Token") == token)
          return Describe(Read(row, "Name"));
      }
      return tr("unmapped");
    };
    for (const auto& entry : Read<xiiVariantArray>(replyData, "Contacts"))
    {
      const auto& row = entry.Get<xiiVariantDictionary>();
      lines << tr("Contact %1  %2 [%3] / %4 [%5]\n  Distance %6 m  Position %7\n  Normal %8  World force %9 N")
                 .arg(Read<xiiUInt32>(row, "Index"))
                 .arg(modelName(Read<xiiUInt64>(row, "ModelA")))
                 .arg(Read<int>(row, "GeometryA"))
                 .arg(modelName(Read<xiiUInt64>(row, "ModelB")))
                 .arg(Read<int>(row, "GeometryB"))
                 .arg(Describe(Read(row, "Distance")))
                 .arg(Describe(Read(row, "Position")))
                 .arg(Describe(Read(row, "Normal")))
                 .arg(Read<bool>(row, "HasForce") ? Describe(Read(row, "Force")) : tr("unavailable"));
    }
    if (lines.isEmpty())
      lines << tr("No contacts in this page.");
    const auto contactText = lines.join("\n\n");
    if (m_pDetails->toPlainText() != contactText)
      m_pDetails->setPlainText(contactText);
  }
}

void xiiQtMuJoCoSceneInspector::ShowSelected(const xiiVariantDictionary& values, bool runtime)
{
  const auto type = static_cast<xiiMuJoCoElementType>(m_Sync.m_iElementType);
  m_bHasPosition  = Read(values, "Position").IsA<xiiVec3>();
  m_vPosition     = Read<xiiVec3>(values, "Position", xiiVec3::MakeZero());
  m_BoundObject   = Read<xiiUuid>(values, "Object");
  m_sSelectedName = Read<xiiString>(values, "Name");
  m_pBind->setEnabled(m_Document.GetGameMode() == GameMode::Off && Read<bool>(values, "Bindable") &&
                      (type == xiiMuJoCoElementType::Body || type == xiiMuJoCoElementType::Joint || type == xiiMuJoCoElementType::Actuator || type == xiiMuJoCoElementType::Sensor));
  m_pLocate->setEnabled(m_bHasPosition || m_BoundObject.IsValid());
  if (!m_pContacts->isChecked())
  {
    QStringList lines;
    for (const auto* key : {"Name", "Kind", "Description", "Body", "NativeType", "Target", "TargetType", "TargetName", "BindingName", "Position", "Axis", "Reference",
                            "Nq", "Nv", "Limited", "Lower", "Upper", "Qpos", "Qvel", "AppliedForce", "ControlCount", "OutputCount", "Dynamics", "Gain",
                            "Bias", "Signature", "Controls", "Forces", "Activation", "Dimension", "Datatype", "Stage", "ObjectType", "ObjectId",
                            "ReferenceType", "ReferenceId", "Cutoff", "Noise", "Time", "Readings", "Notice"})
    {
      const auto value = Read(values, key);
      if (value.IsValid())
        lines << QString::fromLatin1(key) + "  " + Describe(value);
    }
    if (type == xiiMuJoCoElementType::Actuator)
      for (const auto& value : Read<xiiVariantArray>(values, "Ranges"))
      {
        const auto& range = value.Get<xiiVariantDictionary>();
        lines << (Read<bool>(range, "Limited") ? tr("Control range  %1 to %2").arg(Describe(Read(range, "Lower")), Describe(Read(range, "Upper"))) : tr("Control range  unlimited"));
      }
    const auto details = lines.join('\n');
    if (m_pDetails->toPlainText() != details)
      m_pDetails->setPlainText(details);
  }

  const bool joint    = type == xiiMuJoCoElementType::Joint && Read(values, "Qpos").IsValid();
  const bool actuator = type == xiiMuJoCoElementType::Actuator && Read(values, "Controls").IsValid();
  m_bCanEdit          = runtime && (joint || actuator);
  const int target    = joint ? static_cast<int>(Command::SetJointPosition) : actuator ? static_cast<int>(Command::SetActuatorControls) :
                                                                                         -1;
  if (m_pEditMode->count() == 0 ? target >= 0 : m_pEditMode->itemData(0).toInt() != target)
  {
    QSignalBlocker block(m_pEditMode);
    m_pEditMode->clear();
    if (joint)
    {
      m_pEditMode->addItem(tr("Position"), static_cast<int>(Command::SetJointPosition));
      m_pEditMode->addItem(tr("Velocity"), static_cast<int>(Command::SetJointVelocity));
    }
    else if (actuator)
      m_pEditMode->addItem(tr("Control"), static_cast<int>(Command::SetActuatorControls));
  }
  m_pApply->setEnabled(m_bCanEdit);
  m_pValues->setEnabled(m_bCanEdit);
  m_pResetJoint->setEnabled(runtime && joint);
  const auto command = static_cast<Command>(m_pEditMode->currentData().toInt());
  m_uiPositionCount  = Read<xiiUInt32>(values, "Nq");
  m_uiVelocityCount  = Read<xiiUInt32>(values, "Nv");
  m_uiExpectedValues = Read<xiiUInt32>(values, "ControlCount");
  const auto current = Read(values, joint ? (command == Command::SetJointVelocity ? "Qvel" : "Qpos") : "Controls");
  m_pValues->setPlaceholderText(Describe(current));
  const auto ranges = Read<xiiVariantArray>(values, "Ranges");
  bool       slider = false;
  if (runtime && actuator && ranges.GetCount() == 1)
  {
    const auto& range = ranges[0].Get<xiiVariantDictionary>();
    m_fSliderMin      = Read<double>(range, "Lower");
    m_fSliderMax      = Read<double>(range, "Upper");
    slider            = Read<bool>(range, "Limited") && xiiMath::IsFinite(m_fSliderMax - m_fSliderMin) && m_fSliderMax > m_fSliderMin;
    if (slider && !m_pSlider->isSliderDown())
    {
      QSignalBlocker block(m_pSlider);
      const double   value = current.Get<xiiVariantArray>()[0].ConvertTo<double>();
      if (xiiMath::IsFinite(value))
        m_pSlider->setValue(static_cast<int>(1000 * xiiMath::Clamp((value - m_fSliderMin) / (m_fSliderMax - m_fSliderMin), 0.0, 1.0)));
    }
  }
  m_pSlider->setVisible(slider);
}

void xiiQtMuJoCoSceneInspector::ApplyValues(Command command)
{
  if (!m_bCanEdit || (m_bWaiting && static_cast<Command>(m_Sync.m_uiCommand) != Command::Inspect))
    return;
  xiiHybridArray<double, 8> values;
  for (const auto& part : m_pValues->text().split(','))
  {
    bool         valid = false;
    const double value = part.trimmed().toDouble(&valid);
    if (!valid || !xiiMath::IsFinite(value))
    {
      m_pStatus->setText(tr("Enter finite numeric coordinates separated by commas."));
      return;
    }
    values.PushBack(value);
  }
  const auto expected = command == Command::SetJointPosition ? m_uiPositionCount : command == Command::SetJointVelocity ? m_uiVelocityCount :
                                                                                                                          m_uiExpectedValues;
  if (values.GetCount() != expected)
  {
    m_pStatus->setText(tr("Expected %1 coordinates.").arg(expected));
    return;
  }
  m_Sync.m_Values = values.GetArrayPtr();
  Request(command);
}

void xiiQtMuJoCoSceneInspector::Locate()
{
  if (m_BoundObject.IsValid())
  {
    if (auto* scene = xiiDynamicCast<xiiScene2Document*>(&m_Document))
    {
      xiiHybridArray<xiiSceneDocument*, 8> layers;
      scene->GetLoadedLayers(layers);
      for (auto* layer : layers)
      {
        const auto* objects = layer == scene ? scene->GetSceneObjectManager() : layer->GetObjectManager();
        if (objects->GetObject(m_BoundObject) && scene->GetActiveLayer() != layer->GetGuid())
        {
          scene->SetActiveLayer(layer->GetGuid()).LogFailure();
          break;
        }
      }
    }
  }
  auto* document = m_Document.GetRedirectedGameObjectDoc();
  if (const auto* object = document && m_BoundObject.IsValid() ? document->GetObjectManager()->GetObject(m_BoundObject) : nullptr)
  {
    document->GetSelectionManager()->SetSelection(object);
    document->TriggerFocusOnSelection(true);
    return;
  }
  if (!m_bHasPosition)
    return;
  auto* view = m_Window.GetFocusedViewWidget();
  if (!view && !m_Window.GetViewWidgets().IsEmpty())
    view = m_Window.GetViewWidgets()[0];
  if (view)
  {
    const xiiVec3 direction = xiiVec3(1, 1, -0.6f).GetNormalized();
    view->InterpolateCameraTo(m_vPosition - direction * 3.0f, direction, 45.0f);
  }
}

void xiiQtMuJoCoSceneInspector::BindSelectedElement()
{
  if (m_Document.GetGameMode() != GameMode::Off || !m_pBind->isEnabled())
    return;
  auto* document = m_Document.GetRedirectedGameObjectDoc();
  if (!document || document->IsReadOnly())
    return;
  const int modelIndex = m_pModels->currentIndex();
  if (modelIndex < 0 || static_cast<xiiUInt32>(modelIndex) >= m_Models.GetCount())
    return;
  const auto     model       = Read<xiiUuid>(m_Models[modelIndex].Get<xiiVariantDictionary>(), "Guid");
  const auto     type        = static_cast<xiiMuJoCoElementType>(m_Sync.m_iElementType);
  const xiiRTTI* bindingType = nullptr;
  const char*    property    = nullptr;
  switch (type)
  {
    case xiiMuJoCoElementType::Body:
      bindingType = xiiGetStaticRTTI<xiiMuJoCoBodyComponent>();
      property    = "BodyName";
      break;
    case xiiMuJoCoElementType::Joint:
      bindingType = xiiGetStaticRTTI<xiiMuJoCoJointComponent>();
      property    = "JointName";
      break;
    case xiiMuJoCoElementType::Actuator:
      bindingType = xiiGetStaticRTTI<xiiMuJoCoActuatorComponent>();
      property    = "ActuatorName";
      break;
    case xiiMuJoCoElementType::Sensor:
      bindingType = xiiGetStaticRTTI<xiiMuJoCoSensorComponent>();
      property    = "SensorName";
      break;
    default:
      return;
  }
  xiiHybridArray<const xiiDocumentObject*, 8> candidates;
  for (const auto* selected : document->GetSelectionManager()->GetSelection())
  {
    if (selected->GetTypeAccessor().GetType()->IsDerivedFrom(bindingType) && !candidates.Contains(selected))
      candidates.PushBack(selected);
    for (const auto* child : selected->GetChildren())
      if (child->GetTypeAccessor().GetType()->IsDerivedFrom(bindingType) && !candidates.Contains(child))
        candidates.PushBack(child);
  }
  xiiHybridArray<const xiiDocumentObject*, 8> targets;
  for (const auto* selected : candidates)
  {
    bool scoped = false;
    for (auto* ancestor = selected->GetParent(); ancestor && !scoped; ancestor = ancestor->GetParent())
    {
      const xiiDocumentObject* anchor = nullptr;
      for (const auto* child : ancestor->GetChildren())
        if (child->GetTypeAccessor().GetType()->IsDerivedFrom<xiiMuJoCoModelComponent>())
        {
          if (anchor)
          {
            m_pStatus->setText(tr("Use one model component per binding anchor."));
            return;
          }
          anchor = child;
        }
      if (anchor)
      {
        if (anchor->GetGuid() != model)
        {
          m_pStatus->setText(tr("The selected binding belongs to a different model instance."));
          return;
        }
        scoped = true;
      }
    }
    if (scoped || m_Models.GetCount() == 1)
      targets.PushBack(selected);
  }
  if (targets.IsEmpty())
  {
    m_pStatus->setText(tr("Select a scene object with a matching MuJoCo binding component under this model instance."));
    return;
  }
  auto* accessor = document->GetObjectAccessor();
  accessor->StartTransaction("Bind MuJoCo Element");
  for (const auto* target : targets)
  {
    const auto result = accessor->SetValueByName(target, property, m_sSelectedName);
    if (result.Failed())
    {
      accessor->CancelTransaction();
      m_pStatus->setText(QString::fromUtf8(result.GetMessageString().GetData()));
      return;
    }
  }
  accessor->FinishTransaction();
}

void xiiQtMuJoCoSceneInspector::OpenModelAsset()
{
  const int index = m_pModels->currentIndex();
  if (index < 0 || static_cast<xiiUInt32>(index) >= m_Models.GetCount())
    return;
  const auto resource = Read<xiiString>(m_Models[index].Get<xiiVariantDictionary>(), "Resource");
  const auto asset    = xiiAssetCurator::GetSingleton()->FindSubAsset(resource);
  if (asset.isValid())
    xiiQtEditorApp::GetSingleton()->OpenDocumentQueued(asset->m_pAssetInfo->m_Path.GetAbsolutePath());
  else
    m_pStatus->setText(tr("The selected resource has no matching asset document."));
}

XII_STATICLINK_FILE(EditorPluginMujoco, EditorPluginMujoco_Authoring_Implementation_MuJoCoSceneInspector);
