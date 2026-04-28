/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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
XII_BEGIN_COMPONENT_TYPE(xiiAnimationControllerComponent, 3, xiiComponentMode::Static);
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_MEMBER_PROPERTY("AnimGraph", m_hAnimGraph)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Graph")),

    XII_ENUM_MEMBER_PROPERTY("RootMotionMode", xiiRootMotionMode, m_RootMotionMode),
    XII_ENUM_MEMBER_PROPERTY("InvisibleUpdateRate", xiiAnimationInvisibleUpdateRate, m_InvisibleUpdateRate),
    XII_MEMBER_PROPERTY("EnableIK", m_bEnableIK),
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
  s << m_bEnableIK;
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

  if (uiVersion >= 3)
  {
    s >> m_bEnableIK;
  }
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
  xiiTime                  tMinStep = xiiTime::MakeFromSeconds(0);
  xiiVisibilityState::Enum visType  = GetOwner()->GetVisibilityState();

  if (visType != xiiVisibilityState::Direct)
  {
    if (m_InvisibleUpdateRate == xiiAnimationInvisibleUpdateRate::Pause && visType == xiiVisibilityState::Invisible)
      return;

    tMinStep = xiiAnimationInvisibleUpdateRate::GetTimeStep(m_InvisibleUpdateRate);
  }

  m_ElapsedTimeSinceUpdate += GetWorld()->GetClock().GetTimeDiff();

  if (m_ElapsedTimeSinceUpdate < tMinStep)
    return;

  m_AnimController.Update(m_ElapsedTimeSinceUpdate, GetOwner(), m_bEnableIK);
  m_ElapsedTimeSinceUpdate = xiiTime::MakeZero();

  xiiVec3  translation;
  xiiAngle rotationX;
  xiiAngle rotationY;
  xiiAngle rotationZ;
  m_AnimController.GetRootMotion(translation, rotationX, rotationY, rotationZ);

  xiiRootMotionMode::Apply(m_RootMotionMode, GetOwner(), translation, rotationX, rotationY, rotationZ);
}

//////////////////////////////////////////////////////////////////////////


xiiAnimationControllerComponentManager::xiiAnimationControllerComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<class xiiAnimationControllerComponent, xiiBlockStorageType::FreeList>(pWorld)
{
}

void xiiAnimationControllerComponentManager::Initialize()
{
  auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiAnimationControllerComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase                     = xiiWorldUpdatePhase::PreAsync; // TODO: currently can't run in Async phase

  this->RegisterUpdateFunction(desc);

  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiAnimationControllerComponentManager::ResourceEvent, this));
}

void xiiAnimationControllerComponentManager::Deinitialize()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiAnimationControllerComponentManager::ResourceEvent, this));
}

void xiiAnimationControllerComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  {
    for (auto hComponent : m_ComponentsToReset)
    {
      xiiAnimationControllerComponent* pComp;
      if (GetWorld()->TryGetComponent(hComponent, pComp))
      {
        pComp->OnSimulationStarted(); // just run this again
      }
    }

    m_ComponentsToReset.Clear();
  }

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

void xiiAnimationControllerComponentManager::ResourceEvent(const xiiResourceEvent& e)
{
  if (e.m_Type == xiiResourceEvent::Type::ResourceContentUnloading)
  {
    if (e.m_pResource->GetDynamicRTTI() == xiiGetStaticRTTI<xiiAnimGraphResource>())
    {
      xiiAnimGraphResourceHandle hResource((xiiAnimGraphResource*)(e.m_pResource));

      for (auto it = GetComponents(); it.IsValid(); it.Next())
      {
        if (!it->IsActiveAndSimulating())
          continue;

        if (it->m_hAnimGraph == hResource)
        {
          if (!m_ComponentsToReset.Contains(it->GetHandle()))
          {
            m_ComponentsToReset.PushBack(it->GetHandle());
          }
        }
      }
    }
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimationControllerComponent);
