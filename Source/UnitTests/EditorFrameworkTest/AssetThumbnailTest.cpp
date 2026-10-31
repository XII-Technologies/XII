/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <QApplication>
#include <QImage>
#include <ToolsFoundation/Document/DocumentManager.h>

XII_CREATE_SIMPLE_TEST_GROUP(AssetThumbnails);

XII_CREATE_SIMPLE_TEST(AssetThumbnails, CacheVersion)
{
  xiiAssetDocument::ThumbnailInfo current;
  current.SetFileHashAndVersion(1234, 7);
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  XII_TEST_RESULT(current.Serialize(writer));
  XII_TEST_INT(storage.GetStorageSize64(), current.GetSerializedSize());
  xiiMemoryStreamReader           reader(&storage);
  xiiAssetDocument::ThumbnailInfo restored;
  XII_TEST_RESULT(restored.Deserialize(reader));
  XII_TEST_BOOL(restored.IsThumbnailUpToDate(1234, 7));
  XII_TEST_BOOL(!restored.IsThumbnailUpToDate(1235, 7));
  XII_TEST_BOOL(!restored.IsThumbnailUpToDate(1234, 8));

  // Earlier thumbnails wrote zero into the existing reserved field.
  writer.SetWritePosition(current.GetSerializedSize() - sizeof(xiiUInt16));
  writer << xiiUInt16(0);
  reader.SetReadPosition(0);
  XII_TEST_RESULT(restored.Deserialize(reader));
  XII_TEST_BOOL(!restored.IsThumbnailUpToDate(1234, 7));
  restored.SetFileHashAndVersion(1234, 7);
  XII_TEST_BOOL(restored.IsThumbnailUpToDate(1234, 7));
}

XII_CREATE_SIMPLE_TEST(AssetThumbnails, EngineProcessCapture)
{
  auto&           editor       = *xiiQtEditorApp::GetSingleton();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  const xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/ThumbnailPreferences");
  editor.StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(editor.CloseProject(); QApplication::processEvents(QEventLoop::AllEvents, 10); editor.ShutdownEditor());
  QApplication::processEvents(QEventLoop::AllEvents, 10);
  const xiiStringBuilder project(preferences, "/Project/xiiProject");
  if (!XII_TEST_RESULT(editor.CreateOrOpenProject(true, project)))
    return;
  xiiAssetProcessor::GetSingleton()->StopProcessTask(false, true);
  xiiVisualShaderTypeRegistry::GetSingleton()->UpdateNodeData();
  auto*      curator = xiiAssetCurator::GetSingleton();
  const auto check   = [](const xiiStatus& status) {
    XII_TEST_STATUS(status);
    return status.Succeeded();
  };

  // The Editor normally leaves runtime redirection to its engine process.
  const auto previousTable                           = xiiDataDirectory::FolderType::s_sRedirectionFile;
  const auto previousPrefix                          = xiiDataDirectory::FolderType::s_sRedirectionPrefix;
  xiiDataDirectory::FolderType::s_sRedirectionFile   = "AssetCache/Default.xiiAidlt";
  xiiDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/";
  XII_SCOPE_EXIT(xiiDataDirectory::FolderType::s_sRedirectionFile = previousTable; xiiDataDirectory::FolderType::s_sRedirectionPrefix = previousPrefix; xiiFileSystem::ReloadAllExternalDataDirectoryConfigs());

  // Keep authored sources and their identities intact while exercising fresh runtime resources.
  for (xiiStringView reference : {"Standard.xiiMaterialAsset", "Base/Materials/Common/Pattern.xiiMaterialAsset", "Base/Meshes/Box.xiiMeshAsset", "Base/Prefabs/Sphere.xiiPrefab"})
  {
    xiiStringBuilder path(project.GetFileDirectory());
    path.AppendPath(reference.GetFileNameAndExtension());
    xiiAssetDocument* document = nullptr;
    if (reference == "Standard.xiiMaterialAsset")
    {
      document = xiiDynamicCast<xiiAssetDocument*>(editor.CreateDocument(path, xiiDocumentFlags::None));
    }
    else
    {
      xiiUuid sourceId;
      if (!check(curator->ResolveAsset(reference, sourceId)))
        return;
      xiiString source;
      {
        const auto asset = curator->GetSubAsset(sourceId);
        if (!XII_TEST_BOOL(static_cast<bool>(asset)))
          return;
        source = asset->m_pAssetInfo->m_Path.GetAbsolutePath();
      }
      const xiiDocumentTypeDescriptor* type = nullptr;
      if (!XII_TEST_RESULT(xiiDocumentManager::FindDocumentTypeFromPath(source, false, type)))
        return;
      xiiUuid cloneId;
      if (!check(type->m_pManager->CloneDocument(source, path, cloneId)))
        return;
      document = xiiDynamicCast<xiiAssetDocument*>(editor.OpenDocument(path, xiiDocumentFlags::None));
    }
    if (!XII_TEST_BOOL(document != nullptr))
      return;
    XII_SCOPE_EXIT(document->GetDocumentManager()->CloseDocument(document));
    const auto transformed = curator->TransformAsset(document->GetGuid(), xiiTransformFlags::TriggeredManually);
    if (!XII_TEST_BOOL_MSG(transformed.Succeeded(), "%s", transformed.m_sMessage.GetData()))
      return;
    // No Qt event-loop tick may be required to publish a newly transformed identity.
    if (!XII_TEST_RESULT(curator->WriteAssetTables()))
      return;
    xiiFileSystem::ReloadAllExternalDataDirectoryConfigs();
    xiiStringBuilder canonical, expected;
    xiiConversionUtils::ToString(document->GetGuid(), expected);
    if (!check(xiiFileSystem::ResolveAssetIdentifier(path, canonical)))
      return;
    XII_TEST_STRING(canonical, expected);
    const auto thumbnail = document->CreateThumbnail();
    if (!XII_TEST_BOOL_MSG(thumbnail.Succeeded(), "%s", thumbnail.m_sMessage.GetData()))
      return;
    const xiiString thumbnailPath = document->GetAssetDocumentManager()->GenerateResourceThumbnailPath(path);
    const QImage    image(QString::fromUtf8(thumbnailPath.GetData()));
    if (!XII_TEST_BOOL(!image.isNull()))
      return;
    xiiUInt32 visiblePixels = 0;
    for (int y = 0; y < image.height(); ++y)
      for (int x = 0; x < image.width(); ++x)
      {
        const auto pixel = image.pixel(x, y);
        if (qMax(qRed(pixel), qMax(qGreen(pixel), qBlue(pixel))) > 16)
          ++visiblePixels;
      }
    XII_TEST_BOOL_MSG(visiblePixels > static_cast<xiiUInt32>(image.width() * image.height() / 100), "Thumbnail '%s' has only %u visible pixels.", thumbnailPath.GetData(), visiblePixels);
  }
}
