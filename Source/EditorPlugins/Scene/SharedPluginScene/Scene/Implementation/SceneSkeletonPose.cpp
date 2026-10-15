/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/World/GameObject.h>
#include <GraphicsCore/AnimationSystem/SkeletonComponent.h>
#include <SharedPluginScene/Scene/SceneSkeletonPose.h>

xiiResult xiiCollectSceneSkeletonPose(const xiiGameObject& object, xiiString& out_sSkeletonResource, xiiMap<xiiString, xiiTransform>& out_transforms)
{
  out_sSkeletonResource.Clear();
  out_transforms.Clear();
  const xiiSkeletonComponent* animated = nullptr;
  if (object.TryGetComponentOfBaseType(animated))
  {
    XII_SUCCEED_OR_RETURN(animated->CopyCurrentLocalPose(out_transforms));
    out_sSkeletonResource = animated->GetSkeleton().GetResourceID();
    return XII_SUCCESS;
  }
  const xiiSkeletonPoseComponent* pose = nullptr;
  if (object.TryGetComponentOfBaseType(pose))
  {
    XII_SUCCEED_OR_RETURN(pose->CopyCurrentLocalPose(out_transforms));
    out_sSkeletonResource = pose->GetSkeleton().GetResourceID();
    return XII_SUCCESS;
  }
  return XII_FAILURE;
}
