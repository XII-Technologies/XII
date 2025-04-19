#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/TwoBoneIKComponent.h>
#include <GraphicsCore/AnimationSystem/AnimPoseGenerator.h>
#include <GraphicsCore/AnimationSystem/Skeleton.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiTwoBoneIKComponent, 1, xiiComponentMode::Dynamic);
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("JointStart", m_sJointStart),
    XII_MEMBER_PROPERTY("JointMiddle", m_sJointMiddle),
    XII_MEMBER_PROPERTY("JointEnd", m_sJointEnd),
    XII_ENUM_MEMBER_PROPERTY("MidAxis", xiiBasisAxis, m_MidAxis)->AddAttributes(new xiiDefaultValueAttribute(xiiBasisAxis::PositiveZ)),
    XII_ACCESSOR_PROPERTY("PoleVector", DummyGetter, SetPoleVectorReference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
    XII_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    //XII_MEMBER_PROPERTY("Soften", m_fSoften)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    //XII_MEMBER_PROPERTY("TwistAngle", m_TwistAngle)->AddAttributes(new xiiClampValueAttribute(xiiAngle::MakeFromDegree(-180), xiiAngle::MakeFromDegree(180))),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Animation"),
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

xiiTwoBoneIKComponent::xiiTwoBoneIKComponent()  = default;
xiiTwoBoneIKComponent::~xiiTwoBoneIKComponent() = default;

void xiiTwoBoneIKComponent::SetPoleVectorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hPoleVector = resolver(szReference, GetHandle(), "PoleVector");
}

void xiiTwoBoneIKComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fWeight;
  s << m_sJointStart;
  s << m_sJointMiddle;
  s << m_sJointEnd;
  s << m_MidAxis;
  inout_stream.WriteGameObjectHandle(m_hPoleVector);
  // s << m_fSoften;
  // s << m_TwistAngle;
}

void xiiTwoBoneIKComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fWeight;
  s >> m_sJointStart;
  s >> m_sJointMiddle;
  s >> m_sJointEnd;
  s >> m_MidAxis;
  m_hPoleVector = inout_stream.ReadGameObjectHandle();
  // s >> m_fSoften;
  // s >> m_TwistAngle;
}

void xiiTwoBoneIKComponent::OnMsgAnimationPoseGeneration(xiiMsgAnimationPoseGeneration& msg) const
{
  const xiiTransform targetTrans    = msg.m_pGenerator->GetTargetObject()->GetGlobalTransform();
  const xiiTransform ownerTransform = xiiTransform::MakeGlobalTransform(targetTrans, msg.m_pGenerator->GetSkeleton()->GetDescriptor().m_RootTransform);
  const xiiTransform localTarget    = xiiTransform::MakeLocalTransform(ownerTransform, GetOwner()->GetGlobalTransform());

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

  if (m_uiJointIdxStart == 0 && m_uiJointIdxMiddle == 0)
  {
    auto& skel         = msg.m_pGenerator->GetSkeleton()->GetDescriptor().m_Skeleton;
    m_uiJointIdxStart  = skel.FindJointByName(m_sJointStart);
    m_uiJointIdxMiddle = skel.FindJointByName(m_sJointMiddle);
    m_uiJointIdxEnd    = skel.FindJointByName(m_sJointEnd);
  }

  if (m_uiJointIdxStart != xiiInvalidJointIndex && m_uiJointIdxMiddle != xiiInvalidJointIndex && m_uiJointIdxEnd != xiiInvalidJointIndex)
  {
    auto& cmdIk              = msg.m_pGenerator->AllocCommandTwoBoneIK();
    cmdIk.m_uiJointIdxStart  = m_uiJointIdxStart;
    cmdIk.m_uiJointIdxMiddle = m_uiJointIdxMiddle;
    cmdIk.m_uiJointIdxEnd    = m_uiJointIdxEnd;
    cmdIk.m_Inputs.PushBack(msg.m_pGenerator->GetFinalCommand());
    cmdIk.m_vTargetPosition = localTarget.m_vPosition;
    cmdIk.m_vPoleVector     = vPoleVectorPos;
    cmdIk.m_vMidAxis        = xiiBasisAxis::GetBasisVector(m_MidAxis);
    cmdIk.m_fWeight         = m_fWeight;
    cmdIk.m_fSoften         = 1.0f;                 // m_fSoften;
    cmdIk.m_TwistAngle      = xiiAngle::MakeZero(); // m_TwistAngle;

    msg.m_pGenerator->SetFinalCommand(cmdIk.GetCommandID());
  }
}


XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_TwoBoneIKComponent);
