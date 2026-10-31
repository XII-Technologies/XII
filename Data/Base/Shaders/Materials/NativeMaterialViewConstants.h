/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiNativeMaterialViewConstants)
{
  MAT4(MaterialWorldToView);
  MAT4(MaterialViewToWorld);
  MAT4(MaterialViewToClip);
  MAT4(MaterialClipToView);
  FLOAT4(MaterialCameraPosition);
  FLOAT4(MaterialCameraForward);
  FLOAT4(MaterialCameraRight);
  FLOAT4(MaterialCameraUp);
  FLOAT4(MaterialTimes); // global seconds, world seconds, manual camera exposure, padding
};
#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiNativeMaterialViewConstants) == 336);
#endif
