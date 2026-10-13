/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFrameworkTest/AssetDependencyTrackingTest.h>
#include <EditorFrameworkTest/AssetTestDocument.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <TestFramework/Utilities/TestLogInterface.h>

void TestAssetDependencyTracking(xiiQtEditorApp& editor, xiiStringView projectDirectory)
{
  xiiStringBuilder directory(projectDirectory);
  directory.AppendPath("DependencyTracking");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(directory));
  ResaveTestDocument* documents[3] = {};
  xiiUuid             ids[3];
  xiiString           paths[3];
  XII_SCOPE_EXIT(for (const auto id : ids) if (auto* doc = xiiDocumentManager::GetDocumentByGuid(id)) doc->GetDocumentManager()->CloseDocument(doc));
  for (xiiUInt32 i = 0; i < 3; ++i)
  {
    xiiStringBuilder path(directory);
    path.AppendPath(i == 0 ? "Source.xiiResaveTest" : i == 1 ? "Consumer.xiiResaveTest" :
                                                               "Indirect.xiiResaveTest");
    documents[i] = xiiDynamicCast<ResaveTestDocument*>(editor.CreateDocument(path, xiiDocumentFlags::None));
    if (!XII_TEST_BOOL(documents[i] != nullptr))
      return;
    ids[i]   = documents[i]->GetGuid();
    paths[i] = path;
  }
  auto* curator = xiiAssetCurator::GetSingleton();
  auto  set     = [&](xiiUInt32 index, const char* property, xiiStringView value) {
    auto* accessor = documents[index]->GetObjectAccessor();
    accessor->StartTransaction("Set tracked dependency");
    XII_TEST_STATUS(accessor->SetValueByName(documents[index]->GetPropertyObject(), property, value));
    accessor->FinishTransaction();
    XII_TEST_STATUS(documents[index]->SaveDocument());
    curator->NotifyOfFileChange(paths[index]);
  };
  xiiStringBuilder source, consumer, indirect;
  xiiConversionUtils::ToString(ResaveTestManager::GetSubAssetId(ids[0]), source);
  xiiConversionUtils::ToString(ids[1], consumer);
  xiiConversionUtils::ToString(ids[2], indirect);
  // Save references before the source exists in the curator. Its sub-asset must resolve later.
  set(1, "Package", source);
  set(2, "Package", consumer);
  set(0, "Value", "Initial source settings");
  curator->MainThreadTick(false);
  xiiSet<xiiUuid> uses;
  curator->FindAllUses(ResaveTestManager::GetSubAssetId(ids[0]), uses, false);
  XII_TEST_INT(uses.GetCount(), 1);
  XII_TEST_BOOL(uses.Contains(ids[1]));
  uses.Clear();
  curator->FindAllUses(ids[0], uses, true);
  XII_TEST_INT(uses.GetCount(), 2);
  XII_TEST_BOOL(uses.Contains(ids[1]) && uses.Contains(ids[2]));
  uses.Clear();
  uses.Insert(ids[1]); // Existing output must not suppress traversal through this consumer.
  curator->FindAllUses(ids[0], uses, true);
  XII_TEST_BOOL(uses.Contains(ids[2]));
  {
    const auto asset = curator->GetSubAsset(ids[0]);
    if (!XII_TEST_BOOL(static_cast<bool>(asset)))
      return;
    xiiSet<xiiUuid> hull;
    curator->GenerateInverseTransitiveHull(asset->m_pAssetInfo, hull, false, false, true);
    XII_TEST_INT(hull.GetCount(), 3);
    hull.Clear();
    curator->GenerateInverseTransitiveHull(asset->m_pAssetInfo, hull, true, true, false);
    XII_TEST_INT(hull.GetCount(), 1);
  }
  struct Hashes
  {
    xiiUInt64 asset = 0, thumbnail = 0, package = 0;
  };
  auto hash = [&](xiiUInt32 index, Hashes& out, bool force = false) {
    return curator->IsAssetUpToDate(ids[index], curator->GetActiveAssetProfile(), documents[index]->GetAssetDocumentTypeDescriptor(), out.asset, out.thumbnail, out.package, force);
  };
  Hashes before[3], after[3];
  for (xiiUInt32 i = 0; i < 3; ++i)
  {
    const auto state = hash(i, before[i]);
    XII_TEST_BOOL(state == xiiAssetInfo::UpToDate || state == xiiAssetInfo::NeedsTransform);
    XII_TEST_BOOL(before[i].asset != 0 && before[i].thumbnail != 0 && before[i].package != 0);
  }
  set(0, "Value", "Changed source settings");
  for (xiiUInt32 i = 0; i < 3; ++i)
  {
    hash(i, after[i]); // Deliberately use cached mode, without bForce.
    XII_TEST_BOOL(before[i].thumbnail != after[i].thumbnail && before[i].package != after[i].package);
    XII_TEST_BOOL(i == 0 ? before[i].asset != after[i].asset : before[i].asset == after[i].asset);
  }
  // A raw runtime file contributes to package hashes without forcing source transformation.
  xiiStringBuilder runtimeFile(directory);
  runtimeFile.AppendPath("Runtime.txt");
  auto write = [&](xiiStringView content) {
    xiiOSFile file;
    XII_TEST_RESULT(file.Open(runtimeFile, xiiFileOpenMode::Write));
    XII_TEST_RESULT(file.Write(content.GetStartPointer(), content.GetElementCount()));
    file.Close();
    curator->NotifyOfFileChange(runtimeFile);
  };
  write("First runtime content");
  set(0, "Package", runtimeFile);
  for (xiiUInt32 i = 0; i < 3; ++i)
    hash(i, before[i]);
  write("Changed runtime content");
  for (xiiUInt32 i = 0; i < 3; ++i)
  {
    hash(i, after[i]);
    XII_TEST_BOOL(before[i].asset == after[i].asset);
    XII_TEST_BOOL(before[i].thumbnail != after[i].thumbnail && before[i].package != after[i].package);
  }
  XII_TEST_RESULT(xiiOSFile::DeleteFile(runtimeFile));
  curator->NotifyOfFileChange(runtimeFile);
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope scope(&log);
    log.ExpectMessage("retrieve stats failed", xiiLogMsgType::ErrorMsg);
    XII_TEST_BOOL(hash(0, after[0]) == xiiAssetInfo::MissingPackageDependency);
  }
  write("Restored runtime content");
  XII_TEST_BOOL(hash(0, after[0]) != xiiAssetInfo::MissingPackageDependency);
  XII_TEST_BOOL(after[0].package != before[0].package);

  // A package edge closing a cycle must be diagnosed before recursive hashing starts.
  set(0, "Package", indirect);
  XII_TEST_BOOL(hash(0, after[0]) == xiiAssetInfo::CircularDependency);
  set(0, "Package", "");
  XII_TEST_BOOL(hash(0, after[0]) != xiiAssetInfo::CircularDependency);
  // Retargeting and removing aliases must leave no stale reverse references.
  set(1, "Thumbnail", paths[0]);
  set(1, "Package", "");
  uses.Clear();
  curator->FindAllUses(ids[0], uses, false);
  XII_TEST_BOOL(uses.Contains(ids[1]));
  set(1, "Thumbnail", "");
  uses.Clear();
  curator->FindAllUses(ids[0], uses, true);
  XII_TEST_BOOL(uses.IsEmpty());
  set(1, "Package", paths[0]);
  uses.Clear();
  curator->FindAllUses(ids[0], uses, true);
  XII_TEST_BOOL(uses.Contains(ids[1]) && uses.Contains(ids[2]));
  set(1, "Package", "");
  hash(1, before[1]);
  hash(2, before[2]);
  set(0, "Value", "Unreferenced source settings");
  hash(1, after[1]);
  hash(2, after[2]);
  for (xiiUInt32 i = 1; i < 3; ++i)
    XII_TEST_BOOL(before[i].asset == after[i].asset && before[i].thumbnail == after[i].thumbnail && before[i].package == after[i].package);

  // Remove a resolved sub-asset's owner, then remove its reference after its identity is gone.
  set(1, "Package", source);
  hash(1, before[1]);
  xiiDynamicArray<xiiUInt8> sourceContent;
  {
    xiiOSFile file;
    if (!XII_TEST_RESULT(file.Open(paths[0], xiiFileOpenMode::Read)))
      return;
    file.ReadAll(sourceContent);
  }
  documents[0]->GetDocumentManager()->CloseDocument(documents[0]);
  documents[0]        = nullptr;
  xiiResult  removed  = XII_FAILURE;
  const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(2);
  do
  {
    removed = xiiOSFile::DeleteFile(paths[0]);
    if (removed.Failed())
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
  } while (removed.Failed() && xiiTime::Now() < deadline);
  if (!XII_TEST_RESULT(removed))
    return;
  curator->NotifyOfFileChange(paths[0]);
  curator->MainThreadTick(false);
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope scope(&log);
    log.ExpectMessage("Failed to hash dependency asset", xiiLogMsgType::ErrorMsg);
    XII_TEST_BOOL(hash(1, after[1], true) == xiiAssetInfo::MissingPackageDependency);
  }
  set(1, "Package", "");
  {
    xiiOSFile file;
    if (!XII_TEST_RESULT(file.Open(paths[0], xiiFileOpenMode::Write)))
      return;
    XII_TEST_RESULT(file.Write(sourceContent.GetData(), sourceContent.GetCount()));
  }
  curator->NotifyOfFileChange(paths[0]);
  documents[0] = xiiDynamicCast<ResaveTestDocument*>(editor.OpenDocument(paths[0], xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(documents[0] != nullptr))
    return;
  XII_TEST_BOOL(documents[0]->GetGuid() == ids[0]);
  uses.Clear();
  curator->FindAllUses(ids[0], uses, true);
  XII_TEST_BOOL(uses.IsEmpty());
}
