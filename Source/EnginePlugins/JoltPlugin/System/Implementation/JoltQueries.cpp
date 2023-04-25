#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltUserData.h>

void FillCastResult(xiiPhysicsCastResult& ref_result, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const JPH::BodyID& bodyId, const JPH::SubShapeID& subShapeId, const JPH::BodyLockInterface& lockInterface, const JPH::BodyInterface& bodyInterface, const xiiJoltWorldModule* pModule)
{
  JPH::BodyLockRead bodyLock(lockInterface, bodyId);
  const auto&       body        = bodyLock.GetBody();
  ref_result.m_vNormal          = xiiJoltConversionUtils::ToVec3(body.GetWorldSpaceSurfaceNormal(subShapeId, xiiJoltConversionUtils::ToVec3(ref_result.m_vPosition)));
  ref_result.m_uiObjectFilterID = body.GetCollisionGroup().GetGroupID();

  if (xiiComponent* pShapeComponent = xiiJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetShape()->GetSubShapeUserData(subShapeId))))
  {
    ref_result.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
  }

  if (xiiComponent* pActorComponent = xiiJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetUserData())))
  {
    ref_result.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
  }

  if (const xiiJoltMaterial* pMaterial = static_cast<const xiiJoltMaterial*>(bodyInterface.GetMaterial(bodyId, subShapeId)))
  {
    ref_result.m_hSurface = pMaterial->m_pSurface->GetResourceHandle();
  }

  const size_t uiBodyId              = bodyId.GetIndexAndSequenceNumber();
  const size_t uiShapeId             = subShapeId.GetValue();
  ref_result.m_pInternalPhysicsActor = reinterpret_cast<void*>(uiBodyId);
  ref_result.m_pInternalPhysicsShape = reinterpret_cast<void*>(uiShapeId);
}

class xiiRayCastCollector : public JPH::CastRayCollector
{
public:
  JPH::RayCastResult m_Result;
  bool               m_bAnyHit   = false;
  bool               m_bFoundAny = false;

  virtual void AddHit(const JPH::RayCastResult& result) override
  {
    if (result.mFraction < m_Result.mFraction)
    {
      m_Result    = result;
      m_bFoundAny = true;

      if (m_bAnyHit)
      {
        ForceEarlyOut();
      }
    }
  }
};

bool xiiJoltWorldModule::Raycast(xiiPhysicsCastResult& out_result, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection /*= xiiPhysicsHitCollection::Closest*/) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  JPH::RRayCast ray;
  ray.mOrigin    = xiiJoltConversionUtils::ToVec3(vStart);
  ray.mDirection = xiiJoltConversionUtils::ToVec3(vDir * fDistance);

  xiiRayCastCollector collector;
  collector.m_bAnyHit = collection == xiiPhysicsHitCollection::Any;

  xiiJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  xiiJoltBodyFilter            bodyFilter(params.m_uiIgnoreObjectFilterID);
  xiiJoltObjectLayerFilter     objectFilter(params.m_uiCollisionLayer);

  if (params.m_bIgnoreInitialOverlap)
  {
    JPH::RayCastSettings opt;
    opt.mBackFaceMode       = JPH::EBackFaceMode::IgnoreBackFaces;
    opt.mTreatConvexAsSolid = false;

    query.CastRay(ray, opt, collector, broadphaseFilter, objectFilter, bodyFilter);

    if (collector.m_bFoundAny == false)
      return false;
  }
  else
  {
    if (!query.CastRay(ray, collector.m_Result, broadphaseFilter, objectFilter, bodyFilter))
      return false;
  }

  out_result.m_fDistance = collector.m_Result.mFraction * fDistance;
  out_result.m_vPosition = vStart + fDistance * collector.m_Result.mFraction * vDir;

  FillCastResult(out_result, vStart, vDir, fDistance, collector.m_Result.mBodyID, collector.m_Result.mSubShapeID2, m_pSystem->GetBodyLockInterfaceNoLock(), m_pSystem->GetBodyInterfaceNoLock(), this);

  return true;
}

class xiiRayCastCollectorAll : public JPH::CastRayCollector
{
public:
  xiiArrayPtr<JPH::RayCastResult> m_Results;
  xiiUInt32                       m_uiFound = 0;

  virtual void AddHit(const JPH::RayCastResult& result) override
  {
    m_Results[m_uiFound] = result;
    ++m_uiFound;
  }
};

bool xiiJoltWorldModule::RaycastAll(xiiPhysicsCastResultArray& out_results, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  JPH::RRayCast ray;
  ray.mOrigin    = xiiJoltConversionUtils::ToVec3(vStart);
  ray.mDirection = xiiJoltConversionUtils::ToVec3(vDir * fDistance);

  xiiRayCastCollectorAll collector;
  collector.m_Results = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), JPH::RayCastResult, 256);

  xiiJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  xiiJoltBodyFilter            bodyFilter(params.m_uiIgnoreObjectFilterID);
  xiiJoltObjectLayerFilter     objectFilter(params.m_uiCollisionLayer);

  JPH::RayCastSettings opt;
  opt.mBackFaceMode       = params.m_bIgnoreInitialOverlap ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
  opt.mTreatConvexAsSolid = !params.m_bIgnoreInitialOverlap;

  query.CastRay(ray, opt, collector, broadphaseFilter, objectFilter, bodyFilter);

  if (collector.m_uiFound == 0)
    return false;

  out_results.m_Results.SetCount(collector.m_uiFound);

  for (xiiUInt32 i = 0; i < collector.m_uiFound; ++i)
  {
    out_results.m_Results[i].m_fDistance = collector.m_Results[i].mFraction * fDistance;
    out_results.m_Results[i].m_vPosition = vStart + fDistance * collector.m_Results[i].mFraction * vDir;

    FillCastResult(out_results.m_Results[i], vStart, vDir, fDistance, collector.m_Results[i].mBodyID, collector.m_Results[i].mSubShapeID2, m_pSystem->GetBodyLockInterfaceNoLock(), m_pSystem->GetBodyInterfaceNoLock(), this);
  }

  return true;
}

class xiiJoltShapeCastCollector : public JPH::CastShapeCollector
{
public:
  JPH::ShapeCastResult m_Result;
  bool                 m_bFoundAny = false;
  bool                 m_bAnyHit   = false;

  virtual void AddHit(const JPH::ShapeCastResult& result) override
  {
    if (result.mIsBackFaceHit)
      return;

    if (result.mFraction >= GetEarlyOutFraction())
      return;

    m_bFoundAny = true;
    m_Result    = result;

    UpdateEarlyOutFraction(result.mFraction);

    if (m_bAnyHit)
      ForceEarlyOut();
  }
};

bool xiiJoltWorldModule::SweepTestSphere(xiiPhysicsCastResult& out_result, float fSphereRadius, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection) const
{
  if (fSphereRadius <= 0.0f)
    return false;

  const JPH::SphereShape shape(fSphereRadius);

  return SweepTest(out_result, shape, JPH::Mat44::sTranslation(xiiJoltConversionUtils::ToVec3(vStart)), vDir, fDistance, params, collection);
}

bool xiiJoltWorldModule::SweepTestBox(xiiPhysicsCastResult& out_result, xiiVec3 vBoxExtends, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection) const
{
  const JPH::BoxShape shape(xiiJoltConversionUtils::ToVec3(vBoxExtends * 0.5f));

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(xiiJoltConversionUtils::ToQuat(transform.m_qRotation), xiiJoltConversionUtils::ToVec3(transform.m_vPosition));

  return SweepTest(out_result, shape, trans, vDir, fDistance, params, collection);
}

bool xiiJoltWorldModule::SweepTestCapsule(xiiPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection) const
{
  if (fCapsuleRadius <= 0.0f)
    return false;

  const JPH::CapsuleShape shape(fCapsuleHeight * 0.5f, fCapsuleRadius);

  xiiQuat qFixRot;
  qFixRot.SetFromAxisAndAngle(xiiVec3(1, 0, 0), xiiAngle::Degree(90.0f));

  xiiQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(xiiJoltConversionUtils::ToQuat(qRot), xiiJoltConversionUtils::ToVec3(transform.m_vPosition));

  return SweepTest(out_result, shape, trans, vDir, fDistance, params, collection);
}

bool xiiJoltWorldModule::SweepTest(xiiPhysicsCastResult& out_Result, const JPH::Shape& shape, const JPH::Mat44& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection) const
{
  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  xiiJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  xiiJoltBodyFilter            bodyFilter(params.m_uiIgnoreObjectFilterID);
  xiiJoltObjectLayerFilter     objectFilter(params.m_uiCollisionLayer);

  JPH::RShapeCast cast(&shape, JPH::Vec3(1, 1, 1), transform, xiiJoltConversionUtils::ToVec3(vDir * fDistance));

  xiiJoltShapeCastCollector collector;
  collector.m_bAnyHit = collection == xiiPhysicsHitCollection::Any;

  query.CastShape(cast, {}, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, bodyFilter);

  if (!collector.m_bFoundAny)
    return false;

  const auto& res = collector.m_Result;

  out_Result.m_fDistance = res.mFraction * fDistance;
  out_Result.m_vPosition = xiiJoltConversionUtils::ToVec3(res.mContactPointOn2);

  FillCastResult(out_Result, xiiJoltConversionUtils::ToVec3(transform.GetTranslation()), vDir, fDistance, res.mBodyID2, res.mSubShapeID2, m_pSystem->GetBodyLockInterfaceNoLock(), m_pSystem->GetBodyInterfaceNoLock(), this);

  return true;
}

class xiiJoltShapeCollectorAny : public JPH::CollideShapeCollector
{
public:
  bool m_bFoundAny = false;

  virtual void AddHit(const JPH::CollideShapeResult& result) override
  {
    m_bFoundAny = true;
    ForceEarlyOut();
  }
};

class xiiJoltShapeCollectorAll : public JPH::CollideShapeCollector
{
public:
  xiiHybridArray<JPH::CollideShapeResult, 32, xiiAlignedAllocatorWrapper> m_Results;

  virtual void AddHit(const JPH::CollideShapeResult& result) override
  {
    m_Results.PushBack(result);

    if (m_Results.GetCount() >= 256)
    {
      ForceEarlyOut();
    }
  }
};

bool xiiJoltWorldModule::OverlapTestSphere(float fSphereRadius, const xiiVec3& vPosition, const xiiPhysicsQueryParameters& params) const
{
  if (fSphereRadius <= 0.0f)
    return false;

  const JPH::SphereShape shape(fSphereRadius);

  return OverlapTest(shape, JPH::Mat44::sTranslation(xiiJoltConversionUtils::ToVec3(vPosition)), params);
}

bool xiiJoltWorldModule::OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const
{
  if (fCapsuleRadius <= 0.0f)
    return false;

  const JPH::CapsuleShape shape(fCapsuleHeight * 0.5f, fCapsuleRadius);

  xiiQuat qFixRot;
  qFixRot.SetFromAxisAndAngle(xiiVec3(1, 0, 0), xiiAngle::Degree(90.0f));

  xiiQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(xiiJoltConversionUtils::ToQuat(qRot), xiiJoltConversionUtils::ToVec3(transform.m_vPosition));

  return OverlapTest(shape, trans, params);
}

bool xiiJoltWorldModule::OverlapTest(const JPH::Shape& shape, const JPH::Mat44& transform, const xiiPhysicsQueryParameters& params) const
{
  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  xiiJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  xiiJoltBodyFilter            bodyFilter(params.m_uiIgnoreObjectFilterID);
  xiiJoltObjectLayerFilter     objectFilter(params.m_uiCollisionLayer);

  xiiJoltShapeCollectorAny collector;
  query.CollideShape(&shape, JPH::Vec3(1, 1, 1), transform, {}, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, bodyFilter);

  return collector.m_bFoundAny;
}

void xiiJoltWorldModule::QueryShapesInSphere(xiiPhysicsOverlapResultArray& out_results, float fSphereRadius, const xiiVec3& vPosition, const xiiPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();

  if (fSphereRadius <= 0.0f)
    return;

  const JPH::SphereShape       shape(fSphereRadius);
  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  xiiJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  xiiJoltObjectLayerFilter     objectFilter(params.m_uiCollisionLayer);
  xiiJoltBodyFilter            bodyFilter(params.m_uiIgnoreObjectFilterID);

  xiiJoltShapeCollectorAll collector;
  query.CollideShape(&shape, JPH::RVec3(1, 1, 1), JPH::Mat44::sTranslation(xiiJoltConversionUtils::ToVec3(vPosition)), {}, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, bodyFilter);

  out_results.m_Results.SetCount(collector.m_Results.GetCount());

  auto& lockInterface = m_pSystem->GetBodyLockInterfaceNoLock();

  for (xiiUInt32 i = 0; i < collector.m_Results.GetCount(); ++i)
  {
    auto& overlapResult = out_results.m_Results[i];
    auto& overlapHit    = collector.m_Results[i];

    JPH::BodyLockRead bodyLock(lockInterface, overlapHit.mBodyID2);
    const auto&       body = bodyLock.GetBody();

    overlapResult.m_uiObjectFilterID = body.GetCollisionGroup().GetGroupID();

    if (xiiComponent* pShapeComponent = xiiJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetShape()->GetSubShapeUserData(overlapHit.mSubShapeID2))))
    {
      overlapResult.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
    }

    if (xiiComponent* pActorComponent = xiiJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetUserData())))
    {
      overlapResult.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
    }
  }
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltQueries);
