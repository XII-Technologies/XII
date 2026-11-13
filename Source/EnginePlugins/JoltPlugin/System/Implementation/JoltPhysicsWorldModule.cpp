/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Math.h>
#include <JoltPlugin/System/JoltPhysicsWorldModule.h>

#include <JoltPlugin/System/Implementation/JoltJobSystem.h>

#include <Jolt/Jolt.h>

#include <Jolt/Core/FPFlushDenormals.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
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
    m_BodiesByComponent(pAllocator),
    m_OwnersByBody(pAllocator)
  {
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
  };

  /// One body per owning component. The owner handles must be live in this world.
  /// The supplied layer uses XII's collision-layer index. Motion type determines broadphase placement.
  [[nodiscard]] xiiResult CreateBody(const xiiWorld& world, xiiComponentHandle hComponent, xiiUInt8 uiCollisionLayer, const JPH::BodyCreationSettings& settings)
  {
    if (hComponent.IsInvalidated() || hComponent.GetInternalID().m_WorldIndex != world.GetIndex() || m_BodiesByComponent.Contains(hComponent))
      return XII_FAILURE;

    const xiiComponent* pComponent = nullptr;
    if (!world.TryGetComponent(hComponent, pComponent) || pComponent->GetOwner() == nullptr)
      return XII_FAILURE;

    JPH::BodyCreationSettings bodySettings = settings;
    bodySettings.mObjectLayer              = xiiJoltCollisionFilters::MakeObjectLayer(uiCollisionLayer, settings.mMotionType);

    const auto activation = settings.mMotionType == JPH::EMotionType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate;
    const auto bodyId     = m_PhysicsSystem.GetBodyInterface().CreateAndAddBody(bodySettings, activation);
    if (bodyId.IsInvalid())
      return XII_FAILURE;

    m_BodiesByComponent.Insert(hComponent, bodyId);
    m_OwnersByBody.Insert(bodyId.GetIndexAndSequenceNumber(), BodyOwner{pComponent->GetOwner()->GetHandle(), hComponent});
    return XII_SUCCESS;
  }

  JPH::BodyID FindBody(xiiComponentHandle hComponent) const
  {
    JPH::BodyID bodyId;
    m_BodiesByComponent.TryGetValue(hComponent, bodyId);
    return bodyId;
  }

  /// Use native IDs from the current step. XII handles must still be resolved before accessing their objects.
  bool TryGetBodyOwner(JPH::BodyID bodyId, BodyOwner& out_owner) const
  {
    return m_OwnersByBody.TryGetValue(bodyId.GetIndexAndSequenceNumber(), out_owner);
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

void xiiJoltPhysicsWorldModule::Simulate(const UpdateContext&)
{
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

  if (errors != JPH::EPhysicsUpdateError::None)
  {
    xiiLog::Warning("Jolt simulation in world '{}' exceeded its contact capacity (flags {}).", GetWorld()->GetName(), static_cast<xiiUInt32>(errors));
  }
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_Implementation_JoltPhysicsWorldModule);
