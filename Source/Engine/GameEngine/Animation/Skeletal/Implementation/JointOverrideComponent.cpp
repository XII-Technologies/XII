#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/JointOverrideComponent.h>
#include <GraphicsCore/AnimationSystem/AnimationPose.h>
#include <GraphicsCore/AnimationSystem/Skeleton.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJointOverrideComponent, 1, xiiComponentMode::Dynamic);
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("JointName", GetJointName, SetJointName),
    XII_MEMBER_PROPERTY("OverridePosition", m_bOverridePosition)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("OverrideRotation", m_bOverrideRotation)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("OverrideScale", m_bOverrideScale)->AddAttributes(new xiiDefaultValueAttribute(false)),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
      new xiiCategoryAttribute("Animation"),
  }
  XII_END_ATTRIBUTES;

  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgAnimationPosePreparing, OnAnimationPosePreparing)
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiJointOverrideComponent::xiiJointOverrideComponent()  = default;
xiiJointOverrideComponent::~xiiJointOverrideComponent() = default;

void xiiJointOverrideComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sJointToOverride;
  s << m_bOverridePosition;
  s << m_bOverrideRotation;
  s << m_bOverrideScale;
}

void xiiJointOverrideComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_sJointToOverride;
  s >> m_bOverridePosition;
  s >> m_bOverrideRotation;
  s >> m_bOverrideScale;

  m_uiJointIndex = xiiInvalidJointIndex;
}

void xiiJointOverrideComponent::SetJointName(const char* szName)
{
  m_sJointToOverride.Assign(szName);
  m_uiJointIndex = xiiInvalidJointIndex;
}

const char* xiiJointOverrideComponent::GetJointName() const
{
  return m_sJointToOverride.GetData();
}

void xiiJointOverrideComponent::OnAnimationPosePreparing(xiiMsgAnimationPosePreparing& msg) const
{
  using namespace ozz::math;

  if (m_uiJointIndex == xiiInvalidJointIndex)
  {
    m_uiJointIndex = msg.m_pSkeleton->FindJointByName(m_sJointToOverride);
  }

  if (m_uiJointIndex == xiiInvalidJointIndex)
    return;

  const int soaIdx    = m_uiJointIndex / 4;
  const int soaSubIdx = m_uiJointIndex % 4;

  const xiiTransform t = GetOwner()->GetLocalTransform();

  if (m_bOverridePosition)
  {
    SimdFloat4 vx = ozz::math::simd_float4::Load1(t.m_vPosition.x);
    SimdFloat4 vy = ozz::math::simd_float4::Load1(t.m_vPosition.y);
    SimdFloat4 vz = ozz::math::simd_float4::Load1(t.m_vPosition.z);

    auto val = msg.m_LocalTransforms[soaIdx].translation;

    val.x = ozz::math::SetI(val.x, vx, soaSubIdx);
    val.y = ozz::math::SetI(val.y, vy, soaSubIdx);
    val.z = ozz::math::SetI(val.z, vz, soaSubIdx);

    msg.m_LocalTransforms[soaIdx].translation = val;
  }

  if (m_bOverrideRotation)
  {
    SimdFloat4 vx = ozz::math::simd_float4::Load1(t.m_qRotation.x);
    SimdFloat4 vy = ozz::math::simd_float4::Load1(t.m_qRotation.y);
    SimdFloat4 vz = ozz::math::simd_float4::Load1(t.m_qRotation.z);
    SimdFloat4 vw = ozz::math::simd_float4::Load1(t.m_qRotation.w);

    SoaQuaternion val = msg.m_LocalTransforms[soaIdx].rotation;

    val.x = ozz::math::SetI(val.x, vx, soaSubIdx);
    val.y = ozz::math::SetI(val.y, vy, soaSubIdx);
    val.z = ozz::math::SetI(val.z, vz, soaSubIdx);
    val.w = ozz::math::SetI(val.w, vw, soaSubIdx);

    msg.m_LocalTransforms[soaIdx].rotation = val;
  }

  if (m_bOverrideScale)
  {
    SimdFloat4 vx = ozz::math::simd_float4::Load1(t.m_vScale.x);
    SimdFloat4 vy = ozz::math::simd_float4::Load1(t.m_vScale.y);
    SimdFloat4 vz = ozz::math::simd_float4::Load1(t.m_vScale.z);

    auto val = msg.m_LocalTransforms[soaIdx].scale;

    val.x = ozz::math::SetI(val.x, vx, soaSubIdx);
    val.y = ozz::math::SetI(val.y, vy, soaSubIdx);
    val.z = ozz::math::SetI(val.z, vz, soaSubIdx);

    msg.m_LocalTransforms[soaIdx].scale = val;
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_JointOverrideComponent);
