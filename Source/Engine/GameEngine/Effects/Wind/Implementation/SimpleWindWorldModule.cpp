#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <GameEngine/Effects/Wind/SimpleWindWorldModule.h>
#include <GameEngine/Effects/Wind/WindVolumeComponent.h>

// clang-format off
XII_IMPLEMENT_WORLD_MODULE(xiiSimpleWindWorldModule);

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimpleWindWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSimpleWindWorldModule::xiiSimpleWindWorldModule(xiiWorld* pWorld) :
  xiiWindWorldModuleInterface(pWorld)
{
  m_vFallbackWind.SetZero();
}

xiiSimpleWindWorldModule::~xiiSimpleWindWorldModule() = default;

xiiVec3 xiiSimpleWindWorldModule::GetWindAt(const xiiVec3& vPosition) const
{
  if (auto pSpatial = GetWorld()->GetSpatialSystem())
  {
    xiiHybridArray<xiiGameObject*, 16> volumes;

    xiiSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = xiiWindVolumeComponent::SpatialDataCategory.GetBitmask();

    pSpatial->FindObjectsInSphere(xiiBoundingSphere::MakeFromCenterAndRadius(vPosition, 0.5f), queryParams, volumes);

    const xiiSimdVec4f pos   = xiiSimdConversion::ToVec3(vPosition);
    xiiSimdVec4f       force = xiiSimdVec4f::MakeZero();

    for (xiiGameObject* pObj : volumes)
    {
      xiiWindVolumeComponent* pVol;
      if (pObj->TryGetComponentOfBaseType(pVol))
      {
        force += pVol->ComputeForceAtGlobalPosition(pos);
      }
    }

    return m_vFallbackWind + xiiSimdConversion::ToVec3(force);
  }

  return m_vFallbackWind;
}

void xiiSimpleWindWorldModule::SetFallbackWind(const xiiVec3& vWind)
{
  m_vFallbackWind = vWind;
}



XII_STATICLINK_FILE(GameEngine, GameEngine_Effects_Wind_Implementation_SimpleWindWorldModule);
