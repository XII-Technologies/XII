#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Gravity.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Continuous.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomColor.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_VelocityCone.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>
#include <QBoxLayout>
#include <QComboBox>
#include <QInputDialog>
#include <QToolButton>
#include <SharedPluginAssets/Common/Messages.h>
#include <ToolsFoundation/Command/TreeCommands.h>

xiiQtParticleEffectAssetDocumentWindow::xiiQtParticleEffectAssetDocumentWindow(xiiAssetDocument* pDocument) :
  xiiQtEngineDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiQtParticleEffectAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiQtParticleEffectAssetDocumentWindow::StructureEventHandler, this));


  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "ParticleEffectAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "ParticleEffectAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ParticleEffectAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  xiiDocumentObject* pRootObject = pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0];

  // xiiQtDocumentPanel* pMainPropertyPanel = new xiiQtDocumentPanel(this);
  xiiQtDocumentPanel* pEffectPanel      = new xiiQtDocumentPanel(this, pDocument);
  xiiQtDocumentPanel* pReactionsPanel   = new xiiQtDocumentPanel(this, pDocument);
  xiiQtDocumentPanel* pSystemsPanel     = new xiiQtDocumentPanel(this, pDocument);
  xiiQtDocumentPanel* pEmitterPanel     = new xiiQtDocumentPanel(this, pDocument);
  xiiQtDocumentPanel* pInitializerPanel = new xiiQtDocumentPanel(this, pDocument);
  xiiQtDocumentPanel* pBehaviorPanel    = new xiiQtDocumentPanel(this, pDocument);
  xiiQtDocumentPanel* pTypePanel        = new xiiQtDocumentPanel(this, pDocument);

  // Property Grid
  //{
  //  pMainPropertyPanel->setObjectName("ParticleEffectAssetDockWidget");
  //  pMainPropertyPanel->setWindowTitle("Particle Effect Properties");
  //  pMainPropertyPanel->show();

  //  xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pMainPropertyPanel, pDocument);
  //  pMainPropertyPanel->setWidget(pPropertyGrid);

  //  addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pMainPropertyPanel);

  //  pDocument->GetSelectionManager()->SetSelection(pRootObject);
  //}

  // Particle Systems Panel
  {
    pSystemsPanel->setObjectName("ParticleEffectAssetDockWidget_Systems");
    pSystemsPanel->setWindowTitle("Systems");
    pSystemsPanel->show();

    QWidget* pMainWidget = new QWidget(pSystemsPanel);
    pMainWidget->setContentsMargins(0, 0, 0, 0);
    pMainWidget->setLayout(new QVBoxLayout(pMainWidget));
    pMainWidget->layout()->setContentsMargins(0, 0, 0, 0);

    {
      QWidget* pGroup = new QWidget(pMainWidget);
      pGroup->setContentsMargins(0, 0, 0, 0);
      pGroup->setLayout(new QHBoxLayout(pGroup));
      pGroup->layout()->setContentsMargins(0, 0, 0, 0);

      m_pSystemsCombo = new QComboBox(pSystemsPanel);
      connect(m_pSystemsCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onSystemSelected(int)));

      m_pAddSystem = new QToolButton(pSystemsPanel);
      connect(m_pAddSystem, &QAbstractButton::clicked, this, &xiiQtParticleEffectAssetDocumentWindow::onAddSystem);

      m_pRemoveSystem = new QToolButton(pSystemsPanel);
      connect(m_pRemoveSystem, &QAbstractButton::clicked, this, &xiiQtParticleEffectAssetDocumentWindow::onRemoveSystem);

      m_pRenameSystem = new QToolButton(pSystemsPanel);
      connect(m_pRenameSystem, &QAbstractButton::clicked, this, &xiiQtParticleEffectAssetDocumentWindow::onRenameSystem);

      m_pAddSystem->setIcon(QIcon(":/GuiFoundation/Icons/Add16.png"));
      m_pRemoveSystem->setIcon(QIcon(":/GuiFoundation/Icons/Delete16.png"));
      m_pRenameSystem->setIcon(QIcon(":/GuiFoundation/Icons/Rename16.png"));

      pGroup->layout()->addWidget(m_pRenameSystem);
      pGroup->layout()->addWidget(m_pSystemsCombo);
      pGroup->layout()->addWidget(m_pAddSystem);
      pGroup->layout()->addWidget(m_pRemoveSystem);

      pMainWidget->layout()->addWidget(pGroup);
    }

    m_pPropertyGridSystems = new xiiQtPropertyGridWidget(pSystemsPanel, pDocument);
    m_pPropertyGridSystems->SetSelectionIncludeExcludeProperties(nullptr, "Name;Emitters;Initializers;Behaviors;Types");
    pMainWidget->layout()->addWidget(m_pPropertyGridSystems);

    if (!pRootObject->GetChildren().IsEmpty())
    {
      xiiDeque<const xiiDocumentObject*> sel;
      sel.PushBack(pRootObject->GetChildren()[0]);
      m_pPropertyGridSystems->SetSelection(sel);
    }

    pMainWidget->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding));
    pSystemsPanel->setWidget(pMainWidget);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pSystemsPanel);
  }

  // Effect Properties
  {
    pEffectPanel->setObjectName("ParticleEffectAssetDockWidget_Effect");
    pEffectPanel->setWindowTitle("Effect");
    pEffectPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pEffectPanel, pDocument, false);
    pEffectPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pEffectPanel);

    xiiDeque<const xiiDocumentObject*> sel;
    sel.PushBack(pRootObject);
    pPropertyGrid->SetSelectionIncludeExcludeProperties(nullptr, "EventReactions;ParticleSystems");
    pPropertyGrid->SetSelection(sel);
  }

  // Event Reactions
  {
    pReactionsPanel->setObjectName("ParticleEffectAssetDockWidget_Reactions");
    pReactionsPanel->setWindowTitle("Event Reactions");
    pReactionsPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pReactionsPanel, pDocument, false);
    pReactionsPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pReactionsPanel);

    xiiDeque<const xiiDocumentObject*> sel;
    sel.PushBack(pRootObject);
    pPropertyGrid->SetSelectionIncludeExcludeProperties("EventReactions");
    pPropertyGrid->SetSelection(sel);
  }

  // System Emitters
  {
    pEmitterPanel->setObjectName("ParticleEffectAssetDockWidget_Emitter");
    pEmitterPanel->setWindowTitle("Emitter");
    pEmitterPanel->show();

    m_pPropertyGridEmitter = new xiiQtPropertyGridWidget(pEmitterPanel, pDocument, false);
    m_pPropertyGridEmitter->SetSelectionIncludeExcludeProperties("Emitters");
    pEmitterPanel->setWidget(m_pPropertyGridEmitter);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pEmitterPanel);
  }

  // System Initializers
  {
    pInitializerPanel->setObjectName("ParticleEffectAssetDockWidget_Initializer");
    pInitializerPanel->setWindowTitle("Initializers");
    pInitializerPanel->show();

    m_pPropertyGridInitializer = new xiiQtPropertyGridWidget(pInitializerPanel, pDocument, false);
    m_pPropertyGridInitializer->SetSelectionIncludeExcludeProperties("Initializers");
    pInitializerPanel->setWidget(m_pPropertyGridInitializer);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pInitializerPanel);
  }

  // System Behaviors
  {
    pBehaviorPanel->setObjectName("ParticleEffectAssetDockWidget_Behavior");
    pBehaviorPanel->setWindowTitle("Behaviors");
    pBehaviorPanel->show();

    m_pPropertyGridBehavior = new xiiQtPropertyGridWidget(pBehaviorPanel, pDocument, false);
    m_pPropertyGridBehavior->SetSelectionIncludeExcludeProperties("Behaviors");
    pBehaviorPanel->setWidget(m_pPropertyGridBehavior);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pBehaviorPanel);
  }

  // System Types
  {
    pTypePanel->setObjectName("ParticleEffectAssetDockWidget_Type");
    pTypePanel->setWindowTitle("Renderers");
    pTypePanel->show();

    m_pPropertyGridType = new xiiQtPropertyGridWidget(pTypePanel, pDocument, false);
    m_pPropertyGridType->SetSelectionIncludeExcludeProperties("Types");
    pTypePanel->setWidget(m_pPropertyGridType);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pTypePanel);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(xiiVec3(-1.6f, 0, 0), xiiVec3(0, 0, 0), xiiVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new xiiQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureRelative(xiiVec3(0), xiiVec3(5.0f), xiiVec3(-2, 0, 0.5f), 1.0f);

    AddViewWidget(m_pViewWidget);
    xiiQtViewWidgetContainer* pContainer = new xiiQtViewWidgetContainer(this, m_pViewWidget, "ParticleEffectAssetViewToolBar");
    setCentralWidget(pContainer);
  }

  m_pAssetDoc = static_cast<xiiParticleEffectAssetDocument*>(pDocument);

  tabifyDockWidget(pEffectPanel, pSystemsPanel);
  tabifyDockWidget(pEffectPanel, pReactionsPanel);

  tabifyDockWidget(pEmitterPanel, pInitializerPanel);
  tabifyDockWidget(pEmitterPanel, pBehaviorPanel);
  tabifyDockWidget(pEmitterPanel, pTypePanel);

  pSystemsPanel->raise();
  pEmitterPanel->raise();

  FinishWindowCreation();

  UpdateSystemList();
  SendLiveResourcePreview();

  GetParticleDocument()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtParticleEffectAssetDocumentWindow::ParticleEventHandler, this));
}

xiiQtParticleEffectAssetDocumentWindow::~xiiQtParticleEffectAssetDocumentWindow()
{
  GetParticleDocument()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtParticleEffectAssetDocumentWindow::ParticleEventHandler, this));

  RestoreResource();

  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtParticleEffectAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtParticleEffectAssetDocumentWindow::PropertyEventHandler, this));
}

const char* xiiQtParticleEffectAssetDocumentWindow::GetWindowLayoutGroupName() const
{
  return "ParticleEffectAsset2";
}

xiiParticleEffectAssetDocument* xiiQtParticleEffectAssetDocumentWindow::GetParticleDocument()
{
  return static_cast<xiiParticleEffectAssetDocument*>(GetDocument());
}

void xiiQtParticleEffectAssetDocumentWindow::SelectSystem(xiiDocumentObject* pObject)
{
  if (pObject == nullptr)
  {
    m_sSelectedSystem.Clear();

    m_pPropertyGridSystems->ClearSelection();
    m_pPropertyGridEmitter->ClearSelection();
    m_pPropertyGridInitializer->ClearSelection();
    m_pPropertyGridBehavior->ClearSelection();
    m_pPropertyGridType->ClearSelection();

    m_pSystemsCombo->setCurrentIndex(-1);
  }
  else
  {
    m_sSelectedSystem = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<xiiString>();

    xiiDeque<const xiiDocumentObject*> sel;
    sel.PushBack(pObject);
    GetDocument()->GetSelectionManager()->SetSelection(pObject);
    m_pPropertyGridSystems->SetSelection(sel);

    m_pPropertyGridEmitter->SetSelection(sel);
    m_pPropertyGridInitializer->SetSelection(sel);
    m_pPropertyGridBehavior->SetSelection(sel);
    m_pPropertyGridType->SetSelection(sel);

    m_pSystemsCombo->setCurrentText(m_sSelectedSystem.GetData());
  }
}

void xiiQtParticleEffectAssetDocumentWindow::onSystemSelected(int index)
{
  if (index >= 0)
  {
    xiiDocumentObject* pObject = static_cast<xiiDocumentObject*>(m_pSystemsCombo->itemData(index).value<void*>());

    SelectSystem(pObject);
  }
  else
  {
    SelectSystem(nullptr);
  }
}

void xiiQtParticleEffectAssetDocumentWindow::onAddSystem(bool)
{
  bool    ok = false;
  QString sName;

  while (true)
  {
    sName = QInputDialog::getText(this, "New Particle System", "Name:", QLineEdit::Normal, QString(), &ok);

    if (!ok)
      return;

    if (sName.isEmpty())
    {
      xiiQtUiServices::GetSingleton()->MessageBoxInformation("Invalid particle system name.");
      continue;
    }

    if (m_ParticleSystems.Find(sName.toUtf8().data()).IsValid())
    {
      xiiQtUiServices::GetSingleton()->MessageBoxInformation("A particle system with this name exists already.");
      continue;
    }

    break;
  }

  xiiDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  GetDocument()->GetObjectAccessor()->StartTransaction("Add Particle System");
  xiiUuid systemGuid;
  systemGuid.CreateNewUuid();

  {
    xiiAddObjectCommand cmd;
    cmd.m_Parent          = pRootObject->GetGuid();
    cmd.m_Index           = -1;
    cmd.m_pType           = xiiGetStaticRTTI<xiiParticleSystemDescriptor>();
    cmd.m_NewObjectGuid   = systemGuid;
    cmd.m_sParentProperty = "ParticleSystems";

    if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
    {
      GetDocument()->GetObjectAccessor()->CancelTransaction();
      return;
    }
  }

  m_sSelectedSystem = sName.toUtf8().data();

  {
    xiiSetObjectPropertyCommand cmd;
    cmd.m_Object    = systemGuid;
    cmd.m_NewValue  = sName.toUtf8().data();
    cmd.m_sProperty = "Name";
    cmd.m_Index     = 0;

    if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
    {
      GetDocument()->GetObjectAccessor()->CancelTransaction();
      return;
    }
  }

  // default system setup
  {
    const xiiDocumentObject* pSystemObject = GetDocument()->GetObjectAccessor()->GetObject(systemGuid);

    // default life
    {
      const xiiHybridArray<xiiDocumentObject*, 8>& children = pSystemObject->GetChildren();

      for (auto pChild : children)
      {
        if (xiiStringUtils::IsEqual(pChild->GetParentProperty(), "LifeTime"))
        {
          xiiSetObjectPropertyCommand cmd;
          cmd.m_Object    = pChild->GetGuid();
          cmd.m_NewValue  = xiiTime::Seconds(1);
          cmd.m_sProperty = "Value";
          cmd.m_Index     = 0;

          if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
          {
            GetDocument()->GetObjectAccessor()->CancelTransaction();
            return;
          }

          break;
        }
      }
    }

    // add emitter
    {
      xiiAddObjectCommand cmd;
      cmd.m_Parent          = systemGuid;
      cmd.m_Index           = -1;
      cmd.m_pType           = xiiGetStaticRTTI<xiiParticleEmitterFactory_Continuous>();
      cmd.m_sParentProperty = "Emitters";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }
    }

    // add cone velocity initializer
    {
      xiiAddObjectCommand cmd;
      cmd.m_Parent          = systemGuid;
      cmd.m_Index           = -1;
      cmd.m_pType           = xiiGetStaticRTTI<xiiParticleInitializerFactory_VelocityCone>();
      cmd.m_sParentProperty = "Initializers";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }

      const xiiDocumentObject* pConeObject = GetDocument()->GetObjectAccessor()->GetObject(cmd.m_NewObjectGuid);

      // default speed
      {
        const xiiHybridArray<xiiDocumentObject*, 8>& children = pConeObject->GetChildren();

        for (auto pChild : children)
        {
          if (xiiStringUtils::IsEqual(pChild->GetParentProperty(), "Speed"))
          {
            xiiSetObjectPropertyCommand cmd2;
            cmd2.m_Object    = pChild->GetGuid();
            cmd2.m_NewValue  = 4.0f;
            cmd2.m_sProperty = "Value";
            cmd2.m_Index     = 0;

            if (GetDocument()->GetCommandHistory()->AddCommand(cmd2).Failed())
            {
              GetDocument()->GetObjectAccessor()->CancelTransaction();
              return;
            }

            break;
          }
        }
      }
    }

    // add color initializer
    {
      xiiAddObjectCommand cmd;
      cmd.m_Parent          = systemGuid;
      cmd.m_Index           = -1;
      cmd.m_pType           = xiiGetStaticRTTI<xiiParticleInitializerFactory_RandomColor>();
      cmd.m_sParentProperty = "Initializers";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }

      // color 1
      {
        xiiSetObjectPropertyCommand cmd2;
        cmd2.m_Object    = cmd.m_NewObjectGuid;
        cmd2.m_sProperty = "Color1";
        cmd2.m_NewValue  = xiiColor::Red;

        if (GetDocument()->GetCommandHistory()->AddCommand(cmd2).Failed())
        {
          GetDocument()->GetObjectAccessor()->CancelTransaction();
          return;
        }
      }

      // color 2
      {
        xiiSetObjectPropertyCommand cmd2;
        cmd2.m_Object    = cmd.m_NewObjectGuid;
        cmd2.m_sProperty = "Color2";
        cmd2.m_NewValue  = xiiColor::Yellow;

        if (GetDocument()->GetCommandHistory()->AddCommand(cmd2).Failed())
        {
          GetDocument()->GetObjectAccessor()->CancelTransaction();
          return;
        }
      }
    }

    // add gravity behavior
    {
      xiiAddObjectCommand cmd;
      cmd.m_Parent          = systemGuid;
      cmd.m_Index           = -1;
      cmd.m_pType           = xiiGetStaticRTTI<xiiParticleBehaviorFactory_Gravity>();
      cmd.m_sParentProperty = "Behaviors";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }
    }

    // add quad renderer
    {
      xiiAddObjectCommand cmd;
      cmd.m_Parent          = systemGuid;
      cmd.m_Index           = -1;
      cmd.m_pType           = xiiGetStaticRTTI<xiiParticleTypeQuadFactory>();
      cmd.m_sParentProperty = "Types";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }
    }
  }

  GetDocument()->GetObjectAccessor()->FinishTransaction();
}

void xiiQtParticleEffectAssetDocumentWindow::onRemoveSystem(bool)
{
  const int index = m_pSystemsCombo->findText(m_sSelectedSystem.GetData());
  if (index < 0)
    return;

  const xiiDocumentObject* pObject = static_cast<xiiDocumentObject*>(m_pSystemsCombo->itemData(index).value<void*>());

  xiiDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  GetDocument()->GetObjectAccessor()->StartTransaction("Rename Particle System");

  xiiRemoveObjectCommand cmd;
  cmd.m_Object = pObject->GetGuid();

  if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
  {
    GetDocument()->GetObjectAccessor()->CancelTransaction();
    return;
  }

  GetDocument()->GetObjectAccessor()->FinishTransaction();
}

void xiiQtParticleEffectAssetDocumentWindow::onRenameSystem(bool)
{
  const int index = m_pSystemsCombo->findText(m_sSelectedSystem.GetData());
  if (index < 0)
    return;

  const xiiDocumentObject* pObject = static_cast<xiiDocumentObject*>(m_pSystemsCombo->itemData(index).value<void*>());

  bool          ok       = false;
  const QString sOrgName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<xiiString>().GetData();
  QString       sName;

  while (true)
  {
    sName = QInputDialog::getText(this, "Rename Particle System", "Name:", QLineEdit::Normal, sOrgName, &ok);

    if (!ok || sName == sOrgName)
      return;

    if (sName.isEmpty())
    {
      xiiQtUiServices::GetSingleton()->MessageBoxInformation("Invalid particle system name.");
      continue;
    }

    if (m_ParticleSystems.Find(sName.toUtf8().data()).IsValid())
    {
      xiiQtUiServices::GetSingleton()->MessageBoxInformation("A particle system with this name exists already.");
      continue;
    }

    break;
  }

  xiiDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  m_sSelectedSystem = sName.toUtf8().data();

  GetDocument()->GetObjectAccessor()->StartTransaction("Rename Particle System");

  xiiSetObjectPropertyCommand cmd2;
  cmd2.m_Object    = pObject->GetGuid();
  cmd2.m_NewValue  = sName.toUtf8().data();
  cmd2.m_sProperty = "Name";
  cmd2.m_Index     = 0;

  if (GetDocument()->GetCommandHistory()->AddCommand(cmd2).Failed())
  {
    GetDocument()->GetObjectAccessor()->CancelTransaction();
    return;
  }

  GetDocument()->GetObjectAccessor()->FinishTransaction();
}

void xiiQtParticleEffectAssetDocumentWindow::SendLiveResourcePreview()
{
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  xiiResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Particle Effect";

  xiiStringBuilder tmp;
  msg.m_sResourceID = xiiConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  xiiContiguousMemoryStreamStorage streamStorage;
  xiiMemoryStreamWriter            memoryWriter(&streamStorage);

  // Write Path
  xiiStringBuilder sAbsFilePath = GetParticleDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("xiiParticleEffect");

  // Write Header
  memoryWriter << sAbsFilePath;
  const xiiUInt64    uiHash = xiiAssetCurator::GetSingleton()->GetAssetDependencyHash(GetParticleDocument()->GetGuid());
  xiiAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetParticleDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  GetParticleDocument()->WriteResource(memoryWriter);
  msg.m_Data = xiiArrayPtr<const xiiUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void xiiQtParticleEffectAssetDocumentWindow::PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == "Name" || e.m_sProperty == "ParticleSystems")
  {
    UpdateSystemList();
  }

  SendLiveResourcePreview();
}

void xiiQtParticleEffectAssetDocumentWindow::StructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case xiiDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectMoved2:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectRemoved:
      UpdateSystemList();
      SendLiveResourcePreview();
      break;

    default:
      break;
  }
}


void xiiQtParticleEffectAssetDocumentWindow::ParticleEventHandler(const xiiParticleEffectAssetEvent& e)
{
  switch (e.m_Type)
  {
    case xiiParticleEffectAssetEvent::RestartEffect:
    {
      xiiEditorEngineRestartSimulationMsg msg;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    break;

    case xiiParticleEffectAssetEvent::AutoRestartChanged:
    {
      xiiEditorEngineLoopAnimationMsg msg;
      msg.m_bLoop = GetParticleDocument()->GetAutoRestart();
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    break;

    default:
      break;
  }
}

void xiiQtParticleEffectAssetDocumentWindow::UpdateSystemList()
{
  xiiMap<xiiString, xiiDocumentObject*> newParticleSystems;

  xiiDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  xiiStringBuilder s;

  for (xiiDocumentObject* pChild : pRootObject->GetChildren())
  {
    if (xiiStringUtils::IsEqual(pChild->GetParentProperty(), "ParticleSystems"))
    {
      s                     = pChild->GetTypeAccessor().GetValue("Name").ConvertTo<xiiString>();
      newParticleSystems[s] = pChild;
    }
  }

  // early out
  if (m_ParticleSystems == newParticleSystems)
    return;

  m_ParticleSystems.Swap(newParticleSystems);

  {
    xiiQtScopedBlockSignals _1(m_pSystemsCombo);
    m_pSystemsCombo->clear();

    for (auto it = m_ParticleSystems.GetIterator(); it.IsValid(); ++it)
    {
      m_pSystemsCombo->addItem(it.Key().GetData(), QVariant::fromValue<void*>(it.Value()));
    }
  }

  if (!m_ParticleSystems.Find(m_sSelectedSystem).IsValid())
    m_sSelectedSystem.Clear();

  if (m_sSelectedSystem.IsEmpty() && !m_ParticleSystems.IsEmpty())
    m_sSelectedSystem = m_ParticleSystems.GetIterator().Key();

  if (!m_ParticleSystems.IsEmpty())
  {
    SelectSystem(m_ParticleSystems[m_sSelectedSystem]);
  }
  else
  {
    SelectSystem(nullptr);
  }

  const bool hasSelection = !m_ParticleSystems.IsEmpty();

  m_pSystemsCombo->setEnabled(hasSelection);
  m_pRemoveSystem->setEnabled(hasSelection);
  m_pRenameSystem->setEnabled(hasSelection);
}


void xiiQtParticleEffectAssetDocumentWindow::InternalRedraw()
{
  xiiEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  xiiQtEngineDocumentWindow::InternalRedraw();
}


void xiiQtParticleEffectAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    xiiSimulationSettingsMsgToEngine msg;
    msg.m_bSimulateWorld   = !GetParticleDocument()->GetSimulationPaused();
    msg.m_fSimulationSpeed = GetParticleDocument()->GetSimulationSpeed();
    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void xiiQtParticleEffectAssetDocumentWindow::RestoreResource()
{
  xiiRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Particle Effect";

  xiiStringBuilder tmp;
  msg.m_sResourceID = xiiConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
