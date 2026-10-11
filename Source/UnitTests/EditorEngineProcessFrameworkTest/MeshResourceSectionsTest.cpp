/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <TestFramework/Utilities/TestLogInterface.h>

XII_CREATE_SIMPLE_TEST(EditorViews, MeshResourceSections)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(2), true);
  xiiMeshResourceDescriptor descriptor;
  descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
  descriptor.AddMaterialSlot("");
  descriptor.AddMaterialSlot("");
  descriptor.AddSection(5, 0, 0);
  descriptor.AddSection(7, 5, 1);
  auto                             handle = xiiResourceManager::CreateResource<xiiMeshResource>("EditorMeshSectionResource", std::move(descriptor));
  xiiResourceLock<xiiMeshResource> mesh(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(mesh && mesh.GetAcquireResult() == xiiResourceAcquireResult::Final))
    return;
  XII_TEST_INT(mesh->GetMeshletCount(), 2);
  XII_TEST_INT(mesh->GetSections()[0].m_uiFirstMeshlet, 0);
  XII_TEST_INT(mesh->GetSections()[1].m_uiFirstMeshlet, 1);
  XII_TEST_INT(mesh->GetLODs()[0].m_uiMeshletCount, 2);
  xiiResourceLock<xiiMeshBufferResource> buffer(mesh->GetMeshBuffer(), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  XII_TEST_BOOL(buffer->GetVertexBuffer() != nullptr && buffer->GetIndexBuffer() != nullptr && buffer->GetMeshletBuffer() != nullptr);
  XII_TEST_INT(buffer->GetMeshlets()[0].m_uiMaterialIndex, 0);
  XII_TEST_INT(buffer->GetMeshlets()[1].m_uiMaterialIndex, 1);

  // A second resource can share immutable GPU data without rebuilding or erasing its section metadata.
  auto                             sharedDescriptor = mesh->GetDescriptor();
  auto                             sharedHandle     = xiiResourceManager::CreateResource<xiiMeshResource>("EditorMeshSectionSharedResource", std::move(sharedDescriptor));
  xiiResourceLock<xiiMeshResource> shared(sharedHandle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (XII_TEST_BOOL(shared && shared.GetAcquireResult() == xiiResourceAcquireResult::Final))
  {
    XII_TEST_BOOL(shared->GetMeshBuffer() == mesh->GetMeshBuffer());
    XII_TEST_INT(shared->GetSections()[1].m_uiFirstMeshlet, 1);
    XII_TEST_INT(shared->GetLODs()[0].m_uiMeshletCount, 2);
  }

  auto makeInvalid = [&]() {
    xiiMeshResourceDescriptor invalid;
    invalid.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
    invalid.AddSection(13, 0, 0); // Only twelve source triangles exist.
    return invalid;
  };
  const auto                       invalidHandle = xiiResourceManager::CreateResource<xiiMeshResource>("EditorInvalidMeshSections", makeInvalid());
  xiiResourceLock<xiiMeshResource> invalid(invalidHandle, xiiResourceAcquireMode::PointerOnly);
  XII_TEST_BOOL(invalid->GetLoadingState() == xiiResourceState::LoadedResourceMissing);
  XII_TEST_BOOL(!invalid->GetMeshBuffer().IsValid());

  // Also exercise UpdateContent, which must propagate compilation failure to ResourceManager.
  xiiTestLogInterface   log;
  xiiTestLogSystemScope logScope(&log, true);
  log.ExpectMessage("Missing Resource of Type 'xiiMeshResource'", xiiLogMsgType::SeriousWarningMsg);
  const auto                                streamedHandle = xiiResourceManager::LoadResource<xiiMeshResource>("EditorInvalidStreamedMeshSections");
  xiiUniquePtr<xiiResourceLoaderFromMemory> loader         = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
  xiiMemoryStreamWriter                     writer(&loader->m_CustomData);
  makeInvalid().Save(writer);
  xiiResourceManager::UpdateResourceWithCustomLoader(streamedHandle, std::move(loader));
  xiiResourceLock<xiiMeshResource> streamed(streamedHandle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  XII_TEST_BOOL(!streamed);
  xiiResourceLock<xiiMeshResource> streamedState(streamedHandle, xiiResourceAcquireMode::PointerOnly);
  XII_TEST_BOOL(streamedState->GetLoadingState() == xiiResourceState::LoadedResourceMissing);
  XII_TEST_BOOL(!streamedState->GetMeshBuffer().IsValid());
}
