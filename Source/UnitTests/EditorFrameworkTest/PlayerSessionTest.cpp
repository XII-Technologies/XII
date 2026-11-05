/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <EditorFramework/Project/PlayerSession.h>
#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <Foundation/Application/Config/ProjectManifest.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <QApplication>

XII_CREATE_SIMPLE_TEST_GROUP(PlayerSessions);

XII_CREATE_SIMPLE_TEST(PlayerSessions, ManagedProcesses)
{
  const xiiStringBuilder output(xiiTestFramework::GetInstance()->GetAbsOutputPath());
  XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(output, "PlayerSessionTest", "player-test", xiiDataDirUsage::AllowWrites));
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("PlayerSessionTest"));
  auto writeProject = [](const char* name) {
    xiiStringBuilder marker;
    marker.SetFormat(":player-test/{}/xiiProject", name);
    xiiStringBuilder absolute;
    XII_TEST_RESULT(xiiFileSystem::ResolvePath(marker, &absolute, nullptr));
    XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(absolute.GetFileDirectory()));
    xiiProjectManifest manifest;
    manifest.Initialize(absolute);
    manifest.m_sDefaultScene = "Main.xiiBinScene";
    XII_TEST_STATUS(manifest.Save(absolute));
    xiiFileWriter    file;
    xiiStringBuilder scene;
    scene.SetFormat(":player-test/{}/Main.xiiBinScene", name);
    XII_TEST_RESULT(file.Open(scene));
    xiiAssetFileHeader header;
    XII_TEST_RESULT(header.Write(file));
    XII_TEST_RESULT(file.WriteBytes("[xiiBinaryScene]", 16));
    xiiWorldDescription description("Player process test scene");
    xiiWorld            world(description);
    XII_LOCK(world.GetWriteMarker());
    xiiGameObjectDescription object;
    object.m_sName.Assign("Session fixture object");
    world.CreateObject(object);
    xiiWorldWriter writer;
    writer.WriteWorld(file, world);
  };
  writeProject("Project A");
  writeProject("Project B");
  const xiiStringBuilder projectA(output, "/Project A");
  const xiiStringBuilder projectB(output, "/Project B");
  xiiQtPlayerSession     sessions;
  auto                   waitFor = [&](xiiUInt32 index, xiiStringView expected, double seconds = 45.0) {
    const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(seconds);
    while (xiiTime::Now() < deadline)
    {
      QApplication::processEvents();
      sessions.Tick();
      if (sessions.GetInstanceState(index) == expected)
        return true;
      if (sessions.GetInstanceState(index) == "failed" && expected != "failed")
      {
        xiiLog::Error("Player {} failed: {}\n{}", index, sessions.GetInstanceDetail(index), sessions.GetInstanceLog(index));
        return false;
      }
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    }
    xiiLog::Error("Player {} timed out in state '{}': {}", index, sessions.GetInstanceState(index), sessions.GetInstanceDetail(index));
    return false;
  };
  xiiPlayerLaunchConfig launch;
  // No -scene override: exercise the project manifest startup scene.
  launch.m_uiInstances = 2;
  launch.m_Arguments.PushBack("-profile");
  launch.m_Arguments.PushBack("Default");
  launch.m_Arguments.PushBack("-renderer");
  launch.m_Arguments.PushBack("Vulkan");
  launch.m_Arguments.PushBack("-App.FrameLimit");
  launch.m_Arguments.PushBack("30");
  launch.m_Arguments.PushBack("-window-mode");
  launch.m_Arguments.PushBack("window");
  launch.m_Arguments.PushBack("-width");
  launch.m_Arguments.PushBack("640");
  launch.m_Arguments.PushBack("-height");
  launch.m_Arguments.PushBack("360");

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Two real Players become ready; individual stop preserves another project")
  {
    const auto result = sessions.Launch(launch, projectA, {});
    if (!XII_TEST_BOOL_MSG(result.Succeeded(), "{}", result.GetMessageString()))
      return;
    XII_TEST_BOOL(waitFor(0, "running"));
    XII_TEST_BOOL(waitFor(1, "running"));
    XII_TEST_BOOL(sessions.GetInstanceId(0) != sessions.GetInstanceId(1));
    XII_TEST_STRING(sessions.GetSessionId(0), sessions.GetSessionId(1));
    launch.m_uiInstances = 1;
    const auto other     = sessions.Launch(launch, projectB, {});
    if (!XII_TEST_BOOL_MSG(other.Succeeded(), "{}", other.GetMessageString()))
      return;
    XII_TEST_BOOL(waitFor(2, "running"));
    // Runtime transitions preserve the current scene on failure and commit the latest request only.
    const xiiStringBuilder nextScene(projectA, "/Next.xiiBinScene");
    XII_TEST_RESULT(xiiOSFile::CopyFile(xiiStringBuilder(projectA, "/Main.xiiBinScene"), nextScene));
    auto revision = sessions.GetInstanceSceneRevision(1);
    XII_TEST_RESULT(sessions.TransitionInstance(1, "Missing.xiiBinScene"));
    XII_TEST_BOOL(waitFor(1, "failed"));
    XII_TEST_INT(sessions.GetInstanceSceneRevision(1), revision);
    XII_TEST_RESULT(sessions.TransitionInstance(1, "Next.xiiBinScene"));
    const auto transitionDeadline = xiiTime::Now() + xiiTime::MakeFromSeconds(20);
    while (sessions.GetInstanceSceneRevision(1) == revision && xiiTime::Now() < transitionDeadline)
    {
      sessions.Tick();
      QApplication::processEvents();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    }
    XII_TEST_BOOL(waitFor(1, "running"));
    XII_TEST_INT(sessions.GetInstanceSceneRevision(1), revision + 1);
    XII_TEST_BOOL(sessions.GetInstanceDetail(1).FindSubString("Next.xiiBinScene") != nullptr);
    revision = sessions.GetInstanceSceneRevision(1);
    XII_TEST_RESULT(sessions.TransitionInstance(1, "Main.xiiBinScene"));
    XII_TEST_RESULT(sessions.CancelTransition(1));
    const auto cancellationDeadline = xiiTime::Now() + xiiTime::MakeFromMilliseconds(700);
    while (xiiTime::Now() < cancellationDeadline)
    {
      sessions.Tick();
      QApplication::processEvents();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    }
    XII_TEST_INT(sessions.GetInstanceSceneRevision(1), revision);
    XII_TEST_STRING(sessions.GetInstanceState(1), "running");
    XII_TEST_RESULT(sessions.TransitionInstance(1, "Next.xiiBinScene"));
    XII_TEST_RESULT(sessions.TransitionInstance(1, "Main.xiiBinScene"));
    const auto overlapDeadline = xiiTime::Now() + xiiTime::MakeFromSeconds(20);
    while (sessions.GetInstanceSceneRevision(1) == revision && xiiTime::Now() < overlapDeadline)
    {
      sessions.Tick();
      QApplication::processEvents();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    }
    XII_TEST_BOOL(waitFor(1, "running"));
    XII_TEST_INT(sessions.GetInstanceSceneRevision(1), revision + 1);
    XII_TEST_BOOL(sessions.GetInstanceDetail(1).FindSubString("Main.xiiBinScene") != nullptr);
    const xiiString firstSession = sessions.GetSessionId(0);
    sessions.StopInstance(0);
    XII_TEST_BOOL(waitFor(0, "exited", 10));
    XII_TEST_STRING(sessions.GetInstanceState(1), "running");
    XII_TEST_STRING(sessions.GetInstanceState(2), "running");
    sessions.StopSession(firstSession);
    XII_TEST_BOOL(waitFor(1, "exited", 10));
    XII_TEST_STRING(sessions.GetInstanceState(2), "running");
    sessions.StopAll();
    XII_TEST_BOOL(waitFor(2, "exited", 10));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Private settings/logs and repeated startup cancellation")
  {
    xiiStringBuilder key;
    key.SetFormat("{}", xiiArgU(xiiHashingUtils::StringHash(projectA), 16, true, 16));
    const xiiStringBuilder directory(xiiOSFile::GetUserDataFolder(), "/xiiPlayer/", key, "/Sessions/");
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      const xiiStringBuilder log(directory, sessions.GetInstanceId(i), "/Player.html");
      XII_TEST_BOOL_MSG(xiiOSFile::ExistsFile(log), "Missing instance log: {}", log);
    }
    for (xiiUInt32 cycle = 0; cycle < 3; ++cycle)
    {
      const auto index = sessions.GetInstanceCount();
      XII_TEST_BOOL(sessions.Launch(launch, projectA, {}).Succeeded());
      sessions.StopInstance(index);
      XII_TEST_BOOL(waitFor(index, "exited", 10));
    }
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Early process failure is attributed and reserved arguments are rejected")
  {
    xiiPlayerLaunchConfig peer;
    peer.m_sApplication = xiiStringBuilder(xiiOSFile::GetApplicationDirectory(), "/EditorFrameworkTest.exe");
    peer.m_Arguments.PushBack("-playerSessionExitPeer");
    const auto index = sessions.GetInstanceCount();
    XII_TEST_BOOL(sessions.Launch(peer, projectA, {}).Succeeded());
    XII_TEST_BOOL(waitFor(index, "failed", 10));
    XII_TEST_BOOL(sessions.GetInstanceDetail(index).FindSubString("23") != nullptr);
    peer.m_Arguments.PushBack("-player-ipc");
    peer.m_Arguments.PushBack("another-session");
    XII_TEST_BOOL(sessions.Launch(peer, projectA, {}).Failed());
    XII_TEST_INT(sessions.GetInstanceCount(), index + 1);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Application-initiated shutdown is not misclassified as lost control")
  {
    xiiPlayerLaunchConfig peer;
    peer.m_sApplication = xiiStringBuilder(xiiOSFile::GetApplicationDirectory(), "/EditorFrameworkTest.exe");
    peer.m_Arguments.PushBack("-playerSessionCleanExitPeer");
    const auto index = sessions.GetInstanceCount();
    XII_TEST_BOOL(sessions.Launch(peer, projectA, {}).Succeeded());
    XII_TEST_BOOL(waitFor(index, "exited", 10));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Unresponsive owned child is stopped within the shutdown deadline")
  {
    xiiPlayerLaunchConfig peer;
    peer.m_sApplication = xiiStringBuilder(xiiOSFile::GetApplicationDirectory(), "/EditorFrameworkTest.exe");
    peer.m_Arguments.PushBack("-playerSessionIgnoreStopPeer");
    XII_TEST_BOOL(sessions.Launch(peer, projectA, {}).Succeeded());
    const auto begin = xiiTime::Now();
    sessions.Shutdown();
    XII_TEST_BOOL((xiiTime::Now() - begin).GetSeconds() < 7.5);
    XII_TEST_INT(sessions.GetInstanceCount(), 0);
  }
}
