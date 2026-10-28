/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Declarations.h>

bool xiiViewRenderMode::IsSupportedVisualizationMode(Enum mode)
{
  if (IsSurfaceVisualizationMode(mode) || IsRayTracingVisualizationMode(mode))
    return true;
  switch (mode)
  {
    case Default:
    case Overdraw:
    case Wireframe:
    case InstanceID:
    case BaseColor:
    case Metallic:
    case Roughness:
    case Specular:
    case Emissive:
    case AmbientOcclusion:
    case NormalMap:
    case DepthLinear:
    case DepthNonLinear:
    case MotionVectors:
    case IndirectLighting:
    case ReflectionContribution:
    case Bloom:
    case Exposure:
    case ToneMappingCurve:
    case ColorGradingLUT:
    case ClusterID:
    case LightingOnly:
    case DiffuseOnly:
    case SpecularOnly:
    case LightComplexity:
    case ShadowMask:
    case ShadowCascadeIndex:
    case CullingOutcome:
    case OcclusionHeatmap:
      return true;
    default:
      return false;
  }
}

bool xiiViewRenderMode::IsSurfaceVisualizationMode(Enum mode)
{
  switch (mode)
  {
    case TriangleSize:
    case VertexNormals:
    case VertexTangents:
    case VertexBitangents:
    case VertexColors:
    case UV0:
    case UV1:
    case UVDensity:
    case LODLevel:
    case StaticVsDynamic:
    case MeshletID:
    case VisibilityBuffer:
    case ShadingModelID:
      return true;
    default:
      return false;
  }
}

bool xiiViewRenderMode::IsRayTracingVisualizationMode(Enum mode)
{
  return mode >= RayTracingRayCount && mode <= RayTracingInstanceMask;
}

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiShadingQualityLevel, 1U)
  XII_ENUM_CONSTANT(xiiShadingQualityLevel::Low),
  XII_ENUM_CONSTANT(xiiShadingQualityLevel::Medium),
  XII_ENUM_CONSTANT(xiiShadingQualityLevel::High),
  XII_ENUM_CONSTANT(xiiShadingQualityLevel::Ultra),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderPath, 1U)
  XII_ENUM_CONSTANT(xiiRenderPath::ForwardPlus),
  XII_ENUM_CONSTANT(xiiRenderPath::Deferred),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiCameraUsageHint, 1U)
  XII_ENUM_CONSTANT(xiiCameraUsageHint::None),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::MainView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::EditorView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::CinematicView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::RenderTarget),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::Thumbnail),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::UIOverlay),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::ShadowMap),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::ShadowCascade),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::ShadowProbe),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::ReflectionProbe),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::IrradianceProbe),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::SpecularProbe),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::SkyCapture),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::CullingOnly),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::VisibilityBuffer),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::MeshletCulling),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::RayTracingCulling),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::DebugView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::GPUProfilerView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::LightingDebug),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::MaterialDebug),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::MotionVectorsDebug),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::RayTracingView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::PathTracingView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::ComputeView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::LowFrequencyView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::SensorRGB),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::SensorInfrared),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::SensorDepth),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::SensorLiDAR),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiViewRenderMode, 1U)
  XII_ENUM_CONSTANT(xiiViewRenderMode::None),
  XII_ENUM_CONSTANT(xiiViewRenderMode::Wireframe),
  XII_ENUM_CONSTANT(xiiViewRenderMode::Overdraw),
  XII_ENUM_CONSTANT(xiiViewRenderMode::TriangleSize),
  XII_ENUM_CONSTANT(xiiViewRenderMode::VertexNormals),
  XII_ENUM_CONSTANT(xiiViewRenderMode::VertexTangents),
  XII_ENUM_CONSTANT(xiiViewRenderMode::VertexBitangents),
  XII_ENUM_CONSTANT(xiiViewRenderMode::VertexColors),
  XII_ENUM_CONSTANT(xiiViewRenderMode::UV0),
  XII_ENUM_CONSTANT(xiiViewRenderMode::UV1),
  XII_ENUM_CONSTANT(xiiViewRenderMode::UVDensity),
  XII_ENUM_CONSTANT(xiiViewRenderMode::LODLevel),
  XII_ENUM_CONSTANT(xiiViewRenderMode::StaticVsDynamic),
  XII_ENUM_CONSTANT(xiiViewRenderMode::VisibilityBuffer),
  XII_ENUM_CONSTANT(xiiViewRenderMode::InstanceID),
  XII_ENUM_CONSTANT(xiiViewRenderMode::MeshletID),
  XII_ENUM_CONSTANT(xiiViewRenderMode::ClusterID),
  XII_ENUM_CONSTANT(xiiViewRenderMode::CullingOutcome),
  XII_ENUM_CONSTANT(xiiViewRenderMode::OcclusionHeatmap),
  XII_ENUM_CONSTANT(xiiViewRenderMode::BaseColor),
  XII_ENUM_CONSTANT(xiiViewRenderMode::Metallic),
  XII_ENUM_CONSTANT(xiiViewRenderMode::Roughness),
  XII_ENUM_CONSTANT(xiiViewRenderMode::Specular),
  XII_ENUM_CONSTANT(xiiViewRenderMode::Emissive),
  XII_ENUM_CONSTANT(xiiViewRenderMode::AmbientOcclusion),
  XII_ENUM_CONSTANT(xiiViewRenderMode::NormalMap),
  XII_ENUM_CONSTANT(xiiViewRenderMode::DepthLinear),
  XII_ENUM_CONSTANT(xiiViewRenderMode::DepthNonLinear),
  XII_ENUM_CONSTANT(xiiViewRenderMode::MotionVectors),
  XII_ENUM_CONSTANT(xiiViewRenderMode::ShadingModelID),
  XII_ENUM_CONSTANT(xiiViewRenderMode::LightingOnly),
  XII_ENUM_CONSTANT(xiiViewRenderMode::DiffuseOnly),
  XII_ENUM_CONSTANT(xiiViewRenderMode::SpecularOnly),
  XII_ENUM_CONSTANT(xiiViewRenderMode::LightComplexity),
  XII_ENUM_CONSTANT(xiiViewRenderMode::ShadowCascadeIndex),
  XII_ENUM_CONSTANT(xiiViewRenderMode::ShadowMask),
  XII_ENUM_CONSTANT(xiiViewRenderMode::IndirectLighting),
  XII_ENUM_CONSTANT(xiiViewRenderMode::ReflectionContribution),
  XII_ENUM_CONSTANT(xiiViewRenderMode::RayTracingRayCount),
  XII_ENUM_CONSTANT(xiiViewRenderMode::RayTracingBVHTraversal),
  XII_ENUM_CONSTANT(xiiViewRenderMode::RayTracingHitDistance),
  XII_ENUM_CONSTANT(xiiViewRenderMode::RayTracingMissShaderID),
  XII_ENUM_CONSTANT(xiiViewRenderMode::RayTracingInstanceMask),
  XII_ENUM_CONSTANT(xiiViewRenderMode::Exposure),
  XII_ENUM_CONSTANT(xiiViewRenderMode::Bloom),
  XII_ENUM_CONSTANT(xiiViewRenderMode::ToneMappingCurve),
  XII_ENUM_CONSTANT(xiiViewRenderMode::ColorGradingLUT),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Declarations);
