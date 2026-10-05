/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// Evaluates the finite solar emitter in radiance units.
///
/// The directional-light source radius is sin(apparent half-angle). For a
/// constant-radiance disc, integrating radiance over the projected emitter is
/// exactly PI * sin(half-angle)^2, so dividing authored illuminance by that
/// projected solid angle preserves physical lux at normal incidence.
///
/// A linear limb-darkening law models the reduced radiance near the apparent
/// solar edge. Its mean over the projected disc is (1 - u / 3); normalizing by
/// that value keeps total illuminance invariant as the authored coefficient
/// changes. A coefficient of zero recovers a uniform emitter.
float3 xiiEvaluateSunDiskRadiance(float3 viewDirection, float3 sunDirection, float sourceSinHalfAngle,
                                  float limbDarkening, float3 solarIlluminance, float3 transmittance)
{
  const float sourceRadius = saturate(sourceSinHalfAngle);
  if (sourceRadius <= 1e-5f)
    return 0.0f;

  const float cosTheta     = dot(normalize(viewDirection), normalize(sunDirection));
  const float cosHalfAngle = sqrt(max(1.0f - sourceRadius * sourceRadius, 0.0f));
  if (cosTheta <= cosHalfAngle)
    return 0.0f;

  const float sinThetaSquared        = max(1.0f - cosTheta * cosTheta, 0.0f);
  const float projectedRadiusSquared = saturate(sinThetaSquared / max(sourceRadius * sourceRadius, 1e-8f));
  const float surfaceCosine          = sqrt(max(1.0f - projectedRadiusSquared, 0.0f));

  const float u                   = saturate(limbDarkening);
  const float limbProfile         = (1.0f - u * (1.0f - surfaceCosine)) / max(1.0f - u / 3.0f, 1e-4f);
  const float projectedSolidAngle = 3.14159265359f * sourceRadius * sourceRadius;

  return max(solarIlluminance, 0.0f) * max(transmittance, 0.0f) * limbProfile /
    max(projectedSolidAngle, 1e-6f);
}
