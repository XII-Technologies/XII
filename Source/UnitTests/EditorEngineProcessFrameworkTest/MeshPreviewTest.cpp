/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderPipelineProfileConfig.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <SharedPluginAssets/Common/Messages.h>

namespace
{
  class MeshPreviewTestContext : public xiiMeshContext
  {
  public:
    using xiiMeshContext::CreateViewContext;
    using xiiMeshContext::DestroyViewContext;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, MeshPreviewMainProfile)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  // Use the same asset lookup as the Editor, not the canonical test helper's named aliases.
  const auto      oldFile         = xiiDataDirectory::FolderType::s_sRedirectionFile;
  const auto      oldPrefix       = xiiDataDirectory::FolderType::s_sRedirectionPrefix;
  const xiiString previousSceneId = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(xiiRenderPipelineProfileConfig::s_szScenePipeline).GetResourceID();
  xiiResourceManager::UnregisterNamedResource(xiiRenderPipelineProfileConfig::s_szScenePipeline);
  XII_SCOPE_EXIT(if (previousSceneId != xiiRenderPipelineProfileConfig::s_szScenePipeline) xiiResourceManager::RegisterNamedResource(xiiRenderPipelineProfileConfig::s_szScenePipeline, previousSceneId));
  xiiStringBuilder fixture(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/MeshPreview");
  xiiStringBuilder cache(fixture, "/AssetCache/Common");
  if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(cache)))
    return;
  xiiStringBuilder source(xiiFileSystem::GetSdkRootDirectory(), "/Data/Base/AssetCache/Common/RenderPipelines/Scene.xiiBinRenderPipeline");
  source.MakeCleanPath();
  if (!XII_TEST_RESULT(xiiOSFile::CopyFile(source, xiiStringBuilder(cache, "/Scene.xiiBinRenderPipeline"))))
    return;
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(fixture, "MeshPreviewTest", "MeshPreviewTest", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("MeshPreviewTest"));
  {
    xiiFileWriter writer;
    if (!XII_TEST_RESULT(writer.Open(":MeshPreviewTest/AssetCache/MeshPreview.xiiAidlt")))
      return;
    const xiiStringBuilder entry(xiiRenderPipelineProfileConfig::s_szScenePipeline, ";Common/Scene.xiiBinRenderPipeline\n");
    if (!XII_TEST_RESULT(writer.WriteBytes(entry.GetData(), entry.GetElementCount())))
      return;
  }
  xiiDataDirectory::FolderType::s_sRedirectionFile   = "AssetCache/MeshPreview.xiiAidlt";
  xiiDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/";
  xiiFileSystem::ReloadAllExternalDataDirectoryConfigs();
  XII_SCOPE_EXIT(xiiDataDirectory::FolderType::s_sRedirectionFile = oldFile; xiiDataDirectory::FolderType::s_sRedirectionPrefix = oldPrefix; xiiFileSystem::ReloadAllExternalDataDirectoryConfigs());

  const auto       documentId = xiiUuid::MakeStableUuidFromString("MeshPreview/MainProfile");
  xiiStringBuilder meshId;
  xiiConversionUtils::ToString(documentId, meshId);
  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor descriptor;
  descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, false);
  descriptor.AddSection(12, 0, 0);
  descriptor.ComputeBounds();
  const auto                    mesh = xiiResourceManager::CreateResource<xiiMeshResource>(meshId, std::move(descriptor));
  xiiMaterialResourceDescriptor materialDescription;
  materialDescription.m_BaseColor = xiiColor(0.8f, 0.1f, 0.05f);
  const auto material             = xiiResourceManager::CreateResource<xiiMaterialResource>("MeshPreview/Material", std::move(materialDescription));

  xiiPlatformProfile profile;
  profile.AddMissingConfigs();
  MeshPreviewTestContext document;
  const auto             initialized = document.Initialize(documentId, {}, nullptr, "Mesh");
  XII_TEST_STATUS(initialized);
  if (initialized.Failed())
    return;
  XII_SCOPE_EXIT(document.Deinitialize());
  auto* context = document.CreateViewContext();
  if (!XII_TEST_BOOL(context != nullptr))
    return;
  XII_SCOPE_EXIT(document.DestroyViewContext(context));
  context->SetRenderPipelineProfile(&profile);
  document.m_bDisplayGrid = false;
  xiiEditorEngineSetMaterialsMsg materials;
  materials.m_Materials.PushBack(material.GetResourceID());
  document.HandleMessage(&materials);
  xiiGALTextureCreationDescription target;
  target.m_Type      = xiiGALResourceDimension::Texture2D;
  target.m_Size      = {64, 64};
  target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  target.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto output        = environment.GetDevice()->CreateTexture(target);
  if (!XII_TEST_BOOL(output != nullptr))
    return;
  context->SetupRenderTarget(nullptr, output->GetDefaultView(xiiGALTextureViewType::RenderTarget), 64, 64);
  auto* world = document.GetWorld();
  XII_LOCK(world->GetWriteMarker());
  world->SetWorldSimulationEnabled(false);
  xiiView* view = nullptr;
  if (!XII_TEST_BOOL(world->GetOrCreateModule<xiiRenderWorldModule>()->TryGetView(context->GetViewHandle(), view)))
    return;
  for (xiiUInt32 frame = 0; frame < 8; ++frame)
  {
    context->Redraw(false);
    world->Update();
    environment.GetDevice()->WaitIdle();
  }
  XII_TEST_RESULT(view->GetLastRenderResult());
  XII_TEST_BOOL(view->GetRenderPipelineInstance().GetResourceAcquireResult() == xiiResourceAcquireResult::Final);
  XII_TEST_STRING(view->GetRenderPipelineResource().GetResourceID(), profile.GetTypeConfig<xiiRenderPipelineProfileConfig>()->m_sMainRenderPipeline);
  xiiUInt32 before = 0, after = 0;
  if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), output.Borrow(), 32, 32, before))
    return;
  XII_TEST_BOOL((before & 0x00FFFFFFU) != 0);
  world->GetOrCreateComponentManager<xiiMeshComponentManager>()->GetComponents()->SetActiveFlag(false);
  context->Redraw(false);
  world->Update();
  environment.GetDevice()->WaitIdle();
  if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), output.Borrow(), 32, 32, after))
    return;
  XII_TEST_BOOL(before != after);
}
