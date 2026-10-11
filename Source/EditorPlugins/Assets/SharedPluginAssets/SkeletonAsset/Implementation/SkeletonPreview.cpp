/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <Foundation/Containers/HashSet.h>
#include <SharedPluginAssets/SkeletonAsset/SkeletonPreview.h>

namespace
{
  constexpr xiiUInt32 s_uiSegments = 32;

  void AddLine(xiiGeometry& geometry, const xiiMat4& transform, const xiiVec3& start, const xiiVec3& end, const xiiColor& color)
  {
    const auto first  = geometry.AddVertex(transform.TransformPosition(start), xiiVec3(1, 0, 0), xiiVec2::MakeZero(), color);
    const auto second = geometry.AddVertex(transform.TransformPosition(end), xiiVec3(1, 0, 0), xiiVec2::MakeZero(), color);
    geometry.AddLine(first, second);
  }

  void AddArc(xiiGeometry& geometry, const xiiMat4& transform, const xiiVec3& center, const xiiVec3& firstAxis,
              const xiiVec3& secondAxis, float start, float end, const xiiColor& color)
  {
    auto previous = center + firstAxis * xiiMath::Cos(xiiAngle::MakeFromRadian(start)) + secondAxis * xiiMath::Sin(xiiAngle::MakeFromRadian(start));
    for (xiiUInt32 i = 1; i <= s_uiSegments; ++i)
    {
      const auto angle = xiiAngle::MakeFromRadian(xiiMath::Lerp(start, end, static_cast<float>(i) / s_uiSegments));
      const auto next  = center + firstAxis * xiiMath::Cos(angle) + secondAxis * xiiMath::Sin(angle);
      AddLine(geometry, transform, previous, next, color);
      previous = next;
    }
  }

  bool IsFiniteNonNegative(float value)
  {
    return xiiMath::IsFinite(value) && value >= 0;
  }

  xiiResult AddColliders(xiiGeometry& geometry, const xiiEditableSkeletonJoint& joint, const xiiMat4& model, float boneLength, const xiiQuat& boneDirection, const xiiColor& color)
  {
    for (const auto& shape : joint.m_BoneShapes)
    {
      if (shape.m_Geometry == xiiSkeletonJointGeometryType::None)
        continue;
      if (!shape.m_vOffset.IsValid() || !shape.m_qRotation.IsValid() || !IsFiniteNonNegative(shape.m_fLength) ||
          !IsFiniteNonNegative(shape.m_fWidth) || !IsFiniteNonNegative(shape.m_fThickness))
        return XII_FAILURE;
      const float length    = shape.m_fLength > 0 ? shape.m_fLength : boneLength;
      const float radius    = shape.m_fThickness;
      const auto  transform = model * xiiTransform::Make(shape.m_vOffset, shape.m_qRotation * boneDirection).GetAsMat4();
      if (!transform.IsValid())
        return XII_FAILURE;

      if (shape.m_Geometry == xiiSkeletonJointGeometryType::Box)
      {
        xiiGeometry::GeoOptions options;
        options.m_Transform = transform * xiiMat4::MakeTranslation(xiiVec3(length * 0.5f, 0, 0));
        options.m_Color     = color;
        geometry.AddLineBox(xiiVec3(length, shape.m_fWidth, radius), options);
      }
      else if (shape.m_Geometry == xiiSkeletonJointGeometryType::Sphere)
      {
        AddArc(geometry, transform, xiiVec3::MakeZero(), xiiVec3(radius, 0, 0), xiiVec3(0, radius, 0), 0, xiiMath::Pi<float>() * 2, color);
        AddArc(geometry, transform, xiiVec3::MakeZero(), xiiVec3(radius, 0, 0), xiiVec3(0, 0, radius), 0, xiiMath::Pi<float>() * 2, color);
        AddArc(geometry, transform, xiiVec3::MakeZero(), xiiVec3(0, radius, 0), xiiVec3(0, 0, radius), 0, xiiMath::Pi<float>() * 2, color);
      }
      else if (shape.m_Geometry == xiiSkeletonJointGeometryType::Capsule)
      {
        for (xiiUInt32 end = 0; end < 2; ++end)
          AddArc(geometry, transform, xiiVec3(length * end, 0, 0), xiiVec3(0, radius, 0), xiiVec3(0, 0, radius), 0, xiiMath::Pi<float>() * 2, color);
        for (xiiUInt32 axis = 0; axis < 2; ++axis)
        {
          const auto radial = axis == 0 ? xiiVec3(0, radius, 0) : xiiVec3(0, 0, radius);
          AddLine(geometry, transform, radial, xiiVec3(length, 0, 0) + radial, color);
          AddLine(geometry, transform, -radial, xiiVec3(length, 0, 0) - radial, color);
          AddArc(geometry, transform, xiiVec3::MakeZero(), radial, xiiVec3(-radius, 0, 0), 0, xiiMath::Pi<float>(), color);
          AddArc(geometry, transform, xiiVec3(length, 0, 0), radial, xiiVec3(radius, 0, 0), 0, xiiMath::Pi<float>(), color);
        }
      }
      else
        return XII_FAILURE;
    }

    for (const auto& collider : joint.m_BoneColliders)
    {
      if (collider.m_TriangleIndices.GetCount() % 3 != 0)
        return XII_FAILURE;
      for (const auto& point : collider.m_VertexPositions)
        if (!point.IsValid())
          return XII_FAILURE;
      for (xiiUInt32 index = 0; index < collider.m_TriangleIndices.GetCount(); index += 3)
      {
        for (xiiUInt32 edge = 0; edge < 3; ++edge)
        {
          const auto first  = collider.m_TriangleIndices[index + edge];
          const auto second = collider.m_TriangleIndices[index + (edge + 1) % 3];
          if (first >= collider.m_VertexPositions.GetCount() || second >= collider.m_VertexPositions.GetCount())
            return XII_FAILURE;
          AddLine(geometry, model, collider.m_VertexPositions[first], collider.m_VertexPositions[second], color);
        }
      }
    }
    return XII_SUCCESS;
  }
} // namespace

void xiiSkeletonPreview::Clear()
{
  m_Geometry.Clear();
  m_Bounds = xiiBoundingBoxSphere::MakeInvalid();
}

xiiResult xiiSkeletonPreview::Build(xiiArrayPtr<const xiiSkeletonJoint> joints, const xiiSkeletonPreviewSettings& settings, const xiiEditableSkeleton* pAuthoring)
{
  Clear();
  bool success = false;
  XII_SCOPE_EXIT(if (!success) Clear());
  xiiDynamicArray<const xiiEditableSkeletonJoint*> authoring;
  xiiQuat                                          boneDirection = xiiQuat::MakeIdentity();
  if (pAuthoring != nullptr)
  {
    if (pAuthoring->m_BoneDirection < xiiBasisAxis::PositiveX || pAuthoring->m_BoneDirection > xiiBasisAxis::NegativeZ)
      return XII_FAILURE;
    boneDirection = xiiBasisAxis::GetBasisRotation_PosX(pAuthoring->m_BoneDirection);
    struct Entry
    {
      XII_DECLARE_POD_TYPE();
      const xiiEditableSkeletonJoint* m_pJoint;
      xiiUInt16                       m_uiParent;
    };
    xiiDynamicArray<Entry>                      pending;
    xiiHashSet<const xiiEditableSkeletonJoint*> visited;
    for (xiiUInt32 i = pAuthoring->m_Children.GetCount(); i > 0; --i)
      pending.PushBack({pAuthoring->m_Children[i - 1], xiiMath::MaxValue<xiiUInt16>()});
    while (!pending.IsEmpty())
    {
      const auto entry = pending.PeekBack();
      pending.PopBack();
      const auto index = authoring.GetCount();
      if (entry.m_pJoint == nullptr || visited.Insert(entry.m_pJoint) || index >= joints.GetCount() ||
          entry.m_pJoint->m_sName != joints[index].m_sName || entry.m_uiParent != joints[index].m_uiParentIndex)
        return XII_FAILURE;
      authoring.PushBack(entry.m_pJoint);
      for (xiiUInt32 i = entry.m_pJoint->m_Children.GetCount(); i > 0; --i)
        pending.PushBack({entry.m_pJoint->m_Children[i - 1], static_cast<xiiUInt16>(index)});
    }
    if (authoring.GetCount() != joints.GetCount())
      return XII_FAILURE;
  }

  xiiHybridArray<xiiStringView, 16> highlighted;
  settings.m_sHighlightedBones.Split(false, highlighted, ";");
  xiiHashSet<xiiString> highlightedNames;
  for (const auto& name : highlighted)
    highlightedNames.Insert(name);
  xiiBoundingBox bounds   = xiiBoundingBox::MakeInvalid();
  const auto     identity = xiiMat4::MakeIdentity();
  for (xiiUInt32 index = 0; index < joints.GetCount(); ++index)
  {
    const auto& joint = joints[index];
    if (!joint.m_ModelRestPose.IsValid() || (joint.m_uiParentIndex != xiiMath::MaxValue<xiiUInt16>() && joint.m_uiParentIndex >= index))
      return XII_FAILURE;
    const auto position = joint.m_ModelRestPose.GetTranslationVector();
    bounds.ExpandToInclude(position);
    const bool  selected       = highlightedNames.Contains(joint.m_sName.GetView());
    const auto  color          = selected ? xiiColor::Yellow : xiiColor::LightBlue;
    const auto  parentPosition = joint.m_uiParentIndex < index ? joints[joint.m_uiParentIndex].m_ModelRestPose.GetTranslationVector() : position;
    const float length         = (position - parentPosition).GetLength();
    if (!xiiMath::IsFinite(length))
      return XII_FAILURE;
    const float marker = xiiMath::Max(0.01f, length * 0.05f);
    if (settings.m_bBones)
    {
      if (joint.m_uiParentIndex < index)
        AddLine(m_Geometry, identity, parentPosition, position, color);
      for (xiiUInt32 axis = 0; axis < 3; ++axis)
      {
        const auto offset = xiiBasisAxis::GetBasisVector(static_cast<xiiBasisAxis::Enum>(axis)) * marker;
        AddLine(m_Geometry, identity, position - offset, position + offset, color);
      }
    }
    if (pAuthoring == nullptr)
      continue;
    const auto& authored = *authoring[index];
    if (settings.m_bColliders)
      XII_SUCCEED_OR_RETURN(AddColliders(m_Geometry, authored, joint.m_ModelRestPose, joint.m_LocalRestPose.m_vPosition.GetLength(), boneDirection, selected ? xiiColor::Yellow : xiiColor::LimeGreen));

    if (!settings.m_bJoints && !settings.m_bSwingLimits && !settings.m_bTwistLimits)
      continue;
    if (!authored.m_qLocalJointRotation.IsValid())
      return XII_FAILURE;
    const auto  frame  = joint.m_ModelRestPose * (authored.m_qLocalJointRotation * boneDirection).GetAsMat4();
    const float radius = xiiMath::Max(0.05f, joint.m_LocalRestPose.m_vPosition.GetLength() * 0.2f);
    if (settings.m_bJoints)
    {
      AddLine(m_Geometry, frame, xiiVec3::MakeZero(), xiiVec3(radius, 0, 0), xiiColor::Red);
      AddLine(m_Geometry, frame, xiiVec3::MakeZero(), xiiVec3(0, radius, 0), xiiColor::Green);
      AddLine(m_Geometry, frame, xiiVec3::MakeZero(), xiiVec3(0, 0, radius), xiiColor::Blue);
    }
    if (authored.m_JointType != xiiSkeletonJointType::SwingTwist)
      continue;
    if (settings.m_bSwingLimits)
    {
      const auto swingY = authored.m_SwingLimitY;
      const auto swingZ = authored.m_SwingLimitZ;
      if (!IsFiniteNonNegative(swingY.GetRadian()) || !IsFiniteNonNegative(swingZ.GetRadian()) || swingY.GetDegree() > 170 || swingZ.GetDegree() > 170)
        return XII_FAILURE;
      xiiVec3 previous = xiiVec3::MakeZero();
      for (xiiUInt32 i = 0; i <= s_uiSegments; ++i)
      {
        const auto angle = xiiAngle::MakeFromRadian(xiiMath::Pi<float>() * 2 * i / s_uiSegments);
        // Elliptical swing in the exponential-map plane stays finite even for limits exceeding 90 degrees.
        const float y         = swingY.GetRadian() * xiiMath::Cos(angle);
        const float z         = swingZ.GetRadian() * xiiMath::Sin(angle);
        const float magnitude = xiiMath::Sqrt(y * y + z * z);
        const float factor    = magnitude > 0 ? xiiMath::Sin(xiiAngle::MakeFromRadian(magnitude)) / magnitude : 1;
        const auto  point     = xiiVec3(xiiMath::Cos(xiiAngle::MakeFromRadian(magnitude)), y * factor, z * factor) * radius;
        if (i > 0)
          AddLine(m_Geometry, frame, previous, point, xiiColor::DeepPink);
        if (i % (s_uiSegments / 4) == 0)
          AddLine(m_Geometry, frame, xiiVec3::MakeZero(), point, xiiColor::DeepPink);
        previous = point;
      }
    }
    if (settings.m_bTwistLimits)
    {
      const float half   = authored.m_TwistLimitHalfAngle.GetRadian();
      const float center = authored.m_TwistLimitCenterAngle.GetRadian();
      if (!IsFiniteNonNegative(half) || !xiiMath::IsFinite(center) || half > xiiMath::Pi<float>())
        return XII_FAILURE;
      AddArc(m_Geometry, frame, xiiVec3::MakeZero(), xiiVec3(0, radius, 0), xiiVec3(0, 0, radius), center - half, center + half, xiiColor::LightPink);
      for (float angle : {center - half, center + half})
        AddLine(m_Geometry, frame, xiiVec3::MakeZero(), xiiVec3(0, xiiMath::Cos(xiiAngle::MakeFromRadian(angle)), xiiMath::Sin(xiiAngle::MakeFromRadian(angle))) * radius, xiiColor::LightPink);
    }
  }
  for (const auto& vertex : m_Geometry.GetVertices())
  {
    if (!vertex.m_vPosition.IsValid())
      return XII_FAILURE;
    bounds.ExpandToInclude(vertex.m_vPosition);
  }
  m_Bounds = bounds.IsValid() ? xiiBoundingBoxSphere::MakeFromBox(bounds) : xiiBoundingBoxSphere::MakeZero();
  // Center / half-extent conversion can round an extremal point inward. Include a relative margin
  // so focus and thumbnail bounds contain the submitted geometry as well as the joint positions.
  m_Bounds.m_vBoxHalfExtents += m_Bounds.m_vBoxHalfExtents.CompMax(m_Bounds.m_vCenter.Abs()).CompMax(xiiVec3(1)) * 0.000001f;
  m_Bounds.m_fSphereRadius = m_Bounds.m_vBoxHalfExtents.GetLength();
  success                  = true;
  return XII_SUCCESS;
}
