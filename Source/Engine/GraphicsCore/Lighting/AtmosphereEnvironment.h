/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Lighting/AtmosphereLookup.h>
#include <GraphicsCore/Lighting/EnvironmentMap.h>

/// Immutable physical sky capture and convolved lighting at one altitude and solar state.
/// The direct sun disk is excluded: analytic directional illumination already accounts for it.
class XII_GRAPHICSCORE_DLL xiiAtmosphereEnvironment final : public xiiRefCounted
{
public:
  static xiiStatus Create(const xiiSharedPtr<const xiiAtmosphereLookup>& lookup, xiiVec4 directionAndIlluminance, xiiVec3 color, float altitudeMeters, xiiSharedPtr<const xiiAtmosphereEnvironment>& out_environment);
  bool             Matches(const xiiSharedPtr<const xiiAtmosphereLookup>& lookup, xiiVec4 directionAndIlluminance, xiiVec3 color, float altitudeMeters) const;

  xiiAtmosphereEnvironment(xiiSharedPtr<const xiiAtmosphereLookup> lookup, xiiSharedPtr<const xiiEnvironmentMap> environment, xiiVec4 directionAndIlluminance, xiiVec3 color, float altitudeMeters);

  const xiiSharedPtr<const xiiAtmosphereLookup> m_Lookup;
  const xiiSharedPtr<const xiiEnvironmentMap>   m_Environment;
  const xiiVec4                                 m_DirectionAndIlluminance;
  const xiiVec3                                 m_Color;
  const float                                   m_fAltitudeMeters;
};
