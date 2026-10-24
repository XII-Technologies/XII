/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <ModelImporter/AnimationImport.h>
#include <cmath>

XII_CREATE_SIMPLE_TEST(EditorViews, AnimationImport)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiStringBuilder folder(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/AnimationImport-骨");
  if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(folder)))
    return;
  const xiiStringBuilder source(folder, "/Clips.gltf"), bufferPath(folder, "/Keys.bin");
  const char*            json  = R"JSON({
    "asset":{"version":"2.0"}, "scene":0, "scenes":[{"nodes":[0]}],
    "nodes":[{"name":"Root","translation":[1,2,3],"scale":[2,3,4],"children":[1]},
             {"name":"Static","translation":[0,7,0]}],
    "buffers":[{"uri":"Keys.bin","byteLength":132}],
    "bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":12},
                   {"buffer":0,"byteOffset":12,"byteLength":36},
                   {"buffer":0,"byteOffset":48,"byteLength":48},
                   {"buffer":0,"byteOffset":96,"byteLength":36}],
    "accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"SCALAR","min":[0],"max":[2]},
                 {"bufferView":1,"componentType":5126,"count":3,"type":"VEC3"},
                 {"bufferView":2,"componentType":5126,"count":3,"type":"VEC4"},
                 {"bufferView":3,"componentType":5126,"count":3,"type":"VEC3"}],
    "animations":[
      {"name":"移动","samplers":[{"input":0,"output":1,"interpolation":"LINEAR"},
                                {"input":0,"output":2,"interpolation":"LINEAR"},
                                {"input":0,"output":3,"interpolation":"LINEAR"}],
       "channels":[{"sampler":0,"target":{"node":0,"path":"translation"}},
                   {"sampler":1,"target":{"node":0,"path":"rotation"}},
                   {"sampler":2,"target":{"node":0,"path":"scale"}}]},
      {"name":"Other","samplers":[{"input":0,"output":1}],
       "channels":[{"sampler":0,"target":{"node":0,"path":"translation"}}]}
    ]
  })JSON";
  auto                   write = [&](xiiStringView text, bool cubic = false, bool cubicRotation = false) -> xiiResult {
    xiiOSFile binary;
    XII_SUCCEED_OR_RETURN(binary.Open(bufferPath, xiiFileOpenMode::Write));
    const float times[]          = {0, 1, 2};
    const float positions[]      = {1, 2, 3, 3, 2, 3, 5, 2, 3};
    const float cubicPositions[] = {0, 0, 0, 1, 2, 3, 4, 0, 0,
                                    0, 0, 0, 3, 2, 3, 0, 0, 0,
                                    0, 0, 0, 5, 2, 3, 0, 0, 0};
    const float rotations[]      = {0, 0, 0, 1, 0, 0, 0.70710678f, 0.70710678f, 0, 0, 1, 0};
    const float cubicRotations[] = {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0,
                                    0, 0, 0, 0, 0, 0, 0.70710678f, 0.70710678f, 0, 0, 0, 0,
                                    0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0};
    const float scales[]         = {2, 3, 4, 4, 6, 8, 6, 9, 12};
    XII_SUCCEED_OR_RETURN(binary.Write(times, sizeof(times)));
    XII_SUCCEED_OR_RETURN(binary.Write(cubic ? cubicPositions : positions, cubic ? sizeof(cubicPositions) : sizeof(positions)));
    XII_SUCCEED_OR_RETURN(binary.Write(cubicRotation ? cubicRotations : rotations, cubicRotation ? sizeof(cubicRotations) : sizeof(rotations)));
    XII_SUCCEED_OR_RETURN(binary.Write(scales, sizeof(scales)));
    binary.Close();
    xiiOSFile file;
    XII_SUCCEED_OR_RETURN(file.Open(source, xiiFileOpenMode::Write));
    return file.Write(text.GetStartPointer(), text.GetElementCount());
  };
  if (!XII_TEST_RESULT(write(json)))
    return;
  xiiModelAnimationImportOptions options;
  xiiModelAnimationImport        imported;
  xiiStringBuilder               error;
  auto                           read = [&]() -> bool {
    const auto result = xiiModelAnimationImport::ReadFile(source, options, imported, &error);
    return XII_TEST_BOOL_MSG(result.Succeeded(), "%s", error.GetData());
  };
  if (!read())
    return;
  xiiDynamicArray<xiiString> discovered;
  XII_TEST_RESULT(xiiModelAnimationImport::ReadClipNames(source, discovered, &error));
  XII_TEST_BOOL(discovered == imported.m_AvailableClips);
  XII_TEST_INT(imported.m_AvailableClips.GetCount(), 2);
  XII_TEST_STRING(imported.m_AvailableClips[0], "移动");
  XII_TEST_DOUBLE(imported.m_Clip.m_Duration.GetSeconds(), 2, 0.0001);
  XII_TEST_FLOAT(imported.m_Clip.m_fSampleRate, 30, 0);
  if (!XII_TEST_INT(imported.m_Clip.m_JointTracks.GetCount(), 1))
    return;
  const auto& track = imported.m_Clip.m_JointTracks[0];
  XII_TEST_STRING(track.m_sJointName.GetString(), "Root");
  XII_TEST_INT(track.m_uiJointIndex, xiiMath::MaxValue<xiiUInt16>());
  XII_TEST_INT(track.m_PositionKeys.GetCount(), 3);
  XII_TEST_VEC3(track.m_PositionKeys[1].m_Value, xiiVec3(3, 2, 3), 0.0001f);
  XII_TEST_VEC3(track.m_ScaleKeys[2].m_Value, xiiVec3(6, 9, 12), 0.0001f);
  XII_TEST_BOOL(track.m_RotationKeys[1].m_Value.IsEqualRotation(xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(90)), 0.001f));

  xiiSkeletonResourceDescriptor skeletonDescription;
  skeletonDescription.AddJoint("Static", xiiMath::MaxValue<xiiUInt16>(), xiiTransform::Make(xiiVec3(0, 7, 0)));
  skeletonDescription.AddJoint("Root", xiiMath::MaxValue<xiiUInt16>(), xiiTransform::Make(xiiVec3(1, 2, 3), xiiQuat::MakeIdentity(), xiiVec3(2, 3, 4)));
  skeletonDescription.BuildModelSpaceRestPose();
  const auto                           skeletonHandle = xiiResourceManager::CreateResource<xiiSkeletonResource>("ImportedAnimationSkeleton", std::move(skeletonDescription));
  xiiResourceLock<xiiSkeletonResource> skeleton(skeletonHandle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(skeleton.GetPointer() != nullptr))
    return;
  xiiUInt32 revision = 0;
  auto      sample   = [&](float seconds, xiiAnimationPose& pose) -> bool {
    auto description       = imported.m_Clip;
    description.m_bLooping = false;
    xiiStringBuilder id;
    id.SetFormat("ImportedAnimationClip{}", revision++);
    const auto                                handle = xiiResourceManager::CreateResource<xiiAnimationClipResource>(id, std::move(description));
    xiiResourceLock<xiiAnimationClipResource> resource(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(resource.GetPointer() != nullptr))
      return false;
    resource->SampleLocalPose(*skeleton.GetPointer(), xiiTime::MakeFromSeconds(seconds), pose);
    return true;
  };
  xiiAnimationPose pose;
  if (!sample(0.5f, pose))
    return;
  XII_TEST_VEC3(pose.m_LocalTransforms[1].m_vPosition, xiiVec3(2, 2, 3), 0.0001f);
  XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(0, 7, 0), 0.0001f);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Imported native asset round trip")
  {
    const char*   path = ":imgout/AnimationImport/Imported.xiiBinAnimationClip";
    xiiFileWriter file;
    if (!XII_TEST_RESULT(file.Open(path)))
      return;
    xiiAssetFileHeader header;
    header.SetFileHashAndVersion(123, 6);
    if (!XII_TEST_RESULT(header.Write(file)) || !XII_TEST_RESULT(imported.m_Clip.Serialize(file)))
      return;
    file.Close();
    const auto                                handle = xiiResourceManager::LoadResource<xiiAnimationClipResource>(path);
    xiiResourceLock<xiiAnimationClipResource> loaded(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(loaded && loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
      return;
    XII_TEST_INT(loaded->GetRuntimeHash(), imported.m_Clip.m_uiRuntimeHash);
    loaded->SampleLocalPose(*skeleton.GetPointer(), xiiTime::MakeFromSeconds(0.5), pose);
    XII_TEST_VEC3(pose.m_LocalTransforms[1].m_vPosition, xiiVec3(2, 2, 3), 0.0001f);
    XII_TEST_VEC3(pose.m_LocalTransforms[1].m_vScale, xiiVec3(3, 4.5f, 6), 0.0001f);
    XII_TEST_VEC3(pose.m_LocalTransforms[0].m_vPosition, xiiVec3(0, 7, 0), 0.0001f);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Frame trimming, clip selection and single-frame clips")
  {
    options.m_fSampleRate  = 2;
    options.m_uiFirstFrame = 1;
    options.m_uiNumFrames  = 3;
    if (!read())
      return;
    XII_TEST_DOUBLE(imported.m_Clip.m_Duration.GetSeconds(), 1, 0.0001);
    XII_TEST_VEC3(imported.m_Clip.m_JointTracks[0].m_PositionKeys[0].m_Value, xiiVec3(2, 2, 3), 0.0001f);
    XII_TEST_VEC3(imported.m_Clip.m_JointTracks[0].m_PositionKeys.PeekBack().m_Value, xiiVec3(4, 2, 3), 0.0001f);
    options.m_uiNumFrames = 1;
    if (!read())
      return;
    XII_TEST_DOUBLE(imported.m_Clip.m_Duration.GetSeconds(), 0, 0);
    XII_TEST_INT(imported.m_Clip.m_JointTracks[0].m_PositionKeys.GetCount(), 1);
    options             = {};
    options.m_sClipName = "Other";
    if (!read())
      return;
    XII_TEST_VEC3(imported.m_Clip.m_JointTracks[0].m_ScaleKeys[0].m_Value, xiiVec3(2, 3, 4), 0.0001f);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "STEP boundaries retain native float sampling semantics")
  {
    options = {};
    xiiStringBuilder step(json);
    step.ReplaceAll("\"output\":1,\"interpolation\":\"LINEAR\"", "\"output\":1,\"interpolation\":\"STEP\"");
    if (!XII_TEST_RESULT(write(step)) || !read())
      return;
    if (!sample(std::nextafter(1.0f, 0.0f), pose))
      return;
    XII_TEST_VEC3(pose.m_LocalTransforms[1].m_vPosition, xiiVec3(1, 2, 3), 0);
    if (!sample(1, pose))
      return;
    XII_TEST_VEC3(pose.m_LocalTransforms[1].m_vPosition, xiiVec3(3, 2, 3), 0);
    options.m_fSampleRate  = 2;
    options.m_uiFirstFrame = 1;
    options.m_uiNumFrames  = 3;
    if (!read() || !sample(0.25f, pose))
      return;
    XII_TEST_VEC3(pose.m_LocalTransforms[1].m_vPosition, xiiVec3(1, 2, 3), 0);
    if (!sample(0.5f, pose))
      return;
    XII_TEST_VEC3(pose.m_LocalTransforms[1].m_vPosition, xiiVec3(3, 2, 3), 0);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Cubic Hermite values bake at authored frame times")
  {
    options               = {};
    options.m_fSampleRate = 2;
    xiiStringBuilder cubic(json);
    cubic.ReplaceAll("\"byteLength\":132", "\"byteLength\":204");
    cubic.ReplaceAll("\"byteOffset\":12,\"byteLength\":36", "\"byteOffset\":12,\"byteLength\":108");
    cubic.ReplaceAll("\"byteOffset\":48", "\"byteOffset\":120");
    cubic.ReplaceAll("\"byteOffset\":96", "\"byteOffset\":168");
    cubic.ReplaceAll("\"bufferView\":1,\"componentType\":5126,\"count\":3", "\"bufferView\":1,\"componentType\":5126,\"count\":9");
    cubic.ReplaceAll("\"output\":1,\"interpolation\":\"LINEAR\"", "\"output\":1,\"interpolation\":\"CUBICSPLINE\"");
    // The second clip shares this accessor and therefore needs the same interpolation.
    cubic.ReplaceAll("\"input\":0,\"output\":1}", "\"input\":0,\"output\":1,\"interpolation\":\"CUBICSPLINE\"}");
    if (!XII_TEST_RESULT(write(cubic, true)) || !read() || !sample(0.5f, pose))
      return;
    XII_TEST_INT(imported.m_Clip.m_JointTracks[0].m_PositionKeys.GetCount(), 5);
    XII_TEST_VEC3(pose.m_LocalTransforms[1].m_vPosition, xiiVec3(2.5f, 2, 3), 0.0001f);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Quaternion STEP and cubic component tangents")
  {
    options = {};
    xiiStringBuilder step(json);
    step.ReplaceAll("\"output\":2,\"interpolation\":\"LINEAR\"", "\"output\":2,\"interpolation\":\"STEP\"");
    if (!XII_TEST_RESULT(write(step)) || !read() || !sample(std::nextafter(1.0f, 0.0f), pose))
      return;
    XII_TEST_BOOL(pose.m_LocalTransforms[1].m_qRotation.IsEqualRotation(xiiQuat::MakeIdentity(), 0.0001f));
    if (!sample(1, pose))
      return;
    XII_TEST_BOOL(pose.m_LocalTransforms[1].m_qRotation.IsEqualRotation(xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(90)), 0.0001f));
    xiiStringBuilder cubic(json);
    cubic.ReplaceAll("\"byteLength\":132", "\"byteLength\":228");
    cubic.ReplaceAll("\"byteOffset\":48,\"byteLength\":48", "\"byteOffset\":48,\"byteLength\":144");
    cubic.ReplaceAll("\"byteOffset\":96", "\"byteOffset\":192");
    cubic.ReplaceAll("\"bufferView\":2,\"componentType\":5126,\"count\":3", "\"bufferView\":2,\"componentType\":5126,\"count\":9");
    cubic.ReplaceAll("\"output\":2,\"interpolation\":\"LINEAR\"", "\"output\":2,\"interpolation\":\"CUBICSPLINE\"");
    options.m_fSampleRate = 2;
    if (!XII_TEST_RESULT(write(cubic, false, true)) || !read() || !sample(0.5f, pose))
      return;
    auto expected = xiiQuat::MakeFromElements(0, 0, 0.47855339f, 0.85355339f);
    expected.Normalize();
    XII_TEST_BOOL(pose.m_LocalTransforms[1].m_qRotation.IsEqualRotation(expected, 0.0001f));
    for (const auto& key : imported.m_Clip.m_JointTracks[0].m_RotationKeys)
      XII_TEST_FLOAT(key.m_Value.Dot(key.m_Value), 1, 0.0001f);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Additive deltas and unanimated joints")
  {
    if (!XII_TEST_RESULT(write(json)))
      return;
    options             = {};
    options.m_bAdditive = true;
    if (!read() || !sample(1, pose))
      return;
    XII_TEST_BOOL(imported.m_Clip.m_bAdditive);
    XII_TEST_VEC3(pose.m_LocalTransforms[1].m_vPosition, xiiVec3(2, 0, 0), 0.0001f);
    XII_TEST_VEC3(pose.m_LocalTransforms[1].m_vScale, xiiVec3(2), 0.0001f);
    XII_TEST_BOOL(pose.m_LocalTransforms[0].IsEqual(xiiTransform::MakeIdentity(), 0.0001f));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Failure clears clip data and retains discovered clip metadata")
  {
    options             = {};
    options.m_sClipName = "Missing";
    XII_TEST_BOOL(xiiModelAnimationImport::ReadFile(source, options, imported, &error).Failed());
    XII_TEST_BOOL(!error.IsEmpty() && imported.m_Clip.m_JointTracks.IsEmpty());
    XII_TEST_INT(imported.m_AvailableClips.GetCount(), 2);
    options                = {};
    options.m_uiFirstFrame = 1000;
    XII_TEST_BOOL(xiiModelAnimationImport::ReadFile(source, options, imported, &error).Failed());
    options               = {};
    options.m_fSampleRate = xiiMath::NaN<float>();
    XII_TEST_BOOL(xiiModelAnimationImport::ReadFile(source, options, imported, &error).Failed());
    XII_TEST_BOOL(imported.m_Clip.m_JointTracks.IsEmpty() && imported.m_AvailableClips.IsEmpty());
    options = {};
    XII_TEST_BOOL(xiiModelAnimationImport::ReadFile("Relative.gltf", options, imported, &error).Failed());
    xiiStringBuilder duplicate(json);
    duplicate.ReplaceAll("\"Other\"", "\"移动\"");
    if (!XII_TEST_RESULT(write(duplicate)))
      return;
    XII_TEST_RESULT(xiiModelAnimationImport::ReadFile(source, options, imported, &error));
    XII_TEST_RESULT(xiiModelAnimationImport::ReadClipNames(source, discovered, &error));
    XII_TEST_INT(discovered.GetCount(), 2);
    XII_TEST_STRING(discovered[1], "移动_1");
    options.m_sClipName = discovered[1];
    XII_TEST_RESULT(xiiModelAnimationImport::ReadFile(source, options, imported, &error));
  }
}
