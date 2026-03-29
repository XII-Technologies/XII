#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Frustum.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiShadingQualityLevel, 1)
  XII_ENUM_CONSTANT(xiiShadingQualityLevel::Low),
  XII_ENUM_CONSTANT(xiiShadingQualityLevel::Medium),
  XII_ENUM_CONSTANT(xiiShadingQualityLevel::High),
  XII_ENUM_CONSTANT(xiiShadingQualityLevel::Ultra),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiCameraUsageHint, 1)
  XII_ENUM_CONSTANT(xiiCameraUsageHint::None)->AddAttributes(new xiiGroupAttribute("Default")),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::MainView)->AddAttributes(new xiiGroupAttribute("Primary")),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::EditorView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::CinematicView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::RenderTarget)->AddAttributes(new xiiGroupAttribute("Offscreen")),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::Thumbnail),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::UIOverlay),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::ShadowMap)->AddAttributes(new xiiGroupAttribute("Shadows")),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::ShadowCascade),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::ShadowProbe),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::ReflectionProbe)->AddAttributes(new xiiGroupAttribute("Reflection")),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::IrradianceProbe),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::SpecularProbe),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::SkyCapture),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::CullingOnly)->AddAttributes(new xiiGroupAttribute("Visibility")),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::VisibilityBuffer),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::MeshletCulling),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::RayTracingCulling),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::DebugView)->AddAttributes(new xiiGroupAttribute("Debug")),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::GPUProfilerView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::LightingDebug),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::MaterialDebug),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::MotionVectorsDebug),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::RayTracingView)->AddAttributes(new xiiGroupAttribute("Special")),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::PathTracingView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::ComputeView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::LowFrequencyView),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiViewRenderMode, 1)
  XII_ENUM_CONSTANT(xiiViewRenderMode::None)->AddAttributes(new xiiGroupAttribute("Default")),
  XII_ENUM_CONSTANT(xiiViewRenderMode::Wireframe)->AddAttributes(new xiiGroupAttribute("Geometry")),
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
  XII_ENUM_CONSTANT(xiiViewRenderMode::VisibilityBuffer)->AddAttributes(new xiiGroupAttribute("Culling")),
  XII_ENUM_CONSTANT(xiiViewRenderMode::InstanceID),
  XII_ENUM_CONSTANT(xiiViewRenderMode::MeshletID),
  XII_ENUM_CONSTANT(xiiViewRenderMode::ClusterID),
  XII_ENUM_CONSTANT(xiiViewRenderMode::CullingOutcome),
  XII_ENUM_CONSTANT(xiiViewRenderMode::OcclusionHeatmap),
  XII_ENUM_CONSTANT(xiiViewRenderMode::BaseColor)->AddAttributes(new xiiGroupAttribute("Material")),
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
  XII_ENUM_CONSTANT(xiiViewRenderMode::LightingOnly)->AddAttributes(new xiiGroupAttribute("Lighting")),
  XII_ENUM_CONSTANT(xiiViewRenderMode::DiffuseOnly),
  XII_ENUM_CONSTANT(xiiViewRenderMode::SpecularOnly),
  XII_ENUM_CONSTANT(xiiViewRenderMode::LightComplexity),
  XII_ENUM_CONSTANT(xiiViewRenderMode::ShadowCascadeIndex),
  XII_ENUM_CONSTANT(xiiViewRenderMode::ShadowMask),
  XII_ENUM_CONSTANT(xiiViewRenderMode::IndirectLighting),
  XII_ENUM_CONSTANT(xiiViewRenderMode::ReflectionContribution),
  XII_ENUM_CONSTANT(xiiViewRenderMode::RayTracingRayCount)->AddAttributes(new xiiGroupAttribute("RayTracing")),
  XII_ENUM_CONSTANT(xiiViewRenderMode::RayTracingBVHTraversal),
  XII_ENUM_CONSTANT(xiiViewRenderMode::RayTracingHitDistance),
  XII_ENUM_CONSTANT(xiiViewRenderMode::RayTracingMissShaderID),
  XII_ENUM_CONSTANT(xiiViewRenderMode::RayTracingInstanceMask),
  XII_ENUM_CONSTANT(xiiViewRenderMode::Exposure)->AddAttributes(new xiiGroupAttribute("PostProcessing")),
  XII_ENUM_CONSTANT(xiiViewRenderMode::Bloom),
  XII_ENUM_CONSTANT(xiiViewRenderMode::ToneMappingCurve),
  XII_ENUM_CONSTANT(xiiViewRenderMode::ColorGradingLUT),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiView, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiView::xiiView() = default;

xiiView::~xiiView() = default;

void xiiView::SetName(xiiStringView sName)
{
  m_sName.Assign(sName);
}

void xiiView::SetCameraUsageHint(xiiEnum<xiiCameraUsageHint> val)
{
  m_Data.m_CameraUsageHint = val;
}

void xiiView::SetViewRenderMode(xiiEnum<xiiViewRenderMode> value)
{
  m_Data.m_ViewRenderMode = value;
}

void xiiView::SetViewport(const xiiRectFloat& viewport)
{
  m_Data.m_ViewPortRect = viewport;
}

void xiiView::ComputeCullingFrustum(xiiFrustum& out_frustum) const
{
  const xiiCamera* pCamera              = GetCullingCamera();
  const float      fViewportAspectRatio = m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height;

  xiiMat4 viewMatrix = pCamera->GetViewMatrix();

  xiiMat4 projectionMatrix;
  pCamera->GetProjectionMatrix(fViewportAspectRatio, projectionMatrix);

  out_frustum = xiiFrustum::MakeFromMVP(projectionMatrix * viewMatrix);
}

void xiiView::UpdateCachedMatrices() const
{
  const xiiCamera* pCamera = GetCamera();

  bool bUpdateVP = false;

  if (m_uiLastCameraOrientationModification != pCamera->GetOrientationModificationCounter())
  {
    bUpdateVP                             = true;
    m_uiLastCameraOrientationModification = pCamera->GetOrientationModificationCounter();

    m_Data.m_ViewMatrix[0] = pCamera->GetViewMatrix(xiiCameraEye::Left);
    m_Data.m_ViewMatrix[1] = pCamera->GetViewMatrix(xiiCameraEye::Right);

    // Some of our matrices contain very small values so that the matrix inversion will fall below the default epsilon.
    // We pass zero as epsilon here since all view and projection matrices are invertible.
    m_Data.m_InverseViewMatrix[0] = m_Data.m_ViewMatrix[0].GetInverse(0.0f);
    m_Data.m_InverseViewMatrix[1] = m_Data.m_ViewMatrix[1].GetInverse(0.0f);
  }

  const float fViewportAspectRatio = m_Data.m_ViewPortRect.HasNonZeroArea() ? m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height : 1.0f;
  if (m_uiLastCameraSettingsModification != pCamera->GetSettingsModificationCounter() || m_fLastViewportAspectRatio != fViewportAspectRatio)
  {
    bUpdateVP                          = true;
    m_uiLastCameraSettingsModification = pCamera->GetSettingsModificationCounter();
    m_fLastViewportAspectRatio         = fViewportAspectRatio;


    pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_Data.m_ProjectionMatrix[0], xiiCameraEye::Left);
    m_Data.m_InverseProjectionMatrix[0] = m_Data.m_ProjectionMatrix[0].GetInverse(0.0f);

    pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_Data.m_ProjectionMatrix[1], xiiCameraEye::Right);
    m_Data.m_InverseProjectionMatrix[1] = m_Data.m_ProjectionMatrix[1].GetInverse(0.0f);
  }

  if (bUpdateVP)
  {
    for (int i = 0; i < 2; ++i)
    {
      m_Data.m_ViewProjectionMatrix[i]        = m_Data.m_ProjectionMatrix[i] * m_Data.m_ViewMatrix[i];
      m_Data.m_InverseViewProjectionMatrix[i] = m_Data.m_ViewProjectionMatrix[i].GetInverse(0.0f);
    }
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_View);
