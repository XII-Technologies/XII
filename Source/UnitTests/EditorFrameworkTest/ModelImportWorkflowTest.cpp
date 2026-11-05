/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <QApplication>
#include <QStringList>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Project/ToolsProject.h>

XII_CREATE_SIMPLE_TEST_GROUP(ModelImportWorkflow);

XII_CREATE_SIMPLE_TEST(ModelImportWorkflow, StaticModelDependencies)
{
  auto&           editor       = *xiiQtEditorApp::GetSingleton();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  const xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/StaticModelImport");
  const xiiStringBuilder project(preferences, "/Project/xiiProject");
  const auto             created = xiiToolsProject::CreateProject(project);
  XII_TEST_STATUS(created);
  if (created.Failed())
    return;
  xiiToolsProject::CloseProject();
  editor.StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(editor.ShutdownEditor());
  if (!XII_TEST_RESULT(editor.CreateOrOpenProject(false, project)))
    return;
  xiiStringBuilder source = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-importModel");
  if (source.IsEmpty())
  {
    auto write = [](xiiStringView path, xiiStringView contents) {
      xiiOSFile file;
      XII_TEST_RESULT(file.Open(path, xiiFileOpenMode::Write));
      XII_TEST_RESULT(file.Write(contents.GetStartPointer(), contents.GetElementCount()));
    };
    source.Set(project.GetFileDirectory(), "/Triangle.obj");
    write(source, "mtllib Triangle.mtl\no Triangle\nv 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 0 1\nusemtl Painted\nf 1/1 2/2 3/3\n");
    write(xiiStringBuilder(project.GetFileDirectory(), "/Triangle.mtl"), "newmtl Painted\nKd 1 1 1\nmap_Kd Painted.tga\n");
    const xiiUInt8 image[] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 24, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
    xiiOSFile      file;
    XII_TEST_RESULT(file.Open(xiiStringBuilder(project.GetFileDirectory(), "/Painted.tga"), xiiFileOpenMode::Write));
    XII_TEST_RESULT(file.Write(image, sizeof(image)));
  }
  const xiiStringBuilder path(project.GetFileDirectory(), "/Imported.xiiMeshAsset");
  auto*                  document = xiiDynamicCast<xiiAssetDocument*>(editor.CreateDocument(path, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(document != nullptr))
    return;
  XII_SCOPE_EXIT(document->GetDocumentManager()->CloseDocument(document));
  auto* accessor = document->GetObjectAccessor();
  accessor->StartTransaction("Import static model");
  XII_TEST_STATUS(accessor->SetValueByName(document->GetObjectManager()->GetRootObject()->GetChildren()[0], "MeshFile", source.GetView()));
  accessor->FinishTransaction();
  XII_TEST_STATUS(document->SaveDocument());
  auto* curator = xiiAssetCurator::GetSingleton();
  // Receive an import without pumping the editor's directory watcher. Sidecar
  // discovery must be explicit rather than an incidental effect of GUI timing.
  {
    xiiEditorProcessCommunicationChannel worker;
    XII_SCOPE_EXIT(worker.CloseConnection());
    QStringList arguments;
    arguments << "-appname" << "xiiEditor" << "-appid" << "9001" << "-project" << project.GetData();
    arguments << "-outputDir" << preferences.GetData() << "-renderer" << "Vulkan" << "-norecent";
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    const char* executable = "xiiEditorProcessor.exe";
#else
    const char* executable = "xiiEditorProcessor";
#endif
    if (!XII_TEST_RESULT(worker.StartClientProcess(executable, arguments, false)) || !XII_TEST_RESULT(worker.WaitForConnection(xiiTime::MakeFromSeconds(30))))
      return;
    xiiProcessAssetMsg request;
    if (!XII_TEST_RESULT(xiiAssetProcessor::PrepareAssetRequest(document->GetGuid(), request)))
      return;
    request.m_bImport = true;
    xiiDynamicArray<xiiString> files;
    auto                       receive = [&](xiiProcessMessage* message) {
      const auto* response = static_cast<const xiiProcessAssetResponseMsg*>(message);
      XII_TEST_BOOL_MSG(response->m_Status.Succeeded(), "%s", response->m_Status.m_sMessage.GetData());
      files = response->m_ImportFiles;
      for (const auto& entry : response->m_LogEntries)
        XII_TEST_BOOL_MSG(entry.m_Type != xiiLogMsgType::ErrorMsg, "%s", entry.m_sMsg.GetData());
      return true;
    };
    xiiProcessCommunicationChannel::WaitForMessageCallback callback = receive;
    if (!XII_TEST_BOOL(worker.SendMessage(&request)) || !XII_TEST_RESULT(worker.WaitForMessage(xiiGetStaticRTTI<xiiProcessAssetResponseMsg>(), xiiTime::MakeFromSeconds(120), &callback)))
      return;
    bool material = false, texture = false;
    for (const auto& file : files)
    {
      material |= file.HasExtension("xiiMaterialAsset");
      texture |= file.HasExtension("xiiTextureAsset");
      curator->NotifyOfFileChange(file);
    }
    XII_TEST_BOOL(material && texture);
    for (const auto& file : files)
      if (file.HasExtension("xiiMaterialAsset") || file.HasExtension("xiiTextureAsset"))
      {
        const auto asset = curator->FindSubAsset(file);
        if (XII_TEST_BOOL(asset.isValid()))
          XII_TEST_BOOL(curator->GetAssetDependencyHash(asset->m_Data.m_Guid) != 0);
      }
  }
  auto*      processor = xiiAssetProcessor::GetSingleton();
  const auto guid      = document->GetGuid();
  XII_TEST_STATUS(processor->QueueTransformAssets(xiiMakeArrayPtr(&guid, 1), xiiTransformFlags::TriggeredManually | xiiTransformFlags::ForceTransform));
  const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(120);
  while ((processor->GetTransformProgress().m_bActive || processor->GetProcessTaskState() != xiiAssetProcessor::ProcessTaskState::Stopped) && xiiTime::Now() < deadline)
  {
    QApplication::processEvents();
    xiiAssetCurator::GetSingleton()->MainThreadTick(true);
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
  }
  XII_TEST_BOOL(!processor->GetTransformProgress().m_bActive);
  XII_TEST_INT(processor->GetTransformProgress().m_uiFailed, 0);
  XII_TEST_INT(processor->GetTransformProgress().m_uiNeedsImport, 0);
  XII_TEST_BOOL(xiiAssetCurator::GetSingleton()->GetAssetDependencyHash(document->GetGuid()) != 0);
}

XII_CREATE_SIMPLE_TEST(ModelImportWorkflow, CompleteModel)
{
  const auto source = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-importModel");
  if (source.IsEmpty())
  {
    xiiTestFramework::Output(xiiTestOutput::Message, "Supply -importModel <absolute skinned GLB/FBX> to exercise the complete production import workflow.");
    return;
  }
  auto check = [](const auto& status) {
    XII_TEST_STATUS(status);
    return status.Succeeded();
  };
  auto&           editor       = *xiiQtEditorApp::GetSingleton();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  const xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/ModelImport");
  const xiiStringBuilder project(preferences, "/Project/xiiProject");
  if (!check(xiiToolsProject::CreateProject(project)))
    return;
  xiiToolsProject::CloseProject();
  editor.StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(editor.ShutdownEditor());
  if (!XII_TEST_RESULT(editor.CreateOrOpenProject(false, project)))
    return;
  const xiiStringBuilder localSource(project.GetFileDirectory(), source.GetFileNameAndExtension());
  if (!XII_TEST_RESULT(xiiOSFile::CopyFile(source, localSource)))
    return;
  xiiHybridArray<xiiAssetDocumentGenerator*, 16> generators;
  xiiAssetDocumentGenerator::CreateGenerators(generators);
  XII_SCOPE_EXIT(xiiAssetDocumentGenerator::DestroyGenerators(generators));
  xiiAssetDocumentGenerator* importer = nullptr;
  for (auto* generator : generators)
    if (generator->GetDocumentExtension() == "xiiAnimatedMeshAsset")
      importer = generator;
  if (!XII_TEST_BOOL(importer != nullptr))
    return;
  xiiDynamicArray<xiiDocument*> documents;
  const auto                    generated = importer->Generate(localSource, "AnimatedMeshImport.Complete", documents);
  if (!check(generated))
    return;
  XII_SCOPE_EXIT(for (auto* document : documents) document->GetDocumentManager()->CloseDocument(document));
  XII_TEST_BOOL(documents.GetCount() >= 3);
  for (auto* document : documents)
  {
    auto* asset = xiiDynamicCast<xiiAssetDocument*>(document);
    if (!XII_TEST_BOOL(asset != nullptr) || !check(asset->SaveDocument()))
      return;
    // Run the foreground transform, including generated materials and embedded
    // images, through the same exporter as an interactive Editor import.
    const auto transformed = asset->TransformAsset(xiiTransformFlags::TriggeredManually | xiiTransformFlags::ForceTransform);
    if (!XII_TEST_BOOL_MSG(transformed.Succeeded(), "%s", transformed.m_sMessage.GetData()))
      return;
  }
  xiiStringBuilder meshPath(localSource);
  meshPath.ChangeFileExtension("xiiAnimatedMeshAsset");
  xiiDocument* mesh = nullptr;
  for (auto* document : documents)
    if (document->GetDocumentPath() == meshPath)
      mesh = document;
  if (!XII_TEST_BOOL(mesh != nullptr))
    return;
  const auto dependencies = xiiAssetCurator::GetSingleton()->TransformAsset(mesh->GetGuid(), xiiTransformFlags::TriggeredManually | xiiTransformFlags::ForceTransform);
  XII_TEST_BOOL_MSG(dependencies.Succeeded(), "%s", dependencies.m_sMessage.GetData());
  XII_TEST_RESULT(xiiAssetCurator::GetSingleton()->WriteAssetTables());
  const auto* properties = mesh->GetObjectManager()->GetRootObject()->GetChildren()[0];
  xiiVariant  skeletonId;
  XII_TEST_STATUS(mesh->GetObjectAccessor()->GetValueByName(properties, "DefaultSkeleton", skeletonId));
  XII_TEST_BOOL(skeletonId.IsValid() && !skeletonId.ConvertTo<xiiString>().IsEmpty());
  xiiInt32 materialCount = 0;
  XII_TEST_STATUS(mesh->GetObjectAccessor()->GetCountByName(properties, "Materials", materialCount));
  XII_TEST_BOOL(materialCount > 0);
  // Import again before closing documents or waiting for a filesystem scan.
  // It must reuse identities and authored settings rather than create duplicates.
  xiiDynamicArray<xiiDocument*> repeated;
  XII_TEST_STATUS(importer->Generate(localSource, "AnimatedMeshImport.Complete", repeated));
  XII_TEST_BOOL(repeated.IsEmpty());
}
