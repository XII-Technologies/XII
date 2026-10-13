/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFrameworkTest/AssetResaveTest.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <Foundation/Strings/StringConversion.h>
#endif

class ResaveTestProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(ResaveTestProperties, xiiReflectedClass);

public:
  xiiString m_sTransform;
  xiiString m_sThumbnail;
  xiiString m_sPackage;
};

class ResaveTestDocument : public xiiSimpleAssetDocument<ResaveTestProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(ResaveTestDocument, xiiAssetDocument);

public:
  ResaveTestDocument(xiiStringView path) : xiiSimpleAssetDocument(path, xiiAssetDocEngineConnection::None) {}
  void MakeReadOnly(bool value) { SetReadOnly(value); }
  void MakePartiallyUnderstood(bool value)
  {
    xiiSet<xiiString> types;
    if (value)
      types.Insert("MissingResaveTestType");
    SetUnknownObjectTypes(types, value ? 1 : 0);
  }

protected:
  xiiTransformStatus InternalTransformAsset(xiiStreamWriter&, xiiStringView, const xiiPlatformProfile*, const xiiAssetFileHeader&, xiiBitflags<xiiTransformFlags>) override
  {
    return xiiTransformStatus("This fixture tests document resaving, with transformation disabled.");
  }
};

class ResaveTestManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(ResaveTestManager, xiiAssetDocumentManager);

public:
  ResaveTestManager()
  {
    m_Type.m_sDocumentTypeName  = "ResaveTest";
    m_Type.m_sFileExtension     = "xiiResaveTest";
    m_Type.m_sAssetCategory     = "Testing";
    m_Type.m_pDocumentType      = xiiGetStaticRTTI<ResaveTestDocument>();
    m_Type.m_pManager           = this;
    m_Type.m_AssetDocumentFlags = xiiAssetDocumentFlags::DisableTransform;
  }
  ~ResaveTestManager() override { CloseAllDocumentsOfManager(); }
  bool           GeneratesProfileSpecificAssets() const override { return false; }
  static xiiUuid GetSubAssetId(xiiUuid owner)
  {
    owner.CombineWithSeed(xiiUuid::MakeStableUuidFromString("ResaveTestChild"));
    return owner;
  }
  void FillOutSubAssetList(const xiiAssetDocumentInfo& info, xiiDynamicArray<xiiSubAssetData>& out_subAssets) const override
  {
    auto& sub   = out_subAssets.ExpandAndGetRef();
    sub.m_Guid  = GetSubAssetId(info.m_DocumentID);
    sub.m_sName = "Child";
    sub.m_sSubAssetsDocumentTypeName.Assign("ResaveTest");
  }

private:
  xiiStatus InternalCreateDocument(xiiStringView, xiiStringView path, bool, xiiDocument*& out_document, const xiiDocumentObject*) override
  {
    out_document = new ResaveTestDocument(path);
    return XII_SUCCESS;
  }
  void                           InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& types) const override { types.PushBack(&m_Type); }
  xiiAssetDocumentTypeDescriptor m_Type;
};

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(ResaveTestProperties, 1, xiiRTTIDefaultAllocator<ResaveTestProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Transform", m_sTransform)->AddAttributes(new xiiAssetBrowserAttribute("ResaveTest", xiiDependencyFlags::Transform)),
    XII_MEMBER_PROPERTY("Thumbnail", m_sThumbnail)->AddAttributes(new xiiAssetBrowserAttribute("ResaveTest", xiiDependencyFlags::Thumbnail)),
    XII_MEMBER_PROPERTY("Package", m_sPackage)->AddAttributes(new xiiAssetBrowserAttribute("ResaveTest", xiiDependencyFlags::Package)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(ResaveTestDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(ResaveTestManager, 1, xiiRTTIDefaultAllocator<ResaveTestManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void TestAssetResave(xiiQtEditorApp& editor, xiiStringView projectDirectory)
{
  xiiStringBuilder directory = projectDirectory;
  directory.AppendPath("Resave");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(directory));
  ResaveTestDocument* documents[4] = {};
  xiiUuid             ids[4];
  xiiString           paths[4];
  for (xiiUInt32 i = 0; i < 4; ++i)
  {
    xiiStringBuilder path = directory;
    path.AppendPath(i == 0 ? "AConsumer.xiiResaveTest" : i == 1 ? "BSource.xiiResaveTest" :
                      i == 2                                    ? "CIsolated.xiiResaveTest" :
                                                                  "DConsumer.xiiResaveTest");
    documents[i] = xiiDynamicCast<ResaveTestDocument*>(editor.CreateDocument(path, xiiDocumentFlags::None));
    if (!XII_TEST_BOOL(documents[i] != nullptr))
      return;
    ids[i]   = documents[i]->GetGuid();
    paths[i] = path;
  }
  XII_SCOPE_EXIT(for (const auto id : ids) if (auto* doc = xiiDocumentManager::GetDocumentByGuid(id)) doc->GetDocumentManager()->CloseDocument(doc));
  auto setReference = [&](xiiUInt32 index, const char* property, xiiStringView reference) {
    auto* accessor = documents[index]->GetObjectAccessor();
    accessor->StartTransaction("Set resave dependency");
    XII_TEST_STATUS(accessor->SetValueByName(documents[index]->GetPropertyObject(), property, reference));
    accessor->FinishTransaction();
    XII_TEST_STATUS(documents[index]->SaveDocument());
  };
  xiiStringBuilder sourceId, consumerId;
  xiiConversionUtils::ToString(ResaveTestManager::GetSubAssetId(ids[1]), sourceId);
  xiiConversionUtils::ToString(ids[0], consumerId);
  setReference(0, "Package", sourceId);
  setReference(0, "Thumbnail", paths[1]);                     // The same dependency by path and UUID creates parallel edges.
  setReference(0, "Transform", "DependencySource.xiiPrefab"); // Outside the selected subtree.
  setReference(3, "Transform", consumerId);
  auto* curator = xiiAssetCurator::GetSingleton();
  for (const auto& path : paths)
    curator->NotifyOfFileChange(path);
  curator->MainThreadTick(false);
  {
    const auto subAsset = curator->FindSubAsset(sourceId);
    if (!XII_TEST_BOOL(static_cast<bool>(subAsset)))
      return;
    XII_TEST_BOOL(!subAsset->m_bMainAsset);
    XII_TEST_BOOL(subAsset->m_pAssetInfo->m_Info->m_DocumentID == ids[1]);
  }
  xiiDynamicArray<xiiUuid> order, repeated;
  XII_TEST_STATUS(curator->GetAssetResaveOrder(directory, order));
  XII_TEST_INT(order.GetCount(), 4);
  XII_TEST_BOOL(order.IndexOf(ids[1]) < order.IndexOf(ids[0]));
  XII_TEST_BOOL(order.IndexOf(ids[0]) < order.IndexOf(ids[3]));
  XII_TEST_STATUS(curator->GetAssetResaveOrder(directory, repeated));
  XII_TEST_BOOL(order == repeated);
  xiiStringBuilder emptyDirectory = directory;
  emptyDirectory.AppendPath("NoAssets");
  XII_TEST_STATUS(curator->GetAssetResaveOrder(emptyDirectory, repeated));
  XII_TEST_BOOL(repeated.IsEmpty());
  XII_TEST_STATUS(curator->ResaveAllAssets(emptyDirectory));

  xiiDynamicArray<xiiUuid> saved;
  auto                     recordSave = [&](const xiiDocumentEvent& event) {
    if (event.m_Type == xiiDocumentEvent::Type::DocumentSaved && order.Contains(event.m_pDocument->GetGuid()))
      saved.PushBack(event.m_pDocument->GetGuid());
  };
  auto subscription = xiiDocument::s_EventsAny.AddEventHandler(recordSave);
  XII_SCOPE_EXIT(xiiDocument::s_EventsAny.RemoveEventHandler(subscription));
  // Only the consumer stays open. Batch resave must leave it open and close its own documents.
  for (xiiUInt32 i = 1; i < 4; ++i)
  {
    documents[i]->GetDocumentManager()->CloseDocument(documents[i]);
    documents[i] = nullptr;
  }
  xiiProgress progress;
  auto*       previousProgress = xiiProgress::GetGlobalProgressbar();
  xiiProgress::SetGlobalProgressbar(&progress);
  XII_SCOPE_EXIT(xiiProgress::SetGlobalProgressbar(previousProgress));
  XII_TEST_STATUS(curator->ResaveAllAssets(directory));
  XII_TEST_BOOL(saved == order);
  XII_TEST_BOOL(xiiDocumentManager::GetDocumentByGuid(ids[0]) == documents[0]);
  for (xiiUInt32 i = 1; i < 4; ++i)
    XII_TEST_BOOL(xiiDocumentManager::GetDocumentByGuid(ids[i]) == nullptr);

  saved.Clear();
  auto cancel = [&](const xiiProgressEvent& event) {
    if (event.m_Type == xiiProgressEvent::Type::ProgressStarted)
      progress.UserClickedCancel();
  };
  auto progressSubscription = progress.m_Events.AddEventHandler(cancel);
  XII_TEST_BOOL(curator->ResaveAllAssets(directory).Failed());
  progress.m_Events.RemoveEventHandler(progressSubscription);
  XII_TEST_BOOL(saved.IsEmpty());

  bool rejectedReentry = false;
  auto reenter         = [&](const xiiProgressEvent& event) {
    if (event.m_Type == xiiProgressEvent::Type::ProgressStarted)
      rejectedReentry = curator->ResaveAllAssets(directory).Failed();
  };
  progressSubscription = progress.m_Events.AddEventHandler(reenter);
  XII_TEST_STATUS(curator->ResaveAllAssets(directory));
  progress.m_Events.RemoveEventHandler(progressSubscription);
  XII_TEST_BOOL(rejectedReentry);
  XII_TEST_BOOL(saved == order);

  // A worker must be able to snapshot the curator while progress callbacks run. Waiting only
  // inside this callback exposes a retained global lock; joining afterwards also keeps a failing
  // regression from deadlocking the test process.
  xiiThreadSignal queried;
  xiiTaskGroupID  queryGroup;
  xiiResult       queriedSuccessfully = XII_FAILURE;
  auto            queryDuringProgress = [&](const xiiProgressEvent& event) {
    if (event.m_Type != xiiProgressEvent::Type::ProgressStarted)
      return;
    auto task  = XII_DEFAULT_NEW(xiiDelegateTask<void>, "Resave curator snapshot", xiiTaskNesting::Never, [&] {
      xiiDynamicArray<xiiUuid> snapshot;
      queriedSuccessfully = curator->GetAssetResaveOrder(directory, snapshot).GetResult();
      queried.RaiseSignal();
    });
    queryGroup = xiiTaskSystem::StartSingleTask(task, xiiTaskPriority::LongRunning);
    XII_TEST_BOOL(queried.WaitForSignal(xiiTime::MakeFromSeconds(2)) == xiiThreadSignal::WaitResult::Signaled);
  };
  progressSubscription = progress.m_Events.AddEventHandler(queryDuringProgress);
  XII_TEST_STATUS(curator->ResaveAllAssets(directory));
  progress.m_Events.RemoveEventHandler(progressSubscription);
  if (XII_TEST_BOOL(queryGroup.IsValid()))
    xiiTaskSystem::WaitForGroup(queryGroup);
  XII_TEST_RESULT(queriedSuccessfully);

  saved.Clear();
  documents[0]->MakeReadOnly(true);
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope scope(&log);
    log.ExpectMessage("Cannot resave read-only", xiiLogMsgType::ErrorMsg);
    XII_TEST_BOOL(curator->ResaveAllAssets(directory).Failed());
  }
  documents[0]->MakeReadOnly(false);
  XII_TEST_INT(saved.GetCount(), 3);
  XII_TEST_BOOL(!saved.Contains(ids[0]));

  saved.Clear();
  documents[0]->MakePartiallyUnderstood(true);
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope scope(&log);
    log.ExpectMessage("partially understood asset", xiiLogMsgType::ErrorMsg);
    XII_TEST_BOOL(curator->ResaveAllAssets(directory).Failed());
  }
  documents[0]->MakePartiallyUnderstood(false);
  XII_TEST_INT(saved.GetCount(), 3);
  XII_TEST_BOOL(!saved.Contains(ids[0]));

  saved.Clear();
  xiiDynamicArray<xiiUInt8> removedContent;
  {
    xiiOSFile input;
    if (!XII_TEST_RESULT(input.Open(paths[0], xiiFileOpenMode::Read)))
      return;
    input.ReadAll(removedContent);
  }
  bool removedDuringProgress = false;
  auto removeCurrent         = [&](const xiiProgressEvent& event) {
    if (!removedDuringProgress && event.m_Type == xiiProgressEvent::Type::ProgressChanged &&
        progress.GetStepDisplayText() == "AConsumer.xiiResaveTest")
    {
      removedDuringProgress = true;
      const auto deadline   = xiiTime::Now() + xiiTime::MakeFromSeconds(2);
      xiiResult  removed    = XII_FAILURE;
      do
      {
        removed = xiiOSFile::DeleteFile(paths[0]);
        if (removed.Failed())
          xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
      } while (removed.Failed() && xiiTime::Now() < deadline);
      if (!XII_TEST_RESULT(removed))
        return;
      curator->NotifyOfFileChange(paths[0]);
    }
  };
  progressSubscription = progress.m_Events.AddEventHandler(removeCurrent);
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope scope(&log);
    log.ExpectMessage("asset no longer exists", xiiLogMsgType::ErrorMsg);
    XII_TEST_BOOL(curator->ResaveAllAssets(directory).Failed());
  }
  progress.m_Events.RemoveEventHandler(progressSubscription);
  XII_TEST_BOOL(removedDuringProgress);
  XII_TEST_BOOL(!xiiOSFile::ExistsFile(paths[0]));
  XII_TEST_BOOL(!saved.Contains(ids[0]));
  // Editor asset-removal events may close the pre-existing document. Restore the source bytes
  // independently, then reacquire the document; a pointer retained across removal may be stale.
  {
    xiiOSFile output;
    if (!XII_TEST_RESULT(output.Open(paths[0], xiiFileOpenMode::Write)))
      return;
    XII_TEST_RESULT(output.Write(removedContent.GetData(), removedContent.GetCount()));
  }
  curator->NotifyOfFileChange(paths[0]);
  curator->MainThreadTick(false);
  documents[0] = xiiDynamicCast<ResaveTestDocument*>(editor.OpenDocument(paths[0], xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(documents[0] != nullptr))
    return;
  XII_TEST_BOOL(documents[0]->GetGuid() == ids[0]);
  XII_TEST_STATUS(curator->GetAssetResaveOrder(directory, repeated));
  XII_TEST_BOOL(repeated == order);

  // A self-reference is rejected before any document is written, with no partial order output.
  setReference(0, "Package", consumerId);
  curator->NotifyOfFileChange(paths[0]);
  curator->MainThreadTick(false);
  saved.Clear();
  const auto cycle = curator->GetAssetResaveOrder(directory, repeated);
  XII_TEST_BOOL(cycle.Failed());
  XII_TEST_BOOL(cycle.GetMessageString().FindSubString("AConsumer") != nullptr);
  XII_TEST_BOOL(repeated.IsEmpty());
  XII_TEST_BOOL(curator->ResaveAllAssets(directory).Failed());
  XII_TEST_BOOL(saved.IsEmpty());
  setReference(0, "Package", sourceId);
  curator->NotifyOfFileChange(paths[0]);
  curator->MainThreadTick(false);
  XII_TEST_STATUS(curator->GetAssetResaveOrder(directory, repeated));
  XII_TEST_BOOL(repeated == order);

  documents[1] = xiiDynamicCast<ResaveTestDocument*>(editor.OpenDocument(paths[1], xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(documents[1] != nullptr))
    return;
  setReference(1, "Package", consumerId);
  curator->NotifyOfFileChange(paths[1]);
  curator->MainThreadTick(false);
  saved.Clear();
  const auto multiCycle = curator->GetAssetResaveOrder(directory, repeated);
  XII_TEST_BOOL(multiCycle.Failed());
  XII_TEST_BOOL(multiCycle.GetMessageString().FindSubString("AConsumer") != nullptr);
  XII_TEST_BOOL(multiCycle.GetMessageString().FindSubString("BSource") != nullptr);
  XII_TEST_BOOL(multiCycle.GetMessageString().FindSubString("DConsumer") == nullptr); // Blocked tail is not part of the cycle.
  XII_TEST_BOOL(repeated.IsEmpty());
  XII_TEST_BOOL(curator->ResaveAllAssets(directory).Failed());
  XII_TEST_BOOL(saved.IsEmpty());
  setReference(1, "Package", "");
  curator->NotifyOfFileChange(paths[1]);
  curator->MainThreadTick(false);
  XII_TEST_STATUS(curator->GetAssetResaveOrder(directory, repeated));
  XII_TEST_BOOL(repeated == order);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  // Keep both the tracked and physical modification times unchanged. Only the explicit known-write
  // notification can refresh these dependencies; polling or waiting for a newer timestamp cannot.
  xiiDynamicArray<xiiUInt8> bytes;
  xiiOSFile                 input;
  if (!XII_TEST_RESULT(input.Open(paths[0], xiiFileOpenMode::Read)))
    return;
  input.ReadAll(bytes);
  input.Close();
  const xiiString original(xiiStringView(reinterpret_cast<const char*>(bytes.GetData()), bytes.GetCount()));
  HANDLE          handle = CreateFileW(xiiStringWChar(paths[0]).GetData(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                       nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (!XII_TEST_BOOL(handle != INVALID_HANDLE_VALUE))
    return;
  FILETIME   modificationTime;
  const bool gotTime = GetFileTime(handle, nullptr, nullptr, &modificationTime) != FALSE;
  CloseHandle(handle);
  if (!XII_TEST_BOOL(gotTime))
    return;
  xiiFileStatus trackedBefore;
  XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->FindFile(paths[0], trackedBefore));
  auto writePreservingTime = [&](xiiStringView text) -> xiiResult {
    HANDLE     file     = INVALID_HANDLE_VALUE;
    const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(2);
    do
    {
      file = CreateFileW(xiiStringWChar(paths[0]).GetData(), GENERIC_WRITE | FILE_WRITE_ATTRIBUTES, 0,
                         nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
      if (file == INVALID_HANDLE_VALUE)
        xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
    } while (file == INVALID_HANDLE_VALUE && xiiTime::Now() < deadline);
    if (file == INVALID_HANDLE_VALUE)
      return XII_FAILURE;
    // Acquire the file first, so a reader cannot hold it open while waiting on our model lock.
    auto       files   = xiiFileSystemModel::GetSingleton()->GetFiles();
    DWORD      written = 0;
    const bool result  = WriteFile(file, text.GetStartPointer(), text.GetElementCount(), &written, nullptr) != FALSE &&
      written == text.GetElementCount() && SetFileTime(file, nullptr, nullptr, &modificationTime) != FALSE;
    CloseHandle(file);
    return result ? XII_SUCCESS : XII_FAILURE;
  };
  bool restored = false;
  XII_SCOPE_EXIT(if (!restored) {writePreservingTime(original).IgnoreResult(); curator->NotifyOfFileChange(paths[0]); });
  xiiStringBuilder changed = original;
  XII_TEST_INT(changed.ReplaceAll(sourceId, consumerId), 2);
  if (!XII_TEST_RESULT(writePreservingTime(changed)))
    return;
  xiiFileStatus trackedAfter;
  xiiFileStats  physicalAfter;
  XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->FindFile(paths[0], trackedAfter));
  XII_TEST_RESULT(xiiOSFile::GetFileStats(paths[0], physicalAfter));
  XII_TEST_BOOL(trackedBefore.m_LastModified.Compare(trackedAfter.m_LastModified, xiiTimestamp::CompareMode::Identical));
  XII_TEST_BOOL(trackedBefore.m_LastModified.Compare(physicalAfter.m_LastModificationTime, xiiTimestamp::CompareMode::Identical));
  curator->NotifyOfFileChange(paths[0]);
  XII_TEST_BOOL(curator->GetAssetResaveOrder(directory, repeated).Failed());
  XII_TEST_BOOL(repeated.IsEmpty());
  if (!XII_TEST_RESULT(writePreservingTime(original)))
    return;
  curator->NotifyOfFileChange(paths[0]);
  restored = true;
  XII_TEST_STATUS(curator->GetAssetResaveOrder(directory, repeated));
  XII_TEST_BOOL(repeated == order);
#endif
}
