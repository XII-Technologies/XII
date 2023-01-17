#include <JoltPlugin/JoltPluginPCH.h>

#include <GameEngine/Physics/CollisionFilter.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>

namespace xiiJoltCollisionFiltering
{
  xiiCollisionFilterConfig s_CollisionFilterConfig;

  JPH::ObjectLayer ConstructObjectLayer(xiiUInt8 uiCollisionGroup, xiiJoltBroadphaseLayer broadphase)
  {
    return static_cast<JPH::ObjectLayer>(static_cast<xiiUInt16>(broadphase) << 8 | static_cast<xiiUInt16>(uiCollisionGroup));
  }

  void LoadCollisionFilters()
  {
    XII_LOG_BLOCK("xiiJoltCore::LoadCollisionFilters");

    if (s_CollisionFilterConfig.Load("RuntimeConfigs/CollisionLayers.cfg").Failed())
    {
      xiiLog::Info("Collision filter config file could not be found ('RuntimeConfigs/CollisionLayers.cfg'). Using default values.");

      // setup some default config

      s_CollisionFilterConfig.SetGroupName(0, "Default");
      s_CollisionFilterConfig.EnableCollision(0, 0);
    }
  }

  xiiCollisionFilterConfig& GetCollisionFilterConfig()
  {
    return s_CollisionFilterConfig;
  }

  xiiUInt32 GetBroadphaseCollisionMask(xiiJoltBroadphaseLayer broadphase)
  {
    // this mapping defines which types of objects can generally collide with each other
    // if a flag is not included here, those types will never collide, no matter what their collision group is and other filter settings are
    // note that this is only used for the simulation, raycasts and shape queries can use their own mapping

    switch (broadphase)
    {
      case Static:
        return XII_BIT(xiiJoltBroadphaseLayer::Dynamic) | XII_BIT(xiiJoltBroadphaseLayer::Character) | XII_BIT(xiiJoltBroadphaseLayer::Ragdoll) | XII_BIT(xiiJoltBroadphaseLayer::Rope);

      case Dynamic:
        return XII_BIT(xiiJoltBroadphaseLayer::Static) | XII_BIT(xiiJoltBroadphaseLayer::Dynamic) | XII_BIT(xiiJoltBroadphaseLayer::Trigger) | XII_BIT(xiiJoltBroadphaseLayer::Character) | XII_BIT(xiiJoltBroadphaseLayer::Ragdoll) | XII_BIT(xiiJoltBroadphaseLayer::Rope);

      case Query:
        // query shapes never interact with anything in the simulation
        return 0;

      case Trigger:
        // triggers specifically exclude detail objects such as ropes, ragdolls and queries (also used for hitboxes) for performance reasons
        // if necessary, these shapes can still be found with overlap queries
        return XII_BIT(xiiJoltBroadphaseLayer::Dynamic) | XII_BIT(xiiJoltBroadphaseLayer::Character);

      case Character:
        return XII_BIT(xiiJoltBroadphaseLayer::Static) | XII_BIT(xiiJoltBroadphaseLayer::Dynamic) | XII_BIT(xiiJoltBroadphaseLayer::Trigger) | XII_BIT(xiiJoltBroadphaseLayer::Character);

      case Ragdoll:
        return XII_BIT(xiiJoltBroadphaseLayer::Static) | XII_BIT(xiiJoltBroadphaseLayer::Dynamic) | XII_BIT(xiiJoltBroadphaseLayer::Ragdoll) | XII_BIT(xiiJoltBroadphaseLayer::Rope);

      case Rope:
        return XII_BIT(xiiJoltBroadphaseLayer::Static) | XII_BIT(xiiJoltBroadphaseLayer::Dynamic) | XII_BIT(xiiJoltBroadphaseLayer::Ragdoll) | XII_BIT(xiiJoltBroadphaseLayer::Rope);

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return 0;
  };

} // namespace xiiJoltCollisionFiltering

xiiUInt32 xiiJoltObjectToBroadphaseLayer::GetNumBroadPhaseLayers() const
{
  return xiiJoltBroadphaseLayer::ENUM_COUNT;
}

JPH::BroadPhaseLayer xiiJoltObjectToBroadphaseLayer::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
{
  return JPH::BroadPhaseLayer(inLayer >> 8);
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char* xiiJoltObjectToBroadphaseLayer::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
{
  switch (inLayer)
  {
    case Static:
      return "Static";

    case Dynamic:
      return "Dynamic";

    case Query:
      return "QueryShapes";

    case Trigger:
      return "Trigger";

    case Character:
      return "Character";

    case Ragdoll:
      return "Ragdoll";

    case Rope:
      return "Rope";

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}
#endif

// if any of these asserts fails, xiiPhysicsShapeType and xiiJoltBroadphaseLayer are out of sync
static_assert(xiiPhysicsShapeType::Static == XII_BIT(xiiJoltBroadphaseLayer::Static));
static_assert(xiiPhysicsShapeType::Dynamic == XII_BIT(xiiJoltBroadphaseLayer::Dynamic));
static_assert(xiiPhysicsShapeType::Query == XII_BIT(xiiJoltBroadphaseLayer::Query));
static_assert(xiiPhysicsShapeType::Trigger == XII_BIT(xiiJoltBroadphaseLayer::Trigger));
static_assert(xiiPhysicsShapeType::Character == XII_BIT(xiiJoltBroadphaseLayer::Character));
static_assert(xiiPhysicsShapeType::Ragdoll == XII_BIT(xiiJoltBroadphaseLayer::Ragdoll));
static_assert(xiiPhysicsShapeType::Rope == XII_BIT(xiiJoltBroadphaseLayer::Rope));
static_assert(xiiPhysicsShapeType::Count == xiiJoltBroadphaseLayer::ENUM_COUNT);

bool xiiJoltObjectLayerFilter::ShouldCollide(JPH::ObjectLayer inLayer) const
{
  return xiiJoltCollisionFiltering::s_CollisionFilterConfig.IsCollisionEnabled(m_uiCollisionLayer, static_cast<xiiUInt32>(inLayer) & 0xFF);
}

bool xiiJoltObjectVsBroadPhaseLayerFilter::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
{
  const xiiUInt32 uiMask1 = XII_BIT(inLayer1 >> 8);
  const xiiUInt32 uiMask2 = xiiJoltCollisionFiltering::GetBroadphaseCollisionMask(static_cast<xiiJoltBroadphaseLayer>((xiiUInt8)inLayer2));

  return (uiMask1 & uiMask2) != 0;
}

bool xiiJoltObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const
{
  return xiiJoltCollisionFiltering::s_CollisionFilterConfig.IsCollisionEnabled(static_cast<xiiUInt32>(inObject1) & 0xFF, static_cast<xiiUInt32>(inObject2) & 0xFF);
}
