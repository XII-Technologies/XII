#include <EditorTest/EditorTestPCH.h>

#include "TestClass.h"
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <QMimeData>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

xiiEditorTestApplication::xiiEditorTestApplication() :
  xiiApplication("xiiEditor")
{
  EnableMemoryLeakReporting(true);

  m_pEditorApp = new xiiQtEditorApp;
}

xiiResult xiiEditorTestApplication::BeforeCoreSystemsStartup()
{
  if (SUPER::BeforeCoreSystemsStartup().Failed())
    return XII_FAILURE;

  xiiStartup::AddApplicationTag("tool");
  xiiStartup::AddApplicationTag("editor");
  xiiStartup::AddApplicationTag("editorapp");

  xiiQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());
  return XII_SUCCESS;
}

void xiiEditorTestApplication::AfterCoreSystemsShutdown()
{
  xiiQtEditorApp::GetSingleton()->DeInitQt();

  delete m_pEditorApp;
  m_pEditorApp = nullptr;
}

xiiApplication::Execution xiiEditorTestApplication::Run()
{
  qApp->processEvents();
  return xiiApplication::Execution::Continue;
}

void xiiEditorTestApplication::AfterCoreSystemsStartup()
{
  XII_PROFILE_SCOPE("AfterCoreSystemsStartup");
  // We override the user data dir to not pollute the editor settings.
  xiiStringBuilder userDataDir = xiiOSFile::GetUserDataFolder();
  userDataDir.AppendPath("xiiEngine Project", "EditorTest");
  userDataDir.MakeCleanPath();

  xiiQtEditorApp::GetSingleton()->StartupEditor(xiiQtEditorApp::StartupFlags::SafeMode | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::UnitTest, userDataDir);
  // Disable msg boxes.
  xiiQtUiServices::SetHeadless(true);
  xiiFileSystem::SetSpecialDirectory("testout", xiiTestFramework::GetInstance()->GetAbsOutputPath());

  xiiFileSystem::AddDataDirectory(">xiitest/", "ImageComparisonDataDir", "imgout", xiiFileSystem::AllowWrites).IgnoreResult();
}

void xiiEditorTestApplication::BeforeHighLevelSystemsShutdown()
{
  XII_PROFILE_SCOPE("BeforeHighLevelSystemsShutdown");
  xiiQtEditorApp::GetSingleton()->ShutdownEditor();
}

//////////////////////////////////////////////////////////////////////////

xiiEditorTest::xiiEditorTest()
{
  xiiQtEngineViewWidget::s_FixedResolution = xiiSizeU32(512, 512);
}

xiiEditorTest::~xiiEditorTest() = default;

xiiEditorTestApplication* xiiEditorTest::CreateApplication()
{
  return XII_DEFAULT_NEW(xiiEditorTestApplication);
}

xiiResult xiiEditorTest::GetImage(xiiImage& img)
{
  if (!m_CapturedImage.IsValid())
    return XII_FAILURE;

  img.ResetAndMove(std::move(m_CapturedImage));
  return XII_SUCCESS;
}

xiiResult xiiEditorTest::InitializeTest()
{
  m_pApplication = CreateApplication();
  m_sProjectPath.Clear();

  if (m_pApplication == nullptr)
    return XII_FAILURE;

  XII_SUCCEED_OR_RETURN(xiiRun_Startup(m_pApplication));

  static bool s_bCheckedReferenceDriver = false;
  static bool s_bIsReferenceDriver      = false;
  static bool s_bIsAMDDriver            = false;

  if (!s_bCheckedReferenceDriver)
  {
    s_bCheckedReferenceDriver = true;

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    xiiUniquePtr<xiiGALDevice>      pDevice;
    xiiGALDeviceCreationDescription DeviceInit;

    pDevice = xiiGALDeviceFactory::CreateDevice(xiiGameApplication::GetActiveRenderer(), xiiFoundation::GetDefaultAllocator(), DeviceInit);

    XII_SUCCEED_OR_RETURN(pDevice->Init());

    if (pDevice->GetCapabilities().m_sAdapterName == "Microsoft Basic Render Driver" || pDevice->GetCapabilities().m_sAdapterName.StartsWith_NoCase("Intel(R) UHD Graphics"))
    {
      s_bIsReferenceDriver = true;
    }
    else if (pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("AMD") || pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Radeon"))
    {
      s_bIsAMDDriver = true;
    }

    XII_SUCCEED_OR_RETURN(pDevice->Shutdown());
    pDevice.Clear();
#endif
  }

  if (xiiStringUtils::IsEqual_NoCase(xiiGameApplication::GetActiveRenderer(), "DX11") && s_bIsReferenceDriver)
  {
    // Use different images for comparison when running the D3D11 Reference Device
    xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_D3D11Ref");
  }
  else if (xiiStringUtils::IsEqual_NoCase(xiiGameApplication::GetActiveRenderer(), "DX11") && s_bIsAMDDriver)
  {
    // Line rendering on DX11 is different on AMD and requires separate images for tests rendering lines.
    xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_AMD");
  }
  else
  {
    xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("");
  }

  return XII_SUCCESS;
}

xiiResult xiiEditorTest::DeInitializeTest()
{
  CloseCurrentProject();

  if (m_pApplication)
  {
    xiiRun_Shutdown(m_pApplication);

    XII_DEFAULT_DELETE(m_pApplication);
  }


  return XII_SUCCESS;
}

xiiResult xiiEditorTest::CreateAndLoadProject(const char* name)
{
  XII_PROFILE_SCOPE("CreateAndLoadProject");
  xiiStringBuilder relPath;
  relPath = ":APPDATA";
  relPath.AppendPath(name);

  xiiStringBuilder absPath;
  if (xiiFileSystem::ResolvePath(relPath, &absPath, nullptr).Failed())
  {
    xiiLog::Error("Failed to resolve project path '{0}'.", relPath);
    return XII_FAILURE;
  }
  if (xiiOSFile::DeleteFolder(absPath).Failed())
  {
    xiiLog::Error("Failed to delete old project folder '{0}'.", absPath);
    return XII_FAILURE;
  }

  xiiStringBuilder projectFile = absPath;
  projectFile.AppendPath("xiiProject");
  if (m_pApplication->m_pEditorApp->CreateOrOpenProject(true, projectFile).Failed())
  {
    xiiLog::Error("Failed to create project '{0}'.", projectFile);
    return XII_FAILURE;
  }

  m_sProjectPath = absPath;
  return XII_SUCCESS;
}

xiiResult xiiEditorTest::OpenProject(const char* path)
{
  XII_PROFILE_SCOPE("OpenProject");
  xiiStringBuilder relPath;
  relPath = ">sdk";
  relPath.AppendPath(path);

  xiiStringBuilder absPath;
  if (xiiFileSystem::ResolveSpecialDirectory(relPath, absPath).Failed())
  {
    xiiLog::Error("Failed to resolve project path '{0}'.", relPath);
    return XII_FAILURE;
  }

  // Copy project to temp folder
  xiiStringBuilder projectName = xiiPathUtils::GetFileName(path);
  xiiStringBuilder relTempPath;
  relTempPath = ":APPDATA";
  relTempPath.AppendPath(projectName);

  xiiStringBuilder absTempPath;
  if (xiiFileSystem::ResolvePath(relTempPath, &absTempPath, nullptr).Failed())
  {
    xiiLog::Error("Failed to resolve project temp path '{0}'.", relPath);
    return XII_FAILURE;
  }
  if (xiiOSFile::DeleteFolder(absTempPath).Failed())
  {
    xiiLog::Error("Failed to delete old project temp folder '{0}'.", absTempPath);
    return XII_FAILURE;
  }
  if (xiiOSFile::CopyFolder(absPath, absTempPath).Failed())
  {
    xiiLog::Error("Failed to copy project '{0}' to temp location: '{1}'.", absPath, absTempPath);
    return XII_FAILURE;
  }


  xiiStringBuilder projectFile = absTempPath;
  projectFile.AppendPath("xiiProject");
  if (m_pApplication->m_pEditorApp->CreateOrOpenProject(false, projectFile).Failed())
  {
    xiiLog::Error("Failed to open project '{0}'.", projectFile);
    return XII_FAILURE;
  }

  m_sProjectPath = absTempPath;
  return XII_SUCCESS;
}

xiiDocument* xiiEditorTest::OpenDocument(const char* subpath)
{
  xiiStringBuilder fullpath;
  fullpath = m_sProjectPath;
  fullpath.AppendPath(subpath);

  xiiDocument* pDoc = m_pApplication->m_pEditorApp->OpenDocument(fullpath, xiiDocumentFlags::RequestWindow);

  if (pDoc)
  {
    ProcessEvents();
  }

  return pDoc;
}

void xiiEditorTest::ExecuteDocumentAction(const char* szActionName, xiiDocument* pDocument, const xiiVariant& argument /*= xiiVariant()*/)
{
  XII_TEST_BOOL(xiiActionManager::ExecuteAction(nullptr, szActionName, pDocument, argument).Succeeded());
}

xiiResult xiiEditorTest::CaptureImage(xiiQtDocumentWindow* pWindow, const char* szImageName)
{
  xiiStringBuilder sImgPath = xiiOSFile::GetUserDataFolder("EditorTests");
  sImgPath.AppendFormat("/{}.tga", szImageName);

  xiiOSFile::DeleteFile(sImgPath).IgnoreResult();

  pWindow->CreateImageCapture(sImgPath);

  for (int i = 0; i < 10; ++i)
  {
    ProcessEvents();

    if (xiiOSFile::ExistsFile(sImgPath))
      break;

    xiiThreadUtils::Sleep(xiiTime::Milliseconds(100));
  }

  if (!xiiOSFile::ExistsFile(sImgPath))
    return XII_FAILURE;

  XII_SUCCEED_OR_RETURN(m_CapturedImage.LoadFrom(sImgPath));

  return XII_SUCCESS;
}

void xiiEditorTest::CloseCurrentProject()
{
  XII_PROFILE_SCOPE("CloseCurrentProject");
  m_sProjectPath.Clear();
  m_pApplication->m_pEditorApp->CloseProject();
}

void xiiEditorTest::SafeProfilingData()
{
  xiiFileWriter fileWriter;
  if (fileWriter.Open(":appdata/profiling.json") == XII_SUCCESS)
  {
    xiiProfilingSystem::ProfilingData profilingData;
    xiiProfilingSystem::Capture(profilingData);
    profilingData.Write(fileWriter).IgnoreResult();
  }
}

void xiiEditorTest::ProcessEvents(xiiUInt32 uiIterations)
{
  XII_PROFILE_SCOPE("ProcessEvents");
  if (qApp)
  {
    for (xiiUInt32 i = 0; i < uiIterations; i++)
    {
      qApp->processEvents();
    }
  }
}

std::unique_ptr<QMimeData> xiiEditorTest::AssetsToDragMimeData(xiiArrayPtr<xiiUuid> assetGuids)
{
  std::unique_ptr<QMimeData> mimeData(new QMimeData());
  QByteArray                 encodedData;
  QDataStream                stream(&encodedData, QIODevice::WriteOnly);

  QString     sGuids;
  QList<QUrl> urls;

  xiiStringBuilder tmp;

  stream << (int)1;
  for (xiiUInt32 i = 0; i < assetGuids.GetCount(); ++i)
  {
    QString sGuid(xiiConversionUtils::ToString(assetGuids[i], tmp).GetData());
    stream << sGuid;
  }

  mimeData->setData("application/xiiEditor.AssetGuid", encodedData);
  return std::move(mimeData);
}

std::unique_ptr<QMimeData> xiiEditorTest::ObjectsDragMimeData(const xiiDeque<const xiiDocumentObject*>& objects)
{
  std::unique_ptr<QMimeData> mimeData(new QMimeData());
  QByteArray                 encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  int iCount = (int)objects.GetCount();
  stream << iCount;

  for (const xiiDocumentObject* pObject : objects)
  {
    stream.writeRawData((const char*)&pObject, sizeof(void*));
  }

  mimeData->setData("application/xiiEditor.ObjectSelection", encodedData);
  return std::move(mimeData);
}

void xiiEditorTest::MoveObjectsToLayer(xiiScene2Document* pDoc, const xiiDeque<const xiiDocumentObject*>& objects, const xiiUuid& layer, xiiDeque<const xiiDocumentObject*>& new_objects)
{
  pDoc->GetSelectionManager()->SetSelection(objects);

  xiiQtLayerAdapter adapter(pDoc);
  auto              mimeData = ObjectsDragMimeData(objects);
  xiiDragDropInfo   info;
  info.m_iTargetObjectInsertChildIndex = -1;
  info.m_pMimeData                     = mimeData.get();
  info.m_sTargetContext                = "layertree";
  info.m_TargetDocument                = pDoc->GetGuid();
  info.m_TargetObject                  = pDoc->GetLayerObject(layer)->GetGuid();
  info.m_bCtrlKeyDown                  = false;
  info.m_bShiftKeyDown                 = false;
  info.m_pAdapter                      = &adapter;
  if (!XII_TEST_BOOL(xiiDragDropHandler::DropOnly(&info)))
    return;

  new_objects = pDoc->GetLayerDocument(layer)->GetSelectionManager()->GetSelection();
}

const xiiDocumentObject* xiiEditorTest::DropAsset(xiiScene2Document* pDoc, const char* szAssetGuidOrPath, bool bShift /*= false*/, bool bCtrl /*= false*/)
{
  const xiiAssetCurator::xiiLockedSubAsset asset = xiiAssetCurator::GetSingleton()->FindSubAsset(szAssetGuidOrPath);
  if (XII_TEST_BOOL(asset.isValid()))
  {
    xiiUuid              assetGuid = asset->m_Data.m_Guid;
    xiiArrayPtr<xiiUuid> assets(&assetGuid, 1);
    auto                 mimeData = AssetsToDragMimeData(assets);

    xiiDragDropInfo info;
    info.m_pMimeData                     = mimeData.get();
    info.m_TargetDocument                = pDoc->GetGuid();
    info.m_sTargetContext                = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_iTargetObjectSubID            = 0;
    info.m_bShiftKeyDown                 = bShift;
    info.m_bCtrlKeyDown                  = bCtrl;

    if (XII_TEST_BOOL(xiiDragDropHandler::DropOnly(&info)))
    {
      return pDoc->GetSelectionManager()->GetCurrentObject();
    }
  }
  return {};
}

const xiiDocumentObject* xiiEditorTest::CreateGameObject(xiiScene2Document* pDoc)
{
  auto pAccessor = pDoc->GetObjectAccessor();
  pAccessor->StartTransaction("Add Game Object");

  xiiUuid guid;
  XII_TEST_STATUS(pAccessor->AddObject(pDoc->GetObjectManager()->GetRootObject(), "Children", -1, xiiRTTI::FindTypeByName("xiiGameObject"), guid));
  pAccessor->FinishTransaction();

  return pAccessor->GetObject(guid);
}
