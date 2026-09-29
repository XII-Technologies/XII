/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Reflection/Reflection.h>

class xiiCamera;

/// Authoring and quality controls for stabilized cascaded shadow maps.
struct XII_GRAPHICSCORE_DLL xiiShadowCascadeSettings
{
  xiiUInt32 m_uiCascadeCount         = 4U;
  float     m_fSplitLambda           = 0.65f; ///< 0 = uniform splits, 1 = logarithmic splits.
  float     m_fMaximumShadowDistance = 250.0f;
  float     m_fDepthPadding          = 50.0f; ///< Extra light-space depth for off-frustum casters.
  xiiUInt32 m_uiShadowMapResolution  = 4096U;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiShadowCascadeSettings);

struct XII_GRAPHICSCORE_DLL xiiShadowCascadeDescription
{
  xiiMat4 m_mViewProjection = xiiMat4::MakeIdentity();
  float   m_fSplitNear      = 0.0f;
  float   m_fSplitFar       = 0.0f;
};

/// CPU cascade construction shared by the renderer and deterministic tests.
class XII_GRAPHICSCORE_DLL xiiShadowCascadeUtils
{
public:
  /// Builds tightly fitted, texel-stabilized orthographic cascades for a directional light.
  static xiiResult Build(const xiiCamera& camera, float fAspectRatio, const xiiVec3& vLightDirection,
    const xiiShadowCascadeSettings& settings, xiiStaticArray<xiiShadowCascadeDescription, 4>& out_cascades);
};
