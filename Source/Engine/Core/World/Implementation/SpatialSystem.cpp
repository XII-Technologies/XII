/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/GameObject.h>
#include <Core/World/SpatialSystem.h>
#include <Core/World/World.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSpatialSystem, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSpatialSystem::xiiSpatialSystem() :
  m_Allocator("Spatial System", xiiFoundation::GetDefaultAllocator())
{
}

xiiSpatialSystem::~xiiSpatialSystem() = default;

void xiiSpatialSystem::StartNewFrame()
{
  ++m_uiFrameCounter;
}

void xiiSpatialSystem::FindObjectsInSphere(const xiiBoundingSphere& sphere, const QueryParams& queryParams, xiiDynamicArray<xiiGameObject*>& out_objects) const
{
  out_objects.Clear();

  FindObjectsInSphere(
    sphere, queryParams,
    [&](xiiGameObject* pObject) {
      out_objects.PushBack(pObject);

      return xiiVisitorExecution::Continue;
    });
}

void xiiSpatialSystem::FindObjectsInBox(const xiiBoundingBox& box, const QueryParams& queryParams, xiiDynamicArray<xiiGameObject*>& out_objects) const
{
  out_objects.Clear();

  FindObjectsInBox(
    box, queryParams,
    [&](xiiGameObject* pObject) {
      out_objects.PushBack(pObject);

      return xiiVisitorExecution::Continue;
    });
}

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
void xiiSpatialSystem::GetInternalStats(xiiStringBuilder& ref_sSb) const
{
  ref_sSb.Clear();
}
#endif

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiScriptExtensionClass_Spatial, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(FindClosestObjectInSphere, In, "World", In, "Category", In, "Center", In, "Radius"),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiScriptExtensionAttribute("Spatial"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on


xiiGameObject* xiiScriptExtensionClass_Spatial::FindClosestObjectInSphere(xiiWorld* pWorld, xiiStringView sCategory, const xiiVec3& vCenter, float fRadius)
{
  xiiGameObject* pClosest = nullptr;

  auto category = xiiSpatialData::FindCategory(sCategory);
  if (category != xiiInvalidSpatialDataCategory)
  {
    xiiSpatialSystem::QueryParams params;
    params.m_uiCategoryBitmask = category.GetBitmask();

    float fDistanceSqr = xiiMath::HighValue<float>();

    pWorld->GetSpatialSystem()->FindObjectsInSphere(xiiBoundingSphere::MakeFromCenterAndRadius(vCenter, fRadius), params, [&](xiiGameObject* go) -> xiiVisitorExecution::Enum {
      const float fSqr = go->GetGlobalPosition().GetSquaredDistanceTo(vCenter);

      if (fSqr < fDistanceSqr)
      {
        fDistanceSqr = fSqr;
        pClosest     = go;
      }

      return xiiVisitorExecution::Continue;
      //
    });
  }

  return pClosest;
}

XII_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem);
