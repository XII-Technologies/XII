/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <EditorFrameworkTest/SceneDuplicationTest.h>

#include <Core/World/GameObject.h>
#include <EditorPluginScene/Commands/SceneCommands.h>
#include <EditorPluginScene/Dialogs/DuplicateDlg.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <QApplication>
#include <QPushButton>
#include <QTimer>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

namespace
{
  class DuplicationDocument : public xiiSceneDocument
  {
  public:
    DuplicationDocument() : xiiSceneDocument("DuplicationTest.xiiScene", DocumentType::Scene)
    {
      m_pDocumentInfo = CreateDocumentInfo();
      SubscribeGameObjectEventHandlers();
    }
  };

  void SetSource(xiiSceneDocument& document, xiiDuplicateObjectsCommand& command)
  {
    xiiAbstractObjectGraph   graph;
    xiiMap<xiiUuid, xiiUuid> parents;
    XII_TEST_BOOL(document.CopySelectedObjects(graph, &parents));
    xiiStringBuilder text, objectId, parentId;
    for (auto it = parents.GetIterator(); it.IsValid(); ++it)
      text.AppendFormat("{}={};", xiiConversionUtils::ToString(it.Key(), objectId), xiiConversionUtils::ToString(it.Value(), parentId));
    command.m_sParentNodes = text;
    xiiContiguousMemoryStreamStorage storage;
    xiiMemoryStreamWriter            writer(&storage);
    xiiAbstractGraphDdlSerializer::Write(writer, &graph);
    command.m_sGraphTextFormat = xiiStringView(reinterpret_cast<const char*>(storage.GetData()), storage.GetStorageSize32());
  }

  xiiUuid AddObject(DuplicationDocument& document, const xiiDocumentObject* parent, const char* name, xiiVec3 position)
  {
    auto*   accessor = document.GetObjectAccessor();
    xiiUuid id;
    XII_TEST_STATUS(accessor->AddObjectByName(parent, "Children", -1, xiiGetStaticRTTI<xiiGameObject>(), id));
    const auto* object = accessor->GetObject(id);
    XII_TEST_STATUS(accessor->SetValueByName(object, "Name", name));
    XII_TEST_STATUS(accessor->SetValueByName(object, "LocalPosition", position));
    return id;
  }
} // namespace

void TestSceneDuplicationAction(xiiSceneDocument& document, const xiiUuid& source)
{
  const auto* object = document.GetObjectManager()->GetObject(source);
  if (!XII_TEST_BOOL(object != nullptr))
    return;
  document.GetSelectionManager()->SetSelection(object);
  const auto originalCount     = document.GetObjectManager()->GetRootObject()->GetChildren().GetCount();
  const auto copies            = xiiQtDuplicateDlg::s_uiNumberOfCopies;
  const auto grouped           = xiiQtDuplicateDlg::s_bGroupCopies;
  const auto translation       = xiiQtDuplicateDlg::s_vTranslationStep;
  const auto rotation          = xiiQtDuplicateDlg::s_vRotationStep;
  const auto randomTranslation = xiiQtDuplicateDlg::s_vRandomTranslation;
  const auto randomRotation    = xiiQtDuplicateDlg::s_vRandomRotation;
  const auto axis              = xiiQtDuplicateDlg::s_iRevolveAxis;
  XII_SCOPE_EXIT(
    xiiQtDuplicateDlg::s_uiNumberOfCopies   = copies;
    xiiQtDuplicateDlg::s_bGroupCopies       = grouped;
    xiiQtDuplicateDlg::s_vTranslationStep   = translation;
    xiiQtDuplicateDlg::s_vRotationStep      = rotation;
    xiiQtDuplicateDlg::s_vRandomTranslation = randomTranslation;
    xiiQtDuplicateDlg::s_vRandomRotation    = randomRotation;
    xiiQtDuplicateDlg::s_iRevolveAxis       = axis);
  // Start with deterministic parameters while preserving the user's persistent dialog defaults.
  xiiQtDuplicateDlg::s_vTranslationStep.SetZero();
  xiiQtDuplicateDlg::s_vRotationStep.SetZero();
  xiiQtDuplicateDlg::s_vRandomTranslation.SetZero();
  xiiQtDuplicateDlg::s_vRandomRotation.SetZero();
  xiiQtDuplicateDlg::s_iRevolveAxis = 0;

  bool   accepted = false;
  QTimer timer;
  QObject::connect(&timer, &QTimer::timeout, [&] {
    auto* dialog = qobject_cast<xiiQtDuplicateDlg*>(QApplication::activeModalWidget());
    if (dialog == nullptr)
      return;
    timer.stop();
    XII_TEST_BOOL(!dialog->CheckBoxGroupCopies->isHidden());
    XII_TEST_BOOL(dialog->toolButtonTransX->isEnabled());
    dialog->on_toolButtonTransX_clicked();
    XII_TEST_BOOL(dialog->SpinBoxTransX->value() > 0);
    dialog->spinBoxCopies->setValue(2);
    dialog->CheckBoxGroupCopies->setChecked(true);
    dialog->on_DefaultButtons_clicked(dialog->DefaultButtons->button(QDialogButtonBox::Ok));
    accepted = true;
  });
  timer.start(10);
  document.DuplicateSpecial();
  if (!XII_TEST_BOOL(accepted))
    return;
  XII_TEST_INT(document.GetObjectManager()->GetRootObject()->GetChildren().GetCount(), originalCount + 2);
  const auto selection = document.GetSelectionManager()->GetSelection();
  if (!XII_TEST_INT(selection.GetCount(), 3))
    return;
  xiiHybridArray<xiiUuid, 2> groups;
  for (xiiUInt32 i = 1; i < selection.GetCount(); ++i)
  {
    const auto* group = selection[i];
    groups.PushBack(group->GetGuid());
    if (XII_TEST_INT(group->GetChildren().GetCount(), 1))
    {
      const auto* child = group->GetChildren()[0];
      XII_TEST_BOOL(child->GetGuid() != source);
      XII_TEST_INT(child->GetChildren().GetCount(), object->GetChildren().GetCount());
      XII_TEST_BOOL(!child->GetChildren().IsEmpty());
    }
  }
  auto* history = document.GetCommandHistory();
  XII_TEST_STATUS(history->Undo());
  XII_TEST_INT(document.GetObjectManager()->GetRootObject()->GetChildren().GetCount(), originalCount);
  XII_TEST_STATUS(history->Redo());
  for (const auto id : groups)
    XII_TEST_BOOL(document.GetObjectManager()->GetObject(id) != nullptr);
  XII_TEST_STATUS(history->Undo());
  history->ClearRedoHistory();
  XII_TEST_INT(document.GetObjectManager()->GetRootObject()->GetChildren().GetCount(), originalCount);
}

XII_CREATE_SIMPLE_TEST(EditorDocuments, SceneDuplication)
{
  for (bool grouped : {false, true})
  {
    DuplicationDocument document;
    auto*               accessor = document.GetObjectAccessor();
    accessor->StartTransaction("Sources");
    const auto  parentId = AddObject(document, document.GetObjectManager()->GetRootObject(), "Parent", {3, 4, 5});
    const auto* parent   = accessor->GetObject(parentId);
    XII_TEST_STATUS(accessor->SetValueByName(parent, "LocalRotation", xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(30))));
    XII_TEST_STATUS(accessor->SetValueByName(parent, "LocalScaling", xiiVec3(2, 3, 1)));
    const auto  aId = AddObject(document, parent, "A", {1, 2, 3});
    const auto  bId = AddObject(document, parent, "B", {-2, 0, 1});
    const auto* a   = accessor->GetObject(aId);
    const auto* b   = accessor->GetObject(bId);
    XII_TEST_STATUS(accessor->SetValueByName(a, "LocalScaling", xiiVec3(-1, 2, 1)));
    const auto nestedId = AddObject(document, a, "Nested", {0, 1, 0});
    accessor->FinishTransaction();
    const auto                         originalA = document.GetGlobalTransform(a);
    const auto                         originalB = document.GetGlobalTransform(b);
    const auto                         center    = (originalA.m_vPosition + originalB.m_vPosition) * 0.5f;
    xiiDeque<const xiiDocumentObject*> selection;
    selection.PushBack(b);
    selection.PushBack(a);
    document.GetSelectionManager()->SetSelection(selection);
    xiiDuplicateObjectsCommand command;
    SetSource(document, command);
    command.m_uiNumberOfCopies         = 2;
    command.m_bGroupDuplicates         = grouped;
    command.m_vAccumulativeTranslation = {10, 0, 0};
    command.m_vAccumulativeRotation    = {0, 0, 90};
    auto* history                      = document.GetCommandHistory();
    history->StartTransaction("Duplicate source hierarchy");
    const auto status = history->AddCommand(command);
    XII_TEST_STATUS(status);
    if (status.Failed())
    {
      history->CancelTransaction();
      return;
    }
    history->FinishTransaction();
    XII_TEST_INT(parent->GetChildren().GetCount(), grouped ? 4 : 6);
    const auto copiedSelection = document.GetSelectionManager()->GetSelection();
    XII_TEST_INT(copiedSelection.GetCount(), grouped ? 4 : 6);
    xiiHybridArray<xiiUuid, 4>      copyIds;
    xiiHybridArray<xiiTransform, 4> copyTransforms;
    for (xiiUInt32 copy = 0; copy < 2; ++copy)
    {
      const auto* group = grouped ? copiedSelection[2 + copy] : nullptr;
      if (group)
      {
        XII_TEST_BOOL(group->GetParent() == parent);
        XII_TEST_INT(group->GetChildren().GetCount(), 2);
      }
      for (xiiUInt32 item = 0; item < 2; ++item)
      {
        const auto* object   = group ? group->GetChildren()[item] : copiedSelection[2 + copy * 2 + item];
        const auto& original = item == 0 ? originalB : originalA;
        XII_TEST_STRING(object->GetTypeAccessor().GetValue("Name").Get<xiiString>(), item == 0 ? "B" : "A");
        XII_TEST_BOOL(object->GetGuid() != (item == 0 ? bId : aId));
        const auto transform = document.GetGlobalTransform(object);
        xiiVec3    expected  = original.m_vPosition + xiiVec3(10.0f * (copy + 1), 0, 0);
        const auto rotation  = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(90.0f * copy));
        if (grouped)
          expected = center + rotation * (original.m_vPosition - center) + xiiVec3(10.0f * (copy + 1), 0, 0);
        XII_TEST_VEC3(transform.m_vPosition, expected, 0.002f);
        XII_TEST_VEC3(transform.m_vScale, original.m_vScale, 0.002f);
        XII_TEST_BOOL(transform.m_qRotation.IsEqualRotation(rotation * original.m_qRotation, 0.01f));
        if (item == 1)
        {
          if (XII_TEST_INT(object->GetChildren().GetCount(), 1))
            XII_TEST_BOOL(object->GetChildren()[0]->GetGuid() != nestedId);
        }
        copyIds.PushBack(object->GetGuid());
        copyTransforms.PushBack(transform);
      }
    }
    for (xiiUInt32 iteration = 0; iteration < 3; ++iteration)
    {
      XII_TEST_STATUS(history->Undo());
      XII_TEST_INT(parent->GetChildren().GetCount(), 2);
      for (const auto id : copyIds)
        XII_TEST_BOOL(document.GetObjectManager()->GetObject(id) == nullptr);
      XII_TEST_STATUS(history->Redo());
      for (xiiUInt32 i = 0; i < copyIds.GetCount(); ++i)
      {
        const auto* object = document.GetObjectManager()->GetObject(copyIds[i]);
        if (XII_TEST_BOOL(object != nullptr))
          XII_TEST_BOOL(document.GetGlobalTransform(object).IsEqual(copyTransforms[i], 0.002f));
      }
    }
    XII_TEST_STATUS(history->Undo());
    history->ClearRedoHistory();
    XII_TEST_VEC3(document.GetGlobalTransform(a).m_vPosition, originalA.m_vPosition, 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Small random ranges, stationary revolve and invalid inputs")
  {
    DuplicationDocument document;
    auto*               accessor = document.GetObjectAccessor();
    accessor->StartTransaction("Source");
    const auto id = AddObject(document, document.GetObjectManager()->GetRootObject(), "Source", {1, 2, 3});
    accessor->FinishTransaction();
    const auto* object = accessor->GetObject(id);
    document.GetSelectionManager()->SetSelection(object);
    auto*                      history = document.GetCommandHistory();
    xiiDuplicateObjectsCommand command;
    SetSource(document, command);
    command.m_uiNumberOfCopies   = 3;
    command.m_vRandomTranslation = {0.001f, 0.002f, 0.003f};
    command.m_vRandomRotation    = {0.1f, 0.2f, 0.3f};
    history->StartTransaction("Small random ranges");
    XII_TEST_STATUS(history->AddCommand(command));
    history->FinishTransaction();
    const auto copies = document.GetSelectionManager()->GetSelection();
    XII_TEST_INT(copies.GetCount(), 4);
    for (xiiUInt32 i = 1; i < copies.GetCount(); ++i)
    {
      const auto transform = document.GetGlobalTransform(copies[i]);
      XII_TEST_BOOL(transform.IsValid());
      XII_TEST_BOOL((transform.m_vPosition - xiiVec3(1, 2, 3)).Abs().CompMax(command.m_vRandomTranslation) == command.m_vRandomTranslation);
    }
    XII_TEST_STATUS(history->Undo());
    history->ClearRedoHistory();
    document.GetSelectionManager()->SetSelection(object);
    SetSource(document, command);
    command.m_uiNumberOfCopies = 1;
    command.m_vRandomTranslation.SetZero();
    command.m_vRandomRotation.SetZero();
    command.m_iRevolveAxis      = 3;
    command.m_fRevolveRadius    = 5;
    command.m_RevolveStartAngle = xiiAngle::MakeFromDegree(90);
    history->StartTransaction("Stationary revolve");
    XII_TEST_STATUS(history->AddCommand(command));
    history->FinishTransaction();
    XII_TEST_VEC3(document.GetGlobalTransform(document.GetSelectionManager()->GetCurrentObject()).m_vPosition, xiiVec3(-4, 2, 3), 0.001f);
    XII_TEST_STATUS(history->Undo());
    history->ClearRedoHistory();
    command.m_sParentNodes = "invalid=parent;";
    history->StartTransaction("Reject malformed source parents");
    XII_TEST_BOOL(history->AddCommand(command).Failed());
    history->CancelTransaction();
    XII_TEST_INT(document.GetObjectManager()->GetRootObject()->GetChildren().GetCount(), 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Mixed parents and invalid transforms")
  {
    DuplicationDocument document;
    auto*               accessor = document.GetObjectAccessor();
    const auto*         root     = document.GetObjectManager()->GetRootObject();
    accessor->StartTransaction("Sources under different parents");
    const auto  parentId = AddObject(document, root, "Parent", {3, 4, 5});
    const auto* parent   = accessor->GetObject(parentId);
    const auto  aId      = AddObject(document, parent, "A", {1, 0, 0});
    const auto  bId      = AddObject(document, root, "B", {-2, 1, 0});
    accessor->FinishTransaction();
    const auto*                        a         = accessor->GetObject(aId);
    const auto*                        b         = accessor->GetObject(bId);
    const auto                         originalA = document.GetGlobalTransform(a);
    const auto                         originalB = document.GetGlobalTransform(b);
    xiiDeque<const xiiDocumentObject*> selection;
    selection.PushBack(a);
    selection.PushBack(b);
    document.GetSelectionManager()->SetSelection(selection);
    xiiDuplicateObjectsCommand command;
    SetSource(document, command);
    command.m_uiNumberOfCopies = 1;
    command.m_bGroupDuplicates = true;
    auto* history              = document.GetCommandHistory();
    history->StartTransaction("Group across parents");
    const auto status = history->AddCommand(command);
    XII_TEST_STATUS(status);
    if (status.Failed())
    {
      history->CancelTransaction();
      return;
    }
    history->FinishTransaction();
    const auto* group = document.GetSelectionManager()->GetCurrentObject();
    XII_TEST_BOOL(group->GetParent() == root);
    if (XII_TEST_INT(group->GetChildren().GetCount(), 2))
    {
      XII_TEST_BOOL(document.GetGlobalTransform(group->GetChildren()[0]).IsEqual(originalA, 0.001f));
      XII_TEST_BOOL(document.GetGlobalTransform(group->GetChildren()[1]).IsEqual(originalB, 0.001f));
    }
    XII_TEST_STATUS(history->Undo());
    history->ClearRedoHistory();
    XII_TEST_INT(root->GetChildren().GetCount(), 2);

    auto reject = [&] {
      history->StartTransaction("Reject invalid duplicate transform");
      XII_TEST_BOOL(history->AddCommand(command).Failed());
      history->CancelTransaction();
      XII_TEST_INT(root->GetChildren().GetCount(), 2);
      XII_TEST_INT(parent->GetChildren().GetCount(), 1);
    };
    command.m_vRandomTranslation.x = -1;
    reject();
    command.m_vRandomTranslation.SetZero();
    command.m_vAccumulativeTranslation.x = xiiMath::NaN<float>();
    reject();
    command.m_vAccumulativeTranslation.SetZero();
    command.m_iRevolveAxis = 4;
    reject();
    command.m_iRevolveAxis = 0;
    document.GetSelectionManager()->SetSelection(a);
    accessor->StartTransaction("Singular source parent");
    XII_TEST_STATUS(accessor->SetValueByName(parent, "LocalScaling", xiiVec3(0, 1, 1)));
    accessor->FinishTransaction();
    SetSource(document, command);
    reject();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Dialog uses native selection dimensions")
  {
    xiiQtDuplicateDlg dialog(nullptr, xiiVec3(1.234f, 0, 4.567f));
    XII_TEST_BOOL(!dialog.CheckBoxGroupCopies->isHidden());
    XII_TEST_BOOL(dialog.toolButtonTransX->isEnabled());
    XII_TEST_BOOL(!dialog.toolButtonTransY->isEnabled());
    dialog.SpinBoxTransX->setValue(0);
    dialog.on_toolButtonTransX_clicked();
    XII_TEST_DOUBLE(dialog.SpinBoxTransX->value(), 1.23, 0.001);
    dialog.on_toolButtonTransX_clicked();
    XII_TEST_DOUBLE(dialog.SpinBoxTransX->value(), -1.23, 0.001);
    dialog.on_toolButtonTransX_clicked();
    XII_TEST_DOUBLE(dialog.SpinBoxTransX->value(), 0, 0.001);
  }
}
