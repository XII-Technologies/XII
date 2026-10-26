/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Shaders/Common/StorageImageFormat.h>

// Temporal anti-aliasing with velocity reprojection and YCoCg neighbourhood clipping.

#include <Shaders/Common/Common.h>
#include <Shaders/Pipeline/Passes/Temporal/TAAConstants.h>

Texture2D<float4> g_CurrentFrame;
Texture2D<float2> g_VelocityBuffer;
Texture2D<float>  g_SceneDepth;
Texture2D<float4> g_HistoryFrame;
Texture2D<float>  g_HistoryDepth;
#if XII_TAA_HISTORY_TRUST
Texture2D<float> g_HistoryTrust;
#endif
XII_STORAGE_IMAGE_FORMAT("r32f")
RWTexture2D<float> g_RetainedDepth;
XII_STORAGE_IMAGE_FORMAT(XII_TAA_STORAGE_FORMAT)
RWTexture2D<float4> g_Resolved;

float3 RGBToYCoCg(float3 color)
{
  return float3(
    0.25f * color.r + 0.5f * color.g + 0.25f * color.b,
    0.5f * color.r - 0.5f * color.b,
    -0.25f * color.r + 0.5f * color.g - 0.25f * color.b);
}

float3 YCoCgToRGB(float3 color)
{
  float luminanceMinusChroma = color.x - color.z;
  return float3(luminanceMinusChroma + color.y, color.x + color.z, luminanceMinusChroma - color.y);
}

float3 ClipAABB(float3 minimumColor, float3 maximumColor, float3 historyColor)
{
  float3 center            = 0.5f * (maximumColor + minimumColor);
  float3 extent            = 0.5f * (maximumColor - minimumColor) + 1.0e-5f;
  float3 offset            = historyColor - center;
  float3 unitOffset        = abs(offset / extent);
  float  maximumUnitOffset = max(unitOffset.x, max(unitOffset.y, unitOffset.z));
  return maximumUnitOffset > 1.0f ? center + offset / maximumUnitOffset : historyColor;
}

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
  uint2 dimensions;
  g_Resolved.GetDimensions(dimensions.x, dimensions.y);
  if (any(dispatchThreadId.xy >= dimensions))
    return;

  const float2 inverseDimensions = rcp(float2(dimensions));
  const float2 uv                = (float2(dispatchThreadId.xy) + 0.5f) * inverseDimensions;
  float        closestDepth      = 0.0f;
  float2       velocity          = 0.0f;
  [unroll] for (int y = -1; y <= 1; ++y)
  {
    [unroll] for (int x = -1; x <= 1; ++x)
    {
      const float2 sampleUv    = uv + float2(x, y) * inverseDimensions;
      const float  sampleDepth = g_SceneDepth.SampleLevel(PointClampSampler, sampleUv, 0.0f);
      if (sampleDepth > closestDepth)
      {
        closestDepth = sampleDepth;
        velocity     = g_VelocityBuffer.SampleLevel(PointClampSampler, sampleUv, 0.0f);
      }
    }
  }

  const float  depth                   = g_SceneDepth.Load(int3(dispatchThreadId.xy, 0));
  const float4 current                 = g_CurrentFrame.Load(int3(dispatchThreadId.xy, 0));
  const float3 currentColor            = current.rgb;
  bool untrusted = false;
#if XII_TAA_HISTORY_TRUST
  untrusted = g_HistoryTrust.Load(int3(dispatchThreadId.xy, 0)) < 0.999f;
#endif
  // Negative values encode invalid color provenance in private depth history.
  // This rejects a disappeared transparent layer even when this frame has no mask.
  g_RetainedDepth[dispatchThreadId.xy] = untrusted ? -(depth + 1.0f) : depth;
  if (HistoryValid == 0U || untrusted)
  {
    g_Resolved[dispatchThreadId.xy] = current;
    return;
  }

  // Resolved color accumulates on a stable pixel grid. Raster velocity excludes
  // jitter; adding the jitter delta here would move the accumulated image every frame.
  float2 previousUv = uv - velocity * float2(0.5f, -0.5f);
  // Depth history retains raw raster samples, so its lookup follows jitter instead.
  float2 previousSurfaceUv = previousUv + JitterDelta * float2(0.5f, -0.5f);
  float  expectedDepth     = depth;
  if (HasCamera != 0U)
  {
    const float4 world = mul(InverseViewProjection, float4(uv * float2(2, -2) + float2(-1, 1), depth, 1));
    float4       previous;
    if (depth == 0.0f)
    {
      // Sky motion follows camera rotation, without translating an infinite background.
      const float3 direction = world.xyz - CameraPosition.xyz * world.w;
      previous               = mul(PreviousViewProjection, float4(direction, 0));
      previousSurfaceUv      = previous.xy / previous.w * float2(0.5f, -0.5f) + 0.5f;
      previousUv             = previousSurfaceUv - JitterDelta * float2(0.5f, -0.5f);
    }
    else
    {
      previous      = mul(PreviousViewProjection, world);
      expectedDepth = previous.z / previous.w;
    }
    if (previous.w <= 0.0f)
    {
      g_Resolved[dispatchThreadId.xy] = current;
      return;
    }
  }
  if (any(previousUv < 0.0f) || any(previousUv >= 1.0f) || any(previousSurfaceUv < 0.0f) || any(previousSurfaceUv >= 1.0f))
  {
    g_Resolved[dispatchThreadId.xy] = current;
    return;
  }
  const float previousDepth = g_HistoryDepth.SampleLevel(PointClampSampler, previousSurfaceUv, 0);
  // A depth-changing surface is conservatively rejected, including previously covered
  // background. Moving geometry in depth may reject valid history rather than ghost.
  const bool backgroundMismatch = (depth == 0.0f) != (previousDepth == 0.0f);
  if (previousDepth < 0 || backgroundMismatch || abs(previousDepth - expectedDepth) > max(1.0e-6f, abs(expectedDepth) * DepthThreshold))
  {
    g_Resolved[dispatchThreadId.xy] = current;
    return;
  }

  float3 minimumColor = float3(1.0e9f, 1.0e9f, 1.0e9f);
  float3 maximumColor = float3(-1.0e9f, -1.0e9f, -1.0e9f);
  [unroll] for (int y = -1; y <= 1; ++y)
  {
    [unroll] for (int x = -1; x <= 1; ++x)
    {
      const float2 sampleUv    = uv + float2(x, y) * inverseDimensions;
      const float3 sampleColor = RGBToYCoCg(g_CurrentFrame.SampleLevel(LinearClampSampler, sampleUv, 0.0f).rgb);
      minimumColor             = min(minimumColor, sampleColor);
      maximumColor             = max(maximumColor, sampleColor);
    }
  }

  float3 historyColor = currentColor;
  if (all(previousUv >= 0.0f) && all(previousUv <= 1.0f))
  {
    historyColor = g_HistoryFrame.SampleLevel(LinearClampSampler, previousUv, 0.0f).rgb;
    historyColor = YCoCgToRGB(ClipAABB(minimumColor, maximumColor, RGBToYCoCg(historyColor)));
  }

  const float blendAlpha          = saturate(BaseBlendAlpha + length(velocity) * MotionBlendScale);
  g_Resolved[dispatchThreadId.xy] = float4(max(lerp(historyColor, currentColor, blendAlpha), 0.0f), current.a);
}
