/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Threading/TaskSystem.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundationTest/Object/DocumentSaveTestUtils.h>

using namespace xiiDocumentSaveTestUtils;

namespace
{
  xiiStatus ChangeSavedInteger(xiiDocument& document, const xiiUuid& object, xiiInt32 value)
  {
    auto* accessor = document.GetObjectAccessor();
    accessor->StartTransaction("Change pending save payload");
    auto result = accessor->SetValueByName(accessor->GetObject(object), "Int32", value);
    if (result.Failed())
      accessor->CancelTransaction();
    else
      accessor->FinishTransaction();
    return result;
  }

  bool CheckSavedInteger(xiiStringView path, const xiiUuid& object, xiiInt32 expected)
  {
    xiiUniquePtr<xiiAbstractObjectGraph> header, objects, types;
    if (!XII_TEST_BOOL(xiiDocument::ReadDocument(path, header, objects, types).Succeeded()))
      return false;
    const auto* node = objects->GetNode(object);
    if (!XII_TEST_BOOL(node != nullptr))
      return false;
    const auto* value = node->FindProperty("Int32");
    return XII_TEST_BOOL(value != nullptr && value->m_Value.IsA<xiiInt32>()) && XII_TEST_INT(value->m_Value.Get<xiiInt32>(), expected);
  }
} // namespace

XII_CREATE_SIMPLE_TEST(DocumentPersistence, EditsDuringAsyncSaveRemainDirty)
{
  xiiStringBuilder output = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  output.AppendPath("DocumentAsyncSave");
  if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(output)) ||
      !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(output, "DocumentAsyncSave", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("DocumentAsyncSave"));
  SaveTestManager manager;
  for (xiiUInt32 operation = 0; operation < 3; ++operation)
  {
    xiiStringBuilder path = output;
    path.AppendPath("Pending.xiiSaveTest");
    xiiUuid object, other;
    if (!MakeSeed(manager, path, object, other))
      return;
    xiiDocument* base = nullptr;
    if (!XII_TEST_BOOL(manager.OpenDocument("SaveValidation", path, base, xiiDocumentFlags::None).Succeeded()))
      return;
    XII_SCOPE_EXIT(manager.CloseDocument(base));
    auto& document = *static_cast<SaveTestDocument*>(base);
    XII_TEST_STATUS(ChangeSavedInteger(document, object, 9001));
    XII_TEST_STATUS(ChangeSavedInteger(document, object, 9002));
    xiiInt32 snapshot = 9002;
    xiiInt32 current  = 9003;
    if (operation == 2)
    {
      XII_TEST_STATUS(document.GetCommandHistory()->Undo());
      snapshot = 9001;
      current  = 9002;
    }
    xiiUInt32  callbacks = 0;
    const auto task      = document.SaveDocumentAsync([&](xiiDocument* saved, xiiStatus result) {
      XII_TEST_BOOL(saved == &document && result.Succeeded());
      XII_TEST_BOOL(document.IsModified());
      ++callbacks;
    });
    if (!XII_TEST_BOOL(task.IsValid()))
      return;
    // Completion is a main-thread task, so these transactions deterministically precede it.
    if (operation == 0)
    {
      XII_TEST_STATUS(ChangeSavedInteger(document, object, current));
    }
    else if (operation == 1)
    {
      XII_TEST_STATUS(document.GetCommandHistory()->Undo());
      current = 9001;
    }
    else
    {
      XII_TEST_STATUS(document.GetCommandHistory()->Redo());
    }
    xiiTaskSystem::WaitForGroup(task);
    XII_TEST_INT(callbacks, 1);
    XII_TEST_BOOL(document.IsModified());
    CheckSavedInteger(path, object, snapshot);
    XII_TEST_BOOL(document.SaveDocument().Succeeded());
    XII_TEST_BOOL(!document.IsModified());
    CheckSavedInteger(path, object, current);
  }
}

XII_CREATE_SIMPLE_TEST(DocumentPersistence, SaveNotificationEditsRemainDirty)
{
  xiiStringBuilder output = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  output.AppendPath("DocumentSaveNotification");
  if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(output)) ||
      !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(output, "DocumentSaveNotification", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("DocumentSaveNotification"));
  SaveTestManager  manager;
  xiiStringBuilder path = output;
  path.AppendPath("Notification.xiiSaveTest");
  xiiUuid object, other;
  if (!MakeSeed(manager, path, object, other))
    return;
  for (bool global : {false, true})
  {
    xiiDocument* document = nullptr;
    if (!XII_TEST_BOOL(manager.OpenDocument("SaveValidation", path, document, xiiDocumentFlags::None).Succeeded()))
      return;
    XII_SCOPE_EXIT(manager.CloseDocument(document));
    XII_TEST_STATUS(ChangeSavedInteger(*document, object, 9001));
    xiiUInt32 savedEvents = 0;
    xiiUInt32 dirtyEvents = 0;
    auto      handler     = [&](const xiiDocumentEvent& event) {
      if (event.m_pDocument != document)
        return;
      if (event.m_Type == xiiDocumentEvent::Type::ModifiedChanged && document->IsModified())
        ++dirtyEvents;
      if (event.m_Type == xiiDocumentEvent::Type::DocumentSaved)
      {
        ++savedEvents;
        XII_TEST_BOOL(!document->IsModified());
        XII_TEST_STATUS(ChangeSavedInteger(*document, object, 9002));
      }
    };
    auto subscription = global ? xiiDocument::s_EventsAny.AddEventHandler(handler) : document->m_EventsOne.AddEventHandler(handler);
    XII_TEST_BOOL(document->SaveDocument().Succeeded());
    if (global)
      xiiDocument::s_EventsAny.RemoveEventHandler(subscription);
    else
      document->m_EventsOne.RemoveEventHandler(subscription);
    XII_TEST_INT(savedEvents, 1);
    XII_TEST_INT(dirtyEvents, 1);
    XII_TEST_BOOL(document->IsModified());
    CheckSavedInteger(path, object, 9001);
    XII_TEST_BOOL(document->SaveDocument().Succeeded());
    CheckSavedInteger(path, object, 9002);
  }
}

XII_CREATE_SIMPLE_TEST(DocumentPersistence, ClosingDrainsQueuedSaveCallbacks)
{
  xiiStringBuilder output = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  output.AppendPath("DocumentSaveQueue");
  if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(output)) ||
      !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(output, "DocumentSaveQueue", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("DocumentSaveQueue"));
  SaveTestManager  manager;
  xiiStringBuilder path = output;
  path.AppendPath("Queue.xiiSaveTest");
  xiiUuid object, other;
  if (!MakeSeed(manager, path, object, other))
    return;
  xiiDocument* base = nullptr;
  if (!XII_TEST_BOOL(manager.OpenDocument("SaveValidation", path, base, xiiDocumentFlags::None).Succeeded()))
    return;
  auto&     document     = *static_cast<SaveTestDocument*>(base);
  xiiUInt32 callbacks    = 0;
  xiiUInt32 savedEvents  = 0;
  auto      subscription = document.m_EventsOne.AddEventHandler([&](const xiiDocumentEvent& event) {
    if (event.m_Type == xiiDocumentEvent::Type::DocumentSaved)
      ++savedEvents;
  });
  XII_TEST_STATUS(ChangeSavedInteger(document, object, 9001));
  const auto first = document.SaveDocumentAsync([&](xiiDocument*, xiiStatus result) { XII_TEST_BOOL(result.Succeeded()); ++callbacks; });
  XII_TEST_STATUS(ChangeSavedInteger(document, object, 9002));
  const auto second = document.SaveDocumentAsync([&](xiiDocument*, xiiStatus result) { XII_TEST_BOOL(result.Succeeded()); ++callbacks; });
  if (!XII_TEST_BOOL(first.IsValid() && second.IsValid()))
    return;
  xiiTaskSystem::WaitForGroup(first);
  // Exercise the actual close drain without destroying the document on a broken baseline.
  document.FinishPendingSaves();
  XII_TEST_INT(callbacks, 2);
  XII_TEST_INT(savedEvents, 2);
  xiiTaskSystem::WaitForGroup(second);
  XII_TEST_BOOL(!document.IsModified());
  CheckSavedInteger(path, object, 9002);
  document.m_EventsOne.RemoveEventHandler(subscription);

  xiiHybridArray<xiiTaskGroupID, 4> tasks;
  callbacks = 0;
  xiiDocument::AfterSaveCallback enqueueNext;
  enqueueNext = [&](xiiDocument* saved, xiiStatus result) {
    XII_TEST_BOOL(saved == &document && result.Succeeded());
    if (++callbacks < 3)
    {
      XII_TEST_STATUS(ChangeSavedInteger(document, object, 9100 + callbacks));
      tasks.PushBack(document.SaveDocumentAsync(enqueueNext));
    }
  };
  XII_TEST_STATUS(ChangeSavedInteger(document, object, 9100));
  tasks.PushBack(document.SaveDocumentAsync(enqueueNext));
  document.FinishPendingSaves();
  XII_TEST_INT(callbacks, 3);
  // Keep negative-control executions safe even if closing forgot tasks enqueued by callbacks.
  for (xiiUInt32 i = 0; i < tasks.GetCount(); ++i)
  {
    const auto task = tasks[i];
    xiiTaskSystem::WaitForGroup(task);
  }
  XII_TEST_INT(callbacks, 3);
  XII_TEST_BOOL(!document.IsModified());
  CheckSavedInteger(path, object, 9102);
}

XII_CREATE_SIMPLE_TEST(DocumentPersistence, QueuedSaveResultsDescribeTheirOwnSnapshot)
{
  xiiStringBuilder output = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  output.AppendPath("DocumentSaveResults");
  if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(output)) ||
      !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(output, "DocumentSaveResults", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("DocumentSaveResults"));
  SaveTestManager  manager;
  xiiStringBuilder path = output;
  path.AppendPath("Results.xiiSaveTest");
  xiiUuid object, other;
  if (!MakeSeed(manager, path, object, other))
    return;
  xiiDocument* base = nullptr;
  if (!XII_TEST_BOOL(manager.OpenDocument("SaveValidation", path, base, xiiDocumentFlags::None).Succeeded()))
    return;
  auto&                   document    = *static_cast<SaveTestDocument*>(base);
  constexpr xiiStringView unavailable = ":UnregisteredSaveRoot/Failure.xiiSaveTest";
  document.DocumentRenamed(unavailable);
  XII_TEST_STATUS(ChangeSavedInteger(document, object, 9001));
  xiiUInt32  callbacks = 0;
  const auto failed    = document.SaveDocumentAsync([&](xiiDocument* saved, xiiStatus result) {
    XII_TEST_BOOL(saved == &document && result.Failed());
    XII_TEST_BOOL(result.GetMessageString().FindSubString(unavailable) != nullptr);
    XII_TEST_BOOL(document.IsModified());
    ++callbacks;
  });
  document.DocumentRenamed(path);
  XII_TEST_STATUS(ChangeSavedInteger(document, object, 9002));
  const auto successful = document.SaveDocumentAsync([&](xiiDocument* saved, xiiStatus result) {
    XII_TEST_BOOL(saved == &document && result.Succeeded());
    XII_TEST_BOOL(!document.IsModified());
    ++callbacks;
  });
  if (!XII_TEST_BOOL(failed.IsValid() && successful.IsValid()))
    return;
  xiiTaskSystem::WaitForGroup(successful);
  XII_TEST_INT(callbacks, 2);
  XII_TEST_INT(document.m_uiAfterSaveCount, 1);
  CheckSavedInteger(path, object, 9002);
}

XII_CREATE_SIMPLE_TEST(DocumentPersistence, StateChangesWhileWaitingAreRevalidated)
{
  xiiStringBuilder output = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  output.AppendPath("DocumentSaveWaitValidation");
  if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(output)) ||
      !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(output, "DocumentSaveWaitValidation", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("DocumentSaveWaitValidation"));
  SaveTestManager  manager;
  xiiStringBuilder path = output;
  path.AppendPath("WaitValidation.xiiSaveTest");
  xiiUuid object, other;
  if (!MakeSeed(manager, path, object, other))
    return;
  xiiDocument* base = nullptr;
  if (!XII_TEST_BOOL(manager.OpenDocument("SaveValidation", path, base, xiiDocumentFlags::None).Succeeded()))
    return;
  auto& document = *static_cast<SaveTestDocument*>(base);
  XII_TEST_STATUS(ChangeSavedInteger(document, object, 9001));
  xiiUInt32  callbacks = 0;
  const auto pending   = document.SaveDocumentAsync([&](xiiDocument* saved, xiiStatus result) {
    XII_TEST_BOOL(saved == &document && result.Succeeded());
    document.SetWritable(false);
    ++callbacks;
  });
  if (!XII_TEST_BOOL(pending.IsValid()))
    return;
  // The document is writable at entry; waiting invokes the earlier save's callback.
  XII_TEST_BOOL(document.SaveDocument(true).Failed());
  XII_TEST_INT(callbacks, 1);
  XII_TEST_INT(document.m_uiSerializationCount, 1);
  CheckSavedInteger(path, object, 9001);
  document.SetWritable(true);
  XII_TEST_STATUS(ChangeSavedInteger(document, object, 9002));
  XII_TEST_BOOL(document.SaveDocument().Succeeded());
  XII_TEST_INT(document.m_uiSerializationCount, 2);
  CheckSavedInteger(path, object, 9002);
}
