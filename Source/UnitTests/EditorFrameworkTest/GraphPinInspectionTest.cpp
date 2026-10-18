/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QApplication>
#include <QDoubleSpinBox>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>

class xiiInspectorPinSettings : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInspectorPinSettings, xiiReflectedClass);

public:
  float     m_fValue    = 0.25f;
  bool      m_bEnabled  = true;
  xiiString m_sContract = "Read only metadata";
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInspectorPinSettings, 1, xiiRTTIDefaultAllocator<xiiInspectorPinSettings>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Value", m_fValue)->AddAttributes(new xiiDefaultValueAttribute(0.25f), new xiiClampValueAttribute(0.0f, 1.0f)),
      XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_MEMBER_PROPERTY("Contract", m_sContract)->AddAttributes(new xiiReadOnlyAttribute()),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiInspectorGraphConnection : public xiiDocumentObject_ConnectionBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInspectorGraphConnection, xiiDocumentObject_ConnectionBase);

public:
  float     m_fWeight = 1.0f;
  xiiString m_sLabel;
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInspectorGraphConnection, 1, xiiRTTIDefaultAllocator<xiiInspectorGraphConnection>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Weight", m_fWeight),
      XII_MEMBER_PROPERTY("Label", m_sLabel),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiInspectorGraphNode : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInspectorGraphNode, xiiReflectedClass);

public:
  bool                     m_bHasInput = true;
  xiiString                m_sLabel;
  xiiInspectorPinSettings* m_pSettings = nullptr;
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInspectorGraphNode, 1, xiiRTTIDefaultAllocator<xiiInspectorGraphNode>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("HasInput", m_bHasInput)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_MEMBER_PROPERTY("Label", m_sLabel),
      XII_MEMBER_PROPERTY("Settings", m_pSettings)->AddFlags(xiiPropertyFlags::PointerOwner),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  class InspectorNodeManager : public xiiDocumentNodeManager
  {
  public:
    const xiiRTTI*           GetConnectionType() const override { return xiiGetStaticRTTI<xiiInspectorGraphConnection>(); }
    const xiiDocumentObject* GetPinPropertyObject(const xiiPin& pin) const override
    {
      if (pin.GetType() != xiiPin::Type::Input)
        return nullptr;
      return GetObject(pin.GetParent()->GetTypeAccessor().GetValue("Settings").Get<xiiUuid>());
    }

  private:
    bool InternalIsNode(const xiiDocumentObject* object) const override { return object->GetType()->IsDerivedFrom<xiiInspectorGraphNode>(); }
    bool InternalIsDynamicPinProperty(const xiiDocumentObject*, const xiiAbstractProperty* property) const override
    {
      return property->GetPropertyName() == "HasInput" || property->GetPropertyName() == "Label" || property->GetPropertyName() == "Settings";
    }
    void InternalCreatePins(const xiiDocumentObject* object, NodeInternal& node) override
    {
      if (object->GetTypeAccessor().GetValue("HasInput").Get<bool>())
        node.m_Inputs.PushBack(XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Input, "Value", xiiColor::Blue, object));
      node.m_Outputs.PushBack(XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Output, "Value", xiiColor::Green, object));
    }
  };

  class InspectorDocument : public xiiDocument
  {
  public:
    explicit InspectorDocument(xiiStringView path) : xiiDocument(path, XII_DEFAULT_NEW(InspectorNodeManager))
    {
      m_pDocumentInfo   = CreateDocumentInfo();
      m_pObjectAccessor = XII_DEFAULT_NEW(xiiNodeCommandAccessor, GetCommandHistory());
    }
    xiiDocumentInfo* CreateDocumentInfo() override { return XII_DEFAULT_NEW(xiiDocumentInfo); }

  private:
    void AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const override
    {
      xiiDocument::AttachMetaDataBeforeSaving(graph);
      static_cast<const InspectorNodeManager*>(GetObjectManager())->AttachMetaDataBeforeSaving(graph);
    }
    void RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool undoable) override
    {
      xiiDocument::RestoreMetaDataAfterLoading(graph, undoable);
      static_cast<InspectorNodeManager*>(GetObjectManager())->RestoreMetaDataAfterLoading(graph, undoable);
    }
    bool Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& graph, bool picked, xiiStringView) override
    {
      return static_cast<InspectorNodeManager*>(GetObjectManager())->PasteObjects(info, graph, xiiVec2::MakeZero(), picked);
    }
  };

  class InspectorScene : public xiiQtNodeScene
  {
  public:
    xiiQtPin* FindPin(const xiiDocumentObject* object, bool input)
    {
      auto node = m_Nodes.Find(object);
      if (!node.IsValid())
        return nullptr;
      const auto& pins = input ? node.Value()->GetInputPins() : node.Value()->GetOutputPins();
      return pins.IsEmpty() ? nullptr : pins[0];
    }
    void Click(xiiQtPin* pin)
    {
      QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
      press.setButton(Qt::LeftButton);
      press.setScenePos(pin->GetPinPos());
      press.setScreenPos(QPoint(100, 100));
      mousePressEvent(&press);
      QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
      release.setButton(Qt::LeftButton);
      release.setScenePos(pin->GetPinPos());
      release.setScreenPos(QPoint(100, 100));
      release.setButtonDownScreenPos(Qt::LeftButton, QPoint(100, 100));
      mouseReleaseEvent(&release);
    }
    void Drag(xiiQtPin* from, xiiQtPin* to)
    {
      QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
      press.setButton(Qt::LeftButton);
      press.setScenePos(from->GetPinPos());
      mousePressEvent(&press);
      QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
      move.setScenePos(to->GetPinPos());
      mouseMoveEvent(&move);
      QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
      release.setButton(Qt::LeftButton);
      release.setScenePos(to->GetPinPos());
      release.setScreenPos(QPoint(200, 100));
      release.setButtonDownScreenPos(Qt::LeftButton, QPoint(100, 100));
      mouseReleaseEvent(&release);
    }
    void DeleteSelection()
    {
      QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
      move.setScenePos(QPointF(-10000, -10000));
      mouseMoveEvent(&move);
      QKeyEvent remove(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
      keyPressEvent(&remove);
    }
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorDocuments, GraphPinInspection)
{
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "PinInspectionTest", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("PinInspectionTest"));
  xiiStringBuilder  path(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/GraphPinInspection.xiiGraph");
  InspectorDocument document(path);
  auto*             accessor = document.GetObjectAccessor();
  auto*             manager  = static_cast<InspectorNodeManager*>(document.GetObjectManager());
  xiiUuid           nodes[2], settings[2];
  accessor->StartTransaction("Create graph");
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiInspectorGraphNode>(), nodes[i]));
    XII_TEST_STATUS(accessor->AddObjectByName(accessor->GetObject(nodes[i]), "Settings", {}, xiiGetStaticRTTI<xiiInspectorPinSettings>(), settings[i]));
    manager->MoveNode(accessor->GetObject(nodes[i]), xiiVec2(i * 400.0f, 0));
  }
  accessor->FinishTransaction();
  InspectorScene scene;
  scene.InitScene(manager);
  xiiQtPropertyGridWidget grid(nullptr, &document);
  auto*                   owner      = accessor->GetObject(nodes[1]);
  auto*                   properties = accessor->GetObject(settings[1]);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "A pin click selects its persistent properties and uses ordinary reflected widgets")
  {
    scene.Click(scene.FindPin(owner, true));
    XII_TEST_BOOL(document.GetSelectionManager()->GetCurrentObject() == properties);
    XII_TEST_BOOL(scene.FindPin(owner, true)->isSelected());
    xiiQtPropertyWidget* valueWidget    = nullptr;
    xiiQtPropertyWidget* contractWidget = nullptr;
    for (auto* widget : grid.findChildren<xiiQtPropertyWidget*>())
    {
      if (widget->GetProperty() && widget->GetProperty()->GetPropertyName() == "Value")
        valueWidget = widget;
      if (widget->GetProperty() && widget->GetProperty()->GetPropertyName() == "Contract")
        contractWidget = widget;
    }
    if (!XII_TEST_BOOL(valueWidget && contractWidget))
      return;
    XII_TEST_BOOL(valueWidget->GetSelection()[0].m_pObject == properties);
    const auto* contract = contractWidget->findChild<QLineEdit*>();
    XII_TEST_BOOL(contract && contract->isReadOnly());
    auto* spin = valueWidget->findChild<QDoubleSpinBox*>();
    if (!XII_TEST_BOOL(spin != nullptr))
      return;
    XII_TEST_FLOAT(spin->minimum(), 0, 0);
    XII_TEST_FLOAT(spin->maximum(), 1, 0);
    spin->setValue(0.75);
    QMetaObject::invokeMethod(spin, "editingFinished", Qt::DirectConnection);
    XII_TEST_FLOAT(properties->GetTypeAccessor().GetValue("Value").Get<float>(), 0.75f, 0);
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
    XII_TEST_FLOAT(properties->GetTypeAccessor().GetValue("Value").Get<float>(), 0.25f, 0);
    XII_TEST_STATUS(document.GetCommandHistory()->Redo());
    XII_TEST_FLOAT(properties->GetTypeAccessor().GetValue("Value").Get<float>(), 0.75f, 0);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Rebuild selection, drag wiring, dynamic pin removal and undo preserve valid document state")
  {
    accessor->StartTransaction("Rebuild pins");
    XII_TEST_STATUS(accessor->SetValueByName(owner, "Label", "Changed"));
    accessor->FinishTransaction();
    XII_TEST_BOOL(scene.FindPin(owner, true)->isSelected());
    XII_TEST_BOOL(document.GetSelectionManager()->GetCurrentObject() == properties);
    scene.Drag(scene.FindPin(accessor->GetObject(nodes[0]), false), scene.FindPin(owner, true));
    const auto* input = manager->GetInputPinByName(owner, "Value");
    XII_TEST_INT(manager->GetConnections(*input).GetCount(), 1);
    const auto* authoredConnection = manager->GetConnections(*input)[0]->GetParent();
    const auto  connectionId       = authoredConnection->GetGuid();
    accessor->StartTransaction("Configure connection and fan out");
    XII_TEST_STATUS(accessor->SetValueByName(authoredConnection, "Weight", 0.625f));
    XII_TEST_STATUS(accessor->SetValueByName(authoredConnection, "Label", "Authored connection"));
    xiiUuid thirdNode;
    XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiInspectorGraphNode>(), thirdNode));
    const auto* output     = manager->GetOutputPinByName(accessor->GetObject(nodes[0]), "Value");
    const auto* thirdInput = manager->GetInputPinByName(accessor->GetObject(thirdNode), "Value");
    XII_TEST_STATUS(xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *output, *thirdInput));
    accessor->FinishTransaction();
    const auto otherConnectionId = manager->GetConnections(*thirdInput)[0]->GetParent()->GetGuid();
    document.GetSelectionManager()->SetSelection(authoredConnection);
    accessor->StartTransaction("Rebuild a source with multiple connected outputs");
    XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(nodes[0]), "Label", "Rebuilt source"));
    accessor->FinishTransaction();
    input      = manager->GetInputPinByName(owner, "Value");
    thirdInput = manager->GetInputPinByName(accessor->GetObject(thirdNode), "Value");
    XII_TEST_INT(manager->GetConnections(*manager->GetOutputPinByName(accessor->GetObject(nodes[0]), "Value")).GetCount(), 2);
    XII_TEST_BOOL(manager->GetConnections(*input)[0]->GetParent() == authoredConnection);
    XII_TEST_BOOL(manager->GetConnections(*thirdInput)[0]->GetParent()->GetGuid() == otherConnectionId);
    XII_TEST_BOOL(document.GetSelectionManager()->GetCurrentObject() == authoredConnection);
    if (XII_TEST_INT(scene.selectedItems().size(), 1))
      XII_TEST_INT(scene.selectedItems()[0]->type(), xiiQtNodeScene::Connection);
    XII_TEST_FLOAT(authoredConnection->GetTypeAccessor().GetValue("Weight").Get<float>(), 0.625f, 0);
    XII_TEST_STRING(authoredConnection->GetTypeAccessor().GetValue("Label").Get<xiiString>(), "Authored connection");
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
    XII_TEST_STATUS(document.GetCommandHistory()->Redo());
    input = manager->GetInputPinByName(owner, "Value");
    XII_TEST_BOOL(manager->GetConnections(*input)[0]->GetParent()->GetGuid() == connectionId);
    accessor->StartTransaction("Edit connected pin settings");
    XII_TEST_STATUS(accessor->SetValueByName(properties, "Value", 0.5f));
    accessor->FinishTransaction();
    input = manager->GetInputPinByName(owner, "Value");
    XII_TEST_BOOL(manager->GetConnections(*input)[0]->GetParent() == authoredConnection);
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
    input = manager->GetInputPinByName(owner, "Value");
    scene.Click(scene.FindPin(owner, true));
    accessor->StartTransaction("Remove dynamic input");
    XII_TEST_STATUS(accessor->SetValueByName(owner, "HasInput", false));
    accessor->FinishTransaction();
    XII_TEST_BOOL(scene.FindPin(owner, true) == nullptr);
    XII_TEST_BOOL(document.GetSelectionManager()->IsSelectionEmpty());
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
    input = manager->GetInputPinByName(owner, "Value");
    if (!XII_TEST_BOOL(input != nullptr))
      return;
    XII_TEST_INT(manager->GetConnections(*input).GetCount(), 1);
    const auto* connection = manager->GetConnections(*input)[0]->GetParent();
    XII_TEST_BOOL(connection->GetGuid() == connectionId);
    XII_TEST_FLOAT(connection->GetTypeAccessor().GetValue("Weight").Get<float>(), 0.625f, 0);
    document.GetSelectionManager()->SetSelection(connection);
    if (!XII_TEST_INT(scene.selectedItems().size(), 1))
      return;
    XII_TEST_INT(scene.selectedItems()[0]->type(), xiiQtNodeScene::Connection);
    scene.DeleteSelection();
    XII_TEST_INT(manager->GetConnections(*input).GetCount(), 0);
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
    XII_TEST_INT(manager->GetConnections(*input).GetCount(), 1);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Node copy and paste remap owned pin properties and preserve their values")
  {
    document.GetSelectionManager()->SetSelection(owner);
    xiiAbstractObjectGraph graph;
    XII_TEST_BOOL(manager->CopySelectedObjects(graph));
    xiiContiguousMemoryStreamStorage storage;
    xiiMemoryStreamWriter            writer(&storage);
    xiiAbstractGraphDdlSerializer::Write(writer, &graph);
    xiiPasteObjectsCommand paste;
    paste.m_sMimeType        = "application/xiiEditor.PinInspectorTest";
    paste.m_sGraphTextFormat = xiiStringView(reinterpret_cast<const char*>(storage.GetData()), storage.GetStorageSize32());
    accessor->StartTransaction("Duplicate node");
    XII_TEST_STATUS(document.GetCommandHistory()->AddCommand(paste));
    accessor->FinishTransaction();
    const auto* copy = document.GetSelectionManager()->GetCurrentObject();
    if (!XII_TEST_BOOL(copy && copy != owner))
      return;
    const auto* copiedProperties = manager->GetPinPropertyObject(*manager->GetInputPinByName(copy, "Value"));
    XII_TEST_BOOL(copiedProperties && copiedProperties->GetGuid() != properties->GetGuid());
    XII_TEST_FLOAT(copiedProperties->GetTypeAccessor().GetValue("Value").Get<float>(), 0.75f, 0);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Saving and reloading preserves pin identity and edited values")
  {
    XII_TEST_STATUS(document.SaveDocument(true));
    InspectorDocument loaded(path);
    XII_TEST_STATUS(loaded.LoadDocument());
    const auto* loadedProperties = loaded.GetObjectManager()->GetObject(settings[1]);
    if (!XII_TEST_BOOL(loadedProperties != nullptr))
      return;
    XII_TEST_FLOAT(loadedProperties->GetTypeAccessor().GetValue("Value").Get<float>(), 0.75f, 0);
    InspectorScene loadedScene;
    loadedScene.InitScene(static_cast<const InspectorNodeManager*>(loaded.GetObjectManager()));
    loaded.GetSelectionManager()->SetSelection(loadedProperties);
    XII_TEST_BOOL(loadedScene.FindPin(loaded.GetObjectManager()->GetObject(nodes[1]), true)->isSelected());
  }
}
