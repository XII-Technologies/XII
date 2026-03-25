#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

struct XII_SHADER_STRUCT xiiPreviousFrameStats
{
  FLOAT1(GpuFrameTimeMs);
  FLOAT1(GpuSetupTimeMs);
  FLOAT1(GpuUploadTimeMs);
  FLOAT1(GpuPostProcessTimeMs);
  FLOAT1(CpuFrameTimeMs);
  FLOAT1(RenderScale);
  FLOAT1(JitterX);
  FLOAT1(JitterY);
};

// Stored as low/high 32-bit words to keep C++/shader layout identical.
struct XII_SHADER_STRUCT xiiFrameTimestampRange
{
  UINT2(BeginTimestamp);
  UINT2(EndTimestamp);
};

struct XII_SHADER_STRUCT xiiPerFrameCameraUploadData
{
  MAT4(ViewProjectionMatrix);
  MAT4(InverseViewProjectionMatrix);
  FLOAT4(CameraPositionAndNearPlane);
  FLOAT4(CameraForwardAndFarPlane);
};

struct XII_SHADER_STRUCT xiiPerFrameLightUploadData
{
  FLOAT4(MainLightDirectionAndIntensity);
  FLOAT4(MainLightColor);
  FLOAT4(AmbientLightColor);
  UINT1(ActiveLightCount);
  UINT1(Reserved0);
  UINT1(Reserved1);
  UINT1(Reserved2);
};

struct XII_SHADER_STRUCT xiiPerFrameGlobalUploadData
{
  UINT1(FrameIndex);
  FLOAT1(DeltaTimeMs);
  FLOAT1(GlobalTime);
  FLOAT1(WorldTime);
  FLOAT4(RenderScaleJitter);
};

#if !XII_ENABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiPreviousFrameStats) == 32);
static_assert(sizeof(xiiFrameTimestampRange) == 16);
static_assert(sizeof(xiiPerFrameCameraUploadData) == 160);
static_assert(sizeof(xiiPerFrameLightUploadData) == 64);
static_assert(sizeof(xiiPerFrameGlobalUploadData) == 32);
#endif
