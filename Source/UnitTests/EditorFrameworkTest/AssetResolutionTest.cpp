/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFrameworkTest/AssetTestDocument.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <QApplication>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class ResolutionTestDocument : public xiiSimpleAssetDocument<ResaveTestProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(ResolutionTestDocument, xiiAssetDocument);

public:
  ResolutionTestDocument(xiiStringView path) : xiiSimpleAssetDocument(path, xiiAssetDocEngineConnection::None) {}
  static xiiDynamicArray<xiiUuid> s_TransformOrder;

protected:
  xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, xiiStringView, const xiiPlatformProfile* profile, const xiiAssetFileHeader&, xiiBitflags<xiiTransformFlags>) override
  {
    if (const auto dependency = xiiAssetCurator::GetSingleton()->FindSubAsset(GetProperties()->m_sTransform))
    {
      auto*         info   = dependency->m_pAssetInfo;
      const auto    output = info->GetManager()->GetAbsoluteOutputFileName(info->m_pDocumentTypeDescriptor, info->m_Path, "", profile);
      xiiFileReader file;
      if (file.Open(output).Failed())
        return xiiTransformStatus("Dependency output must exist before the consumer transforms.");
      xiiAssetFileHeader header;
      XII_SUCCEED_OR_RETURN(header.Read(file));
    }
    XII_TEST_BOOL(xiiAssetCurator::GetSingleton()->IsAssetBeingTransformed(GetGuid()));
    XII_TEST_BOOL(TransformAsset(xiiTransformFlags::ForceTransform, profile).Failed());
    s_TransformOrder.PushBack(GetGuid());
    return stream.WriteString(GetProperties()->m_sValue);
  }
};

class ResolutionTestManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(ResolutionTestManager, xiiAssetDocumentManager);

public:
  ResolutionTestManager()
  {
    m_Type.m_sDocumentTypeName      = "ResolutionTest";
    m_Type.m_sFileExtension         = "xiiResolutionTest";
    m_Type.m_sResourceFileExtension = "xiiBinResolutionTest";
    m_Type.m_sAssetCategory         = "Testing";
    m_Type.m_pDocumentType          = xiiGetStaticRTTI<ResolutionTestDocument>();
    m_Type.m_pManager               = this;
  }
  ~ResolutionTestManager() override { CloseAllDocumentsOfManager(); }
  bool GeneratesProfileSpecificAssets() const override { return false; }
  void FillOutSubAssetList(const xiiAssetDocumentInfo& info, xiiDynamicArray<xiiSubAssetData>& out_assets) const override
  {
    auto& sub   = out_assets.ExpandAndGetRef();
    sub.m_Guid  = ResaveTestManager::GetSubAssetId(info.m_DocumentID);
    sub.m_sName = "Child";
    sub.m_sSubAssetsDocumentTypeName.Assign("ResolutionTest");
  }

private:
  xiiStatus InternalCreateDocument(xiiStringView, xiiStringView path, bool, xiiDocument*& out_document, const xiiDocumentObject*) override
  {
    out_document = new ResolutionTestDocument(path);
    return XII_SUCCESS;
  }
  void                           InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& types) const override { types.PushBack(&m_Type); }
  xiiAssetDocumentTypeDescriptor m_Type;
};

xiiDynamicArray<xiiUuid> ResolutionTestDocument::s_TransformOrder;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(ResolutionTestDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(ResolutionTestManager, 1, xiiRTTIDefaultAllocator<ResolutionTestManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST_GROUP(AssetResolution);

XII_CREATE_SIMPLE_TEST(AssetResolution, EditorAndRuntime)
{
  auto&           editor       = *xiiQtEditorApp::GetSingleton();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  const xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/ResolutionPreferences");
  const xiiStringBuilder project(preferences, "/Project/xiiProject");
  XII_TEST_STATUS(xiiToolsProject::CreateProject(project));
  xiiToolsProject::CloseProject();
  editor.StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(editor.CloseProject(); QApplication::processEvents(QEventLoop::AllEvents, 10); editor.ShutdownEditor());
  QApplication::processEvents(QEventLoop::AllEvents, 10);
  if (!XII_TEST_RESULT(editor.CreateOrOpenProject(false, project)))
    return;
  xiiAssetProcessor::GetSingleton()->StopProcessTask(false, true);
  auto*                   curator      = xiiAssetCurator::GetSingleton();
  ResolutionTestDocument* documents[3] = {};
  xiiUuid                 ids[3];
  xiiString               paths[3], references[3];
  XII_SCOPE_EXIT(for (const auto id : ids) if (auto* document = xiiDocumentManager::GetDocumentByGuid(id)) document->GetDocumentManager()->CloseDocument(document));
  for (xiiUInt32 i = 0; i < 3; ++i)
  {
    xiiStringBuilder path(project.GetFileDirectory(), "/References/", i == 0 ? "Source.xiiResolutionTest" : i == 1 ? "Consumer.xiiResolutionTest" :
                                                                                                                     "Indirect.xiiResolutionTest");
    documents[i] = xiiDynamicCast<ResolutionTestDocument*>(editor.CreateDocument(path, xiiDocumentFlags::None));
    if (!XII_TEST_BOOL(documents[i] != nullptr))
      return;
    ids[i]   = documents[i]->GetGuid();
    paths[i] = path;
    xiiStringBuilder id;
    xiiConversionUtils::ToString(ids[i], id);
    references[i] = id;
    XII_TEST_STATUS(documents[i]->SaveDocument());
    curator->NotifyOfFileChange(path);
  }
  auto set = [&](xiiUInt32 index, const char* property, xiiStringView value) {
    auto* accessor = documents[index]->GetObjectAccessor();
    accessor->StartTransaction("Edit resolution test");
    XII_TEST_STATUS(accessor->SetValueByName(documents[index]->GetPropertyObject(), property, value));
    accessor->FinishTransaction();
    XII_TEST_STATUS(documents[index]->SaveDocument());
    curator->NotifyOfFileChange(paths[index]);
  };
  auto hash = [&](xiiUInt32 index, bool force = false) {
    xiiUInt64 asset = 0, thumbnail = 0, package = 0;
    curator->IsAssetUpToDate(ids[index], nullptr, nullptr, asset, thumbnail, package, force);
    return asset;
  };
  auto transform = [&](xiiStringView reference) {
    const auto status = curator->TransformAsset(reference, xiiTransformFlags::TriggeredManually);
    return XII_TEST_BOOL_MSG(status.Succeeded(), "%s", status.m_sMessage.GetData());
  };
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Canonical document and sub-asset identity")
  {
    const xiiStringBuilder parent("Project", "/References/Source.xiiResolutionTest");
    const xiiString        forms[] = {references[0], paths[0], "References/Source.xiiResolutionTest", "References/./Folder/../Source.xiiResolutionTest", ":project/References/Source.xiiResolutionTest", parent};
    for (const auto& form : forms)
    {
      xiiUuid id;
      XII_TEST_STATUS(curator->ResolveAsset(form, id));
      XII_TEST_BOOL(id == ids[0]);
    }
    xiiUuid id;
    XII_TEST_STATUS(curator->ResolveAsset("References/Source.xiiResolutionTest|Child", id));
    XII_TEST_BOOL(id == ResaveTestManager::GetSubAssetId(ids[0]));
    XII_TEST_BOOL(curator->ResolveAsset("References/Source.xiiResolutionTest|Missing", id).Failed());
    XII_TEST_BOOL(!id.IsValid());
    XII_TEST_BOOL(curator->ResolveAsset("Missing.xiiResolutionTest", id).Failed());
    XII_TEST_BOOL(!id.IsValid());
  }
  set(1, "Transform", "References/Source.xiiResolutionTest");
  set(2, "Transform", references[1]);
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Dependency-first transforms and transitive path invalidation")
  {
    ResolutionTestDocument::s_TransformOrder.Clear();
    if (!transform(":project/References/Indirect.xiiResolutionTest"))
      return;
    XII_TEST_INT(ResolutionTestDocument::s_TransformOrder.GetCount(), 3);
    for (xiiUInt32 i = 0; i < 3 && i < ResolutionTestDocument::s_TransformOrder.GetCount(); ++i)
      XII_TEST_BOOL(ResolutionTestDocument::s_TransformOrder[i] == ids[i]);
    const auto before = hash(2);
    set(0, "Value", "Changed transitive source");
    XII_TEST_BOOL(hash(2) != before);
    ResolutionTestDocument::s_TransformOrder.Clear();
    XII_TEST_BOOL(transform(references[2]));
    XII_TEST_INT(ResolutionTestDocument::s_TransformOrder.GetCount(), 3);
    ResolutionTestDocument::s_TransformOrder.Clear();
    XII_TEST_BOOL(transform(paths[2]));
    XII_TEST_BOOL(ResolutionTestDocument::s_TransformOrder.IsEmpty());
    set(0, "Transform", references[2]);
    XII_TEST_BOOL(hash(0) == 0);
    XII_TEST_BOOL(curator->TransformAsset(paths[0], xiiTransformFlags::TriggeredManually).Failed());
    XII_TEST_BOOL(ResolutionTestDocument::s_TransformOrder.IsEmpty());
    set(0, "Transform", "");
    XII_TEST_BOOL(transform(paths[2]));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Missing path recovery and compiler input edits")
  {
    xiiStringBuilder input(project.GetFileDirectory(), "/References/CompilerInput.txt");
    XII_TEST_RESULT(xiiOSFile::DeleteFile(input));
    set(0, "Transform", "References/CompilerInput.txt");
    {
      xiiTestLogInterface   log;
      xiiTestLogSystemScope scope(&log);
      log.ExpectMessage("Failed to make path absolute", xiiLogMsgType::ErrorMsg);
      log.ExpectMessage("Failed to hash dependency asset", xiiLogMsgType::ErrorMsg, 2);
      XII_TEST_BOOL(hash(2, true) == 0);
    }
    auto write = [&](xiiStringView text) {
      xiiOSFile file;
      XII_TEST_RESULT(file.Open(input, xiiFileOpenMode::Write));
      XII_TEST_RESULT(file.Write(text.GetStartPointer(), text.GetElementCount()));
      file.Close();
      curator->NotifyOfFileChange(input);
    };
    write("First compiler input");
    const auto before = hash(2);
    XII_TEST_BOOL(before != 0);
    write("Other compiler input");
    curator->NotifyOfFileChange(input);
    XII_TEST_BOOL(hash(2) != before);
    set(0, "Transform", "");
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Renames retain GUID dependencies and invalidate old paths")
  {
    set(1, "Transform", references[0]);
    documents[0]->GetDocumentManager()->CloseDocument(documents[0]);
    documents[0] = nullptr;
    xiiStringBuilder renamed(project.GetFileDirectory(), "/Relocated/Renamed.xiiResolutionTest");
    XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(renamed.GetFileDirectory()));
    XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(paths[0], renamed));
    curator->NotifyOfFileChange(paths[0]);
    curator->NotifyOfFileChange(renamed);
    curator->MainThreadTick(false);
    xiiUuid id;
    XII_TEST_STATUS(curator->ResolveAsset(references[0], id));
    XII_TEST_BOOL(id == ids[0]);
    XII_TEST_STATUS(curator->ResolveAsset(renamed, id));
    XII_TEST_BOOL(id == ids[0]);
    XII_TEST_BOOL(curator->ResolveAsset(paths[0], id).Failed());
    paths[0]     = renamed;
    documents[0] = xiiDynamicCast<ResolutionTestDocument*>(editor.OpenDocument(renamed, xiiDocumentFlags::None));
    if (!XII_TEST_BOOL(documents[0] != nullptr))
      return;
    const auto before = hash(2);
    set(0, "Value", "Changed after relocation");
    XII_TEST_BOOL(hash(2) != before);
    XII_TEST_BOOL(transform(references[2]));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Production resource handles, serialization, concurrent loading and hot reload")
  {
    const xiiStringBuilder surfacePath(project.GetFileDirectory(), "/References/Surface.xiiSurfaceAsset");
    auto*                  surface = xiiDynamicCast<xiiAssetDocument*>(editor.CreateDocument(surfacePath, xiiDocumentFlags::None));
    if (!XII_TEST_BOOL(surface != nullptr))
      return;
    XII_SCOPE_EXIT(surface->GetDocumentManager()->CloseDocument(surface));
    XII_TEST_STATUS(surface->SaveDocument());
    curator->NotifyOfFileChange(surfacePath);
    XII_TEST_BOOL(transform(surfacePath));
    XII_TEST_RESULT(curator->WriteAssetTables(nullptr, true));
    const auto previousTable                           = xiiDataDirectory::FolderType::s_sRedirectionFile;
    const auto previousPrefix                          = xiiDataDirectory::FolderType::s_sRedirectionPrefix;
    xiiDataDirectory::FolderType::s_sRedirectionFile   = xiiStringBuilder("AssetCache/", curator->GetActiveAssetProfile()->GetConfigName(), ".xiiAidlt");
    xiiDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/";
    xiiFileSystem::ReloadAllExternalDataDirectoryConfigs();
    XII_SCOPE_EXIT(xiiDataDirectory::FolderType::s_sRedirectionFile = previousTable; xiiDataDirectory::FolderType::s_sRedirectionPrefix = previousPrefix; xiiFileSystem::ReloadAllExternalDataDirectoryConfigs());
    xiiStringBuilder guid;
    xiiConversionUtils::ToString(surface->GetGuid(), guid);
    auto byGuid     = xiiResourceManager::LoadResource<xiiSurfaceResource>(guid);
    auto byPath     = xiiResourceManager::LoadResource<xiiSurfaceResource>(surfacePath);
    auto byRelative = xiiResourceManager::LoadResource<xiiSurfaceResource>("References/Surface.xiiSurfaceAsset");
    XII_TEST_BOOL(byGuid == byPath && byGuid == byRelative);
    XII_TEST_BOOL(xiiResourceManager::GetExistingResource<xiiSurfaceResource>(":project/References/Surface.xiiSurfaceAsset") == byGuid);
    {
      xiiResourceLock<xiiSurfaceResource> loaded(byGuid, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      XII_TEST_BOOL(loaded.GetAcquireResult() == xiiResourceAcquireResult::Final);
    }
    xiiSurfaceResourceHandle parallel[8];
    xiiTaskGroupID           groups[8];
    for (xiiUInt32 i = 0; i < 8; ++i)
    {
      auto task = XII_DEFAULT_NEW(xiiDelegateTask<void>, "Resolve resource alias", xiiTaskNesting::Never, [&, i] {
        parallel[i] = xiiResourceManager::LoadResource<xiiSurfaceResource>(i % 2 ? guid.GetView() : surfacePath.GetView());
      });
      groups[i] = xiiTaskSystem::StartSingleTask(task, xiiTaskPriority::LongRunning);
    }
    for (auto group : groups)
      xiiTaskSystem::WaitForGroup(group);
    for (const auto& handle : parallel)
      XII_TEST_BOOL(handle == byGuid);
    auto* accessor = surface->GetObjectAccessor();
    accessor->StartTransaction("Change surface for hot reload");
    XII_TEST_STATUS(accessor->SetValueByName(surface->GetObjectManager()->GetRootObject()->GetChildren()[0], "Restitution", 0.75f));
    accessor->FinishTransaction();
    XII_TEST_STATUS(surface->SaveDocument());
    curator->NotifyOfFileChange(surfacePath);
    XII_TEST_BOOL(transform(surfacePath));
    XII_TEST_BOOL(xiiResourceManager::ReloadResource(byPath, true));
    {
      xiiResourceLock<xiiSurfaceResource> loaded(byGuid, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      XII_TEST_FLOAT(loaded->GetDescriptor().m_fPhysicsRestitution, 0.75f, 0.0f);
    }
    // A normal file read must still see the authored DDL document, not its compiled resource.
    xiiFileReader source;
    XII_TEST_RESULT(source.Open(surfacePath));
    xiiUniquePtr<xiiAssetDocumentInfo> sourceInfo;
    XII_TEST_STATUS(surface->GetAssetDocumentManager()->ReadAssetDocumentInfo(sourceInfo, source));
    XII_TEST_BOOL(sourceInfo && sourceInfo->m_DocumentID == surface->GetGuid());
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    writer << byPath;
    xiiMemoryStreamReader    reader(&storage);
    xiiSurfaceResourceHandle restored;
    reader >> restored;
    XII_TEST_BOOL(restored == byGuid);
    XII_TEST_STRING(restored.GetResourceID(), guid);
  }
}
