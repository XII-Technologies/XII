/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/Configuration/PlayerConfig.h>
#include <GameEngine/Utilities/SceneLoadUtil.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>

XII_CREATE_SIMPLE_TEST_GROUP(Player);

XII_CREATE_SIMPLE_TEST(Player, SceneLoading)
{
  XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "PlayerLoadingTest", "imgout", xiiDataDirUsage::AllowWrites));
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("PlayerLoadingTest"));
  const char* szScene = ":imgout/Player/Scene.xiiBinScene";
  {
    xiiWorldDescription description("Scene writer");
    xiiWorld            world(description);
    XII_LOCK(world.GetWriteMarker());
    for (xiiUInt32 i = 0; i < 2500; ++i)
    {
      xiiGameObjectDescription object;
      object.m_sName.Assign("Loaded object");
      world.CreateObject(object);
    }
    xiiFileWriter file;
    if (!XII_TEST_RESULT(file.Open(szScene)))
      return;
    xiiAssetFileHeader header;
    XII_TEST_RESULT(header.Write(file));
    XII_TEST_RESULT(file.WriteBytes("[xiiBinaryScene]", 16));
    xiiWorldWriter writer;
    writer.WriteWorld(file, world);
  }
  auto advance = [](xiiSceneLoadUtility& loader) {
    const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(10);
    while (loader.GetLoadingState() == xiiSceneLoadUtility::LoadingState::Ongoing && xiiTime::Now() < deadline)
    {
      loader.TickSceneLoading();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
    }
  };
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Read task, incremental instantiation and ownership transfer")
  {
    xiiSceneLoadUtility loader;
    loader.StartSceneLoading(szScene, {});
    loader.TickSceneLoading();
    XII_TEST_BOOL(loader.GetLoadingPhase() == xiiSceneLoadUtility::LoadingPhase::Reading);
    advance(loader);
    if (XII_TEST_BOOL(loader.GetLoadingState() == xiiSceneLoadUtility::LoadingState::FinishedSuccessfully))
    {
      auto world = loader.RetrieveLoadedScene();
      XII_LOCK(world->GetReadMarker());
      XII_TEST_INT(world->GetObjectCount(), 2500);
      XII_TEST_BOOL(world->GetWorldSimulationEnabled());
    }
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Cancel IO and partial activation, then load again")
  {
    for (xiiUInt32 i = 0; i < 8; ++i)
    {
      xiiSceneLoadUtility loader;
      loader.StartSceneLoading(szScene, {});
      loader.TickSceneLoading();
      if (i > 3)
      {
        const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
        while (loader.GetLoadingPhase() == xiiSceneLoadUtility::LoadingPhase::Reading && xiiTime::Now() < deadline)
        {
          loader.TickSceneLoading();
          xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
        }
      }
      loader.Cancel();
      XII_TEST_BOOL(loader.GetLoadingState() == xiiSceneLoadUtility::LoadingState::Cancelled);
      loader.TickSceneLoading();
    }
    xiiSceneLoadUtility loader;
    loader.StartSceneLoading(szScene, {});
    advance(loader);
    XII_TEST_BOOL(loader.GetLoadingState() == xiiSceneLoadUtility::LoadingState::FinishedSuccessfully);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Malformed scene provides actionable failure without activation")
  {
    const char* szInvalid = ":imgout/Player/Invalid.xiiBinScene";
    {
      xiiFileWriter file;
      XII_TEST_RESULT(file.Open(szInvalid));
      XII_TEST_RESULT(file.WriteBytes("invalid", 7));
    }
    xiiSceneLoadUtility loader;
    loader.StartSceneLoading(szInvalid, {});
    advance(loader);
    XII_TEST_BOOL(loader.GetLoadingState() == xiiSceneLoadUtility::LoadingState::Failed);
    XII_TEST_BOOL(!loader.GetLoadingFailureReason().IsEmpty());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Truncated component payload returns failure instead of spinning on EOF")
  {
    xiiWorldDescription description("Truncated scene");
    xiiWorld            world(description);
    XII_LOCK(world.GetWriteMarker());
    xiiGameObject* object = nullptr;
    world.CreateObject(xiiGameObjectDescription(), object);
    xiiCameraComponent* camera = nullptr;
    xiiCameraComponent::CreateComponent(object, camera);
    xiiContiguousMemoryStreamStorage storage;
    xiiMemoryStreamWriter            stream(&storage);
    xiiAssetFileHeader               header;
    XII_TEST_RESULT(header.Write(stream));
    XII_TEST_RESULT(stream.WriteBytes("[xiiBinaryScene]", 16));
    xiiWorldWriter writer;
    writer.WriteWorld(stream, world);
    const char* path = ":imgout/Player/Truncated.xiiBinScene";
    {
      xiiFileWriter file;
      XII_TEST_RESULT(file.Open(path));
      XII_TEST_RESULT(file.WriteBytes(storage.GetData(), storage.GetStorageSize32() - 3));
    }
    xiiSceneLoadUtility loader;
    loader.StartSceneLoading(path, {});
    advance(loader);
    XII_TEST_BOOL(loader.GetLoadingState() == xiiSceneLoadUtility::LoadingState::Failed);
    XII_TEST_BOOL(!loader.GetLoadingFailureReason().IsEmpty());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shared project startup and named launch round trip")
  {
    const char*     szConfig = ":imgout/Player/Player.ddl";
    xiiPlayerConfig config;
    auto&           launch = config.m_LaunchConfigurations.ExpandAndGetRef();
    launch.m_sName         = "Two clients";
    launch.m_uiInstances   = 2;
    launch.m_Arguments.PushBack("-App.FrameLimit");
    launch.m_Arguments.PushBack("60");
    XII_TEST_RESULT(config.Save(szConfig));
    xiiPlayerConfig loaded;
    XII_TEST_RESULT(loaded.Load(szConfig));
    const auto* saved = loaded.FindLaunch("Two clients");
    if (XII_TEST_BOOL(saved != nullptr))
      XII_TEST_INT(saved->m_uiInstances, 2);
    loaded.m_LaunchConfigurations[0].m_uiInstances = 0;
    XII_TEST_BOOL(loaded.Validate().Failed());
  }
}
