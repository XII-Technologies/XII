/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFrameworkTest/AssetProcessorTest.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <QStringList>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <Foundation/Strings/StringConversion.h>
#endif

void TestAssetProcessor(xiiQtEditorApp& editor, xiiStringView projectDirectory)
{
  xiiStringBuilder directory(projectDirectory);
  directory.AppendPath("Processor");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(directory));
  xiiStringBuilder sourcePath(directory), consumerPath(directory);
  sourcePath.AppendPath("Source.xiiPrefab");
  consumerPath.AppendPath("Consumer.xiiSurfaceAsset");
  auto* source = xiiDynamicCast<xiiSceneDocument*>(editor.CreateDocument(sourcePath, xiiDocumentFlags::EmptyDocument));
  if (!XII_TEST_BOOL(source != nullptr))
    return;
  XII_SCOPE_EXIT(source->GetDocumentManager()->CloseDocument(source));
  auto* accessor = source->GetObjectAccessor();
  accessor->StartTransaction("Create processor dependency");
  xiiUuid owner;
  XII_TEST_STATUS(accessor->AddObjectByName(source->GetObjectManager()->GetRootObject(), "Children", -1, xiiGetStaticRTTI<xiiGameObject>(), owner));
  XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(owner), "Name", "Before"));
  accessor->FinishTransaction();
  XII_TEST_STATUS(source->SaveDocument());
  auto* consumerBase = xiiDynamicCast<xiiAssetDocument*>(editor.CreateDocument(consumerPath, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(consumerBase != nullptr))
    return;
  if (!XII_TEST_STRING(consumerBase->GetDynamicRTTI()->GetTypeName(), "xiiSurfaceAssetDocument"))
  {
    consumerBase->GetDocumentManager()->CloseDocument(consumerBase);
    return;
  }
  auto* consumer = static_cast<xiiSimpleAssetDocument<xiiSurfaceResourceDescriptor>*>(consumerBase);
  XII_SCOPE_EXIT(consumer->GetDocumentManager()->CloseDocument(consumer));
  xiiStringBuilder sourceId;
  xiiConversionUtils::ToString(source->GetGuid(), sourceId);
  accessor = consumer->GetObjectAccessor();
  accessor->StartTransaction("Set package-only prefab reference");
  XII_TEST_STATUS(accessor->SetValueByName(consumer->GetPropertyObject(), "SlideReaction", sourceId.GetView()));
  accessor->FinishTransaction();
  XII_TEST_STATUS(consumer->SaveDocument());
  const auto* info = consumer->GetAssetDocumentInfo();
  XII_TEST_BOOL(info->m_PackageDependencies.Contains(sourceId));
  XII_TEST_BOOL(!info->m_TransformDependencies.Contains(sourceId));
  XII_TEST_BOOL(!info->m_ThumbnailDependencies.Contains(sourceId));
  auto* curator = xiiAssetCurator::GetSingleton();
  XII_TEST_BOOL(curator->TransformAsset(consumer->GetGuid(), xiiTransformFlags::TriggeredManually).Succeeded());

  xiiEditorProcessCommunicationChannel processor;
  XII_SCOPE_EXIT(processor.CloseConnection());
  xiiStringBuilder project(projectDirectory), preferences(directory);
  project.AppendPath("xiiProject");
  preferences.AppendPath("Preferences");
  QStringList args;
  args << "-appname" << "xiiEditor" << "-appid" << "9000" << "-project" << project.GetData();
  args << "-outputDir" << preferences.GetData() << "-renderer" << "Vulkan" << "-norecent";
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  const char* executable = "xiiEditorProcessor.exe";
#else
  const char* executable = "xiiEditorProcessor";
#endif
  if (!XII_TEST_RESULT(processor.StartClientProcess(executable, args, false)) ||
      !XII_TEST_RESULT(processor.WaitForConnection(xiiTime::MakeFromSeconds(30))))
    return;

  xiiTransformStatus           response;
  xiiDynamicArray<xiiLogEntry> logs;
  auto                         send = [&](xiiProcessAssetMsg& request) {
    response = xiiTransformStatus("No processor response received.");
    logs.Clear();
    auto receive = [&](xiiProcessMessage* message) {
      const auto* reply = static_cast<const xiiProcessAssetResponseMsg*>(message);
      response          = reply->m_Status;
      logs              = reply->m_LogEntries;
      return true;
    };
    xiiProcessCommunicationChannel::WaitForMessageCallback callback = receive;
    if (!XII_TEST_BOOL(processor.SendMessage(&request)))
      return false;
    return XII_TEST_RESULT(processor.WaitForMessage(xiiGetStaticRTTI<xiiProcessAssetResponseMsg>(), xiiTime::MakeFromSeconds(30), &callback));
  };
  auto verifyHashes = [&]() {
    for (const auto& entry : logs)
      XII_TEST_BOOL_MSG(entry.m_sMsg.FindSubString("differs from the state in the editor") == nullptr, "%s", entry.m_sMsg.GetData());
  };
  xiiProcessAssetMsg request;
  if (!XII_TEST_RESULT(xiiAssetProcessor::PrepareAssetRequest(consumer->GetGuid(), request)))
    return;
  XII_TEST_BOOL(request.m_DepRefHull.Contains(sourceId));
  XII_TEST_STRING(request.m_sPlatform, curator->GetActiveAssetProfile()->GetConfigName());
  const auto initialAssetHash   = request.m_AssetHash;
  const auto initialPackageHash = request.m_PackageHash;
  if (!send(request))
    return;
  XII_TEST_BOOL(response.Succeeded());
  verifyHashes();
  // A failed preparation must preserve the previously captured request.
  XII_TEST_BOOL(xiiAssetProcessor::PrepareAssetRequest(xiiUuid::MakeUuid(), request).Failed());
  XII_TEST_BOOL(request.m_AssetGuid == consumer->GetGuid());
  XII_TEST_BOOL(request.m_DepRefHull.Contains(sourceId));
  XII_TEST_BOOL(request.m_AssetHash == initialAssetHash && request.m_PackageHash == initialPackageHash);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  FILETIME originalTime;
  {
    const HANDLE file = CreateFileW(xiiStringWChar(sourcePath).GetData(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!XII_TEST_BOOL(file != INVALID_HANDLE_VALUE))
      return;
    const bool read = GetFileTime(file, nullptr, nullptr, &originalTime) != FALSE;
    CloseHandle(file);
    if (!XII_TEST_BOOL(read))
      return;
  }
#endif
  accessor = source->GetObjectAccessor();
  accessor->StartTransaction("Change warmed package dependency");
  XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(owner), "Name", "After"));
  accessor->FinishTransaction();
  XII_TEST_STATUS(source->SaveDocument());
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  {
    const HANDLE file = CreateFileW(xiiStringWChar(sourcePath).GetData(), FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!XII_TEST_BOOL(file != INVALID_HANDLE_VALUE))
      return;
    FILETIME   actualTime;
    const bool restored = SetFileTime(file, nullptr, nullptr, &originalTime) != FALSE &&
      GetFileTime(file, nullptr, nullptr, &actualTime) != FALSE &&
      CompareFileTime(&originalTime, &actualTime) == 0;
    CloseHandle(file);
    if (!XII_TEST_BOOL(restored))
      return;
  }
#endif
  curator->NotifyOfFileChange(sourcePath);
  if (!XII_TEST_RESULT(xiiAssetProcessor::PrepareAssetRequest(consumer->GetGuid(), request)))
    return;
  XII_TEST_BOOL(request.m_AssetHash == initialAssetHash);
  XII_TEST_BOOL(request.m_PackageHash != initialPackageHash);
  if (!send(request))
    return;
  XII_TEST_BOOL(response.Succeeded());
  verifyHashes();

  // Protocol errors must report failure, and must not poison a later valid job.
  const xiiString profile = request.m_sPlatform;
  request.m_sPlatform     = "MissingProcessorTestProfile";
  if (!send(request))
    return;
  XII_TEST_BOOL(response.Failed());
  XII_TEST_BOOL(response.m_sMessage.FindSubString("MissingProcessorTestProfile") != nullptr);
  request.m_sPlatform = profile;
  request.m_AssetGuid = xiiUuid::MakeUuid();
  if (!send(request))
    return;
  XII_TEST_BOOL(response.Failed());

  accessor = consumer->GetObjectAccessor();
  accessor->StartTransaction("Change compiled surface configuration");
  XII_TEST_STATUS(accessor->SetValueByName(consumer->GetPropertyObject(), "Restitution", 0.75f));
  accessor->FinishTransaction();
  XII_TEST_STATUS(consumer->SaveDocument());
  const auto output   = consumer->GetAssetDocumentManager()->GetAbsoluteOutputFileName(consumer->GetAssetDocumentTypeDescriptor(), consumerPath, "");
  const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(2);
  xiiResult  removed  = XII_FAILURE;
  do
  {
    removed = xiiOSFile::DeleteFile(output);
    if (removed.Failed())
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
  } while (removed.Failed() && xiiTime::Now() < deadline);
  if (!XII_TEST_RESULT(removed))
    return;
  curator->NotifyOfFileChange(output);
  if (!XII_TEST_RESULT(xiiAssetProcessor::PrepareAssetRequest(consumer->GetGuid(), request)) || !send(request))
    return;
  XII_TEST_BOOL(response.Succeeded());
  verifyHashes();
  xiiFileReader reader;
  if (!XII_TEST_RESULT(reader.Open(output)))
    return;
  xiiAssetFileHeader header;
  if (!XII_TEST_RESULT(header.Read(reader)))
    return;
  XII_TEST_BOOL(header.GetFileHash() == request.m_AssetHash);
  xiiSurfaceResourceDescriptor surface;
  surface.Load(reader);
  XII_TEST_FLOAT(surface.m_fPhysicsRestitution, 0.75f, 0.0001f);
  XII_TEST_STRING(surface.GetSlideReactionPrefabFile(), sourceId);
}
