/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFrameworkTest/AssetProcessorTest.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <Foundation/Strings/StringConversion.h>
#endif

void TestAssetProcessorProfileReload(xiiQtEditorApp& editor, xiiStringView projectDirectory, xiiEditorProcessCommunicationChannel& processor)
{
  auto*               curator            = xiiAssetCurator::GetSingleton();
  const auto          originalIndex      = curator->GetActiveAssetProfileIndex();
  auto*               original           = curator->GetAssetProfile(originalIndex);
  auto*               texture            = original->GetTypeConfig<xiiTextureAssetProfileConfig>();
  const auto          originalResolution = texture->m_uiRenderTargetResolution1;
  xiiPlatformProfile* added              = nullptr;
  XII_SCOPE_EXIT(texture->m_uiRenderTargetResolution1 = originalResolution;
                 curator->SetActiveAssetProfileByIndex(originalIndex, true);
                 if (added != nullptr) XII_TEST_RESULT(curator->DeleteAssetProfile(added));
                 curator->SaveRuntimeProfiles(); XII_TEST_RESULT(curator->SaveAssetProfiles()));

  xiiStringBuilder path(projectDirectory), profilePath(projectDirectory);
  path.AppendPath("Processor/ProfileTarget.xiiRenderTargetAsset");
  profilePath.AppendPath("Editor/AssetProfiles.ddl");
  auto* asset = xiiDynamicCast<xiiAssetDocument*>(editor.CreateDocument(path, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(asset != nullptr))
    return;
  XII_SCOPE_EXIT(asset->GetDocumentManager()->CloseDocument(asset));
  auto* accessor = asset->GetObjectAccessor();
  accessor->StartTransaction("Create processor profile-dependent target");
  XII_TEST_STATUS(accessor->SetValueByName(asset->GetObjectManager()->GetRootObject()->GetChildren()[0], "Resolution", static_cast<xiiInt64>(xiiTexture2DResolution::ProfileResolution1)));
  accessor->FinishTransaction();
  XII_TEST_STATUS(asset->SaveDocument());

  xiiTransformStatus           response;
  xiiDynamicArray<xiiLogEntry> logs;
  xiiProcessAssetMsg           request;
  auto                         send = [&]() {
    return SendAssetProcessorRequest(processor, request, response, logs);
  };
  auto transform = [&](xiiUInt32 resolution) {
    if (!XII_TEST_RESULT(xiiAssetProcessor::PrepareAssetRequest(asset->GetGuid(), request)))
      return false;
    const auto output = asset->GetAssetDocumentManager()->GetAbsoluteOutputFileName(asset->GetAssetDocumentTypeDescriptor(), path, "");
    if (!XII_TEST_RESULT(xiiOSFile::DeleteFile(output)) || !send() || !XII_TEST_BOOL_MSG(response.Succeeded(), "%s", response.m_sMessage.GetData()))
      return false;
    for (const auto& entry : logs)
      XII_TEST_BOOL_MSG(entry.m_sMsg.FindSubString("differs from the state in the editor") == nullptr, "%s", entry.m_sMsg.GetData());
    xiiFileReader reader;
    if (!XII_TEST_RESULT(reader.Open(output)))
      return false;
    xiiAssetFileHeader header;
    if (!XII_TEST_RESULT(header.Read(reader)))
      return false;
    xiiGALTextureCreationDescription description;
    reader >> description;
    XII_TEST_BOOL(header.GetFileHash() == request.m_AssetHash);
    XII_TEST_INT(description.m_Size.width, resolution);
    XII_TEST_INT(description.m_Size.height, resolution);
    return true;
  };

  // The same processor has already handled jobs before this definition is edited and saved.
  texture->m_uiRenderTargetResolution1 = 640;
  curator->SaveRuntimeProfiles();
  if (!XII_TEST_RESULT(curator->SaveAssetProfiles()))
    return;
  curator->SetActiveAssetProfileByIndex(originalIndex, true);
  if (!transform(640))
    return;
  const auto firstHash = request.m_AssetHash;

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  FILETIME profileTime;
  {
    const HANDLE file = CreateFileW(xiiStringWChar(profilePath).GetData(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!XII_TEST_BOOL(file != INVALID_HANDLE_VALUE))
      return;
    const bool read = GetFileTime(file, nullptr, nullptr, &profileTime) != FALSE;
    CloseHandle(file);
    if (!XII_TEST_BOOL(read))
      return;
  }
#endif
  texture->m_uiRenderTargetResolution1 = 896;
  curator->SaveRuntimeProfiles();
  if (!XII_TEST_RESULT(curator->SaveAssetProfiles()))
    return;
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  {
    const HANDLE file = CreateFileW(xiiStringWChar(profilePath).GetData(), FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!XII_TEST_BOOL(file != INVALID_HANDLE_VALUE))
      return;
    FILETIME   actual;
    const bool restored = SetFileTime(file, nullptr, nullptr, &profileTime) != FALSE &&
      GetFileTime(file, nullptr, nullptr, &actual) != FALSE && CompareFileTime(&profileTime, &actual) == 0;
    CloseHandle(file);
    if (!XII_TEST_BOOL(restored))
      return;
  }
#endif
  curator->SetActiveAssetProfileByIndex(originalIndex, true);
  if (!transform(896))
    return;
  XII_TEST_BOOL(request.m_AssetHash != firstHash);

  // A new definition must become visible without restarting the processor.
  added = curator->CreateAssetProfile();
  added->SetConfigName("ProcessorReloadProfile");
  added->AddMissingConfigs();
  added->GetTypeConfig<xiiTextureAssetProfileConfig>()->m_uiRenderTargetResolution1 = 768;
  curator->SaveRuntimeProfiles();
  if (!XII_TEST_RESULT(curator->SaveAssetProfiles()))
    return;
  curator->SetActiveAssetProfileByIndex(curator->GetNumAssetProfiles() - 1, true);
  if (!transform(768))
    return;
  curator->SetActiveAssetProfileByIndex(originalIndex, true);
  if (!XII_TEST_RESULT(curator->DeleteAssetProfile(added)))
    return;
  added = nullptr;
  if (!XII_TEST_RESULT(curator->SaveAssetProfiles()) || !transform(896))
    return;

  // Reject incomplete, wrongly typed and ambiguous definitions without destroying live profiles.
  // These documents are valid DDL; failure is a profile-validation result, not a parser assertion.
  for (xiiUInt32 invalid = 0; invalid < 8; ++invalid)
  {
    xiiDeferredFileWriter file;
    file.SetOutput(profilePath);
    xiiOpenDdlWriter ddl;
    ddl.SetOutputStream(&file);
    ddl.BeginObject("AssetProfiles");
    if (invalid != 0)
    {
      ddl.BeginObject("Config");
      if (invalid == 2)
      {
        xiiSurfaceResourceDescriptor wrongType;
        xiiReflectionSerializer::WriteObjectToDDL(ddl, xiiGetStaticRTTI<xiiSurfaceResourceDescriptor>(), &wrongType);
      }
      else if (invalid == 3 || invalid == 4)
      {
        xiiPlatformProfile emptyName;
        const auto*        profile = invalid == 3 ? &emptyName : original;
        xiiReflectionSerializer::WriteObjectToDDL(ddl, xiiGetStaticRTTI<xiiPlatformProfile>(), profile);
      }
      else if (invalid >= 5)
      {
        xiiAbstractObjectGraph graph;
        auto*                  root = graph.AddNode(xiiUuid::MakeUuid(), "xiiPlatformProfile", 1, "root");
        root->AddProperty("Name", "InvalidOwnedConfig");
        const auto      id = xiiUuid::MakeUuid();
        xiiVariantArray configs;
        configs.PushBack(id);
        if (invalid == 5)
          graph.AddNode(id, "xiiSurfaceResourceDescriptor", 1);
        else if (invalid == 7)
        {
          graph.AddNode(id, "xiiTextureAssetProfileConfig", 1);
          configs.PushBack(id);
        }
        // Case 6 deliberately references an absent object.
        root->AddProperty("Configs", configs);
        xiiAbstractGraphDdlSerializer::Write(ddl, &graph);
      }
      ddl.EndObject();
      if (invalid == 4)
      {
        ddl.BeginObject("Config");
        xiiReflectionSerializer::WriteObjectToDDL(ddl, xiiGetStaticRTTI<xiiPlatformProfile>(), original);
        ddl.EndObject();
      }
    }
    ddl.EndObject();
    if (!XII_TEST_RESULT(file.Close()))
      return;
    const auto count = curator->GetNumAssetProfiles();
    XII_TEST_BOOL(curator->ReloadAssetProfiles().Failed());
    XII_TEST_INT(curator->GetNumAssetProfiles(), count);
    XII_TEST_BOOL(curator->GetActiveAssetProfile() == original);
    if (!send())
      return;
    XII_TEST_BOOL(response.Failed());
    XII_TEST_BOOL(response.m_sMessage.FindSubString("reload asset platform configurations") != nullptr);
  }
  if (!XII_TEST_RESULT(curator->SaveAssetProfiles()) || !transform(896))
    return;
}
