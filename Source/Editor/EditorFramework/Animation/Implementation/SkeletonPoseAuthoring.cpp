/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Animation/SkeletonPoseAuthoring.h>
#include <Foundation/Containers/HashSet.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

namespace
{
  xiiResult DecomposeRoot(const xiiMat4& matrix, xiiTransform& out_pose)
  {
    if (!matrix.IsValid())
      return XII_FAILURE;
    auto rotation = matrix.GetRotationalPart();
    auto scale    = rotation.GetScalingFactors();
    if (!scale.IsValid() || scale.x <= 0 || scale.y <= 0 || scale.z <= 0)
      return XII_FAILURE;
    // Skeleton import coordinates may reflect an axis. Keep the reflection in scale, not the quaternion.
    if (rotation.GetDeterminant() < 0)
      scale.x = -scale.x;
    rotation.SetColumn(0, rotation.GetColumn(0) / scale.x);
    rotation.SetColumn(1, rotation.GetColumn(1) / scale.y);
    rotation.SetColumn(2, rotation.GetColumn(2) / scale.z);
    if (!xiiMath::IsEqual(rotation.GetDeterminant(), 1.0f, 0.001f) ||
        !xiiMath::IsZero(rotation.GetColumn(0).Dot(rotation.GetColumn(1)), 0.001f) ||
        !xiiMath::IsZero(rotation.GetColumn(0).Dot(rotation.GetColumn(2)), 0.001f) ||
        !xiiMath::IsZero(rotation.GetColumn(1).Dot(rotation.GetColumn(2)), 0.001f))
      return XII_FAILURE;
    out_pose = xiiTransform::Make(matrix.GetTranslationVector(), xiiQuat::MakeFromMat3(rotation), scale);
    return out_pose.IsValid() ? XII_SUCCESS : XII_FAILURE;
  }
} // namespace

xiiResult xiiResolveSkeletonPoseForAuthoring(xiiArrayPtr<const xiiExposedBone>      metadata,
                                             const xiiMap<xiiString, xiiTransform>& overrides, xiiDynamicArray<xiiExposedBone>& out_bones)
{
  out_bones.Clear();
  xiiTransform root    = xiiTransform::MakeIdentity();
  bool         hasRoot = false;
  for (const auto& bone : metadata)
  {
    if (!bone.m_Transform.IsValid())
      return XII_FAILURE;
    if (bone.m_sName == "<root-transform>")
    {
      if (hasRoot || !bone.m_sParent.IsEmpty())
        return XII_FAILURE;
      root    = bone.m_Transform;
      hasRoot = true;
    }
  }
  for (const auto& transform : overrides)
    if (transform.Key().IsEmpty() || !transform.Value().IsValid())
      return XII_FAILURE;

  xiiDynamicArray<xiiExposedBone> bones;
  for (const auto& bone : metadata)
  {
    if (bone.m_sName == "<root-transform>")
      continue;
    auto resolved = bone;
    if (const auto* transform = overrides.GetValue(bone.m_sName))
      resolved.m_Transform = *transform;
    else if (bone.m_sParent.IsEmpty())
      XII_SUCCEED_OR_RETURN(DecomposeRoot(root.GetAsMat4() * bone.m_Transform.GetAsMat4(), resolved.m_Transform));
    bones.PushBack(std::move(resolved));
  }
  xiiDynamicArray<xiiMat4> parents;
  XII_SUCCEED_OR_RETURN(xiiExposedBone::ComputeParentTransforms(bones, parents));
  out_bones = std::move(bones);
  return XII_SUCCESS;
}

xiiStatus xiiApplySkeletonPoseForAuthoring(xiiObjectAccessorBase& accessor, const xiiDocumentObject* pComponent,
                                           const xiiMap<xiiString, xiiTransform>& transforms)
{
  if (pComponent == nullptr)
    return xiiStatus("Cannot apply a pose without a component.");
  const auto* attribute = pComponent->GetType()->GetAttributeByType<xiiBoneManipulatorAttribute>();
  const auto* property  = attribute != nullptr ? pComponent->GetType()->FindPropertyByName(attribute->GetTransformProperty()) : nullptr;
  if (property == nullptr || property->GetCategory() != xiiPropertyCategory::Map || property->GetSpecificType() != xiiGetStaticRTTI<xiiTransform>())
    return xiiStatus("The component does not expose a native bone transform map.");
  for (const auto& transform : transforms)
    if (transform.Key().IsEmpty() || !transform.Value().IsValid())
      return xiiStatus("Cannot apply a pose with empty joint names or invalid transforms.");

  xiiVariantArray keyValues;
  XII_SUCCEED_OR_RETURN(accessor.GetKeys(pComponent, property, keyValues));
  xiiHashSet<xiiString> keys;
  for (const auto& key : keyValues)
  {
    if (!key.IsA<xiiString>())
      return xiiStatus("The bone transform map has a non-string key.");
    keys.Insert(key.Get<xiiString>());
  }
  for (const auto& transform : transforms)
  {
    if (keys.Contains(transform.Key()))
      XII_SUCCEED_OR_RETURN(accessor.SetValue(pComponent, property, transform.Value(), transform.Key()));
    else
      XII_SUCCEED_OR_RETURN(accessor.InsertValue(pComponent, property, transform.Value(), transform.Key()));
  }
  return XII_SUCCESS;
}
