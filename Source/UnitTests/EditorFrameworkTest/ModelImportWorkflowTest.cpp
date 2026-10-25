/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Project/ToolsProject.h>

XII_CREATE_SIMPLE_TEST_GROUP(ModelImportWorkflow);

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
