#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiAnimationControllerComponent, 1, xiiComponentMode::Static);
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("AnimController", GetAnimationControllerFile, SetAnimationControllerFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Graph")),

    XII_ENUM_MEMBER_PROPERTY("RootMotionMode", xiiRootMotionMode, m_RootMotionMode),
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

void xiiAnimationControllerComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hAnimationController;
  s << m_RootMotionMode;
}

void xiiAnimationControllerComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = stream.GetStream();

  s >> m_hAnimationController;
  s >> m_RootMotionMode;
}

void xiiAnimationControllerComponent::SetAnimationControllerFile(const char* szFile)
{
  xiiAnimGraphResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiAnimGraphResource>(szFile);
  }

  m_hAnimationController = hResource;
}


const char* xiiAnimationControllerComponent::GetAnimationControllerFile() const
{
  if (!m_hAnimationController.IsValid())
    return "";

  return m_hAnimationController.GetResourceID();
}

void xiiAnimationControllerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (!m_hAnimationController.IsValid())
    return;

  xiiMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  if (!msg.m_hSkeleton.IsValid())
    return;

  xiiResourceLock<xiiAnimGraphResource> pAnimController(m_hAnimationController, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimController.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  pAnimController->DeserializeAnimGraphState(m_AnimationGraph);

  m_AnimationGraph.Configure(msg.m_hSkeleton, m_PoseGenerator, xiiBlackboardComponent::FindBlackboard(GetOwner()));
}

void xiiAnimationControllerComponent::Update()
{
  m_AnimationGraph.Update(GetWorld()->GetClock().GetTimeDiff(), GetOwner());

  xiiVec3  translation;
  xiiAngle rotationX;
  xiiAngle rotationY;
  xiiAngle rotationZ;
  m_AnimationGraph.GetRootMotion(translation, rotationX, rotationY, rotationZ);

  xiiRootMotionMode::Apply(m_RootMotionMode, GetOwner(), translation, rotationX, rotationY, rotationZ);
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimationControllerComponent);
