#include <JoltPlugin/JoltPluginPCH.h>

#include <Jolt/AABBTree/TriangleCodec/TriangleCodecIndexed8BitPackSOA4Flags.h>
#include <Jolt/Physics/Collision/CollisionDispatch.h>
#include <JoltPlugin/Shapes/Implementation/JoltCustomShapeInfo.h>

using namespace JPH;

const JPH::PhysicsMaterial* xiiJoltCustomShapeInfo::GetMaterial(const SubShapeID& inSubShapeID) const
{
  if (!m_CustomMaterials.IsEmpty())
  {
    if (mInnerShape->GetSubType() == EShapeSubType::Mesh)
    {
      const JPH::MeshShape* pMeshShape = static_cast<const JPH::MeshShape*>(mInnerShape.GetPtr());

      return m_CustomMaterials[pMeshShape->GetMaterialIndex(inSubShapeID)];
    }

    return m_CustomMaterials[0];
  }

  return mInnerShape->GetMaterial(inSubShapeID);
}

JPH::uint64 xiiJoltCustomShapeInfo::GetSubShapeUserData(const SubShapeID& inSubShapeID) const
{
  return GetUserData();
}

JPH::MassProperties xiiJoltCustomShapeInfo::GetMassProperties() const
{
  MassProperties p = mInnerShape->GetMassProperties();

  p.mMass *= m_fDensity;
  p.mInertia *= m_fDensity;
  p.mInertia(3, 3) = 1.0f;

  return p;
}

JPH::Vec3 xiiJoltCustomShapeInfo::GetCenterOfMass() const
{
  return mInnerShape->GetCenterOfMass();
}

//////////////////////////////////////////////////////////////////////////

JPH::AABox xiiJoltCustomShapeInfo::GetLocalBounds() const
{
  return mInnerShape->GetLocalBounds();
}

float xiiJoltCustomShapeInfo::GetInnerRadius() const
{
  return mInnerShape->GetInnerRadius();
}

JPH::Vec3 xiiJoltCustomShapeInfo::GetSurfaceNormal(const SubShapeID& inSubShapeID, Vec3Arg inLocalSurfacePosition) const
{
  return mInnerShape->GetSurfaceNormal(inSubShapeID, inLocalSurfacePosition);
}

void xiiJoltCustomShapeInfo::GetSubmergedVolume(Mat44Arg inCenterOfMassTransform, Vec3Arg inScale, const Plane& inSurface, float& outTotalVolume, float& outSubmergedVolume, Vec3& outCenterOfBuoyancy
#ifdef JPH_DEBUG_RENDERER // Not using JPH_IF_DEBUG_RENDERER for Doxygen
                                                ,
                                                JPH::RVec3Arg inBaseOffset
#endif
) const
{
  mInnerShape->GetSubmergedVolume(inCenterOfMassTransform, inScale, inSurface, outTotalVolume, outSubmergedVolume, outCenterOfBuoyancy
#ifdef JPH_DEBUG_RENDERER // Not using JPH_IF_DEBUG_RENDERER for Doxygen
                                  ,
                                  inBaseOffset
#endif
  );
}

void xiiJoltCustomShapeInfo::Draw(DebugRenderer* inRenderer, Mat44Arg inCenterOfMassTransform, Vec3Arg inScale, ColorArg inColor, bool inUseMaterialColors, bool inDrawWireframe) const
{
  mInnerShape->Draw(inRenderer, inCenterOfMassTransform, inScale, inColor, inUseMaterialColors, inDrawWireframe);
}

bool xiiJoltCustomShapeInfo::CastRay(const RayCast& inRay, const SubShapeIDCreator& inSubShapeIDCreator, RayCastResult& ioHit) const
{
  return mInnerShape->CastRay(inRay, inSubShapeIDCreator, ioHit);
}

void xiiJoltCustomShapeInfo::CastRay(const JPH::RayCast& inRay, const JPH::RayCastSettings& inRayCastSettings, const JPH::SubShapeIDCreator& inSubShapeIDCreator, JPH::CastRayCollector& ioCollector, const JPH::ShapeFilter& inShapeFilter) const
{
  return mInnerShape->CastRay(inRay, inRayCastSettings, inSubShapeIDCreator, ioCollector, inShapeFilter);
}

void xiiJoltCustomShapeInfo::CollidePoint(JPH::Vec3Arg inPoint, const JPH::SubShapeIDCreator& inSubShapeIDCreator, JPH::CollidePointCollector& ioCollector, const JPH::ShapeFilter& inShapeFilter) const
{
  mInnerShape->CollidePoint(inPoint, inSubShapeIDCreator, ioCollector, inShapeFilter);
}

void xiiJoltCustomShapeInfo::GetTrianglesStart(GetTrianglesContext& ioContext, const AABox& inBox, Vec3Arg inPositionCOM, QuatArg inRotation, Vec3Arg inScale) const
{
  mInnerShape->GetTrianglesStart(ioContext, inBox, inPositionCOM, inRotation, inScale);
}

int xiiJoltCustomShapeInfo::GetTrianglesNext(GetTrianglesContext& ioContext, int inMaxTrianglesRequested, Float3* outTriangleVertices, const PhysicsMaterial** outMaterials /*= nullptr*/) const
{
  return mInnerShape->GetTrianglesNext(ioContext, inMaxTrianglesRequested, outTriangleVertices, outMaterials);
}

float xiiJoltCustomShapeInfo::GetVolume() const
{
  return mInnerShape->GetVolume();
}

void xiiJoltCustomShapeInfo::sRegister()
{
  ShapeFunctions& f = ShapeFunctions::sGet(EShapeSubType::User1);
  f.mConstruct      = []() -> Shape* {
    return new xiiJoltCustomShapeInfo;
  };
  f.mColor = Color::sCyan;

  for (EShapeSubType s : sAllSubShapeTypes)
  {
    CollisionDispatch::sRegisterCollideShape(EShapeSubType::User1, s, sCollideUser1VsShape);
    CollisionDispatch::sRegisterCollideShape(s, EShapeSubType::User1, sCollideShapeVsUser1);
    CollisionDispatch::sRegisterCastShape(EShapeSubType::User1, s, sCastUser1VsShape);
    CollisionDispatch::sRegisterCastShape(s, EShapeSubType::User1, sCastShapeVsUser1);
  }
}

void xiiJoltCustomShapeInfo::sCollideUser1VsShape(const JPH::Shape* inShape1, const JPH::Shape* inShape2, JPH::Vec3Arg inScale1, JPH::Vec3Arg inScale2, JPH::Mat44Arg inCenterOfMassTransform1, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, const JPH::CollideShapeSettings& inCollideShapeSettings, JPH::CollideShapeCollector& ioCollector, const JPH::ShapeFilter& inShapeFilter)
{
  JPH_ASSERT(inShape1->GetSubType() == EShapeSubType::User1);
  const xiiJoltCustomShapeInfo* shape1 = static_cast<const xiiJoltCustomShapeInfo*>(inShape1);

  CollisionDispatch::sCollideShapeVsShape(shape1->mInnerShape, inShape2, inScale1, inScale2, inCenterOfMassTransform1, inCenterOfMassTransform2, inSubShapeIDCreator1, inSubShapeIDCreator2, inCollideShapeSettings, ioCollector, inShapeFilter);
}

void xiiJoltCustomShapeInfo::sCollideShapeVsUser1(const JPH::Shape* inShape1, const JPH::Shape* inShape2, JPH::Vec3Arg inScale1, JPH::Vec3Arg inScale2, JPH::Mat44Arg inCenterOfMassTransform1, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, const JPH::CollideShapeSettings& inCollideShapeSettings, JPH::CollideShapeCollector& ioCollector, const JPH::ShapeFilter& inShapeFilter)
{
  JPH_ASSERT(inShape2->GetSubType() == EShapeSubType::User1);
  const xiiJoltCustomShapeInfo* shape2 = static_cast<const xiiJoltCustomShapeInfo*>(inShape2);

  CollisionDispatch::sCollideShapeVsShape(inShape1, shape2->mInnerShape, inScale1, inScale2, inCenterOfMassTransform1, inCenterOfMassTransform2, inSubShapeIDCreator1, inSubShapeIDCreator2, inCollideShapeSettings, ioCollector, inShapeFilter);
}

void xiiJoltCustomShapeInfo::sCastUser1VsShape(const JPH::ShapeCast& inShapeCast, const JPH::ShapeCastSettings& inShapeCastSettings, const Shape* inShape, JPH::Vec3Arg inScale, const JPH::ShapeFilter& inShapeFilter, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, JPH::CastShapeCollector& ioCollector)
{
  // Fetch offset center of mass shape from cast shape
  JPH_ASSERT(inShapeCast.mShape->GetSubType() == EShapeSubType::User1);
  const xiiJoltCustomShapeInfo* shape1 = static_cast<const xiiJoltCustomShapeInfo*>(inShapeCast.mShape);

  CollisionDispatch::sCastShapeVsShapeLocalSpace(inShapeCast, inShapeCastSettings, inShape, inScale, inShapeFilter, inCenterOfMassTransform2, inSubShapeIDCreator1, inSubShapeIDCreator2, ioCollector);
}

void xiiJoltCustomShapeInfo::sCastShapeVsUser1(const JPH::ShapeCast& inShapeCast, const JPH::ShapeCastSettings& inShapeCastSettings, const Shape* inShape, JPH::Vec3Arg inScale, const JPH::ShapeFilter& inShapeFilter, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, JPH::CastShapeCollector& ioCollector)
{
  JPH_ASSERT(inShape->GetSubType() == EShapeSubType::User1);
  const xiiJoltCustomShapeInfo* shape = static_cast<const xiiJoltCustomShapeInfo*>(inShape);

  CollisionDispatch::sCastShapeVsShapeLocalSpace(inShapeCast, inShapeCastSettings, shape->mInnerShape, inScale, inShapeFilter, inCenterOfMassTransform2, inSubShapeIDCreator1, inSubShapeIDCreator2, ioCollector);
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltCustomShapeInfo);

