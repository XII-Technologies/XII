/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

#define XII_EDITOR_GIZMO_RING     15
#define XII_EDITOR_GIZMO_ON_TOP   1
#define XII_EDITOR_GIZMO_WIRE     2
#define XII_EDITOR_GIZMO_ACTIVE   4
#define XII_EDITOR_GIZMO_HOVER    8
#define XII_EDITOR_GIZMO_DISABLED 16
#define XII_EDITOR_GIZMO_SELECTED 32

struct xiiEditorGizmoInstance
{
#if XII_DISABLED(XII_SHADER_PLATFORM)
  XII_DECLARE_POD_TYPE();
#endif
  MAT4(Transform);
  FLOAT4(Color);
  FLOAT4(Parameters); // stroke pixels, arc start, arc delta, device pixel ratio
  UINT4(Identity);    // shape, visual flags, stable picking ID, reserved
  FLOAT4(GuideStart); // camera-relative drag origin; active translation guides only
};

struct xiiEditorGizmoVertex
{
#if XII_DISABLED(XII_SHADER_PLATFORM)
  XII_DECLARE_POD_TYPE();
#endif
  FLOAT4(Position);  // w: 0 solid, 1 expanded stroke
  FLOAT4(Auxiliary); // normal for solids; second endpoint + corner for strokes
};

DECLARE_CONSTANT_BUFFER_AUTO(xiiEditorGizmoConstants)
{
  MAT4(RelativeViewProjection);
  FLOAT4(Viewport); // dimensions and reciprocals of the overlay target
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiEditorGizmoInstance) == 128);
static_assert(sizeof(xiiEditorGizmoVertex) == 32);
static_assert(sizeof(xiiEditorGizmoConstants) == 80);
#endif
