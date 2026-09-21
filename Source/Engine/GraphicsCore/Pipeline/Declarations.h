/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

/// Defines the shading quality levels used in rendering operations.
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

/// Usage hint of a camera/view.
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

/// Selects a visualization or debug rendering mode for a view.
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

using xiiViewId = xiiGenericId<24, 8>;

class xiiViewHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiViewHandle, xiiViewId);

  friend class xiiRenderWorldModule;
};

/// HashHelper implementation so view handles can be used as key in a hashtable.
template <>
struct xiiHashHelper<xiiViewHandle>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiViewHandle value) { return value.GetInternalID().m_Data * 2654435761U; }

  XII_ALWAYS_INLINE static bool Equal(xiiViewHandle a, xiiViewHandle b) { return a == b; }
};
