/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ModelImporter/ModelImporterPCH.h>

#include <Foundation/Containers/HashSet.h>
#include <ModelImporter/Implementation/AnimationImportInternal.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace
{
  xiiResult ExtractClipNames(const aiScene& scene, xiiDynamicArray<xiiString>& out_names, xiiStringBuilder* error)
  {
    using namespace xiiModelImportInternal;
    if (scene.mNumAnimations == 0 || scene.mAnimations == nullptr)
      return Fail(error, "Source file contains no animation clips.");
    xiiHashSet<xiiString>      names;
    xiiDynamicArray<xiiString> result;
    for (xiiUInt32 i = 0; i < scene.mNumAnimations; ++i)
    {
      if (scene.mAnimations[i] == nullptr)
        return Fail(error, "Source file contains an invalid animation.");
      xiiStringBuilder name(scene.mAnimations[i]->mName.C_Str());
      if (name.IsEmpty())
        name.SetFormat("Animation_{}", i);
      if (names.Insert(name))
        return Fail(error, "Source file contains ambiguous duplicate animation names.");
      result.PushBack(name);
    }
    out_names = std::move(result);
    return XII_SUCCESS;
  }
} // namespace

xiiResult xiiModelAnimationImport::ReadClipNames(xiiStringView sSourceFile, xiiDynamicArray<xiiString>& out_names, xiiStringBuilder* out_pError)
{
  out_names.Clear();
  if (out_pError != nullptr)
    out_pError->Clear();
  if (!sSourceFile.IsAbsolutePath())
    return xiiModelImportInternal::Fail(out_pError, "Animation clip discovery requires an absolute filename.");
  Assimp::Importer importer;
  const xiiString  source(sSourceFile);
  const auto*      scene = importer.ReadFile(source.GetData(), aiProcess_ValidateDataStructure);
  if (scene == nullptr)
    return xiiModelImportInternal::Fail(out_pError, importer.GetErrorString());
  return ExtractClipNames(*scene, out_names, out_pError);
}

xiiResult xiiModelAnimationImport::ReadFile(xiiStringView sSourceFile, const xiiModelAnimationImportOptions& options,
                                            xiiModelAnimationImport& out_import, xiiStringBuilder* out_pError)
{
  using namespace xiiModelImportInternal;
  out_import.m_Clip.Clear();
  out_import.m_AvailableClips.Clear();
  if (out_pError != nullptr)
    out_pError->Clear();
  if (!sSourceFile.IsAbsolutePath() || !xiiMath::IsFinite(options.m_fSampleRate) || options.m_fSampleRate < 1 || options.m_fSampleRate > 1000)
    return Fail(out_pError, "Animation import requires an absolute filename and a sample rate between 1 and 1000 FPS.");
  Assimp::Importer importer;
  const xiiString  source(sSourceFile);
  const auto*      scene = importer.ReadFile(source.GetData(), aiProcess_ValidateDataStructure);
  if (scene == nullptr || scene->mRootNode == nullptr)
    return Fail(out_pError, importer.GetErrorString());
  XII_SUCCEED_OR_RETURN(ExtractClipNames(*scene, out_import.m_AvailableClips, out_pError));
  xiiUInt32 selected = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < out_import.m_AvailableClips.GetCount(); ++i)
    if ((options.m_sClipName.IsEmpty() && i == 0) || out_import.m_AvailableClips[i] == options.m_sClipName)
      selected = i;
  if (selected == xiiInvalidIndex)
    return Fail(out_pError, "Selected animation clip does not exist in the source file.");
  const auto& animation = *scene->mAnimations[selected];
  if (!xiiMath::IsFinite(animation.mDuration) || animation.mDuration < 0 ||
      !xiiMath::IsFinite(animation.mTicksPerSecond) || animation.mTicksPerSecond < 0 ||
      animation.mNumChannels == 0 || animation.mChannels == nullptr ||
      animation.mNumMeshChannels != 0 || animation.mNumMorphMeshChannels != 0)
    return Fail(out_pError, "Source animation has invalid timing, missing joint channels or unsupported vertex/morph animation.");
  // Assimp permits zero for unspecified tick rates. Its conventional fallback
  // is independent of the authored output sample rate.
  const double ticksPerSecond = animation.mTicksPerSecond > 0 ? animation.mTicksPerSecond : 25.0;
  const double sourceDuration = animation.mDuration / ticksPerSecond;
  const double begin          = options.m_uiFirstFrame / double(options.m_fSampleRate);
  double       end            = sourceDuration;
  if (options.m_uiNumFrames != 0)
    end = xiiMath::Min(end, begin + (options.m_uiNumFrames - 1) / double(options.m_fSampleRate));
  if (!xiiMath::IsFinite(sourceDuration) || begin > sourceDuration || !xiiMath::IsFinite(static_cast<float>(end - begin)))
    return Fail(out_pError, "Requested animation frame range lies outside the source duration.");

  xiiHashTable<xiiString, xiiTransform> restPoses;
  xiiHashSet<const aiNode*>             visited;
  xiiDynamicArray<const aiNode*>        pending;
  pending.PushBack(scene->mRootNode);
  while (!pending.IsEmpty())
  {
    const auto* node = pending.PeekBack();
    pending.PopBack();
    if (node == nullptr || visited.Insert(node) || node->mName.length == 0 ||
        restPoses.Contains(node->mName.C_Str()) || restPoses.GetCount() == xiiMath::MaxValue<xiiUInt16>())
      return Fail(out_pError, "Source animation hierarchy contains invalid ownership, ambiguous joint names or too many joints.");
    xiiTransform pose;
    if (ConvertPose(node->mTransformation, pose).Failed())
      return Fail(out_pError, "Source animation rest poses contain non-finite, singular or sheared transforms.");
    restPoses.Insert(node->mName.C_Str(), pose);
    if (node->mNumChildren != 0 && node->mChildren == nullptr)
      return Fail(out_pError, "Source hierarchy contains a missing child array.");
    for (xiiUInt32 i = node->mNumChildren; i > 0; --i)
      pending.PushBack(node->mChildren[i - 1]);
  }

  xiiAnimationClipResourceDescriptor result;
  result.m_Duration    = xiiTime::MakeFromSeconds(end - begin);
  result.m_fSampleRate = options.m_fSampleRate;
  result.m_bAdditive   = options.m_bAdditive;
  xiiHashSet<xiiString> animated;
  xiiUInt64             totalKeys = 0;
  for (xiiUInt32 i = 0; i < animation.mNumChannels; ++i)
  {
    const auto*  channel = animation.mChannels[i];
    xiiTransform rest;
    if (channel == nullptr || animated.Insert(channel->mNodeName.C_Str()) || !restPoses.TryGetValue(channel->mNodeName.C_Str(), rest))
      return Fail(out_pError, "Animation channels reference missing or ambiguous joints.");
    auto&                        track = result.AddJointTrack(channel->mNodeName.C_Str());
    const AnimationCurveSettings curveSettings{ticksPerSecond, begin, end, options.m_fSampleRate, channel->mPreState, channel->mPostState,
                                               sSourceFile.HasExtension("gltf") || sSourceFile.HasExtension("glb")};
    XII_SUCCEED_OR_RETURN(BakeVectorKeys(channel->mPositionKeys, channel->mNumPositionKeys, curveSettings,
                                         rest.m_vPosition, options.m_bAdditive, false, track.m_PositionKeys, out_pError));
    XII_SUCCEED_OR_RETURN(BakeRotationKeys(channel->mRotationKeys, channel->mNumRotationKeys, curveSettings,
                                           rest.m_qRotation, options.m_bAdditive, track.m_RotationKeys, out_pError));
    XII_SUCCEED_OR_RETURN(BakeVectorKeys(channel->mScalingKeys, channel->mNumScalingKeys, curveSettings,
                                         rest.m_vScale, options.m_bAdditive, true, track.m_ScaleKeys, out_pError));
    totalKeys += track.m_PositionKeys.GetCount() + xiiUInt64(track.m_RotationKeys.GetCount()) + track.m_ScaleKeys.GetCount();
    if (totalKeys > 16000000)
      return Fail(out_pError, "Animation exceeds sixteen million output keys.");
  }
  result.ComputeRuntimeHash();
  out_import.m_Clip = std::move(result);
  return XII_SUCCESS;
}
