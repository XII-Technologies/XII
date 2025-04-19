#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/AimIKComponent.h>
#include <GraphicsCore/AnimationSystem/AnimPoseGenerator.h>
#include <GraphicsCore/AnimationSystem/Skeleton.h>
#include <GraphicsCore/Debug/DebugRenderer.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiIkJointEntry, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiIkJointEntry>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Joint", m_sJointName),
    XII_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiAimIKComponent, 1, xiiComponentMode::Dynamic);
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("ForwardVector", xiiBasisAxis, m_ForwardVector)->AddAttributes(new xiiDefaultValueAttribute(xiiBasisAxis::PositiveX)),
    XII_ENUM_MEMBER_PROPERTY("UpVector", xiiBasisAxis, m_UpVector)->AddAttributes(new xiiDefaultValueAttribute(xiiBasisAxis::PositiveZ)),
    XII_ACCESSOR_PROPERTY("PoleVector", DummyGetter, SetPoleVectorReference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
    XII_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ARRAY_MEMBER_PROPERTY("Joints", m_Joints),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
      new xiiCategoryAttribute("Animation"),
      new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveZ, 0.5f),
  }
  XII_END_ATTRIBUTES;

  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgAnimationPoseGeneration, OnMsgAnimationPoseGeneration)
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiResult xiiIkJointEntry::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_sJointName;
  inout_stream << m_fWeight;
  return XII_SUCCESS;
}

xiiResult xiiIkJointEntry::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_sJointName;
  inout_stream >> m_fWeight;
  m_uiJointIdx = 0;
  return XII_SUCCESS;
}

xiiAimIKComponent::xiiAimIKComponent()  = default;
xiiAimIKComponent::~xiiAimIKComponent() = default;

void xiiAimIKComponent::SetPoleVectorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hPoleVector = resolver(szReference, GetHandle(), "PoleVector");
}

void xiiAimIKComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fWeight;
  s << m_ForwardVector;
  s << m_UpVector;
  inout_stream.WriteGameObjectHandle(m_hPoleVector);
  s.WriteArray(m_Joints).AssertSuccess();
}

void xiiAimIKComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fWeight;
  s >> m_ForwardVector;
  s >> m_UpVector;
  m_hPoleVector = inout_stream.ReadGameObjectHandle();
  s.ReadArray(m_Joints).AssertSuccess();
}

void xiiAimIKComponent::OnMsgAnimationPoseGeneration(xiiMsgAnimationPoseGeneration& msg) const
{
  if (m_fWeight <= 0.0f)
    return;

  const xiiTransform targetTrans    = msg.m_pGenerator->GetTargetObject()->GetGlobalTransform();
  const xiiTransform selfTrans      = GetOwner()->GetGlobalTransform();
  const xiiTransform ownerTransform = xiiTransform::MakeGlobalTransform(targetTrans, msg.m_pGenerator->GetSkeleton()->GetDescriptor().m_RootTransform);
  const xiiTransform localTarget    = xiiTransform::MakeLocalTransform(ownerTransform, selfTrans);

  xiiVec3 vPoleVectorPos;

  const xiiGameObject* pPoleVector;
  if (!m_hPoleVector.IsInvalidated() && GetWorld()->TryGetObject(m_hPoleVector, pPoleVector))
  {
    vPoleVectorPos = xiiTransform::MakeLocalTransform(ownerTransform, xiiTransform(pPoleVector->GetGlobalPosition())).m_vPosition;
  }
  else
  {
    // hard-coded "up vector" as pole target
    vPoleVectorPos = xiiTransform::MakeLocalTransform(ownerTransform, xiiTransform(targetTrans * xiiVec3(0, 0, 10))).m_vPosition;
  }

  for (xiiUInt32 i = 0; i < m_Joints.GetCount(); ++i)
  {
    if (m_Joints[i].m_fWeight <= 0.0f)
      continue;

    if (m_Joints[i].m_uiJointIdx == 0)
    {
      m_Joints[i].m_uiJointIdx = msg.m_pGenerator->GetSkeleton()->GetDescriptor().m_Skeleton.FindJointByName(m_Joints[i].m_sJointName);
    }

    if (m_Joints[i].m_uiJointIdx == xiiInvalidJointIndex)
      continue;

    auto& cmdIk        = msg.m_pGenerator->AllocCommandAimIK();
    cmdIk.m_uiJointIdx = m_Joints[i].m_uiJointIdx;
    cmdIk.m_Inputs.PushBack(msg.m_pGenerator->GetFinalCommand());
    cmdIk.m_vTargetPosition = localTarget.m_vPosition;
    cmdIk.m_fWeight         = m_fWeight * m_Joints[i].m_fWeight;
    cmdIk.m_vForwardVector  = xiiBasisAxis::GetBasisVector(m_ForwardVector);
    cmdIk.m_vUpVector       = xiiBasisAxis::GetBasisVector(m_UpVector);
    cmdIk.m_vPoleVector     = vPoleVectorPos;

    // in theory one could limit which joints get their model poses updated,
    // but in practice this doesn't work unless we know that they are definitely just in one straight line (not the case for spines)
    // if (i + 1 < m_Joints.GetCount())
    //{
    //  cmdIk.m_uiRecalcModelPoseToJointIdx = m_Joints[i + 1].m_uiJointIdx;
    //}

    msg.m_pGenerator->SetFinalCommand(cmdIk.GetCommandID());
  }
}


XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AimIKComponent);
