#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsFoundation/Device/SwapChain.h>

/// \brief Defines the shading quality levels used in rendering operations.
///
/// This enumeration allows rendering systems or materials to selectively enable or disable visual features depending on the desired quality level.
/// Lower settings may omit expensive effects (e.g., shadows, complex lighting), while higher levels offer more realistic and detailed shading.
///
/// The quality level can be globally configured or overridden per-pass/material depending on engine support.
///
/// Typical usage:
/// - Low: Minimal shading, suitable for previews or constrained hardware.
/// - Medium: Standard shading with balanced performance and fidelity.
/// - High: Enhanced shading with advanced lighting or material features.
/// - Ultra: Maximum visual fidelity; may include ray tracing or physically-based effects.
struct XII_GRAPHICSCORE_DLL xiiShadingQualityLevel
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Low = 0, ///< Minimal shading features; optimized for speed.
    Medium,  ///< Balanced shading quality; default setting.
    High,    ///< Advanced shading features enabled.
    Ultra,   ///< Maximum-quality shading; highest visual fidelity.

    ENUM_COUNT,

    Default = Medium
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiShadingQualityLevel);

/// \brief Usage hint of a camera/view.
///
/// This enumeration provides context about how a camera or view is intended to be used by the rendering system.
/// It enables systems (renderers, culling, post-processing, profiling, etc.) to adapt behavior depending on the role of the view.
/// Examples include distinguishing primary player cameras from editor viewports, designating offscreen render targets for thumbnails or probes, and selecting specialized views for shadow, reflection, or debugging passes.
struct XII_GRAPHICSCORE_DLL xiiCameraUsageHint
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None = 0U,
    MainView,
    EditorView,
    CinematicView,
    RenderTarget,
    Thumbnail,
    UIOverlay,
    ShadowMap,
    ShadowCascade,
    ShadowProbe,
    ReflectionProbe,
    IrradianceProbe,
    SpecularProbe,
    SkyCapture,
    CullingOnly,
    VisibilityBuffer,
    MeshletCulling,
    RayTracingCulling,
    DebugView,
    GPUProfilerView,
    LightingDebug,
    MaterialDebug,
    MotionVectorsDebug,
    RayTracingView,
    PathTracingView,
    ComputeView,
    LowFrequencyView,

    ENUM_COUNT,

    Default = None
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiCameraUsageHint);

/// \brief Selects a visualization or debug rendering mode for a view.
///
/// This enumeration controls how the scene is shaded for debugging, visualization, or profiling purposes.
/// Modes range from simple wireframe and overdraw visualizations to material/G-Buffer channel inspection, lighting-only renders, and ray-tracing-specific diagnostics.
/// Renderers can switch modes to expose different internal buffers or to assist artists and engineers in understanding rendering behavior.
struct XII_GRAPHICSCORE_DLL xiiViewRenderMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None = 0U,
    Wireframe,
    Overdraw,
    TriangleSize,
    VertexNormals,
    VertexTangents,
    VertexBitangents,
    VertexColors,
    UV0,
    UV1,
    UVDensity,
    LODLevel,
    StaticVsDynamic,
    VisibilityBuffer,
    InstanceID,
    MeshletID,
    ClusterID,
    CullingOutcome,
    OcclusionHeatmap,
    BaseColor,
    Metallic,
    Roughness,
    Specular,
    Emissive,
    AmbientOcclusion,
    NormalMap,
    DepthLinear,
    DepthNonLinear,
    MotionVectors,
    ShadingModelID,
    LightingOnly,
    DiffuseOnly,
    SpecularOnly,
    LightComplexity,
    ShadowCascadeIndex,
    ShadowMask,
    IndirectLighting,
    ReflectionContribution,
    RayTracingRayCount,
    RayTracingBVHTraversal,
    RayTracingHitDistance,
    RayTracingMissShaderID,
    RayTracingInstanceMask,
    Exposure,
    Bloom,
    ToneMappingCurve,
    ColorGradingLUT,

    ENUM_COUNT,

    Default = None
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiViewRenderMode);

/// \brief Holds view data like the viewport, view and projection matrices
struct XII_GRAPHICSCORE_DLL xiiViewData
{
  xiiViewData()
  {
    m_ViewPortRect   = xiiRectFloat(0.0f, 0.0f);
    m_ViewRenderMode = xiiViewRenderMode::None;

    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      m_ViewMatrix[i].SetIdentity();
      m_InverseViewMatrix[i].SetIdentity();
      m_ProjectionMatrix[i].SetIdentity();
      m_InverseProjectionMatrix[i].SetIdentity();
      m_ViewProjectionMatrix[i].SetIdentity();
      m_InverseViewProjectionMatrix[i].SetIdentity();
    }
  }

  xiiRectFloat                m_ViewPortRect;
  xiiEnum<xiiViewRenderMode>  m_ViewRenderMode;
  xiiEnum<xiiCameraUsageHint> m_CameraUsageHint;

  // Each matrix is there for both left and right camera lens.
  xiiMat4 m_ViewMatrix[2];
  xiiMat4 m_InverseViewMatrix[2];
  xiiMat4 m_ProjectionMatrix[2];
  xiiMat4 m_InverseProjectionMatrix[2];
  xiiMat4 m_ViewProjectionMatrix[2];
  xiiMat4 m_InverseViewProjectionMatrix[2];

  /// \brief Calculates the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fNormalizedScreenPosX and fNormalizedScreenPosY are expected to be in [0; 1] range (normalized screen coordinates).
  /// If no ray can be computed, XII_FAILURE is returned.
  XII_ALWAYS_INLINE xiiResult ComputePickingRay(float fNormalizedScreenPosX, float fNormalizedScreenPosY, xiiVec3& out_vRayStartPos, xiiVec3& out_vRayDir, xiiCameraEye eye = xiiCameraEye::Left) const
  {
    xiiVec3 vScreenPos;
    vScreenPos.x = fNormalizedScreenPosX;
    vScreenPos.y = fNormalizedScreenPosY;
    vScreenPos.z = 0.0f;

    return xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_InverseViewProjectionMatrix[static_cast<xiiInt32>(eye)], vScreenPos, out_vRayStartPos, &out_vRayDir);
  }

  /// \brief Calculates the normalized screen-space coordinate ([0; 1] range) that the given world-space point projects to.
  ///
  /// Returns XII_FAILURE, if the point could not be projected into screen-space.
  XII_ALWAYS_INLINE xiiResult ComputeScreenSpacePos(const xiiVec3& vWorldPos, xiiVec3& out_vScreenPosNormalized, xiiCameraEye eye = xiiCameraEye::Left) const
  {
    return xiiGraphicsUtils::ConvertWorldPosToScreenPos(m_ViewProjectionMatrix[static_cast<xiiInt32>(eye)], vWorldPos, out_vScreenPosNormalized);
  }

  /// \brief Calculates the world-space position that the given normalized screen-space coordinate maps to
  XII_ALWAYS_INLINE xiiResult ComputeWorldSpacePos(float fNormalizedScreenPosX, float fNormalizedScreenPosY, xiiVec3& out_vWorldPos, xiiCameraEye eye = xiiCameraEye::Left) const
  {
    return xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_InverseViewProjectionMatrix[static_cast<xiiInt32>(eye)], xiiVec3(fNormalizedScreenPosX, fNormalizedScreenPosY, 0.0f), out_vWorldPos);
  }

  /// \brief Converts a screen-space position from pixel coordinates to normalized coordinates.
  XII_ALWAYS_INLINE void ConvertScreenPixelPosToNormalizedPos(xiiVec3& inout_vPixelPos) const
  {
    xiiUInt32 x = (xiiUInt32)m_ViewPortRect.x;
    xiiUInt32 y = (xiiUInt32)m_ViewPortRect.y;
    xiiUInt32 w = (xiiUInt32)m_ViewPortRect.width;
    xiiUInt32 h = (xiiUInt32)m_ViewPortRect.height;
    xiiGraphicsUtils::ConvertScreenPixelPosToNormalizedPos(x, y, w, h, inout_vPixelPos);
  }

  /// \brief Converts a screen-space position from normalized coordinates to pixel coordinates.
  XII_ALWAYS_INLINE void ConvertScreenNormalizedPosToPixelPos(xiiVec3& inout_vNormalizedPos) const
  {
    xiiUInt32 x = (xiiUInt32)m_ViewPortRect.x;
    xiiUInt32 y = (xiiUInt32)m_ViewPortRect.y;
    xiiUInt32 w = (xiiUInt32)m_ViewPortRect.width;
    xiiUInt32 h = (xiiUInt32)m_ViewPortRect.height;
    xiiGraphicsUtils::ConvertScreenNormalizedPosToPixelPos(x, y, w, h, inout_vNormalizedPos);
  }
};
