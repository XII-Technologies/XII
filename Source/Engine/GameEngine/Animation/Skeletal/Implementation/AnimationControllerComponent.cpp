#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiAnimationControllerComponent, 2, xiiComponentMode::Static);
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("AnimGraph", GetAnimGraphFile, SetAnimGraphFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Graph")),

    XII_ENUM_MEMBER_PROPERTY("RootMotionMode", xiiRootMotionMode, m_RootMotionMode),
    XII_ENUM_MEMBER_PROPERTY("InvisibleUpdateRate", xiiAnimationInvisibleUpdateRate, m_InvisibleUpdateRate),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
      new xiiCategoryAttribute("Animation"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiAnimationControllerComponent::xiiAnimationControllerComponent()  = default;
xiiAnimationControllerComponent::~xiiAnimationControllerComponent() = default;

void xiiAnimationControllerComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hAnimGraph;
  s << m_RootMotionMode;
  s << m_InvisibleUpdateRate;
}

void xiiAnimationControllerComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = inout_stream.GetStream();

  s >> m_hAnimGraph;
  s >> m_RootMotionMode;

  if (uiVersion >= 2)
  {
    s >> m_InvisibleUpdateRate;
  }
}

void xiiAnimationControllerComponent::SetAnimGraphFile(const char* szFile)
{
  xiiAnimGraphResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiAnimGraphResource>(szFile);
  }

  m_hAnimGraph = hResource;
}


const char* xiiAnimationControllerComponent::GetAnimGraphFile() const
{
  if (!m_hAnimGraph.IsValid())
    return "";

  return m_hAnimGraph.GetResourceID();
}

void xiiAnimationControllerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (!m_hAnimGraph.IsValid())
    return;

  xiiMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  if (!msg.m_hSkeleton.IsValid())
    return;

  m_AnimController.Initialize(msg.m_hSkeleton, m_PoseGenerator, xiiBlackboardComponent::FindBlackboard(GetOwner()));
  m_AnimController.AddAnimGraph(m_hAnimGraph);
}

void xiiAnimationControllerComponent::Update()
{
  xiiTime            tMinStep = xiiTime::MakeFromSeconds(0);
  xiiVisibilityState visType  = GetOwner()->GetVisibilityState();

  if (visType != xiiVisibilityState::Direct)
  {
    if (m_InvisibleUpdateRate == xiiAnimationInvisibleUpdateRate::Pause && visType == xiiVisibilityState::Invisible)
      return;

    tMinStep = xiiAnimationInvisibleUpdateRate::GetTimeStep(m_InvisibleUpdateRate);
  }

  m_ElapsedTimeSinceUpdate += GetWorld()->GetClock().GetTimeDiff();

  if (m_ElapsedTimeSinceUpdate < tMinStep)
    return;

  m_AnimController.Update(m_ElapsedTimeSinceUpdate, GetOwner());
  m_ElapsedTimeSinceUpdate = xiiTime::Zero();

  xiiVec3  translation;
  xiiAngle rotationX;
  xiiAngle rotationY;
  xiiAngle rotationZ;
  m_AnimController.GetRootMotion(translation, rotationX, rotationY, rotationZ);

  xiiRootMotionMode::Apply(m_RootMotionMode, GetOwner(), translation, rotationX, rotationY, rotationZ);
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimationControllerComponent);
