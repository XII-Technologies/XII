/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/ScopeExit.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundationTest/Object/DocumentSaveTestUtils.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

using namespace xiiDocumentSaveTestUtils;

XII_CREATE_SIMPLE_TEST_GROUP(DocumentPersistence);

XII_CREATE_SIMPLE_TEST(DocumentPersistence, SkippedObjectsAreNotOverwritten)
{
  xiiStringBuilder output = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  output.AppendPath("DocumentSave");
  if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(output)) || !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(output, "DocumentSave", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("DocumentSave"));
  SaveTestManager  manager;
  xiiStringBuilder path = output;
  path.AppendPath("MissingPlugin.xiiSaveTest");
  xiiUuid known, missing;
  if (!MakeSeed(manager, path, known, missing))
    return;
  xiiUniquePtr<xiiAbstractObjectGraph> header, objects, types;
  if (!XII_TEST_BOOL(xiiDocument::ReadDocument(path, header, objects, types).Succeeded()))
    return;
  // Keep the serialized payload and parent reference, but omit a matching RTTI definition.
  objects->GetNode(missing)->SetType("MissingPluginPayload");
  xiiContiguousMemoryStreamStorage original;
  xiiMemoryStreamWriter            writer(&original);
  xiiAbstractGraphDdlSerializer::WriteDocument(writer, header.Borrow(), objects.Borrow(), types.Borrow(), false);

  for (bool asynchronous : {false, true})
  {
    for (bool force : {false, true})
    {
      for (bool modified : {false, true})
      {
        if (!force && !modified)
          continue;
        if (!XII_TEST_RESULT(WriteContents(path, original)))
          return;
        xiiDocument* base = nullptr;
        {
          xiiTestLogInterface   log;
          xiiTestLogSystemScope scope(&log, true);
          log.ExpectMessage("Can't patch base class, parent type of 'MissingPluginPayload' unknown.", xiiLogMsgType::ErrorMsg);
          log.ExpectMessage("Cannot create node of unknown type 'MissingPluginPayload'", xiiLogMsgType::ErrorMsg);
          if (!XII_TEST_BOOL(manager.OpenDocument("SaveValidation", path, base, xiiDocumentFlags::None).Succeeded()))
            return;
        }
        XII_SCOPE_EXIT(manager.CloseDocument(base));
        auto& document = *static_cast<SaveTestDocument*>(base);
        XII_TEST_INT(document.GetUnknownObjectTypeInstances(), 1);
        XII_TEST_BOOL(document.GetUnknownObjectTypes().Contains("MissingPluginPayload"));
        XII_TEST_BOOL(document.GetObjectManager()->GetObject(known) != nullptr);
        XII_TEST_BOOL(document.GetObjectManager()->GetObject(missing) == nullptr);
        if (modified && !XII_TEST_RESULT(EditContent(document, known)))
          return;
        xiiUInt32 savedEvents  = 0;
        auto      subscription = document.m_EventsOne.AddEventHandler([&](const xiiDocumentEvent& event) {
          if (event.m_Type == xiiDocumentEvent::Type::DocumentSaved)
            ++savedEvents;
        });
        XII_SCOPE_EXIT(document.m_EventsOne.RemoveEventHandler(subscription));
        xiiStatus status(XII_SUCCESS);
        if (asynchronous)
        {
          xiiUInt32  callbacks = 0;
          const auto task      = document.SaveDocumentAsync([&](xiiDocument* saved, xiiStatus result) {
            XII_TEST_BOOL(saved == &document);
            ++callbacks;
            status = result;
          },
                                                       force);
          XII_TEST_BOOL(!task.IsValid());
          XII_TEST_INT(callbacks, 1);
          // Drain the old implementation's real write when running the negative control.
          if (task.IsValid())
            xiiTaskSystem::WaitForGroup(task);
          XII_TEST_INT(callbacks, 1);
        }
        else
          status = document.SaveDocument(force);
        XII_TEST_BOOL(status.Failed());
        XII_TEST_BOOL(status.GetMessageString().FindSubString("unknown") != nullptr);
        XII_TEST_INT(document.m_uiSerializationCount, 0);
        XII_TEST_INT(document.m_uiAfterSaveCount, 0);
        XII_TEST_INT(savedEvents, 0);
        XII_TEST_BOOL(document.IsModified() == modified);
        XII_TEST_INT(document.GetUnknownObjectTypeInstances(), 1);
        XII_TEST_BOOL(document.GetUnknownObjectTypes().Contains("MissingPluginPayload"));
        xiiContiguousMemoryStreamStorage after;
        if (!XII_TEST_RESULT(ReadContents(path, after)))
          return;
        XII_TEST_BOOL(after.GetContiguousMemoryRange(0) == original.GetContiguousMemoryRange(0));
      }
    }
  }
}

XII_CREATE_SIMPLE_TEST(DocumentPersistence, ReadOnlySaveAndRecovery)
{
  xiiStringBuilder output = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  output.AppendPath("DocumentSaveReadOnly");
  if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(output)) || !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(output, "DocumentSaveReadOnly", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("DocumentSaveReadOnly"));
  SaveTestManager  manager;
  xiiStringBuilder path = output;
  path.AppendPath("ReadOnly.xiiSaveTest");
  xiiUuid known, other;
  if (!MakeSeed(manager, path, known, other))
    return;
  xiiContiguousMemoryStreamStorage original;
  if (!XII_TEST_RESULT(ReadContents(path, original)))
    return;
  xiiDocument* base = nullptr;
  if (!XII_TEST_BOOL(manager.OpenDocument("SaveValidation", path, base, xiiDocumentFlags::None).Succeeded()))
    return;
  auto& document = *static_cast<SaveTestDocument*>(base);
  if (!XII_TEST_RESULT(EditContent(document, known)))
    return;
  document.SetWritable(false);
  XII_TEST_BOOL(document.SaveDocument().Failed());
  xiiUInt32  callbacks = 0;
  const auto rejected  = document.SaveDocumentAsync([&](xiiDocument* saved, xiiStatus result) {
    XII_TEST_BOOL(saved == &document && result.Failed());
    ++callbacks;
  },
                                                   true);
  XII_TEST_BOOL(!rejected.IsValid());
  XII_TEST_INT(callbacks, 1);
  if (rejected.IsValid())
    xiiTaskSystem::WaitForGroup(rejected);
  XII_TEST_BOOL(document.IsModified());
  XII_TEST_INT(document.m_uiSerializationCount, 0);
  xiiContiguousMemoryStreamStorage after;
  if (!XII_TEST_RESULT(ReadContents(path, after)))
    return;
  XII_TEST_BOOL(after.GetContiguousMemoryRange(0) == original.GetContiguousMemoryRange(0));
  document.SetWritable(true);
  const auto saved = document.SaveDocumentAsync([&](xiiDocument* savedDocument, xiiStatus result) {
    XII_TEST_BOOL(savedDocument == &document && result.Succeeded());
    ++callbacks;
  },
                                                true);
  if (!XII_TEST_BOOL(saved.IsValid()))
    return;
  // Rejection must not replace the active task or overwrite its successful write result.
  document.SetWritable(false);
  XII_TEST_BOOL(document.SaveDocument(true).Failed());
  const auto rejectedWhileSaving = document.SaveDocumentAsync([&](xiiDocument* savedDocument, xiiStatus result) {
    XII_TEST_BOOL(savedDocument == &document && result.Failed());
    ++callbacks;
  },
                                                              true);
  XII_TEST_BOOL(!rejectedWhileSaving.IsValid());
  XII_TEST_INT(callbacks, 2);
  XII_TEST_BOOL(!document.SaveDocumentAsync({}, true).IsValid());
  if (rejectedWhileSaving.IsValid())
    xiiTaskSystem::WaitForGroup(rejectedWhileSaving);
  xiiTaskSystem::WaitForGroup(saved);
  document.SetWritable(true);
  XII_TEST_INT(callbacks, 3);
  XII_TEST_BOOL(!document.IsModified());
  XII_TEST_INT(document.m_uiSerializationCount, 1);
  XII_TEST_INT(document.m_uiAfterSaveCount, 1);
  manager.CloseDocument(base);
  if (!XII_TEST_BOOL(manager.OpenDocument("SaveValidation", path, base, xiiDocumentFlags::None).Succeeded()))
    return;
  xiiVariant value;
  XII_TEST_STATUS(base->GetObjectAccessor()->GetValueByName(base->GetObjectManager()->GetObject(known), "Int32", value));
  XII_TEST_INT(value.Get<xiiInt32>(), 9001);
}

XII_CREATE_SIMPLE_TEST(DocumentPersistence, EmbeddedPhantomTypesRemainWritable)
{
  xiiStringBuilder output = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  output.AppendPath("DocumentSavePhantom");
  if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(output)) || !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(output, "DocumentSavePhantom", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("DocumentSavePhantom"));
  constexpr xiiStringView    typeName = "xiiSaveTestEmbeddedPayload";
  xiiReflectedTypeDescriptor descriptor;
  xiiToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(xiiGetStaticRTTI<xiiIntegerStruct>(), descriptor);
  descriptor.m_sTypeName = typeName;
  const auto* phantom    = xiiPhantomRttiManager::RegisterType(descriptor);
  if (!XII_TEST_BOOL(phantom != nullptr))
    return;
  XII_SCOPE_EXIT(if (auto* type = xiiRTTI::FindTypeByName(typeName)) xiiPhantomRttiManager::UnregisterType(type));
  SaveTestManager  manager;
  xiiStringBuilder path = output;
  path.AppendPath("EmbeddedType.xiiSaveTest");
  xiiDocument* document = nullptr;
  if (!XII_TEST_BOOL(manager.CreateDocument("SaveValidation", path, document, xiiDocumentFlags::EmptyDocument).Succeeded()))
    return;
  xiiUuid payload;
  auto*   accessor = document->GetObjectAccessor();
  accessor->StartTransaction("Add plugin payload");
  XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, phantom, payload));
  XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(payload), "Int32", 7829));
  accessor->FinishTransaction();
  if (!XII_TEST_BOOL(document->SaveDocument().Succeeded()))
    return;
  manager.CloseDocument(document);
  XII_TEST_BOOL(xiiPhantomRttiManager::UnregisterType(phantom));
  XII_TEST_BOOL(xiiRTTI::FindTypeByName(typeName) == nullptr);
  // Loading must reconstruct the absent type from its saved descriptor without skipping data.
  if (!XII_TEST_BOOL(manager.OpenDocument("SaveValidation", path, document, xiiDocumentFlags::None).Succeeded()))
    return;
  XII_TEST_INT(document->GetUnknownObjectTypeInstances(), 0);
  xiiVariant value;
  XII_TEST_STATUS(document->GetObjectAccessor()->GetValueByName(document->GetObjectManager()->GetObject(payload), "Int32", value));
  XII_TEST_INT(value.Get<xiiInt32>(), 7829);
  if (!XII_TEST_RESULT(EditContent(*document, payload)) || !XII_TEST_BOOL(document->SaveDocument().Succeeded()))
    return;
  manager.CloseDocument(document);
  const auto* restoredType = xiiRTTI::FindTypeByName(typeName);
  if (!XII_TEST_BOOL(restoredType != nullptr))
    return;
  XII_TEST_BOOL(xiiPhantomRttiManager::UnregisterType(restoredType));
  if (!XII_TEST_BOOL(manager.OpenDocument("SaveValidation", path, document, xiiDocumentFlags::None).Succeeded()))
    return;
  XII_TEST_INT(document->GetUnknownObjectTypeInstances(), 0);
  XII_TEST_STATUS(document->GetObjectAccessor()->GetValueByName(document->GetObjectManager()->GetObject(payload), "Int32", value));
  XII_TEST_INT(value.Get<xiiInt32>(), 9001);
}
