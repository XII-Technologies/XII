/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Core/Interfaces/PhysicsQuery.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/Math/Vec3.h>
#include <MCPPlugin/Json.h>
#include <MCPPlugin/Providers/RuntimeProviders.h>
#include <MCPPlugin/Providers/World/World.h>
#include <MCPPlugin/Reflection.h>

namespace
{
  constexpr xiiStringView PhysicsTool = "runtime.physics";
  constexpr xiiStringView QueryTool   = "runtime.physics_query";

  bool HasLivePhysics()
  {
    for (xiiUInt32 uiIndex = 0; uiIndex < xiiWorld::GetWorldCount(); ++uiIndex)
    {
      if (const auto* pWorld = xiiWorld::GetWorld(uiIndex))
      {
        XII_LOCK(pWorld->GetReadMarker());
        if (pWorld->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>() != nullptr)
        {
          return true;
        }
      }
    }
    return false;
  }

  xiiStatus GetPhysics(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto* pWorld = xiiMCPRuntimeProviders::FindWorld(arguments);
    if (pWorld == nullptr)
    {
      return xiiStatus("Select a current world from runtime.worlds.");
    }

    XII_LOCK(pWorld->GetReadMarker());
    const auto* pPhysics = pWorld->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>();
    if (pPhysics == nullptr)
    {
      return xiiStatus("The selected world has no active rigid-body physics module.");
    }

    out_result["module"] = xiiString(pPhysics->GetDynamicRTTI()->GetTypeName());
    XII_VERIFY(xiiMCPReflection::ConvertValue(pPhysics->GetGravity(), out_result["gravity"]), "Gravity must be JSON-compatible.");
    return XII_SUCCESS;
  }

  bool ReadVector(const xiiVariantDictionary& arguments, xiiStringView sName, xiiVec3& out_vValue)
  {
    const auto* pValue = arguments.GetValue(sName);
    float       fValues[3];
    if (pValue == nullptr || !xiiMCPReflection::ReadFloatArray(*pValue, fValues))
    {
      return false;
    }

    out_vValue = xiiVec3(fValues[0], fValues[1], fValues[2]);
    return true;
  }

  bool ReadQueryLength(const xiiVariantDictionary& arguments, xiiStringView sName, float& out_fValue)
  {
    const auto* pValue = arguments.GetValue(sName);
    if (pValue == nullptr || !pValue->IsNumber() || pValue->IsA<bool>() || !xiiMath::IsInRange(pValue->ConvertTo<double>(), 0.0, 1.0e8))
    {
      return false;
    }

    out_fValue = pValue->ConvertTo<float>();
    return out_fValue > 0.0f;
  }

  void WriteHit(const xiiWorld& world, const xiiPhysicsCastResult& hit, xiiVariantDictionary& out_result)
  {
    out_result["distance"] = hit.m_fDistance;
    XII_VERIFY(xiiMCPReflection::ConvertValue(hit.m_vPosition, out_result["position"]), "Impact position must be JSON-compatible.");
    XII_VERIFY(xiiMCPReflection::ConvertValue(hit.m_vNormal, out_result["normal"]), "Impact normal must be JSON-compatible.");
    if (hit.m_hActorObject.GetInternalID().m_WorldIndex == world.GetIndex() && world.IsValidObject(hit.m_hActorObject))
    {
      out_result["actor"] = xiiMCPRuntimeProviders::FormatHandle(hit.m_hActorObject);
    }
    if (hit.m_hShapeObject.GetInternalID().m_WorldIndex == world.GetIndex() && world.IsValidObject(hit.m_hShapeObject))
    {
      out_result["shape"] = xiiMCPRuntimeProviders::FormatHandle(hit.m_hShapeObject);
    }
  }

  xiiStatus QueryPhysics(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto* pWorld = xiiMCPRuntimeProviders::FindWorld(arguments);
    if (pWorld == nullptr)
    {
      return xiiStatus("Select a current world from runtime.worlds.");
    }

    const auto sQuery   = xiiMCPJson::GetString(arguments, "query");
    const bool bOverlap = sQuery == "overlap_sphere";
    const bool bSweep   = sQuery == "sweep_sphere";
    if (!bOverlap && !bSweep && sQuery != "raycast")
    {
      return xiiStatus("Use a raycast, sweep_sphere or overlap_sphere query.");
    }

    xiiVec3 vPosition;
    xiiVec3 vDirection;
    float   fRadius   = 0.0f;
    float   fDistance = 0.0f;
    if (!ReadVector(arguments, "position", vPosition) ||
        ((bOverlap || bSweep) && !ReadQueryLength(arguments, "radius", fRadius)) ||
        (!bOverlap && (!ReadVector(arguments, "direction", vDirection) || vDirection.NormalizeIfNotZero().Failed() || !ReadQueryLength(arguments, "distance", fDistance))))
    {
      return xiiStatus("Provide a valid position and the selected query's radius or direction and distance.");
    }

    const auto* pLayer         = arguments.GetValue("collisionLayer");
    const auto* pIgnoreInitial = arguments.GetValue("ignoreInitialOverlap");
    if ((pLayer != nullptr && (!pLayer->IsA<xiiString>() || pLayer->Get<xiiString>().IsEmpty() || pLayer->Get<xiiString>().GetCharacterCount() > 64)) ||
        (pIgnoreInitial != nullptr && (bOverlap || !pIgnoreInitial->IsA<bool>())))
    {
      return xiiStatus("Provide a collision layer name and use ignoreInitialOverlap only for casts.");
    }

    XII_LOCK(pWorld->GetReadMarker());
    const auto* pPhysics = pWorld->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>();
    if (pPhysics == nullptr || context.HasExpired())
    {
      return xiiStatus("The selected world has no active rigid-body physics module or the query expired.");
    }

    xiiPhysicsQueryParameters parameters;
    if (pLayer != nullptr)
    {
      parameters.m_uiCollisionLayer = pPhysics->GetCollisionLayerByName(pLayer->Get<xiiString>());
      if (parameters.m_uiCollisionLayer == xiiInvalidIndex)
      {
        return xiiStatus("The selected physics module does not recognize the collision layer.");
      }
    }
    if (pIgnoreInitial != nullptr)
    {
      parameters.m_bIgnoreInitialOverlap = pIgnoreInitial->Get<bool>();
    }

    if (bOverlap)
    {
      out_result["hit"] = pPhysics->OverlapTestSphere(fRadius, vPosition, parameters);
      return XII_SUCCESS;
    }

    xiiPhysicsCastResult hit;
    const bool           bHit = bSweep ? pPhysics->SweepTestSphere(hit, fRadius, vPosition, vDirection, fDistance, parameters) :
                                         pPhysics->Raycast(hit, vPosition, vDirection, fDistance, parameters);
    out_result["hit"]         = bHit;
    if (bHit)
    {
      WriteHit(*pWorld, hit, out_result);
    }
    return XII_SUCCESS;
  }
} // namespace

void xiiMCPRuntimeProviders::UpdatePhysics(xiiMCPRegistry& ref_registry, bool& ref_bRegistered)
{
  const bool bAvailable = HasLivePhysics();
  if (bAvailable == ref_bRegistered)
  {
    return;
  }

  if (bAvailable)
  {
    ref_registry.RegisterTool(PhysicsTool, "Inspect the active rigid-body physics module and gravity for a live world.",
                              R"({"type":"object","required":["world"],"properties":{"world":{"type":"string"}}})",
                              xiiMCPPermission::Read, &GetPhysics)
      .IgnoreResult();
    ref_registry.RegisterTool(QueryTool, "Run a nearest-hit raycast or sphere sweep, or test for sphere overlap in a live physics world. Coordinates, distance and radius use world units. Casts normalize a nonzero directional vector.",
                              R"({"type":"object","required":["world","query","position"],"properties":{)"
                              R"("world":{"type":"string"},"query":{"enum":["raycast","sweep_sphere","overlap_sphere"]},)"
                              R"("position":{"$ref":"#/$defs/vector"},"direction":{"$ref":"#/$defs/vector"},)"
                              R"("distance":{"$ref":"#/$defs/length"},"radius":{"$ref":"#/$defs/length"},)"
                              R"("collisionLayer":{"type":"string","minLength":1,"maxLength":64,"description":"Backend layer name. Defaults to native layer 0."},"ignoreInitialOverlap":{"type":"boolean"}},)"
                              R"("allOf":[{"if":{"properties":{"query":{"enum":["raycast","sweep_sphere"]}}},"then":{"required":["direction","distance"]}},)"
                              R"({"if":{"properties":{"query":{"enum":["sweep_sphere","overlap_sphere"]}}},"then":{"required":["radius"]}}],)"
                              R"("$defs":{"vector":{"type":"array","minItems":3,"maxItems":3,"items":{"type":"number","minimum":-100000000,"maximum":100000000}},)"
                              R"("length":{"type":"number","exclusiveMinimum":0,"maximum":100000000}}})",
                              xiiMCPPermission::Read, &QueryPhysics)
      .IgnoreResult();
  }
  else
  {
    ref_registry.Remove(xiiMCPCapability::Tool, PhysicsTool);
    ref_registry.Remove(xiiMCPCapability::Tool, QueryTool);
  }
  ref_bRegistered = bAvailable;
}
