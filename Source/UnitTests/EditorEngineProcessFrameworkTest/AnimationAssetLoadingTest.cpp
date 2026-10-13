/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <TestFramework/Utilities/TestLogInterface.h>

XII_CREATE_SIMPLE_TEST(EditorViews, AnimationAssetLoading)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiAssetFileHeader header;
  header.SetFileHashAndVersion(12345, 10);
  auto write = [&](xiiStringView path, const auto& descriptor) {
    xiiFileWriter file;
    if (!XII_TEST_RESULT(file.Open(path)))
      return;
    XII_TEST_RESULT(header.Write(file));
    XII_TEST_RESULT(descriptor.Serialize(file));
    file.Close();
  };
  const char*                   skeletonPath = ":imgout/NativeAnimation/Character.xiiBinSkeleton";
  const char*                   clipPath     = ":imgout/NativeAnimation/Walk.xiiBinAnimationClip";
  const char*                   graphPath    = ":imgout/NativeAnimation/Locomotion.xiiBinAnimGraph";
  xiiSkeletonResourceDescriptor skeleton;
  skeleton.AddJoint("Root", xiiMath::MaxValue<xiiUInt16>(), xiiTransform::Make(xiiVec3(1, 2, 3)));
  skeleton.BuildModelSpaceRestPose();
  write(skeletonPath, skeleton);
  auto hSkeleton = xiiResourceManager::LoadResource<xiiSkeletonResource>(skeletonPath);
  {
    xiiResourceLock<xiiSkeletonResource> loaded(hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (XII_TEST_BOOL(loaded && loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
    {
      XII_TEST_INT(loaded->GetJointCount(), 1);
      XII_TEST_VEC3(loaded->GetJoint(0).m_LocalRestPose.m_vPosition, xiiVec3(1, 2, 3), 0.0001f);
    }
  }
  xiiAnimationClipResourceDescriptor clip;
  clip.m_Duration = xiiTime::MakeFromSeconds(2);
  clip.AddJointTrack("Root").m_PositionKeys.PushBack({0, xiiVec3(1, 2, 3)});
  write(clipPath, clip);
  auto      hClip    = xiiResourceManager::LoadResource<xiiAnimationClipResource>(clipPath);
  xiiUInt32 clipHash = 0;
  {
    xiiResourceLock<xiiAnimationClipResource> loaded(hClip, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (XII_TEST_BOOL(loaded && loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
    {
      XII_TEST_DOUBLE(loaded->GetDuration().GetSeconds(), 2, 0.0001);
      clipHash = loaded->GetRuntimeHash();
    }
  }
  clip.m_JointTracks[0].m_PositionKeys[0].m_Value.x = 4;
  write(clipPath, clip);
  XII_TEST_BOOL(xiiResourceManager::ReloadResource(hClip, true));
  {
    xiiResourceLock<xiiAnimationClipResource> loaded(hClip, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (XII_TEST_BOOL(loaded && loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
    {
      XII_TEST_BOOL(loaded->GetRuntimeHash() != clipHash);
      clipHash = loaded->GetRuntimeHash();
      XII_TEST_VEC3(loaded->GetDescriptor().m_JointTracks[0].m_PositionKeys[0].m_Value, xiiVec3(4, 2, 3), 0);
    }
  }
  auto& event   = clip.m_Events.ExpandAndGetRef();
  event.m_fTime = 0.5f;
  event.m_sEvent.Assign("Footstep");
  write(clipPath, clip);
  XII_TEST_BOOL(xiiResourceManager::ReloadResource(hClip, true));
  {
    xiiResourceLock<xiiAnimationClipResource> loaded(hClip, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (XII_TEST_BOOL(loaded && loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
    {
      XII_TEST_BOOL(loaded->GetRuntimeHash() != clipHash);
      XII_TEST_INT(loaded->GetDescriptor().m_Events.GetCount(), 1);
      XII_TEST_STRING(loaded->GetDescriptor().m_Events[0].m_sEvent.GetString(), "Footstep");
    }
  }
  xiiAnimGraphResourceDescriptor graph;
  graph.SetOutputNode(graph.AddClipNode("Walk", hClip));
  graph.ComputeRuntimeHash();
  write(graphPath, graph);
  auto hGraph = xiiResourceManager::LoadResource<xiiAnimGraphResource>(graphPath);
  {
    xiiResourceLock<xiiAnimGraphResource> loaded(hGraph, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (XII_TEST_BOOL(loaded && loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
    {
      XII_TEST_INT(loaded->GetDescriptor().m_Nodes.GetCount(), 1);
      XII_TEST_BOOL(loaded->GetDescriptor().m_Nodes[0].m_hClip == hClip);
    }
  }
  // Reload the transformed file through the same registered loader, without any custom memory override.
  skeleton.m_Joints[0].m_LocalRestPose.m_vPosition.Set(4, 5, 6);
  skeleton.BuildModelSpaceRestPose();
  write(skeletonPath, skeleton);
  XII_TEST_BOOL(xiiResourceManager::ReloadResource(hSkeleton, true));
  {
    xiiResourceLock<xiiSkeletonResource> loaded(hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (XII_TEST_BOOL(loaded && loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
      XII_TEST_VEC3(loaded->GetJoint(0).m_LocalRestPose.m_vPosition, xiiVec3(4, 5, 6), 0.0001f);
  }
  // Invalid envelopes fail without triggering the assertion-based AssetFileHeader reader.
  const char* invalidPath = ":imgout/NativeAnimation/Invalid.xiiBinSkeleton";
  {
    xiiFileWriter file;
    XII_TEST_RESULT(file.Open(invalidPath));
    XII_TEST_RESULT(file.WriteBytes("invalid", 7));
    file.Close();
  }
  xiiTestLogInterface   log;
  xiiTestLogSystemScope logScope(&log, true);
  log.ExpectMessage("Missing Resource of Type 'xiiSkeletonResource'", xiiLogMsgType::SeriousWarningMsg);
  auto                                 hInvalid = xiiResourceManager::LoadResource<xiiSkeletonResource>(invalidPath);
  xiiResourceLock<xiiSkeletonResource> resource(hInvalid, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  XII_TEST_BOOL(resource.GetAcquireResult() != xiiResourceAcquireResult::Final);
}
