#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/AnimationSystem/AnimationPose.h>
#include <GraphicsCore/AnimationSystem/SkeletonComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSkeletonComponent, 5, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    XII_MEMBER_PROPERTY("VisualizeSkeleton", m_bVisualizeBones)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("VisualizeColliders", m_bVisualizeColliders),
    XII_MEMBER_PROPERTY("VisualizeJoints", m_bVisualizeJoints),
    XII_MEMBER_PROPERTY("VisualizeSwingLimits", m_bVisualizeSwingLimits),
    XII_MEMBER_PROPERTY("VisualizeTwistLimits", m_bVisualizeTwistLimits),
    XII_ACCESSOR_PROPERTY("BonesToHighlight", GetBonesToHighlight, SetBonesToHighlight),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    XII_MESSAGE_HANDLER(xiiMsgQueryAnimationSkeleton, OnQueryAnimationSkeleton)
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Animation"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSkeletonComponent::xiiSkeletonComponent()  = default;
xiiSkeletonComponent::~xiiSkeletonComponent() = default;

xiiResult xiiSkeletonComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  if (m_MaxBounds.IsValid())
  {
    xiiBoundingBox bbox = m_MaxBounds;
    ref_bounds          = xiiBoundingBoxSphere(bbox);
    ref_bounds.Transform(m_RootTransform.GetAsMat4());
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiSkeletonComponent::Update()
{
  if (m_hSkeleton.IsValid() && (m_bVisualizeBones || m_bVisualizeColliders || m_bVisualizeJoints || m_bVisualizeSwingLimits || m_bVisualizeTwistLimits))
  {
    xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);

    if (pSkeleton.GetAcquireResult() != xiiResourceAcquireResult::Final)
      return;

    if (m_uiSkeletonChangeCounter != pSkeleton->GetCurrentResourceChangeCounter())
    {
      VisualizeSkeletonDefaultState();
    }

    const xiiQuat qBoneDir     = xiiBasisAxis::GetBasisRotation_PosX(pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
    const xiiVec3 vBoneDir     = qBoneDir * xiiVec3(1, 0, 0);
    const xiiVec3 vBoneTangent = qBoneDir * xiiVec3(0, 1, 0);

    xiiDebugRenderer::DrawLines(GetWorld(), m_LinesSkeleton, xiiColor::White, GetOwner()->GetGlobalTransform());

    for (const auto& shape : m_SpheresShapes)
    {
      xiiDebugRenderer::DrawLineSphere(GetWorld(), shape.m_Shape, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_BoxShapes)
    {
      xiiDebugRenderer::DrawLineBox(GetWorld(), shape.m_Shape, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_CapsuleShapes)
    {
      xiiDebugRenderer::DrawLineCapsuleZ(GetWorld(), shape.m_fLength, shape.m_fRadius, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_AngleShapes)
    {
      xiiDebugRenderer::DrawAngle(GetWorld(), shape.m_StartAngle, shape.m_EndAngle, xiiColor::MakeZero(), shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform, vBoneTangent, vBoneDir);
    }

    for (const auto& shape : m_ConeLimitShapes)
    {
      xiiDebugRenderer::DrawLimitCone(GetWorld(), shape.m_Angle1, shape.m_Angle2, xiiColor::MakeZero(), shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_CylinderShapes)
    {
      xiiDebugRenderer::DrawCylinder(GetWorld(), shape.m_fRadius1, shape.m_fRadius2, shape.m_fLength, shape.m_Color, xiiColor::MakeZero(), GetOwner()->GetGlobalTransform() * shape.m_Transform, false, false);
    }
  }
}

void xiiSkeletonComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hSkeleton;
  s << m_bVisualizeBones;
  s << m_sBonesToHighlight;
  s << m_bVisualizeColliders;
  s << m_bVisualizeJoints;
  s << m_bVisualizeSwingLimits;
  s << m_bVisualizeTwistLimits;
}

void xiiSkeletonComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  if (uiVersion <= 4)
    return;

  auto& s = inout_stream.GetStream();

  s >> m_hSkeleton;
  s >> m_bVisualizeBones;
  s >> m_sBonesToHighlight;
  s >> m_bVisualizeColliders;
  s >> m_bVisualizeJoints;
  s >> m_bVisualizeSwingLimits;
  s >> m_bVisualizeTwistLimits;
}

void xiiSkeletonComponent::OnActivated()
{
  SUPER::OnActivated();

  m_MaxBounds = xiiBoundingBox::MakeInvalid();
  VisualizeSkeletonDefaultState();
}

void xiiSkeletonComponent::SetSkeletonFile(const char* szFile)
{
  xiiSkeletonResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiSkeletonResource>(szFile);
  }

  SetSkeleton(hResource);
}

const char* xiiSkeletonComponent::GetSkeletonFile() const
{
  if (!m_hSkeleton.IsValid())
    return "";

  return m_hSkeleton.GetResourceID();
}


void xiiSkeletonComponent::SetSkeleton(const xiiSkeletonResourceHandle& hResource)
{
  if (m_hSkeleton != hResource)
  {
    m_hSkeleton = hResource;

    m_MaxBounds = xiiBoundingBox::MakeInvalid();
    VisualizeSkeletonDefaultState();
  }
}

void xiiSkeletonComponent::SetBonesToHighlight(const char* szFilter)
{
  if (m_sBonesToHighlight != szFilter)
  {
    m_sBonesToHighlight = szFilter;

    m_uiSkeletonChangeCounter = 0xFFFFFFFF;

    VisualizeSkeletonDefaultState();
  }
}

const char* xiiSkeletonComponent::GetBonesToHighlight() const
{
  return m_sBonesToHighlight;
}

void xiiSkeletonComponent::OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& msg)
{
  m_LinesSkeleton.Clear();
  m_SpheresShapes.Clear();
  m_BoxShapes.Clear();
  m_CapsuleShapes.Clear();
  m_AngleShapes.Clear();
  m_ConeLimitShapes.Clear();
  m_CylinderShapes.Clear();

  m_RootTransform = *msg.m_pRootTransform;

  BuildSkeletonVisualization(msg);
  BuildColliderVisualization(msg);
  BuildJointVisualization(msg);

  xiiBoundingBox poseBounds;
  poseBounds = xiiBoundingBox::MakeInvalid();

  for (const auto& bone : msg.m_ModelTransforms)
  {
    poseBounds.ExpandToInclude(bone.GetTranslationVector());
  }

  if (poseBounds.IsValid() && (!m_MaxBounds.IsValid() || !m_MaxBounds.Contains(poseBounds)))
  {
    m_MaxBounds.ExpandToInclude(poseBounds);
    TriggerLocalBoundsUpdate();
  }
  else if (((xiiRenderWorld::GetFrameCounter() + GetUniqueIdForRendering()) & (XII_BIT(10) - 1)) == 0) // reset the bbox every once in a while
  {
    m_MaxBounds = poseBounds;
    TriggerLocalBoundsUpdate();
  }
}

void xiiSkeletonComponent::BuildSkeletonVisualization(xiiMsgAnimationPoseUpdated& msg)
{
  if (!m_bVisualizeBones || !msg.m_pSkeleton)
    return;

  xiiStringBuilder tmp;

  struct Bone
  {
    xiiVec3 pos            = xiiVec3::MakeZero();
    xiiVec3 dir            = xiiVec3::MakeZero();
    float   distToParent   = 0.0f;
    float   minDistToChild = 10.0f;
    bool    highlight      = false;
  };

  xiiHybridArray<Bone, 128> bones;

  bones.SetCount(msg.m_pSkeleton->GetJointCount());
  m_LinesSkeleton.Reserve(m_LinesSkeleton.GetCount() + msg.m_pSkeleton->GetJointCount());

  const xiiVec3 vBoneDir = xiiBasisAxis::GetBasisVector(msg.m_pSkeleton->m_BoneDirection);

  auto renderBone = [&](int iCurrentBone, int iParentBone) {
    if (iParentBone == ozz::animation::Skeleton::kNoParent)
      return;

    const xiiVec3 v0 = *msg.m_pRootTransform * msg.m_ModelTransforms[iParentBone].GetTranslationVector();
    const xiiVec3 v1 = *msg.m_pRootTransform * msg.m_ModelTransforms[iCurrentBone].GetTranslationVector();

    xiiVec3 dirToBone = (v1 - v0);

    auto& bone        = bones[iCurrentBone];
    bone.pos          = v1;
    bone.distToParent = dirToBone.GetLength();
    bone.dir          = *msg.m_pRootTransform * msg.m_ModelTransforms[iCurrentBone].TransformDirection(vBoneDir);
    bone.dir.NormalizeIfNotZero(xiiVec3::MakeZero()).IgnoreResult();

    auto& pb = bones[iParentBone];

    if (!pb.dir.IsZero() && dirToBone.NormalizeIfNotZero(xiiVec3::MakeZero()).Succeeded())
    {
      if (pb.dir.GetAngleBetween(dirToBone) < xiiAngle::MakeFromDegree(45))
      {
        xiiPlane plane    = xiiPlane::MakeFromNormalAndPoint(pb.dir, pb.pos);
        pb.minDistToChild = xiiMath::Min(pb.minDistToChild, plane.GetDistanceTo(v1));
      }
    }
  };

  ozz::animation::IterateJointsDF(msg.m_pSkeleton->GetOzzSkeleton(), renderBone);

  if (m_sBonesToHighlight == "*")
  {
    for (xiiUInt32 b = 0; b < bones.GetCount(); ++b)
    {
      bones[b].highlight = true;
    }
  }
  else if (!m_sBonesToHighlight.IsEmpty())
  {
    const xiiStringBuilder mask(";", m_sBonesToHighlight, ";");

    for (xiiUInt16 b = 0; b < static_cast<xiiUInt16>(bones.GetCount()); ++b)
    {
      const xiiString currentName = msg.m_pSkeleton->GetJointByIndex(b).GetName().GetString();

      tmp.Set(";", currentName, ";");

      if (mask.FindSubString(tmp))
      {
        bones[b].highlight = true;
      }
    }
  }

  for (xiiUInt32 b = 0; b < bones.GetCount(); ++b)
  {
    const auto& bone = bones[b];

    if (!bone.highlight)
    {
      float len = 0.3f;

      if (bone.minDistToChild < 10.0f)
      {
        len = bone.minDistToChild;
      }
      else if (bone.distToParent > 0)
      {
        len = xiiMath::Max(bone.distToParent * 0.5f, 0.1f);
      }
      else
      {
        len = 0.1f;
      }

      xiiVec3 v0 = bone.pos;
      xiiVec3 v1 = bone.pos + bone.dir * len;

      m_LinesSkeleton.PushBack(xiiDebugRenderer::Line(v0, v1));
      m_LinesSkeleton.PeekBack().m_startColor = xiiColor::DarkCyan;
      m_LinesSkeleton.PeekBack().m_endColor   = xiiColor::DarkCyan;
    }
  }

  for (xiiUInt32 b = 0; b < bones.GetCount(); ++b)
  {
    const auto& bone = bones[b];

    if (bone.highlight && !bone.dir.IsZero(0.0001f))
    {
      float len = 0.3f;

      if (bone.minDistToChild < 10.0f)
      {
        len = bone.minDistToChild;
      }
      else if (bone.distToParent > 0)
      {
        len = xiiMath::Max(bone.distToParent * 0.5f, 0.1f);
      }
      else
      {
        len = 0.1f;
      }

      xiiVec3 v0 = bone.pos;
      xiiVec3 v1 = bone.pos + bone.dir * len;

      const xiiVec3 vO1 = bone.dir.GetOrthogonalVector().GetNormalized();
      const xiiVec3 vO2 = bone.dir.CrossRH(vO1).GetNormalized();

      xiiVec3 s[4];
      s[0] = v0 + vO1 * len * 0.1f + bone.dir * len * 0.1f;
      s[1] = v0 + vO2 * len * 0.1f + bone.dir * len * 0.1f;
      s[2] = v0 - vO1 * len * 0.1f + bone.dir * len * 0.1f;
      s[3] = v0 - vO2 * len * 0.1f + bone.dir * len * 0.1f;

      m_LinesSkeleton.PushBack(xiiDebugRenderer::Line(v0, v1));
      m_LinesSkeleton.PeekBack().m_startColor = xiiColor::DarkCyan;
      m_LinesSkeleton.PeekBack().m_endColor   = xiiColor::DarkCyan;

      for (xiiUInt32 si = 0; si < 4; ++si)
      {
        m_LinesSkeleton.PushBack(xiiDebugRenderer::Line(v0, s[si]));
        m_LinesSkeleton.PeekBack().m_startColor = xiiColor::Chartreuse;
        m_LinesSkeleton.PeekBack().m_endColor   = xiiColor::Chartreuse;

        m_LinesSkeleton.PushBack(xiiDebugRenderer::Line(s[si], v1));
        m_LinesSkeleton.PeekBack().m_startColor = xiiColor::Chartreuse;
        m_LinesSkeleton.PeekBack().m_endColor   = xiiColor::Chartreuse;
      }
    }
  }
}

void xiiSkeletonComponent::BuildColliderVisualization(xiiMsgAnimationPoseUpdated& msg)
{
  if (!m_bVisualizeColliders || !msg.m_pSkeleton || !m_hSkeleton.IsValid())
    return;

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);

  const auto    srcBoneDir         = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  const xiiQuat qBoneDirAdjustment = xiiBasisAxis::GetBasisRotation(xiiBasisAxis::PositiveX, srcBoneDir);

  xiiStringBuilder bonesToHighlight(";", m_sBonesToHighlight, ";");
  xiiStringBuilder boneName;
  if (m_sBonesToHighlight == "*")
    bonesToHighlight.Clear();

  xiiQuat qRotZtoX; // the capsule should extend along X, but the debug renderer draws them along Z
  qRotZtoX = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(-90));

  for (const auto& geo : pSkeleton->GetDescriptor().m_Geometry)
  {
    if (geo.m_Type == xiiSkeletonJointGeometryType::None)
      continue;

    xiiMat4 boneTrans;
    xiiQuat boneRot;
    msg.ComputeFullBoneTransform(geo.m_uiAttachedToJoint, boneTrans, boneRot);

    boneName.Set(";", msg.m_pSkeleton->GetJointByIndex(geo.m_uiAttachedToJoint).GetName().GetString(), ";");
    const bool     bHighlight = bonesToHighlight.IsEmpty() || bonesToHighlight.FindLastSubString(boneName) != nullptr;
    const xiiColor hlS        = xiiMath::Lerp(xiiColor::DimGrey, xiiColor::Yellow, bHighlight ? 1.0f : 0.2f);

    const xiiQuat qFinalBoneRot = boneRot * qBoneDirAdjustment;

    xiiTransform st;
    st.SetIdentity();
    st.m_vPosition = boneTrans.GetTranslationVector() + qFinalBoneRot * geo.m_Transform.m_vPosition;
    st.m_qRotation = qFinalBoneRot * geo.m_Transform.m_qRotation;

    if (geo.m_Type == xiiSkeletonJointGeometryType::Sphere)
    {
      auto& shape       = m_SpheresShapes.ExpandAndGetRef();
      shape.m_Transform = st;
      shape.m_Color     = hlS;
      shape.m_Shape     = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), geo.m_Transform.m_vScale.z);
    }

    if (geo.m_Type == xiiSkeletonJointGeometryType::Box)
    {
      auto& shape = m_BoxShapes.ExpandAndGetRef();

      xiiVec3 ext;
      ext.x = geo.m_Transform.m_vScale.x * 0.5f;
      ext.y = geo.m_Transform.m_vScale.y * 0.5f;
      ext.z = geo.m_Transform.m_vScale.z * 0.5f;

      // TODO: if offset desired
      st.m_vPosition += qFinalBoneRot * xiiVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      shape.m_Transform = st;
      shape.m_Shape     = xiiBoundingBox(xiiVec3::MakeZero(), ext);
      shape.m_Color     = hlS;
    }

    if (geo.m_Type == xiiSkeletonJointGeometryType::Capsule)
    {
      st.m_qRotation = st.m_qRotation * qRotZtoX;

      // TODO: if offset desired
      st.m_vPosition += qFinalBoneRot * xiiVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      auto& shape       = m_CapsuleShapes.ExpandAndGetRef();
      shape.m_Transform = st;
      shape.m_fLength   = geo.m_Transform.m_vScale.x;
      shape.m_fRadius   = geo.m_Transform.m_vScale.z;
      shape.m_Color     = hlS;
    }

    if (geo.m_Type == xiiSkeletonJointGeometryType::ConvexMesh)
    {
      st.SetIdentity();
      st = *msg.m_pRootTransform;

      for (xiiUInt32 f = 0; f < geo.m_TriangleIndices.GetCount(); f += 3)
      {
        const xiiUInt32 i0 = geo.m_TriangleIndices[f + 0];
        const xiiUInt32 i1 = geo.m_TriangleIndices[f + 1];
        const xiiUInt32 i2 = geo.m_TriangleIndices[f + 2];

        {
          auto& l        = m_LinesSkeleton.ExpandAndGetRef();
          l.m_startColor = l.m_endColor = hlS;
          l.m_start                     = st * geo.m_VertexPositions[i0];
          l.m_end                       = st * geo.m_VertexPositions[i1];
        }
        {
          auto& l        = m_LinesSkeleton.ExpandAndGetRef();
          l.m_startColor = l.m_endColor = hlS;
          l.m_start                     = st * geo.m_VertexPositions[i1];
          l.m_end                       = st * geo.m_VertexPositions[i2];
        }
        {
          auto& l        = m_LinesSkeleton.ExpandAndGetRef();
          l.m_startColor = l.m_endColor = hlS;
          l.m_start                     = st * geo.m_VertexPositions[i2];
          l.m_end                       = st * geo.m_VertexPositions[i0];
        }
      }
    }
  }
}

void xiiSkeletonComponent::BuildJointVisualization(xiiMsgAnimationPoseUpdated& msg)
{
  if (!m_hSkeleton.IsValid() || (!m_bVisualizeJoints && !m_bVisualizeSwingLimits && !m_bVisualizeTwistLimits))
    return;

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);
  const auto&                          skel = pSkeleton->GetDescriptor().m_Skeleton;

  xiiStringBuilder bonesToHighlight(";", m_sBonesToHighlight, ";");
  xiiStringBuilder boneName;
  if (m_sBonesToHighlight == "*")
    bonesToHighlight.Clear();

  const xiiQuat qBoneDir   = xiiBasisAxis::GetBasisRotation_PosX(pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const xiiQuat qBoneDirT  = xiiBasisAxis::GetBasisRotation(xiiBasisAxis::PositiveY, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const xiiQuat qBoneDirBT = xiiBasisAxis::GetBasisRotation(xiiBasisAxis::PositiveZ, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const xiiQuat qBoneDirT2 = xiiBasisAxis::GetBasisRotation(xiiBasisAxis::NegativeY, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);

  for (xiiUInt16 uiJointIdx = 0; uiJointIdx < skel.GetJointCount(); ++uiJointIdx)
  {
    const auto&     thisJoint   = skel.GetJointByIndex(uiJointIdx);
    const xiiUInt16 uiParentIdx = thisJoint.GetParentIndex();

    if (thisJoint.IsRootJoint())
      continue;

    boneName.Set(";", thisJoint.GetName().GetString(), ";");

    const bool bHighlight = bonesToHighlight.IsEmpty() || bonesToHighlight.FindSubString(boneName) != nullptr;

    xiiMat4 parentTrans;
    xiiQuat parentRot; // contains root transform
    msg.ComputeFullBoneTransform(uiParentIdx, parentTrans, parentRot);

    xiiMat4 thisTrans; // contains root transform
    xiiQuat thisRot;   // contains root transform
    msg.ComputeFullBoneTransform(uiJointIdx, thisTrans, thisRot);

    const xiiVec3 vJointPos = thisTrans.GetTranslationVector();
    const xiiQuat qLimitRot = parentRot * thisJoint.GetLocalOrientation();

    // main directions
    if (m_bVisualizeJoints)
    {
      const xiiColor hlM  = xiiMath::Lerp(xiiColor::OrangeRed, xiiColor::DimGrey, bHighlight ? 0 : 0.8f);
      const xiiColor hlT  = xiiMath::Lerp(xiiColor::LawnGreen, xiiColor::DimGrey, bHighlight ? 0 : 0.8f);
      const xiiColor hlBT = xiiMath::Lerp(xiiColor::BlueViolet, xiiColor::DimGrey, bHighlight ? 0 : 0.8f);

      {
        auto& cyl                   = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color                 = hlM;
        cyl.m_fLength               = 0.07f;
        cyl.m_fRadius1              = 0.002f;
        cyl.m_fRadius2              = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDir;
        cyl.m_Transform.m_vScale.Set(1);
      }

      {
        auto& cyl                   = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color                 = hlT;
        cyl.m_fLength               = 0.07f;
        cyl.m_fRadius1              = 0.002f;
        cyl.m_fRadius2              = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirT;
        cyl.m_Transform.m_vScale.Set(1);
      }

      {
        auto& cyl                   = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color                 = hlBT;
        cyl.m_fLength               = 0.07f;
        cyl.m_fRadius1              = 0.002f;
        cyl.m_fRadius2              = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirBT;
        cyl.m_Transform.m_vScale.Set(1);
      }
    }

    // swing limit
    if (m_bVisualizeSwingLimits && (thisJoint.GetHalfSwingLimitY() > xiiAngle() || thisJoint.GetHalfSwingLimitZ() > xiiAngle()))
    {
      auto& shape    = m_ConeLimitShapes.ExpandAndGetRef();
      shape.m_Angle1 = thisJoint.GetHalfSwingLimitY();
      shape.m_Angle2 = thisJoint.GetHalfSwingLimitZ();
      shape.m_Color  = xiiMath::Lerp(xiiColor::DimGrey, xiiColor::DeepPink, bHighlight ? 1.0f : 0.2f);
      shape.m_Transform.m_vScale.Set(0.05f);
      shape.m_Transform.m_vPosition = vJointPos;
      shape.m_Transform.m_qRotation = qLimitRot * qBoneDir;

      const xiiColor hlM = xiiMath::Lerp(xiiColor::OrangeRed, xiiColor::DimGrey, bHighlight ? 0 : 0.8f);

      {
        auto& cyl                   = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color                 = hlM;
        cyl.m_fLength               = 0.07f;
        cyl.m_fRadius1              = 0.002f;
        cyl.m_fRadius2              = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDir;
        cyl.m_Transform.m_vScale.Set(1);
      }
    }

    // twist limit
    if (m_bVisualizeTwistLimits && thisJoint.GetTwistLimitHalfAngle() > xiiAngle::MakeFromDegree(0))
    {
      auto& shape        = m_AngleShapes.ExpandAndGetRef();
      shape.m_StartAngle = thisJoint.GetTwistLimitLow();
      shape.m_EndAngle   = thisJoint.GetTwistLimitHigh();
      shape.m_Color      = xiiMath::Lerp(xiiColor::DimGrey, xiiColor::LightPink, bHighlight ? 0.8f : 0.2f);
      shape.m_Transform.m_vScale.Set(0.04f);
      shape.m_Transform.m_vPosition = vJointPos;
      shape.m_Transform.m_qRotation = qLimitRot;

      const xiiColor hlT = xiiMath::Lerp(xiiColor::DimGrey, xiiColor::LightPink, bHighlight ? 1.0f : 0.4f);

      {
        auto& cyl                   = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color                 = hlT;
        cyl.m_fLength               = 0.07f;
        cyl.m_fRadius1              = 0.002f;
        cyl.m_fRadius2              = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirT2;
        cyl.m_Transform.m_vScale.Set(1);

        xiiVec3 vDir = cyl.m_Transform.m_qRotation * xiiVec3(1, 0, 0);
        vDir.Normalize();

        xiiVec3 vDirRef = shape.m_Transform.m_qRotation * qBoneDir * xiiVec3(0, 1, 0);
        vDirRef.Normalize();

        const xiiVec3 vRotDir = shape.m_Transform.m_qRotation * qBoneDir * xiiVec3(1, 0, 0);

        xiiQuat qRotRef;
        qRotRef = xiiQuat::MakeFromAxisAndAngle(vRotDir, thisJoint.GetTwistLimitCenterAngle());
        vDirRef = qRotRef * vDirRef;

        // if the current twist is outside the twist limit range, highlight the bone
        if (vDir.GetAngleBetween(vDirRef) > thisJoint.GetTwistLimitHalfAngle())
        {
          cyl.m_Color = xiiColor::Orange;
        }
      }
    }
  }
}

void xiiSkeletonComponent::VisualizeSkeletonDefaultState()
{
  if (!IsActiveAndInitialized())
    return;

  m_uiSkeletonChangeCounter = 0;

  if (m_hSkeleton.IsValid())
  {
    xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pSkeleton.GetAcquireResult() == xiiResourceAcquireResult::Final)
    {
      m_uiSkeletonChangeCounter = pSkeleton->GetCurrentResourceChangeCounter();

      if (pSkeleton->GetDescriptor().m_Skeleton.GetJointCount() > 0)
      {
        ozz::vector<ozz::math::Float4x4> modelTransforms;
        modelTransforms.resize(pSkeleton->GetDescriptor().m_Skeleton.GetJointCount());

        {
          ozz::animation::LocalToModelJob job;
          job.input    = pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_rest_poses();
          job.output   = make_span(modelTransforms);
          job.skeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
          job.Run();
        }

        xiiMsgAnimationPoseUpdated msg;
        msg.m_pRootTransform  = &pSkeleton->GetDescriptor().m_RootTransform;
        msg.m_pSkeleton       = &pSkeleton->GetDescriptor().m_Skeleton;
        msg.m_ModelTransforms = xiiArrayPtr<const xiiMat4>(reinterpret_cast<const xiiMat4*>(&modelTransforms[0]), (xiiUInt32)modelTransforms.size());

        OnAnimationPoseUpdated(msg);
      }
    }
  }

  TriggerLocalBoundsUpdate();
}

xiiDebugRenderer::Line& xiiSkeletonComponent::AddLine(const xiiVec3& vStart, const xiiVec3& vEnd, const xiiColor& color)
{
  auto& line        = m_LinesSkeleton.ExpandAndGetRef();
  line.m_start      = vStart;
  line.m_end        = vEnd;
  line.m_startColor = color;
  line.m_endColor   = color;
  return line;
}

void xiiSkeletonComponent::OnQueryAnimationSkeleton(xiiMsgQueryAnimationSkeleton& msg)
{
  // if we have a skeleton, always overwrite it any incoming message with that
  if (m_hSkeleton.IsValid())
  {
    msg.m_hSkeleton = m_hSkeleton;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_Implementation_SkeletonComponent);
