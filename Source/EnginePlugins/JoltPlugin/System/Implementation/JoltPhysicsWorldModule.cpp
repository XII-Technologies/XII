/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Math.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Shapes/JoltPrimitiveShapeComponent.h>
#include <JoltPlugin/System/JoltPhysicsWorldModule.h>

#include <JoltPlugin/System/Implementation/JoltJobSystem.h>

#include <Jolt/Jolt.h>

#include <Jolt/Core/FPFlushDenormals.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

XII_IMPLEMENT_WORLD_MODULE(xiiJoltPhysicsWorldModule);

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltPhysicsWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  constexpr xiiUInt32 MaxBodies              = 4096;
  constexpr xiiUInt32 MaxBodyPairs           = 16384;
  constexpr xiiUInt32 MaxContactConstraints  = 8192;
  constexpr xiiUInt32 ScratchBufferSize      = 8 * 1024 * 1024;
  constexpr xiiUInt32 CollisionLayerCount    = 256;
  constexpr xiiUInt32 CollisionMaskWordBits  = 64;
  constexpr xiiUInt32 CollisionMaskWordCount = CollisionLayerCount / CollisionMaskWordBits;

  constexpr JPH::BroadPhaseLayer StaticBroadPhase(0);
  constexpr JPH::BroadPhaseLayer MovingBroadPhase(1);

  JPH::Vec3 ToJoltVector(const xiiVec3& vValue) { return JPH::Vec3(vValue.x, vValue.y, vValue.z); }
  JPH::Quat ToJoltRotation(const xiiQuat& qValue) { return JPH::Quat(qValue.x, qValue.y, qValue.z, qValue.w).Normalized(); }

  const xiiJoltActorComponent* FindActiveActor(const xiiGameObject& object)
  {
    for (const auto* pComponent : object.GetComponents())
    {
      if (const auto* pActor = xiiDynamicCast<const xiiJoltActorComponent*>(pComponent); pActor != nullptr && pActor->IsActiveAndSimulating())
        return pActor;
    }
    return nullptr;
  }

  // A nested actor owns its own subtree. Stop at the second shape so invalid setups
  // cannot make the choice of collider depend on component traversal order.
  bool CollectShape(xiiGameObject& object, xiiJoltShapeComponent*& ref_pShape)
  {
    for (auto* pComponent : object.GetComponents())
    {
      if (auto* pShape = xiiDynamicCast<xiiJoltShapeComponent*>(pComponent); pShape != nullptr && pShape->IsActiveAndInitialized())
      {
        if (ref_pShape != nullptr)
          return false;
        ref_pShape = pShape;
      }
    }

    for (auto it = object.GetChildren(); it.IsValid(); ++it)
    {
      if (FindActiveActor(*it) == nullptr && !CollectShape(*it, ref_pShape))
        return false;
    }
    return true;
  }

  JPH::ShapeSettings::ShapeResult CreatePrimitive(const xiiJoltShapeComponent& component, const xiiVec3& vScale, float fDensity)
  {
    if (const auto* pBox = xiiDynamicCast<const xiiJoltBoxShapeComponent*>(&component))
    {
      const xiiVec3 vHalfExtents = pBox->GetDimensions().CompMul(vScale) * 0.5f;
      if (vHalfExtents.IsValid() && vHalfExtents.x > 0.0f && vHalfExtents.y > 0.0f && vHalfExtents.z > 0.0f)
      {
        const float           fMinExtent = xiiMath::Min(vHalfExtents.x, vHalfExtents.y, vHalfExtents.z);
        JPH::BoxShapeSettings settings(ToJoltVector(vHalfExtents), xiiMath::Min(JPH::cDefaultConvexRadius, 0.5f * fMinExtent));
        settings.SetDensity(fDensity);
        return settings.Create();
      }
    }
    else if (const auto* pSphere = xiiDynamicCast<const xiiJoltSphereShapeComponent*>(&component))
    {
      const float fRadius = pSphere->GetRadius() * vScale.x;
      if (xiiMath::IsFinite(fRadius) && fRadius > 0.0f && vScale.IsEqual(xiiVec3(vScale.x), 0.00001f * vScale.x))
      {
        JPH::SphereShapeSettings settings(fRadius);
        settings.SetDensity(fDensity);
        return settings.Create();
      }
    }

    JPH::ShapeSettings::ShapeResult result;
    result.SetError("Box dimensions must be positive and sphere scale must be positive and uniform.");
    return result;
  }

  class xiiJoltCollisionFilters final :
    public JPH::BroadPhaseLayerInterface,
    public JPH::ObjectVsBroadPhaseLayerFilter,
    public JPH::ObjectLayerPairFilter
  {
  public:
    xiiJoltCollisionFilters()
    {
      for (auto& mask : m_CollisionMasks)
      {
        for (auto& uiWord : mask)
        {
          uiWord = ~xiiUInt64{0};
        }
      }
    }

    static JPH::ObjectLayer MakeObjectLayer(xiiUInt8 uiCollisionLayer, JPH::EMotionType motionType)
    {
      return static_cast<JPH::ObjectLayer>(uiCollisionLayer + (motionType == JPH::EMotionType::Static ? 0U : CollisionLayerCount));
    }

    void SetCollisionEnabled(xiiUInt8 uiLayerA, xiiUInt8 uiLayerB, bool bEnabled)
    {
      SetCollisionBit(uiLayerA, uiLayerB, bEnabled);
      SetCollisionBit(uiLayerB, uiLayerA, bEnabled);
    }

    JPH::uint GetNumBroadPhaseLayers() const override { return 2; }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
      return layer < CollisionLayerCount ? StaticBroadPhase : MovingBroadPhase;
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    {
      return layer == StaticBroadPhase ? "Static" : "Moving";
    }
#endif

    bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer broadPhaseLayer) const override
    {
      return layer >= CollisionLayerCount || broadPhaseLayer == MovingBroadPhase;
    }

    bool ShouldCollide(JPH::ObjectLayer layerA, JPH::ObjectLayer layerB) const override
    {
      if (layerA < CollisionLayerCount && layerB < CollisionLayerCount)
        return false;

      const auto uiLayerA = static_cast<xiiUInt8>(layerA);
      const auto uiLayerB = static_cast<xiiUInt8>(layerB);
      return (m_CollisionMasks[uiLayerA][uiLayerB / CollisionMaskWordBits] & CollisionBit(uiLayerB)) != 0;
    }

  private:
    static constexpr xiiUInt64 CollisionBit(xiiUInt8 uiLayer)
    {
      return xiiUInt64{1} << (uiLayer % CollisionMaskWordBits);
    }

    void SetCollisionBit(xiiUInt8 uiLayer, xiiUInt8 uiOtherLayer, bool bEnabled)
    {
      xiiUInt64&      uiWord = m_CollisionMasks[uiLayer][uiOtherLayer / CollisionMaskWordBits];
      const xiiUInt64 uiBit  = CollisionBit(uiOtherLayer);
      if (bEnabled)
      {
        uiWord |= uiBit;
      }
      else
      {
        uiWord &= ~uiBit;
      }
    }

    xiiUInt64 m_CollisionMasks[CollisionLayerCount][CollisionMaskWordCount];
  };
} // namespace

struct xiiJoltPhysicsWorldModule::NativeWorld
{
  explicit NativeWorld(xiiAllocator* pAllocator) :
    m_PendingBodies(pAllocator),
    m_PosesToSync(pAllocator),
    m_BodiesByComponent(pAllocator),
    m_OwnersByBody(pAllocator)
  {
    m_PosesToSync.Reserve(MaxBodies);
    m_PhysicsSystem.Init(MaxBodies, 0, MaxBodyPairs, MaxContactConstraints, m_CollisionFilters, m_CollisionFilters, m_CollisionFilters);
    m_PhysicsSystem.SetGravity(JPH::Vec3(0.0f, 0.0f, -9.81f));
  }

  ~NativeWorld()
  {
    m_JobSystem.DrainTasks();
    ClearBodies();
  }

  struct BodyOwner
  {
    xiiGameObjectHandle m_hObject;
    xiiComponentHandle  m_hComponent;
    bool                m_bDynamic;
  };

  struct DynamicPose
  {
    xiiGameObjectHandle m_hObject;
    xiiVec3             m_vPosition;
    xiiQuat             m_qRotation;
    xiiUInt32           m_uiDepth;
  };

  /// One body per owning component. The owner handles must be live in this world.
  /// The supplied layer uses XII's collision-layer index. Motion type determines broadphase placement.
  [[nodiscard]] xiiResult CreateBody(const xiiWorld& world, xiiComponentHandle hComponent, xiiUInt8 uiCollisionLayer, JPH::BodyCreationSettings& ref_settings, JPH::EActivation activation)
  {
    if (hComponent.IsInvalidated() || hComponent.GetInternalID().m_WorldIndex != world.GetIndex())
      return XII_FAILURE;

    const xiiComponent* pComponent = nullptr;
    if (!world.TryGetComponent(hComponent, pComponent) || pComponent->GetOwner() == nullptr)
      return XII_FAILURE;

    ref_settings.mObjectLayer = xiiJoltCollisionFilters::MakeObjectLayer(uiCollisionLayer, ref_settings.mMotionType);
    auto& bodies              = m_PhysicsSystem.GetBodyInterface();
    auto* pBody               = bodies.CreateBody(ref_settings);
    if (pBody == nullptr)
      return XII_FAILURE;

    const auto bodyId = pBody->GetID();
    RemoveBody(hComponent);
    m_BodiesByComponent.Insert(hComponent, bodyId);
    m_OwnersByBody.Insert(bodyId.GetIndexAndSequenceNumber(), BodyOwner{pComponent->GetOwner()->GetHandle(), hComponent, ref_settings.mMotionType == JPH::EMotionType::Dynamic});
    bodies.AddBody(bodyId, activation);
    return XII_SUCCESS;
  }

  JPH::BodyID FindBody(xiiComponentHandle hComponent) const
  {
    JPH::BodyID bodyId;
    m_BodiesByComponent.TryGetValue(hComponent, bodyId);
    return bodyId;
  }

  bool RemoveBody(xiiComponentHandle hComponent)
  {
    JPH::BodyID bodyId;
    if (!m_BodiesByComponent.Remove(hComponent, &bodyId))
      return false;

    m_OwnersByBody.Remove(bodyId.GetIndexAndSequenceNumber());
    DestroyBody(bodyId);
    return true;
  }

  void ClearBodies()
  {
    m_PendingBodies.Clear();
    m_PosesToSync.Clear();
    for (auto it = m_BodiesByComponent.GetIterator(); it.IsValid(); ++it)
    {
      DestroyBody(it.Value());
    }

    m_OwnersByBody.Clear();
    m_BodiesByComponent.Clear();
  }

  xiiJoltCollisionFilters                  m_CollisionFilters;
  xiiJoltJobSystem                         m_JobSystem;
  JPH::TempAllocatorImplWithMallocFallback m_TempAllocator{ScratchBufferSize};
  JPH::PhysicsSystem                       m_PhysicsSystem;
  xiiHashSet<xiiComponentHandle>           m_PendingBodies;
  xiiDynamicArray<DynamicPose>             m_PosesToSync;

  const auto& GetBodyOwners() const { return m_OwnersByBody; }

private:
  void DestroyBody(JPH::BodyID bodyId)
  {
    auto& bodies = m_PhysicsSystem.GetBodyInterface();
    bodies.RemoveBody(bodyId);
    bodies.DestroyBody(bodyId);
  }

  xiiHashTable<xiiComponentHandle, JPH::BodyID> m_BodiesByComponent;
  xiiHashTable<xiiUInt32, BodyOwner>            m_OwnersByBody;
};

xiiJoltPhysicsWorldModule::xiiJoltPhysicsWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

xiiJoltPhysicsWorldModule::~xiiJoltPhysicsWorldModule() = default;

xiiResult xiiJoltPhysicsWorldModule::SetSimulationStep(xiiTime fixedStep, xiiUInt32 uiMaxCatchUpSteps)
{
  const float fStepSeconds = fixedStep.AsFloatInSeconds();
  if (!xiiMath::IsFinite(fStepSeconds) || fStepSeconds <= 0.0f || uiMaxCatchUpSteps == 0)
    return XII_FAILURE;

  m_FixedStep         = fixedStep;
  m_uiMaxCatchUpSteps = uiMaxCatchUpSteps;
  m_AccumulatedTime   = xiiTime::MakeZero();
  return XII_SUCCESS;
}

xiiResult xiiJoltPhysicsWorldModule::SetCollisionEnabled(xiiUInt8 uiLayerA, xiiUInt8 uiLayerB, bool bEnabled)
{
  if (!m_pNativeWorld || m_pNativeWorld->m_PhysicsSystem.GetNumBodies() != 0)
    return XII_FAILURE;

  m_pNativeWorld->m_CollisionFilters.SetCollisionEnabled(uiLayerA, uiLayerB, bEnabled);
  return XII_SUCCESS;
}

void xiiJoltPhysicsWorldModule::Initialize()
{
  m_pNativeWorld = XII_NEW(xiiFoundation::GetAlignedAllocator(), NativeWorld, GetAllocator());

  auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiJoltPhysicsWorldModule::Simulate, this);
  desc.m_Phase                     = xiiWorldUpdatePhase::PreAsync;
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_fPriority                 = -100000.0f;
  RegisterUpdateFunction(desc);
}

void xiiJoltPhysicsWorldModule::Deinitialize()
{
  auto desc    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiJoltPhysicsWorldModule::Simulate, this);
  desc.m_Phase = xiiWorldUpdatePhase::PreAsync;
  DeregisterUpdateFunction(desc);

  m_pNativeWorld.Clear();
}

void xiiJoltPhysicsWorldModule::OnSimulationStarted()
{
  m_AccumulatedTime = xiiTime::MakeZero();
}

void xiiJoltPhysicsWorldModule::WorldClear()
{
  m_pNativeWorld->ClearBodies();
  m_AccumulatedTime = xiiTime::MakeZero();
}

void xiiJoltPhysicsWorldModule::QueueBodyUpdate(xiiComponentHandle hActor)
{
  if (m_pNativeWorld)
    m_pNativeWorld->m_PendingBodies.Insert(hActor);
}

void xiiJoltPhysicsWorldModule::RemoveBody(xiiComponentHandle hActor)
{
  if (m_pNativeWorld)
  {
    m_pNativeWorld->m_PendingBodies.Remove(hActor);
    m_pNativeWorld->RemoveBody(hActor);
  }
}

JPH::BodyInterface* xiiJoltPhysicsWorldModule::FindDynamicBody(xiiComponentHandle hActor, JPH::BodyID& out_bodyId) const
{
  if (!m_pNativeWorld)
    return nullptr;

  out_bodyId   = m_pNativeWorld->FindBody(hActor);
  auto& bodies = m_pNativeWorld->m_PhysicsSystem.GetBodyInterface();
  return !out_bodyId.IsInvalid() && bodies.GetMotionType(out_bodyId) == JPH::EMotionType::Dynamic ? &bodies : nullptr;
}

void xiiJoltPhysicsWorldModule::RebuildBody(xiiJoltActorComponent& actor)
{
  auto& nativeWorld = *m_pNativeWorld;
  auto& object      = *actor.GetOwner();
  if (FindActiveActor(object) != &actor)
  {
    nativeWorld.RemoveBody(actor.GetHandle());
    xiiLog::Warning("Jolt object '{}' supports one active actor.", object.GetName());
    return;
  }

  xiiJoltShapeComponent* pShape = nullptr;
  if (!CollectShape(object, pShape))
  {
    nativeWorld.RemoveBody(actor.GetHandle());
    xiiLog::Warning("Jolt actor '{}' requires a single active primitive shape.", object.GetName());
    return;
  }
  if (pShape == nullptr)
  {
    nativeWorld.RemoveBody(actor.GetHandle());
    return; // A collider that activates later will queue the actor again.
  }

  object.UpdateGlobalTransform();
  auto* pShapeObject = pShape->GetOwner();
  if (pShapeObject != &object)
    pShapeObject->UpdateGlobalTransform();

  xiiTransform actorPose            = object.GetGlobalTransform();
  actorPose.m_vScale                = xiiVec3(1.0f);
  const xiiTransform shapeTransform = pShapeObject->GetGlobalTransform();
  if (!actorPose.m_vPosition.IsValid() || !actorPose.m_qRotation.IsValid() ||
      !shapeTransform.m_vPosition.IsValid() || !shapeTransform.m_qRotation.IsValid() || !shapeTransform.m_vScale.IsValid())
  {
    nativeWorld.RemoveBody(actor.GetHandle());
    xiiLog::Warning("Jolt actor '{}' has an invalid actor or shape transform.", object.GetName());
    return;
  }

  const auto* pDynamic    = xiiDynamicCast<const xiiJoltDynamicActorComponent*>(&actor);
  auto        shapeResult = CreatePrimitive(*pShape, shapeTransform.m_vScale.Abs(), pDynamic != nullptr ? pDynamic->GetDensity() : 1000.0f);
  if (shapeResult.HasError())
  {
    nativeWorld.RemoveBody(actor.GetHandle());
    xiiLog::Warning("Jolt actor '{}': {}", object.GetName(), shapeResult.GetError().c_str());
    return;
  }

  JPH::RefConst<JPH::Shape> pNativeShape = shapeResult.Get().GetPtr();
  if (pShapeObject != &object)
  {
    const xiiTransform local = xiiTransform::MakeLocalTransform(actorPose, shapeTransform);
    if (!local.m_vPosition.IsValid() || !local.m_qRotation.IsValid())
    {
      nativeWorld.RemoveBody(actor.GetHandle());
      xiiLog::Warning("Jolt actor '{}' has an invalid relative shape transform.", object.GetName());
      return;
    }
    JPH::RotatedTranslatedShapeSettings settings(ToJoltVector(local.m_vPosition), ToJoltRotation(local.m_qRotation), pNativeShape.GetPtr());
    auto                                result = settings.Create();
    if (result.HasError())
    {
      nativeWorld.RemoveBody(actor.GetHandle());
      xiiLog::Warning("Jolt actor '{}': {}", object.GetName(), result.GetError().c_str());
      return;
    }
    pNativeShape = result.Get().GetPtr();
  }

  JPH::BodyCreationSettings settings(pNativeShape.GetPtr(),
                                     JPH::RVec3(actorPose.m_vPosition.x, actorPose.m_vPosition.y, actorPose.m_vPosition.z),
                                     ToJoltRotation(actorPose.m_qRotation), pDynamic != nullptr ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static, 0);
  auto                      activation = JPH::EActivation::DontActivate;
  if (pDynamic != nullptr)
  {
    settings.mGravityFactor  = pDynamic->GetGravityFactor();
    settings.mLinearDamping  = pDynamic->GetLinearDamping();
    settings.mAngularDamping = pDynamic->GetAngularDamping();
    settings.mAllowSleeping  = pDynamic->GetAllowSleeping();
    auto massProperties      = pNativeShape->GetMassProperties();
    if (pDynamic->GetMass() > 0.0f)
      massProperties.ScaleToMass(pDynamic->GetMass());

    const auto inertia = massProperties.mInertia.GetDiagonal3();
    if (!xiiMath::IsFinite(massProperties.mMass) || massProperties.mMass <= 0.0f || !xiiMath::IsFinite(1.0f / massProperties.mMass) ||
        !xiiMath::IsFinite(inertia.GetX()) || !xiiMath::IsFinite(inertia.GetY()) || !xiiMath::IsFinite(inertia.GetZ()) || inertia.ReduceMin() <= 0.0f)
    {
      nativeWorld.RemoveBody(actor.GetHandle());
      xiiLog::Warning("Jolt actor '{}' has invalid mass properties.", object.GetName());
      return;
    }
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
    settings.mMassPropertiesOverride = massProperties;

    const auto oldBody = nativeWorld.FindBody(actor.GetHandle());
    if (!oldBody.IsInvalid())
    {
      const auto& bodies = nativeWorld.m_PhysicsSystem.GetBodyInterface();
      bodies.GetPositionAndRotation(oldBody, settings.mPosition, settings.mRotation);
      bodies.GetLinearAndAngularVelocity(oldBody, settings.mLinearVelocity, settings.mAngularVelocity);
      if (bodies.IsActive(oldBody) || !settings.mAllowSleeping)
        activation = JPH::EActivation::Activate;
    }
    else
    {
      settings.mLinearVelocity  = ToJoltVector(pDynamic->GetInitialLinearVelocity());
      settings.mAngularVelocity = ToJoltVector(pDynamic->GetInitialAngularVelocity());
      if (!pDynamic->GetStartAsleep() || !settings.mAllowSleeping)
        activation = JPH::EActivation::Activate;
    }
  }

  if (nativeWorld.CreateBody(*GetWorld(), actor.GetHandle(), actor.GetCollisionLayer(), settings, activation).Failed())
    xiiLog::Warning("Jolt could not allocate a body for '{}'.", object.GetName());
}

void xiiJoltPhysicsWorldModule::SynchronizeDynamicBodies()
{
  auto&       nativeWorld = *m_pNativeWorld;
  auto&       poses       = nativeWorld.m_PosesToSync;
  const auto& bodies      = nativeWorld.m_PhysicsSystem.GetBodyInterfaceNoLock();
  for (auto it = nativeWorld.GetBodyOwners().GetIterator(); it.IsValid(); ++it)
  {
    const auto& owner = it.Value();
    if (!owner.m_bDynamic)
      continue;

    xiiGameObject* pObject = nullptr;
    if (!GetWorld()->TryGetObject(owner.m_hObject, pObject))
      continue;

    JPH::RVec3 position;
    JPH::Quat  rotation;
    bodies.GetPositionAndRotation(JPH::BodyID(it.Key()), position, rotation);
    auto& pose       = poses.ExpandAndGetRef();
    pose.m_hObject   = owner.m_hObject;
    pose.m_vPosition = xiiVec3(static_cast<float>(position.GetX()), static_cast<float>(position.GetY()), static_cast<float>(position.GetZ()));
    pose.m_qRotation = xiiQuat(rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW());
    pose.m_uiDepth   = 0;
    for (auto* pParent = pObject->GetParent(); pParent != nullptr; pParent = pParent->GetParent())
      ++pose.m_uiDepth;
  }

  poses.Sort([](const auto& a, const auto& b) { return a.m_uiDepth < b.m_uiDepth; });
  for (const auto& pose : poses)
  {
    xiiGameObject* pObject = nullptr;
    if (!GetWorld()->TryGetObject(pose.m_hObject, pObject))
      continue;

    if (pose.m_uiDepth != 0)
      pObject->UpdateGlobalTransform();

    auto          transform = pObject->GetGlobalTransform();
    const xiiQuat qOpposite(-pose.m_qRotation.x, -pose.m_qRotation.y, -pose.m_qRotation.z, -pose.m_qRotation.w);
    if (transform.m_vPosition == pose.m_vPosition && (transform.m_qRotation == pose.m_qRotation || transform.m_qRotation == qOpposite))
      continue;

    transform.m_vPosition = pose.m_vPosition;
    transform.m_qRotation = pose.m_qRotation;
    pObject->SetGlobalTransform(transform);
  }
  poses.Clear();
}

void xiiJoltPhysicsWorldModule::Simulate(const UpdateContext&)
{
  auto& pendingBodies = m_pNativeWorld->m_PendingBodies;
  for (const auto hActor : pendingBodies)
  {
    xiiJoltActorComponent* pActor = nullptr;
    if (GetWorld()->TryGetComponent(hActor, pActor) && pActor->IsActiveAndSimulating())
      RebuildBody(*pActor);
  }
  pendingBodies.Clear();

  const xiiClock& clock         = GetWorld()->GetClock();
  const double    fDeltaSeconds = clock.GetTimeDiff().GetSeconds();
  if (clock.GetPaused() || fDeltaSeconds <= 0.0 || !xiiMath::IsFinite(fDeltaSeconds) || m_pNativeWorld->m_PhysicsSystem.GetNumBodies() == 0)
    return;

  m_AccumulatedTime += xiiTime::MakeFromSeconds(fDeltaSeconds);
  if (m_AccumulatedTime < m_FixedStep)
    return;

  const double fStepSeconds  = m_FixedStep.GetSeconds();
  const double fElapsedSteps = xiiMath::Floor(m_AccumulatedTime.GetSeconds() / fStepSeconds);
  const auto   uiSteps       = static_cast<xiiUInt32>(xiiMath::Min(fElapsedSteps, static_cast<double>(m_uiMaxCatchUpSteps)));

  // Drop whole steps beyond the catch-up limit and retain the fractional remainder.
  if (fElapsedSteps > m_uiMaxCatchUpSteps)
  {
    m_AccumulatedTime = xiiTime::MakeFromSeconds(xiiMath::Mod(m_AccumulatedTime.GetSeconds(), fStepSeconds));
  }
  else
  {
    m_AccumulatedTime = xiiTime::MakeFromSeconds(xiiMath::Max(0.0, m_AccumulatedTime.GetSeconds() - fStepSeconds * uiSteps));
  }

  [[maybe_unused]] const JPH::FPFlushDenormals flushDenormals;
  auto&                                        nativeWorld = *m_pNativeWorld;
  auto                                         errors      = JPH::EPhysicsUpdateError::None;
  for (xiiUInt32 uiStep = 0; uiStep < uiSteps; ++uiStep)
  {
    errors |= nativeWorld.m_PhysicsSystem.Update(m_FixedStep.AsFloatInSeconds(), 1, &nativeWorld.m_TempAllocator, &nativeWorld.m_JobSystem);
  }

  SynchronizeDynamicBodies();

  if (errors != JPH::EPhysicsUpdateError::None)
  {
    xiiLog::Warning("Jolt simulation in world '{}' exceeded its contact capacity (flags {}).", GetWorld()->GetName(), static_cast<xiiUInt32>(errors));
  }
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_Implementation_JoltPhysicsWorldModule);
