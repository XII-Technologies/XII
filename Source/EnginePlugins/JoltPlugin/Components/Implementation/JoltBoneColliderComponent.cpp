#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltQueryShapeActorComponent.h>
#include <JoltPlugin/Components/JoltBoneColliderComponent.h>
#include <JoltPlugin/Shapes/JoltShapeBoxComponent.h>
#include <JoltPlugin/Shapes/JoltShapeCapsuleComponent.h>
#include <JoltPlugin/Shapes/JoltShapeSphereComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltBoneColliderComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("QueryShapeOnly", m_bQueryShapeOnly)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("UpdateThreshold", m_UpdateThreshold),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(GetObjectFilterID),
    XII_SCRIPT_FUNCTION_PROPERTY(RecreatePhysicsShapes),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Animation"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltBoneColliderComponent::xiiJoltBoneColliderComponent()  = default;
xiiJoltBoneColliderComponent::~xiiJoltBoneColliderComponent() = default;

void xiiJoltBoneColliderComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_bQueryShapeOnly;
  s << m_UpdateThreshold;
}

void xiiJoltBoneColliderComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = inout_stream.GetStream();

  s >> m_bQueryShapeOnly;
  s >> m_UpdateThreshold;
}

void xiiJoltBoneColliderComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  RecreatePhysicsShapes();
}

void xiiJoltBoneColliderComponent::OnDeactivated()
{
  if (m_uiObjectFilterID != xiiInvalidIndex)
  {
    xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
  }

  DestroyPhysicsShapes();

  SUPER::OnDeactivated();
}

void xiiJoltBoneColliderComponent::OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& ref_msg)
{
  if (m_UpdateThreshold.IsPositive())
  {
    const xiiTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

    if (tNow - m_LastUpdate < m_UpdateThreshold)
      return;

    m_LastUpdate = tNow;
  }

  for (const auto& shape : m_Shapes)
  {
    xiiMat4 boneTrans;
    xiiQuat boneRot;
    ref_msg.ComputeFullBoneTransform(shape.m_uiAttachedToBone, boneTrans, boneRot);

    xiiTransform pose;
    pose.SetIdentity();
    pose.m_vPosition = boneTrans.GetTranslationVector() + boneRot * shape.m_vOffsetPos;
    pose.m_qRotation = boneRot * shape.m_qOffsetRot;

    xiiGameObject* pGO = nullptr;
    if (GetWorld()->TryGetObject(shape.m_hActorObject, pGO))
    {
      pGO->SetLocalPosition(pose.m_vPosition);
      pGO->SetLocalRotation(pose.m_qRotation);
    }
  }
}

void xiiJoltBoneColliderComponent::RecreatePhysicsShapes()
{
  xiiMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  if (!msg.m_hSkeleton.IsValid())
    return;

  DestroyPhysicsShapes();
  CreatePhysicsShapes(msg.m_hSkeleton);

  m_LastUpdate.SetZero();
}

void xiiJoltBoneColliderComponent::CreatePhysicsShapes(const xiiSkeletonResourceHandle& hSkeleton)
{
  xiiResourceLock<xiiSkeletonResource> pSkeleton(hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);

  const auto& desc = pSkeleton->GetDescriptor();

  XII_ASSERT_DEV(m_Shapes.IsEmpty(), "");
  m_Shapes.Reserve(desc.m_Geometry.GetCount());

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  m_uiObjectFilterID          = pModule->CreateObjectFilterID();

  const auto    srcBoneDir         = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  const xiiQuat qBoneDirAdjustment = xiiBasisAxis::GetBasisRotation(xiiBasisAxis::PositiveX, srcBoneDir);

  const xiiQuat qFinalBoneRot = /*boneRot **/ qBoneDirAdjustment;

  xiiQuat qRotZtoX; // the capsule should extend along X, but the capsule shape goes along Z
  qRotZtoX.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(-90));

  for (xiiUInt32 idx = 0; idx < desc.m_Geometry.GetCount(); ++idx)
  {
    const auto& geo = desc.m_Geometry[idx];

    if (geo.m_Type == xiiSkeletonJointGeometryType::None)
      continue;

    const xiiSkeletonJoint& joint = desc.m_Skeleton.GetJointByIndex(geo.m_uiAttachedToJoint);

    auto& shape = m_Shapes.ExpandAndGetRef();

    xiiGameObject* pGO = nullptr;

    {
      xiiGameObjectDesc god;
      god.m_bDynamic = true;
      god.m_hParent  = GetOwner()->GetHandle();
      god.m_sName    = joint.GetName();
      god.m_uiTeamID = GetOwner()->GetTeamID();

      shape.m_hActorObject = GetWorld()->CreateObject(god, pGO);

      if (m_bQueryShapeOnly)
      {
        xiiJoltQueryShapeActorComponent* pDynAct = nullptr;
        xiiJoltQueryShapeActorComponent::CreateComponent(pGO, pDynAct);

        pDynAct->m_uiCollisionLayer = joint.GetCollisionLayer();
        pDynAct->m_hSurface         = joint.GetSurface();
        pDynAct->SetInitialObjectFilterID(m_uiObjectFilterID);
      }
      else
      {
        xiiJoltDynamicActorComponent* pDynAct = nullptr;
        xiiJoltDynamicActorComponent::CreateComponent(pGO, pDynAct);
        pDynAct->SetKinematic(true);

        pDynAct->m_uiCollisionLayer = joint.GetCollisionLayer();
        pDynAct->m_hSurface         = joint.GetSurface();
        pDynAct->SetInitialObjectFilterID(m_uiObjectFilterID);
      }
    }

    shape.m_uiAttachedToBone = geo.m_uiAttachedToJoint;
    shape.m_vOffsetPos       = /*boneTrans.GetTranslationVector() +*/ qFinalBoneRot * geo.m_Transform.m_vPosition;
    shape.m_qOffsetRot       = qFinalBoneRot * geo.m_Transform.m_qRotation;


    xiiJoltShapeComponent* pShape = nullptr;

    if (geo.m_Type == xiiSkeletonJointGeometryType::Sphere)
    {
      xiiJoltShapeSphereComponent* pShapeComp = nullptr;
      xiiJoltShapeSphereComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetRadius(geo.m_Transform.m_vScale.z);
      pShape = pShapeComp;
    }
    else if (geo.m_Type == xiiSkeletonJointGeometryType::Box)
    {
      xiiVec3 ext;
      ext.x = geo.m_Transform.m_vScale.x;
      ext.y = geo.m_Transform.m_vScale.y;
      ext.z = geo.m_Transform.m_vScale.z;

      // TODO: if offset desired
      shape.m_vOffsetPos += qFinalBoneRot * xiiVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      xiiJoltShapeBoxComponent* pShapeComp = nullptr;
      xiiJoltShapeBoxComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetHalfExtents(ext * 0.5f);
      pShape = pShapeComp;
    }
    else if (geo.m_Type == xiiSkeletonJointGeometryType::Capsule)
    {
      shape.m_qOffsetRot = shape.m_qOffsetRot * qRotZtoX;

      // TODO: if offset desired
      shape.m_vOffsetPos += qFinalBoneRot * xiiVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      xiiJoltShapeCapsuleComponent* pShapeComp = nullptr;
      xiiJoltShapeCapsuleComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetRadius(geo.m_Transform.m_vScale.z);
      pShapeComp->SetHeight(geo.m_Transform.m_vScale.x);
      pShape = pShapeComp;
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

void xiiJoltBoneColliderComponent::DestroyPhysicsShapes()
{
  for (auto& shape : m_Shapes)
  {
    GetWorld()->DeleteObjectDelayed(shape.m_hActorObject);
  }

  m_Shapes.Clear();
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltBoneColliderComponent);
