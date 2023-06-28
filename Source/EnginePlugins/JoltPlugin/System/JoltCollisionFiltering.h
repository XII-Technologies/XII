#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Foundation/Basics.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <Physics/Body/BodyFilter.h>

class xiiCollisionFilterConfig;

namespace JPH
{
  using ObjectLayer = xiiUInt16;
} // namespace JPH

enum class xiiJoltBroadphaseLayer : xiiUInt8
{
  Static,
  Dynamic,
  Query,
  Trigger,
  Character,
  Ragdoll,
  Rope,

  ENUM_COUNT
};

namespace xiiJoltCollisionFiltering
{
  /// \brief Constructs the JPH::ObjectLayer value from the desired collision group index and the broadphase into which the object shall be sorted
  XII_JOLTPLUGIN_DLL JPH::ObjectLayer ConstructObjectLayer(xiiUInt8 uiCollisionGroup, xiiJoltBroadphaseLayer broadphase);

  XII_JOLTPLUGIN_DLL void LoadCollisionFilters();

  XII_JOLTPLUGIN_DLL xiiCollisionFilterConfig& GetCollisionFilterConfig();

  /// \brief Returns the (hard-coded) collision mask that determines which other broad-phases to collide with.
  XII_JOLTPLUGIN_DLL xiiUInt32 GetBroadphaseCollisionMask(xiiJoltBroadphaseLayer broadphase);

}; // namespace xiiJoltCollisionFiltering


class xiiJoltObjectToBroadphaseLayer final : public JPH::BroadPhaseLayerInterface
{
public:
  virtual xiiUInt32 GetNumBroadPhaseLayers() const override;

  virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif
};

class xiiJoltBroadPhaseLayerFilter final : public JPH::BroadPhaseLayerFilter
{
public:
  xiiJoltBroadPhaseLayerFilter(xiiBitflags<xiiPhysicsShapeType> shapeTypes)
  {
    m_uiCollisionMask = shapeTypes.GetValue();
  }

  xiiUInt32 m_uiCollisionMask = 0;

  virtual bool ShouldCollide(JPH::BroadPhaseLayer inLayer) const override
  {
    return (XII_BIT(static_cast<xiiUInt8>(inLayer)) & m_uiCollisionMask) != 0;
  }
};

class xiiJoltObjectLayerFilter final : public JPH::ObjectLayerFilter
{
public:
  xiiUInt32 m_uiCollisionLayer = 0;

  xiiJoltObjectLayerFilter(xiiUInt32 uiCollisionLayer) :
    m_uiCollisionLayer(uiCollisionLayer)
  {
  }

  virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const override;
};

class xiiJoltObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
  xiiJoltObjectVsBroadPhaseLayerFilter() = default;

  virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
};

class xiiJoltObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
{
public:
  xiiJoltObjectLayerPairFilter() = default;

  virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override;
};

class xiiJoltBodyFilter final : public JPH::BodyFilter
{
public:
  xiiUInt32 m_uiObjectFilterIDToIgnore = xiiInvalidIndex - 1;

  xiiJoltBodyFilter(xiiUInt32 uiBodyFilterIdToIgnore = xiiInvalidIndex - 1) :
    m_uiObjectFilterIDToIgnore(uiBodyFilterIdToIgnore)
  {
  }

  void ClearFilter()
  {
    m_uiObjectFilterIDToIgnore = xiiInvalidIndex - 1;
  }

  virtual bool ShouldCollideLocked(const JPH::Body& body) const override
  {
    return body.GetCollisionGroup().GetGroupID() != m_uiObjectFilterIDToIgnore;
  }
};
