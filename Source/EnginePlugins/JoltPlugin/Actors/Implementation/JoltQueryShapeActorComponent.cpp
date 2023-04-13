#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <JoltPlugin/Actors/JoltQueryShapeActorComponent.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

xiiJoltQueryShapeActorComponentManager::xiiJoltQueryShapeActorComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<xiiJoltQueryShapeActorComponent, xiiBlockStorageType::FreeList>(pWorld)
{
}

xiiJoltQueryShapeActorComponentManager::~xiiJoltQueryShapeActorComponentManager() = default;

void xiiJoltQueryShapeActorComponentManager::UpdateMovingQueryShapes()
{
  XII_PROFILE_SCOPE("UpdateMovingQueryShapes");

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  auto*               pSystem = pModule->GetJoltSystem();
  auto*               pBodies = &pSystem->GetBodyInterface();

  for (auto pComponent : m_MovingQueryShapes)
  {
    JPH::BodyID bodyId(pComponent->m_uiJoltBodyID);

    if (bodyId.IsInvalid())
      continue;

    xiiGameObject* pObject = pComponent->GetOwner();

    pObject->UpdateGlobalTransform();

    const xiiSimdVec4f pos = pObject->GetGlobalPositionSimd();
    const xiiSimdQuat  rot = pObject->GetGlobalRotationSimd();

    pBodies->SetPositionAndRotation(bodyId, xiiJoltConversionUtils::ToVec3(pos), xiiJoltConversionUtils::ToQuat(rot), JPH::EActivation::DontActivate);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltQueryShapeActorComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface", xiiDependencyFlags::Package)),
  }
  XII_END_PROPERTIES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiJoltQueryShapeActorComponent::xiiJoltQueryShapeActorComponent()  = default;
xiiJoltQueryShapeActorComponent::~xiiJoltQueryShapeActorComponent() = default;

void xiiJoltQueryShapeActorComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hSurface;
}

void xiiJoltQueryShapeActorComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hSurface;
}

void xiiJoltQueryShapeActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  xiiJoltWorldModule*    pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  const xiiSimdTransform trans   = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  JPH::BodyCreationSettings bodyCfg;

  if (CreateShape(&bodyCfg, 1.0f, GetJoltMaterial()).Failed())
  {
    xiiLog::Error("Jolt query-shape actor component '{}' has no valid shape.", GetOwner()->GetName());
    return;
  }

  xiiJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex          = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  bodyCfg.mPosition      = xiiJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation      = xiiJoltConversionUtils::ToQuat(trans.m_Rotation);
  bodyCfg.mMotionType    = JPH::EMotionType::Static;
  bodyCfg.mObjectLayer   = xiiJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, xiiJoltBroadphaseLayer::Query);
  bodyCfg.mMotionQuality = JPH::EMotionQuality::Discrete;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  // bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter()); // the group filter is only needed for objects constrained via joints
  bodyCfg.mUserData = reinterpret_cast<xiiUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  m_uiJoltBodyID   = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, true);

  if (GetOwner()->IsDynamic())
  {
    GetWorld()->GetOrCreateComponentManager<xiiJoltQueryShapeActorComponentManager>()->m_MovingQueryShapes.PushBack(this);
  }
}

void xiiJoltQueryShapeActorComponent::OnDeactivated()
{
  if (GetOwner()->IsDynamic())
  {
    GetWorld()->GetOrCreateComponentManager<xiiJoltQueryShapeActorComponentManager>()->m_MovingQueryShapes.RemoveAndSwap(this);
  }

  SUPER::OnDeactivated();
}

void xiiJoltQueryShapeActorComponent::SetSurfaceFile(const char* szFile)
{
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    m_hSurface = xiiResourceManager::LoadResource<xiiSurfaceResource>(szFile);
  }

  if (m_hSurface.IsValid())
    xiiResourceManager::PreloadResource(m_hSurface);
}

const char* xiiJoltQueryShapeActorComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}

const xiiJoltMaterial* xiiJoltQueryShapeActorComponent::GetJoltMaterial() const
{
  if (m_hSurface.IsValid())
  {
    xiiResourceLock<xiiSurfaceResource> pSurface(m_hSurface, xiiResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      return static_cast<xiiJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  return nullptr;
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltQueryShapeActorComponent);
