/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/NodeEditor/CommentBlock.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QApplication>
#include <QClipboard>
#include <QDoubleSpinBox>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QImage>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QTimer>
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
  xiiString                m_sName;
  xiiInspectorPinSettings* m_pSettings = nullptr;
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInspectorGraphNode, 1, xiiRTTIDefaultAllocator<xiiInspectorGraphNode>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("HasInput", m_bHasInput)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_MEMBER_PROPERTY("Label", m_sLabel),
      XII_MEMBER_PROPERTY("Name", m_sName),
      XII_MEMBER_PROPERTY("Settings", m_pSettings)->AddFlags(xiiPropertyFlags::PointerOwner),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  class InspectorNodeManager : public xiiDocumentNodeManager
  {
  public:
    xiiUuid                  m_RejectMove;
    const xiiRTTI*           GetConnectionType() const override { return xiiGetStaticRTTI<xiiInspectorGraphConnection>(); }
    const xiiDocumentObject* GetPinPropertyObject(const xiiPin& pin) const override
    {
      if (pin.GetType() != xiiPin::Type::Input)
        return nullptr;
      return GetObject(pin.GetParent()->GetTypeAccessor().GetValue("Settings").Get<xiiUuid>());
    }

  private:
    xiiStatus InternalCanMoveNode(const xiiDocumentObject* object, const xiiVec2&) const override
    {
      return object->GetGuid() == m_RejectMove ? xiiStatus("Movement rejected by test graph.") : xiiStatus(XII_SUCCESS);
    }
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
    xiiVec2          m_vPickedPosition = xiiVec2::MakeZero();
    void             GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_types) const override
    {
      out_types.PushBack("application/xiiEditor.PinInspectorTest");
    }
    bool CopySelectedObjects(xiiAbstractObjectGraph& out_graph, xiiStringBuilder& out_type) const override
    {
      out_type = "application/xiiEditor.PinInspectorTest";
      return static_cast<const InspectorNodeManager*>(GetObjectManager())->CopySelectedObjects(out_graph);
    }

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
      return static_cast<InspectorNodeManager*>(GetObjectManager())->PasteObjects(info, graph, m_vPickedPosition, picked);
    }
  };

  class InspectorScene : public xiiQtNodeScene
  {
  public:
    void DragCommentBlock(const xiiUuid& id, QPointF delta, bool resize)
    {
      auto*                    block = m_CommentBlocks.Find(id).Value();
      const auto               start = resize ? block->mapToScene(QPointF(block->GetBlock().m_vSize.x - 4, block->GetBlock().m_vSize.y - 4)) : block->mapToScene(QPointF(12, 12));
      QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
      press.setButton(Qt::LeftButton);
      press.setPos(block->mapFromScene(start));
      press.setScenePos(start);
      mousePressEvent(&press);
      QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
      move.setButtons(Qt::LeftButton);
      move.setScenePos(start + delta);
      move.setPos(block->mapFromScene(start + delta));
      mouseMoveEvent(&move);
      QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
      release.setButton(Qt::LeftButton);
      release.setScenePos(start + delta);
      release.setPos(block->mapFromScene(start + delta));
      mouseReleaseEvent(&release);
    }
    void SelectCommentBlock(const xiiUuid& id)
    {
      clearSelection();
      m_CommentBlocks.Find(id).Value()->setSelected(true);
    }
    void   AddNodeToSelection(const xiiDocumentObject* object) { m_Nodes.Find(object).Value()->setSelected(true); }
    void   AddCommentBlockToSelection(const xiiUuid& id) { m_CommentBlocks.Find(id).Value()->setSelected(true); }
    QRectF Bounds(const xiiDocumentObject* object) const
    {
      auto* node = m_Nodes.Find(object).Value();
      return node->mapRectToScene(node->boundingRect().united(node->childrenBoundingRect()));
    }
    QRectF BodyBounds(const xiiDocumentObject* object) const { return m_Nodes.Find(object).Value()->boundingRect(); }
    QRectF CommentBounds(const xiiDocumentObject* object) const
    {
      const auto* node = m_Nodes.Find(object).Value();
      for (const auto* item : node->childItems())
        if (item->isVisible() && item->pos().y() < -5)
          return item->mapRectToParent(item->boundingRect());
      return {};
    }
    bool HasComment(const xiiDocumentObject* object, xiiStringView text) const
    {
      const auto* node = m_Nodes.Find(object).Value();
      for (const auto* item : node->childItems())
        if (const auto* label = dynamic_cast<const QGraphicsTextItem*>(item))
          if (label->isVisible() && label->toPlainText() == QString::fromUtf8(text.GetStartPointer(), text.GetElementCount()))
            return true;
      return false;
    }
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
    const xiiString comment    = "Shared graph annotation\nKeep the source in linear space. <plain text>";
    xiiUInt64       beforeHash = 123, afterHash = 123;
    manager->GetMetaDataHash(owner, beforeHash);
    const auto bodyBeforeComment = scene.BodyBounds(owner);
    QMenu      comments;
    scene.AddCommentActions(comments, owner);
    XII_TEST_INT(comments.actions().size(), 1);
    QTimer::singleShot(0, [&]() {
      if (auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget()))
      {
        dialog->setTextValue(QString::fromUtf8(comment.GetData()));
        dialog->accept();
      }
    });
    comments.actions()[0]->trigger();
    XII_TEST_STRING(manager->GetNodeComment(owner), comment);
    XII_TEST_BOOL(scene.HasComment(owner, comment));
    XII_TEST_BOOL(scene.BodyBounds(owner) == bodyBeforeComment);
    XII_TEST_BOOL(scene.CommentBounds(owner).bottom() < scene.BodyBounds(owner).top());
    {
      QImage capture(720, 420, QImage::Format_ARGB32_Premultiplied);
      capture.fill(QColor(32, 34, 38));
      QPainter painter(&capture);
      painter.setRenderHint(QPainter::Antialiasing);
      scene.render(&painter, QRectF(15, 15, 690, 390), scene.Bounds(owner).adjusted(-15, -15, 15, 15), Qt::KeepAspectRatio);
      painter.end();
      XII_TEST_BOOL(capture.save(xiiStringBuilder(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/GraphComment.png").GetData()));
    }
    manager->GetMetaDataHash(owner, afterHash);
    XII_TEST_INT(beforeHash, afterHash);
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
    XII_TEST_BOOL(manager->GetNodeComment(owner).IsEmpty());
    XII_TEST_BOOL(!scene.HasComment(owner, comment));
    XII_TEST_STATUS(document.GetCommandHistory()->Redo());
    XII_TEST_STRING(manager->GetNodeComment(owner), comment);
    XII_TEST_BOOL(scene.HasComment(owner, comment));
    QMenu removeComment;
    scene.AddCommentActions(removeComment, owner);
    XII_TEST_INT(removeComment.actions().size(), 2);
    removeComment.actions()[1]->trigger();
    XII_TEST_BOOL(manager->GetNodeComment(owner).IsEmpty());
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
    document.GetSelectionManager()->SetSelection(owner);
    const auto longCommentBounds = scene.CommentBounds(owner);
    XII_TEST_STATUS(scene.SetNodeComment(owner, "Short note"));
    XII_TEST_BOOL(scene.CommentBounds(owner).width() < longCommentBounds.width());
    XII_TEST_BOOL(scene.BodyBounds(owner) == bodyBeforeComment);
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
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
    XII_TEST_STRING(manager->GetNodeComment(copy), manager->GetNodeComment(owner));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Saving and reloading preserves pin identity and edited values")
  {
    XII_TEST_STATUS(document.SaveDocument(true));
    InspectorDocument loaded(path);
    XII_TEST_STATUS(loaded.LoadDocument());
    const auto* loadedManager = static_cast<const InspectorNodeManager*>(loaded.GetObjectManager());
    XII_TEST_STRING(loadedManager->GetNodeComment(loadedManager->GetObject(nodes[1])), manager->GetNodeComment(owner));
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

XII_CREATE_SIMPLE_TEST(EditorDocuments, GraphCommentBlocks)
{
  const auto output = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(output, "CommentBlocksTest", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("CommentBlocksTest"));
  const xiiStringBuilder path(output, "/CommentBlocks.xiiGraph");
  InspectorDocument      document(path);
  auto*                  manager = static_cast<InspectorNodeManager*>(document.GetObjectManager());
  auto*                  history = document.GetCommandHistory();
  xiiGraphCommentBlock   block;
  block.m_Id        = xiiUuid::MakeUuid();
  block.m_vPosition = xiiVec2(-50, -100);
  block.m_sText     = "Surface layering\nKeep related nodes together.";
  xiiSetGraphCommentBlockCommand command;
  command.m_Id    = block.m_Id;
  command.m_Value = block.ToVariant();
  history->StartTransaction("Create comment block");
  XII_TEST_STATUS(history->AddCommand(command));
  history->FinishTransaction();
  XII_TEST_INT(manager->GetCommentBlocks().GetCount(), 1);
  XII_TEST_INT(manager->GetRootObject()->GetChildren().GetCount(), 0);
  XII_TEST_STATUS(history->Undo());
  XII_TEST_BOOL(manager->GetCommentBlocks().IsEmpty());
  XII_TEST_STATUS(history->Redo());
  XII_TEST_STATUS(document.SaveDocument(true));
  InspectorDocument loaded(path);
  XII_TEST_STATUS(loaded.LoadDocument());
  const auto* restoredManager = static_cast<const InspectorNodeManager*>(loaded.GetObjectManager());
  XII_TEST_INT(restoredManager->GetCommentBlocks().GetCount(), 1);
  if (XII_TEST_BOOL(restoredManager->GetCommentBlocks().Contains(block.m_Id)))
    XII_TEST_BOOL(restoredManager->GetCommentBlocks().Find(block.m_Id).Value().ToVariant() == block.ToVariant());
  InspectorScene scene;
  scene.InitScene(manager);
  auto*   edit = document.GetObjectAccessor();
  xiiUuid nodeIds[2];
  edit->StartTransaction("Create grouped nodes");
  for (xiiUInt32 index = 0; index < 2; ++index)
  {
    XII_TEST_STATUS(edit->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiInspectorGraphNode>(), nodeIds[index]));
    manager->MoveNode(edit->GetObject(nodeIds[index]), xiiVec2(index * 800.0f, 0));
  }
  edit->FinishTransaction();
  const auto historyBefore = history->GetUndoStackSize();
  scene.DragCommentBlock(block.m_Id, QPointF(100, 50), false);
  XII_TEST_INT(history->GetUndoStackSize(), historyBefore + 1);
  XII_TEST_BOOL(manager->GetNodePos(edit->GetObject(nodeIds[0])) == xiiVec2(100, 50));
  XII_TEST_BOOL(manager->GetNodePos(edit->GetObject(nodeIds[1])) == xiiVec2(800, 0));
  XII_TEST_STATUS(history->Undo());
  XII_TEST_BOOL(manager->GetNodePos(edit->GetObject(nodeIds[0])) == xiiVec2::MakeZero());
  XII_TEST_BOOL(manager->GetCommentBlocks().Find(block.m_Id).Value().m_vPosition == block.m_vPosition);
  manager->m_RejectMove = nodeIds[0];
  auto rejectedMove     = block;
  rejectedMove.m_vPosition += xiiVec2(50);
  const auto beforeRejectedMove = history->GetUndoStackSize();
  XII_TEST_BOOL(scene.SetCommentBlock(rejectedMove, true).Failed());
  XII_TEST_INT(history->GetUndoStackSize(), beforeRejectedMove);
  XII_TEST_BOOL(manager->GetCommentBlocks().Find(block.m_Id).Value().ToVariant() == block.ToVariant());
  XII_TEST_BOOL(manager->GetNodePos(edit->GetObject(nodeIds[0])) == xiiVec2::MakeZero());
  manager->m_RejectMove = {};
  scene.DragCommentBlock(block.m_Id, QPointF(120, 90), true);
  XII_TEST_BOOL(manager->GetCommentBlocks().Find(block.m_Id).Value().m_vSize == block.m_vSize + xiiVec2(120, 90));
  XII_TEST_STATUS(history->Undo());
  QMenu actions;
  scene.AddCommentBlockActions(actions, block.m_Id);
  XII_TEST_INT(actions.actions().size(), 5);
  actions.actions()[2]->trigger();
  const auto   fitted = manager->GetCommentBlocks().Find(block.m_Id).Value();
  const QRectF fittedBounds(fitted.m_vPosition.x, fitted.m_vPosition.y, fitted.m_vSize.x, fitted.m_vSize.y);
  XII_TEST_BOOL(fittedBounds.contains(scene.Bounds(edit->GetObject(nodeIds[0]))));
  XII_TEST_BOOL(!fittedBounds.contains(scene.Bounds(edit->GetObject(nodeIds[1]))));
  XII_TEST_STATUS(document.SaveDocument(true));
  InspectorDocument editedReload(path);
  XII_TEST_STATUS(editedReload.LoadDocument());
  const auto* editedManager = static_cast<const InspectorNodeManager*>(editedReload.GetObjectManager());
  XII_TEST_BOOL(editedManager->GetCommentBlocks().Find(block.m_Id).Value().ToVariant() == fitted.ToVariant());
  actions.actions()[3]->trigger();
  XII_TEST_INT(manager->GetCommentBlocks().GetCount(), 2);
  XII_TEST_STATUS(history->Undo());
  {
    QImage image(1000, 500, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(32, 34, 38));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter, QRectF(15, 15, 970, 470), scene.itemsBoundingRect().adjusted(-20, -20, 20, 20), Qt::KeepAspectRatio);
    painter.end();
    XII_TEST_BOOL(image.save(xiiStringBuilder(output, "/OrganizationalBlock.png").GetData()));
  }
  block.m_vSize.x = -1;
  XII_TEST_BOOL(manager->SetCommentBlock(block.m_Id, block.ToVariant()).Failed());
  XII_TEST_BOOL(manager->GetCommentBlocks().Find(block.m_Id).Value().m_vSize.x > 0);
  scene.SelectCommentBlock(block.m_Id);
  scene.DeleteSelection();
  XII_TEST_BOOL(manager->GetCommentBlocks().IsEmpty());
  XII_TEST_STATUS(history->Undo());
  XII_TEST_INT(manager->GetCommentBlocks().GetCount(), 1);
}

XII_CREATE_SIMPLE_TEST(EditorDocuments, GraphBlockClipboard)
{
  const auto output = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(output, "BlockClipboardTest", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("BlockClipboardTest"));
  auto* clipboard         = QApplication::clipboard();
  auto* previousClipboard = new QMimeData;
  if (const auto* previous = clipboard->mimeData())
    for (const auto& format : previous->formats())
      previousClipboard->setData(format, previous->data(format));
  XII_SCOPE_EXIT(clipboard->setMimeData(previousClipboard));

  InspectorDocument      source(xiiStringBuilder(output, "/ClipboardSource.xiiGraph"));
  const xiiStringBuilder targetPath(output, "/ClipboardTarget.xiiGraph");
  InspectorDocument      target(targetPath);
  auto*                  sourceManager = static_cast<InspectorNodeManager*>(source.GetObjectManager());
  auto*                  targetManager = static_cast<InspectorNodeManager*>(target.GetObjectManager());
  target.m_vPickedPosition             = xiiVec2(600, 350);
  InspectorScene sourceScene, targetScene;
  sourceScene.InitScene(sourceManager);
  targetScene.InitScene(targetManager);
  QWidget       window;
  xiiQtNodeView sourceView(&window), targetView(&window);
  QLineEdit     otherEditor(&window);
  sourceView.SetScene(&sourceScene);
  targetView.SetScene(&targetScene);
  sourceView.setGeometry(0, 0, 500, 400);
  targetView.setGeometry(500, 0, 500, 400);
  otherEditor.setGeometry(0, 410, 300, 25);
  window.resize(1000, 450);
  window.show();
  window.activateWindow();
  sourceView.setFocus();
  QApplication::processEvents();
  xiiActionContext sourceContext(&source), targetContext(&target);
  sourceContext.m_pWindow = &window;
  targetContext.m_pWindow = &window;
  xiiEditAction sourceCopy(sourceContext, "Copy", xiiEditAction::ButtonType::Copy);
  xiiEditAction targetCopy(targetContext, "Copy", xiiEditAction::ButtonType::Copy);
  xiiEditAction targetPaste(targetContext, "Paste", xiiEditAction::ButtonType::Paste);
  xiiEditAction targetOriginal(targetContext, "Paste at Original Location", xiiEditAction::ButtonType::PasteAtOriginalLocation);
  xiiEditAction targetDelete(targetContext, "Delete", xiiEditAction::ButtonType::Delete);
  const auto    trigger = [](xiiEditAction& action) {
    XII_TEST_BOOL(action.IsEnabled());
    // Qt action proxies clear focus before executing to commit pending edits.
    // Keep this focus transition in the standalone document/action test too.
    QPointer<QWidget> focus = QApplication::focusWidget();
    if (focus)
      focus->clearFocus();
    action.Execute({});
    if (focus)
      focus->setFocus();
  };
  xiiGraphCommentBlock block;
  block.m_Id        = xiiUuid::MakeUuid();
  block.m_vPosition = xiiVec2(-50, -80);
  block.m_sText     = "Layer composition\nClipboard annotation";
  block.m_Color     = xiiColor(0.2f, 0.4f, 0.6f);
  XII_TEST_STATUS(sourceScene.SetCommentBlock(block));
  auto unselected        = block;
  unselected.m_Id        = xiiUuid::MakeUuid();
  unselected.m_vPosition = xiiVec2(900);
  XII_TEST_STATUS(sourceScene.SetCommentBlock(unselected));
  sourceScene.SelectCommentBlock(block.m_Id);
  XII_TEST_BOOL(source.GetSelectionManager()->IsSelectionEmpty());
  XII_TEST_BOOL(sourceCopy.IsEnabled());
  trigger(sourceCopy);
  if (!XII_TEST_BOOL(clipboard->mimeData() && clipboard->mimeData()->hasFormat("application/xiiEditor.PinInspectorTest")))
    return;
  targetView.setFocus();
  trigger(targetPaste);
  if (!XII_TEST_BOOL(targetManager->GetCommentBlocks().GetCount() == 1))
    return;
  XII_TEST_INT(targetManager->GetRootObject()->GetChildren().GetCount(), 0);
  const auto pasted = targetManager->GetCommentBlocks().GetIterator().Value();
  XII_TEST_BOOL(pasted.m_Id != block.m_Id);
  XII_TEST_BOOL(pasted.m_vPosition + pasted.m_vSize * 0.5f == target.m_vPickedPosition);
  XII_TEST_STRING(pasted.m_sText, block.m_sText);
  XII_TEST_BOOL(pasted.m_Color == block.m_Color && pasted.m_vSize == block.m_vSize);
  XII_TEST_BOOL(targetScene.HasSelectedCommentBlocks());
  XII_TEST_BOOL(targetCopy.IsEnabled());
  XII_TEST_STATUS(target.GetCommandHistory()->Undo());
  XII_TEST_BOOL(targetManager->GetCommentBlocks().IsEmpty());
  XII_TEST_STATUS(target.GetCommandHistory()->Redo());
  XII_TEST_BOOL(targetManager->GetCommentBlocks().Contains(pasted.m_Id));
  trigger(targetPaste);
  XII_TEST_INT(targetManager->GetCommentBlocks().GetCount(), 2);
  XII_TEST_STATUS(target.GetCommandHistory()->Undo());
  targetScene.SelectCommentBlock(pasted.m_Id);
  trigger(targetDelete);
  XII_TEST_BOOL(targetManager->GetCommentBlocks().IsEmpty());
  XII_TEST_STATUS(target.GetCommandHistory()->Undo());
  otherEditor.setFocus();
  XII_TEST_BOOL(!targetCopy.IsEnabled());

  sourceView.setFocus();
  sourceScene.SelectCommentBlock(block.m_Id);
  sourceScene.AddCommentBlockToSelection(unselected.m_Id);
  trigger(sourceCopy);
  targetView.setFocus();
  const auto beforeMultiPaste = target.GetCommandHistory()->GetUndoStackSize();
  trigger(targetPaste);
  XII_TEST_INT(target.GetCommandHistory()->GetUndoStackSize(), beforeMultiPaste + 1);
  XII_TEST_INT(targetManager->GetCommentBlocks().GetCount(), 3);
  const auto blockOffset = target.m_vPickedPosition - (block.m_vPosition + unselected.m_vPosition + block.m_vSize) * 0.5f;
  for (const auto& entry : targetManager->GetCommentBlocks())
    if (entry.Key() != pasted.m_Id)
      XII_TEST_BOOL(entry.Value().m_vPosition == block.m_vPosition + blockOffset || entry.Value().m_vPosition == unselected.m_vPosition + blockOffset);
  XII_TEST_STATUS(target.GetCommandHistory()->Undo());
  XII_TEST_INT(targetManager->GetCommentBlocks().GetCount(), 1);

  auto*   edit = source.GetObjectAccessor();
  xiiUuid nodes[3];
  edit->StartTransaction("Clipboard source nodes");
  for (xiiUInt32 index = 0; index < 3; ++index)
  {
    XII_TEST_STATUS(edit->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiInspectorGraphNode>(), nodes[index]));
    xiiMoveNodeCommand move;
    move.m_Object = nodes[index];
    move.m_NewPos = index < 2 ? xiiVec2(30 + index * 100.0f, 10 + index * 60.0f) : xiiVec2(800);
    XII_TEST_STATUS(source.GetCommandHistory()->AddCommand(move));
  }
  XII_TEST_STATUS(xiiNodeCommands::AddAndConnectCommand(source.GetCommandHistory(), sourceManager->GetConnectionType(), *sourceManager->GetOutputPins(edit->GetObject(nodes[0]))[0], *sourceManager->GetInputPins(edit->GetObject(nodes[1]))[0]));
  edit->FinishTransaction();
  XII_TEST_STATUS(sourceScene.SetNodeComment(edit->GetObject(nodes[0]), "Attached note"));
  sourceView.setFocus();
  sourceScene.SelectCommentBlock(block.m_Id);
  sourceScene.AddNodeToSelection(edit->GetObject(nodes[0]));
  sourceScene.AddNodeToSelection(edit->GetObject(nodes[1]));
  trigger(sourceCopy);
  const auto mixedData = clipboard->mimeData()->data("application/xiiEditor.PinInspectorTest");
  targetView.setFocus();
  trigger(targetOriginal);
  XII_TEST_INT(targetManager->GetCommentBlocks().GetCount(), 2);
  XII_TEST_INT(targetManager->GetRootObject()->GetChildren().GetCount(), 3);
  xiiUuid mixedBlock;
  for (const auto& entry : targetManager->GetCommentBlocks())
    if (entry.Key() != pasted.m_Id)
    {
      mixedBlock = entry.Key();
      XII_TEST_BOOL(entry.Value().m_vPosition == block.m_vPosition);
    }
  xiiUInt32 attachedNotes = 0;
  for (const auto* node : target.GetSelectionManager()->GetSelection())
    if (targetManager->IsNode(node))
    {
      const auto position = targetManager->GetNodePos(node);
      XII_TEST_BOOL(position == xiiVec2(30, 10) || position == xiiVec2(130, 70));
      if (targetManager->GetNodeComment(node) == "Attached note")
        ++attachedNotes;
    }
  XII_TEST_INT(attachedNotes, 1);
  XII_TEST_STATUS(target.GetCommandHistory()->Undo());
  XII_TEST_INT(targetManager->GetCommentBlocks().GetCount(), 1);
  XII_TEST_BOOL(targetManager->GetRootObject()->GetChildren().IsEmpty());
  XII_TEST_STATUS(target.GetCommandHistory()->Redo());
  XII_TEST_BOOL(targetManager->GetCommentBlocks().Contains(mixedBlock));
  trigger(targetPaste);
  const auto offset = target.m_vPickedPosition - xiiVec2(80, 40);
  for (const auto& entry : targetManager->GetCommentBlocks())
    if (entry.Key() != pasted.m_Id && entry.Key() != mixedBlock)
      XII_TEST_BOOL(entry.Value().m_vPosition == block.m_vPosition + offset);
  XII_TEST_INT(targetManager->GetCommentBlocks().GetCount(), 3);
  XII_TEST_INT(targetManager->GetRootObject()->GetChildren().GetCount(), 6);
  for (const auto* node : target.GetSelectionManager()->GetSelection())
    if (targetManager->IsNode(node))
    {
      const auto position = targetManager->GetNodePos(node) - offset;
      XII_TEST_BOOL(position == xiiVec2(30, 10) || position == xiiVec2(130, 70));
    }
  XII_TEST_STATUS(target.SaveDocument(true));
  InspectorDocument loaded(targetPath);
  XII_TEST_STATUS(loaded.LoadDocument());
  const auto& savedBlocks = static_cast<const InspectorNodeManager*>(loaded.GetObjectManager())->GetCommentBlocks();
  XII_TEST_INT(savedBlocks.GetCount(), 3);
  for (const auto& entry : savedBlocks)
    XII_TEST_BOOL(entry.Value().ToVariant() == targetManager->GetCommentBlocks().Find(entry.Key()).Value().ToVariant());

  xiiAbstractObjectGraph   malformed;
  xiiRawMemoryStreamReader reader(mixedData.constData(), mixedData.size());
  XII_TEST_RESULT(xiiAbstractGraphDdlSerializer::Read(reader, &malformed));
  malformed.GetNode(block.m_Id)->ChangeProperty("Size", xiiVec2(-1, 240));
  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);
  xiiAbstractGraphDdlSerializer::Write(writer, &malformed);
  xiiPasteObjectsCommand invalid;
  invalid.m_sMimeType        = "application/xiiEditor.PinInspectorTest";
  invalid.m_sGraphTextFormat = xiiStringView(reinterpret_cast<const char*>(storage.GetData()), storage.GetStorageSize32());
  const auto undoCount       = target.GetCommandHistory()->GetUndoStackSize();
  XII_TEST_BOOL(targetScene.PasteObjects(invalid).Failed());
  XII_TEST_INT(target.GetCommandHistory()->GetUndoStackSize(), undoCount);
  XII_TEST_INT(targetManager->GetCommentBlocks().GetCount(), 3);
  XII_TEST_INT(targetManager->GetRootObject()->GetChildren().GetCount(), 6);
  XII_TEST_INT(sourceManager->GetCommentBlocks().GetCount(), 2);
  XII_TEST_BOOL(sourceManager->GetCommentBlocks().Find(block.m_Id).Value().ToVariant() == block.ToVariant());
}

XII_CREATE_SIMPLE_TEST(NodeLayout, DocumentCommands)
{
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "NodeLayoutTest", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NodeLayoutTest"));
  xiiStringBuilder  path(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/NodeLayout.xiiGraph");
  InspectorDocument document(path);
  auto*             accessor    = document.GetObjectAccessor();
  auto*             manager     = static_cast<InspectorNodeManager*>(document.GetObjectManager());
  auto*             history     = document.GetCommandHistory();
  const char*       names[]     = {"World Extraction", "Depth", "Opaque Surfaces", "Directional and Local Lighting", "Output Conversion", "Present", "Independent Utility"};
  const xiiVec2     scattered[] = {{700, 200}, {70, 90}, {80, 90}, {-100, 400}, {900, 40}, {180, -100}, {700, 100}};
  xiiUuid           ids[XII_ARRAY_SIZE(names)];
  accessor->StartTransaction("Create layout fixture");
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(names); ++i)
  {
    ids[i] = xiiUuid::MakeStableUuidFromString(names[i]);
    XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiInspectorGraphNode>(), ids[i]));
    XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(ids[i]), "Name", names[i]));
    xiiMoveNodeCommand move;
    move.m_Object = ids[i];
    move.m_NewPos = scattered[i];
    XII_TEST_STATUS(history->AddCommand(move));
  }
  accessor->FinishTransaction();
  InspectorScene scene;
  scene.InitScene(manager);
  const xiiUInt32 pairs[][2] = {{0, 1}, {0, 2}, {1, 3}, {2, 3}, {3, 4}, {4, 5}};
  accessor->StartTransaction("Connect layout fixture");
  for (const auto& pair : pairs)
  {
    XII_TEST_STATUS(xiiNodeCommands::AddAndConnectCommand(history, manager->GetConnectionType(), *manager->GetOutputPinByName(accessor->GetObject(ids[pair[0]]), "Value"), *manager->GetInputPinByName(accessor->GetObject(ids[pair[1]]), "Value")));
  }
  accessor->FinishTransaction();
  using Arrangement = xiiQtNodeScene::NodeArrangement;
  auto snapshot     = [&]() {
    xiiDynamicArray<xiiVec2> result;
    for (const auto& id : ids)
      result.PushBack(manager->GetNodePos(accessor->GetObject(id)));
    return result;
  };
  auto checkPositions = [&](xiiArrayPtr<const xiiVec2> expected) {
    const auto actual = snapshot();
    for (xiiUInt32 i = 0; i < actual.GetCount(); ++i)
      XII_TEST_BOOL(actual[i].IsEqual(expected[i], 0.01f));
  };
  auto render = [&](const char* name) {
    QImage image(1600, 850, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(32, 34, 38));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter, QRectF(20, 20, 1560, 810), scene.itemsBoundingRect().adjusted(-30, -30, 30, 30), Qt::KeepAspectRatio);
    painter.end();
    XII_TEST_BOOL(image.save(xiiStringBuilder(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/", name).GetData()));
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Whole graph layout is a single persistent undoable edit")
  {
    render("NodeLayout-Before.png");
    const auto    before = snapshot();
    const auto    count  = history->GetUndoStackSize();
    xiiQtNodeView view;
    view.SetScene(&scene);
    QAction* format = nullptr;
    for (auto* action : view.actions())
      if (action->text() == "Format Graph")
        format = action;
    if (!XII_TEST_BOOL(format != nullptr))
      return;
    XII_TEST_BOOL(format->shortcut() == QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L));
    XII_TEST_INT(format->shortcutContext(), Qt::WidgetWithChildrenShortcut);
    format->trigger();
    XII_TEST_INT(history->GetUndoStackSize(), count + 1);
    const auto formatted = snapshot();
    for (const auto& pair : pairs)
      XII_TEST_BOOL(scene.Bounds(accessor->GetObject(ids[pair[0]])).right() < scene.Bounds(accessor->GetObject(ids[pair[1]])).left());
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(ids); ++i)
      for (xiiUInt32 j = 0; j < i; ++j)
        XII_TEST_BOOL(!scene.Bounds(accessor->GetObject(ids[i])).intersects(scene.Bounds(accessor->GetObject(ids[j]))));
    XII_TEST_STATUS(scene.ArrangeNodes(Arrangement::Format, true));
    checkPositions(formatted);
    XII_TEST_INT(history->GetUndoStackSize(), count + 1);
    XII_TEST_STATUS(history->Undo());
    checkPositions(before);
    XII_TEST_STATUS(history->Redo());
    checkPositions(formatted);
    render("NodeLayout-After.png");
    XII_TEST_STATUS(document.SaveDocument(true));
    InspectorDocument loaded(path);
    XII_TEST_STATUS(loaded.LoadDocument());
    auto* loadedManager = static_cast<InspectorNodeManager*>(loaded.GetObjectManager());
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(ids); ++i)
      XII_TEST_BOOL(loadedManager->GetNodePos(loadedManager->GetObject(ids[i])).IsEqual(formatted[i], 0.01f));
    XII_TEST_INT(loadedManager->GetConnections(*loadedManager->GetInputPinByName(loadedManager->GetObject(ids[3]), "Value")).GetCount(), 2);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Selection formatting respects external wires and fixed nodes")
  {
    xiiDeque<const xiiDocumentObject*> selection;
    selection.PushBack(accessor->GetObject(ids[1]));
    selection.PushBack(accessor->GetObject(ids[2]));
    selection.PushBack(accessor->GetObject(ids[3]));
    document.GetSelectionManager()->SetSelection(selection);
    const auto before = snapshot();
    XII_TEST_STATUS(scene.ArrangeNodes(Arrangement::Format));
    const auto after = snapshot();
    for (auto i : {0, 4, 5, 6})
      XII_TEST_BOOL(before[i] == after[i]);
    const auto count = history->GetUndoStackSize();
    XII_TEST_STATUS(scene.ArrangeNodes(Arrangement::Format));
    checkPositions(after);
    XII_TEST_INT(history->GetUndoStackSize(), count);
    for (xiiUInt32 i = 0; i < 3; ++i)
      for (auto fixed : {0, 4, 5, 6})
        XII_TEST_BOOL(!scene.Bounds(selection[i]).intersects(scene.Bounds(accessor->GetObject(ids[fixed]))));
    XII_TEST_BOOL((after[1] - before[1]).GetLength() < 1000);
    XII_TEST_INT(document.GetSelectionManager()->GetSelection().GetCount(), 3);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "All six alignments use visible bounds and undo precisely")
  {
    for (auto operation : {Arrangement::AlignLeft, Arrangement::AlignHorizontalCenter, Arrangement::AlignRight, Arrangement::AlignTop, Arrangement::AlignVerticalCenter, Arrangement::AlignBottom})
    {
      const auto before = snapshot();
      const auto count  = history->GetUndoStackSize();
      XII_TEST_STATUS(scene.ArrangeNodes(operation));
      auto value = [&](const QRectF& rect) {
        switch (operation)
        {
          case Arrangement::AlignLeft:
            return rect.left();
          case Arrangement::AlignHorizontalCenter:
            return rect.center().x();
          case Arrangement::AlignRight:
            return rect.right();
          case Arrangement::AlignTop:
            return rect.top();
          case Arrangement::AlignVerticalCenter:
            return rect.center().y();
          default:
            return rect.bottom();
        }
      };
      for (auto i : {2, 3})
        XII_TEST_FLOAT(value(scene.Bounds(accessor->GetObject(ids[1]))), value(scene.Bounds(accessor->GetObject(ids[i]))), 0.01);
      if (history->GetUndoStackSize() != count)
      {
        XII_TEST_STATUS(history->Undo());
        checkPositions(before);
      }
    }
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Equal gaps account for sizes; outer anchors stay fixed when space permits")
  {
    for (bool horizontal : {false, true})
    {
      history->StartTransaction("Space test nodes");
      for (xiiUInt32 i = 1; i <= 3; ++i)
      {
        xiiMoveNodeCommand move;
        move.m_Object = ids[i];
        move.m_NewPos = horizontal ? xiiVec2(i * i * 500.0f, 0) : xiiVec2(0, i * i * 500.0f);
        XII_TEST_STATUS(history->AddCommand(move));
      }
      history->FinishTransaction();
      const auto before = snapshot();
      XII_TEST_STATUS(scene.ArrangeNodes(horizontal ? Arrangement::DistributeHorizontally : Arrangement::DistributeVertically));
      const auto a = scene.Bounds(accessor->GetObject(ids[1])), b = scene.Bounds(accessor->GetObject(ids[2])), c = scene.Bounds(accessor->GetObject(ids[3]));
      XII_TEST_FLOAT(horizontal ? b.left() - a.right() : b.top() - a.bottom(), horizontal ? c.left() - b.right() : c.top() - b.bottom(), 0.01);
      const auto after = snapshot();
      XII_TEST_BOOL(before[1] == after[1]);
      XII_TEST_BOOL(before[3] == after[3]);
    }
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Movement restrictions fail atomically without history; too-small selections do nothing")
  {
    // The previous distribution left the group far from its external neighbors.
    // Formatting must not approach them in a succession of clamped steps.
    XII_TEST_STATUS(scene.ArrangeNodes(Arrangement::Format));
    const auto stableSelection = snapshot();
    const auto selectionCount  = history->GetUndoStackSize();
    XII_TEST_STATUS(scene.ArrangeNodes(Arrangement::Format));
    checkPositions(stableSelection);
    XII_TEST_INT(history->GetUndoStackSize(), selectionCount);
    manager->m_RejectMove = ids[3];
    const auto before     = snapshot();
    const auto count      = history->GetUndoStackSize();
    XII_TEST_BOOL(scene.ArrangeNodes(Arrangement::Format, true).Failed());
    checkPositions(before);
    XII_TEST_INT(history->GetUndoStackSize(), count);
    manager->m_RejectMove = {};
    document.GetSelectionManager()->SetSelection(accessor->GetObject(ids[0]));
    XII_TEST_STATUS(scene.ArrangeNodes(Arrangement::Format));
    XII_TEST_STATUS(scene.ArrangeNodes(Arrangement::DistributeVertically));
    XII_TEST_INT(history->GetUndoStackSize(), count);
    QMenu menu;
    scene.AddArrangementActions(menu);
    XII_TEST_BOOL(menu.actions()[0]->isEnabled());
    XII_TEST_BOOL(!menu.actions()[1]->isEnabled());
    document.GetSelectionManager()->Clear();
    XII_TEST_STATUS(scene.ArrangeNodes(Arrangement::Format, true));
  }
}
