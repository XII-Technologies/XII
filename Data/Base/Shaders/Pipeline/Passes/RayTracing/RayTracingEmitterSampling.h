#pragma once

// Samples a physical point on every finite analytical emitter. The including
// shader supplies two independent uniform variates so this helper stays
// independent of effect-specific payload and RNG layouts.
void xiiSampleRayTracingLocalEmitter(xiiGpuLightData lightData, float3 worldPosition, float2 randomSample,
                                     out float3 samplePosition, out float projectedArea)
{
  const uint lightType = GetLightType(lightData);
  samplePosition = GetRepresentativeLightPosition(lightData, worldPosition);
  projectedArea = 0.0f;

  if (lightType == XII_LIGHT_TYPE_POINT)
  {
    const float3 center = lightData.PositionAndInvRange.xyz;
    const float radius = max(lightData.AttenuationAndSize.y, 0.0f);
    if (radius > 1e-4f)
    {
      const float3 centerToPoint = NormalizeAreaLightDirection(worldPosition - center, float3(0.0f, 0.0f, 1.0f));
      float3 tangent;
      float3 bitangent;
      BuildAreaLightBasis(centerToPoint, tangent, bitangent);
      const float diskRadius = sqrt(randomSample.x) * radius;
      const float diskAngle = randomSample.y * 6.28318530718f;
      const float2 diskSample = float2(cos(diskAngle), sin(diskAngle)) * diskRadius;
      const float surfaceHeight = sqrt(max(radius * radius - dot(diskSample, diskSample), 0.0f));
      samplePosition = center + tangent * diskSample.x + bitangent * diskSample.y + centerToPoint * surfaceHeight;
    }
  }
  else if (lightType == XII_LIGHT_TYPE_SPOT)
  {
    const float radius = max(lightData.AttenuationAndSize.y, 0.0f);
    if (radius > 1e-4f)
    {
      const float3 emitterNormal = normalize(lightData.DirectionAndType.xyz);
      const float3 right = normalize(lightData.OrientationRightAndIES.xyz);
      const float3 up = normalize(cross(right, emitterNormal));
      const float diskRadius = sqrt(randomSample.x) * radius;
      const float diskAngle = randomSample.y * 6.28318530718f;
      samplePosition = lightData.PositionAndInvRange.xyz + right * (diskRadius * cos(diskAngle)) + up * (diskRadius * sin(diskAngle));
    }
  }
  else if (lightType == XII_LIGHT_TYPE_RECTANGLE)
  {
    const float3 emitterNormal = normalize(lightData.DirectionAndType.xyz);
    const float3 right = normalize(lightData.OrientationRightAndIES.xyz);
    const float3 up = normalize(cross(right, emitterNormal));
    const float width = max(lightData.SpotAnglesAndRectSize.z, 1e-4f);
    const float height = max(lightData.SpotAnglesAndRectSize.w, 1e-4f);
    const float2 sampleUV = randomSample - 0.5f;
    samplePosition = lightData.PositionAndInvRange.xyz + right * (sampleUV.x * width) + up * (sampleUV.y * height);
    projectedArea = width * height * saturate(dot(emitterNormal, normalize(worldPosition - samplePosition)));
  }
  else if (lightType == XII_LIGHT_TYPE_DISC)
  {
    const float3 emitterNormal = normalize(lightData.DirectionAndType.xyz);
    const float3 right = normalize(lightData.OrientationRightAndIES.xyz);
    const float3 up = normalize(cross(right, emitterNormal));
    const float radius = max(lightData.AttenuationAndSize.y, 1e-4f);
    const float diskRadius = sqrt(randomSample.x) * radius;
    const float diskAngle = randomSample.y * 6.28318530718f;
    samplePosition = lightData.PositionAndInvRange.xyz + right * (diskRadius * cos(diskAngle)) + up * (diskRadius * sin(diskAngle));
    projectedArea = 3.14159265f * radius * radius * saturate(dot(emitterNormal, normalize(worldPosition - samplePosition)));
  }
  else if (lightType == XII_LIGHT_TYPE_SPHERE)
  {
    const float3 center = lightData.PositionAndInvRange.xyz;
    const float radius = max(lightData.AttenuationAndSize.y, 1e-4f);
    const float3 centerToPoint = NormalizeAreaLightDirection(worldPosition - center, float3(0.0f, 0.0f, 1.0f));
    float3 tangent;
    float3 bitangent;
    BuildAreaLightBasis(centerToPoint, tangent, bitangent);
    const float diskRadius = sqrt(randomSample.x) * radius;
    const float diskAngle = randomSample.y * 6.28318530718f;
    const float2 diskSample = float2(cos(diskAngle), sin(diskAngle)) * diskRadius;
    const float surfaceHeight = sqrt(max(radius * radius - dot(diskSample, diskSample), 0.0f));
    samplePosition = center + tangent * diskSample.x + bitangent * diskSample.y + centerToPoint * surfaceHeight;
    projectedArea = 3.14159265f * radius * radius;
  }
  else if (lightType == XII_LIGHT_TYPE_TUBE)
  {
    const float3 center = lightData.PositionAndInvRange.xyz;
    const float3 axis = normalize(lightData.DirectionAndType.xyz);
    const float radius = max(lightData.AttenuationAndSize.y, 1e-4f);
    const float length = max(lightData.AttenuationAndSize.z, 1e-4f);
    const float3 centerToPoint = NormalizeAreaLightDirection(worldPosition - center, float3(0.0f, 0.0f, 1.0f));
    const float axisCosine = dot(centerToPoint, axis);
    const float sideOn = sqrt(saturate(1.0f - axisCosine * axisCosine));
    float3 radialToPoint = centerToPoint - axis * axisCosine;
    if (dot(radialToPoint, radialToPoint) < 1e-6f)
    {
      float3 unused;
      BuildAreaLightBasis(axis, radialToPoint, unused);
    }
    else
    {
      radialToPoint = normalize(radialToPoint);
    }

    const float sideArea = 2.0f * radius * length * sideOn;
    const float capArea = 3.14159265f * radius * radius;
    projectedArea = sideArea + capArea;
    if (randomSample.x * projectedArea < sideArea)
    {
      const float axialOffset = (randomSample.y - 0.5f) * length;
      samplePosition = center + axis * axialOffset + radialToPoint * radius;
    }
    else
    {
      const float endSign = axisCosine >= 0.0f ? 1.0f : -1.0f;
      samplePosition = center + axis * (endSign * length * 0.5f) + centerToPoint * radius;
    }
  }
}
