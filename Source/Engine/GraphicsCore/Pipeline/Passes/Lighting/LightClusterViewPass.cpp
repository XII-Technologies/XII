/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterViewPass.h>
#include <GraphicsCore/Pipeline/View.h>
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLightClusterFrameData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLightClusterViewPass, 1, xiiRTTIDefaultAllocator<xiiLightClusterViewPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Layout", m_Layout)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("UseViewCamera", m_bUseViewCamera)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("InverseViewProjection", m_InverseViewProjection),
    XII_MEMBER_PROPERTY("CameraPosition", m_CameraPosition),
    XII_MEMBER_PROPERTY("CameraForward", m_CameraForward)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0,0,1))),
    XII_MEMBER_PROPERTY("NearDistance", m_fNearDistance)->AddAttributes(new xiiDefaultValueAttribute(0.1f), new xiiClampValueAttribute(0.0001f, xiiVariant())),
    XII_MEMBER_PROPERTY("FarDistance", m_fFarDistance)->AddAttributes(new xiiDefaultValueAttribute(1000.0f), new xiiClampValueAttribute(0.0001f, xiiVariant())),
    XII_MEMBER_PROPERTY("Width", m_uiWidth)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 65536U)),
    XII_MEMBER_PROPERTY("Height", m_uiHeight)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 65536U)),
    XII_MEMBER_PROPERTY("TileSize", m_uiTileSize)->AddAttributes(new xiiDefaultValueAttribute(16U), new xiiClampValueAttribute(1U, 1024U)),
    XII_MEMBER_PROPERTY("DepthSlices", m_uiDepthSlices)->AddAttributes(new xiiDefaultValueAttribute(24U), new xiiClampValueAttribute(1U, 256U)),
    XII_MEMBER_PROPERTY("MaxLightsPerCluster", m_uiMaxLightsPerCluster)->AddAttributes(new xiiDefaultValueAttribute(128U), new xiiClampValueAttribute(1U, 1024U)),
    XII_MEMBER_PROPERTY("MaximumClusters", m_uiMaximumClusters)->AddAttributes(new xiiDefaultValueAttribute(262144U), new xiiClampValueAttribute(1U, 16777216U)),
    XII_MEMBER_PROPERTY("MaximumIndices", m_uiMaximumIndices)->AddAttributes(new xiiDefaultValueAttribute(4194304U), new xiiClampValueAttribute(1U, 16777216U)),
    XII_MEMBER_PROPERTY("LogarithmicDepth", m_bLogarithmicDepth)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiLightClusterViewPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_InverseViewProjection.IsValid() && m_CameraPosition.IsValid() && m_CameraForward.IsValid() && m_CameraForward.GetLengthSquared() > 1e-12f &&
      xiiMath::IsFinite(m_fNearDistance) && xiiMath::IsFinite(m_fFarDistance) && m_fNearDistance > 0 && m_fFarDistance > m_fNearDistance &&
      m_uiWidth <= 65536 && m_uiHeight <= 65536 && m_uiTileSize >= 1 && m_uiTileSize <= 1024 && m_uiDepthSlices >= 1 && m_uiDepthSlices <= 256 &&
      m_uiMaxLightsPerCluster >= 1 && m_uiMaxLightsPerCluster <= 1024 && m_uiMaximumClusters >= 1 && m_uiMaximumClusters <= 16777216 && m_uiMaximumIndices >= 1 && m_uiMaximumIndices <= 16777216 ?
    XII_SUCCESS :
    XII_FAILURE;
}
xiiResult xiiLightClusterViewPass::Build(xiiRenderPipelinePassContext& context)
{
  auto matrix   = m_InverseViewProjection;
  auto position = m_CameraPosition, forward = m_CameraForward;
  auto nearDistance = m_fNearDistance, farDistance = m_fFarDistance;
  auto width = m_uiWidth, height = m_uiHeight;
  if (m_bUseViewCamera)
  {
    if (!context.m_pView || !context.m_pView->GetCamera())
      return context.ReportFailure("Cluster view capture requires a camera or an explicit projection.");
    const auto& view   = *context.m_pView;
    const auto& camera = *view.GetCamera();
    matrix             = view.GetInverseViewProjectionMatrix(xiiCameraEye::Left);
    position           = camera.GetPosition();
    forward            = camera.GetDirForwards();
    nearDistance       = camera.GetNearPlane();
    farDistance        = camera.GetFarPlane();
    if (!width)
      width = view.GetRenderResolutionWidth();
    if (!height)
      height = view.GetRenderResolutionHeight();
  }
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ClusterInverseViewProjection", matrix));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ClusterCameraPosition", position));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ClusterCameraForward", forward));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ClusterWidth", width));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ClusterHeight", height));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ClusterNear", nearDistance));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ClusterFar", farDistance));
  auto projection = matrix;
  if (!matrix.IsValid() || projection.Invert().Failed() || !position.IsValid() || !forward.IsValid() || forward.NormalizeIfNotZero(xiiVec3(0, 0, 1)).Failed() || !xiiMath::IsFinite(nearDistance) || !xiiMath::IsFinite(farDistance) || nearDistance <= 0 || farDistance <= nearDistance || !xiiMath::IsFinite(farDistance / nearDistance) || !width || !height || width > 65536 || height > 65536)
    return context.ReportFailure("Invalid cluster projection, camera, resolution or finite depth range.");
  // Validate finite rays at interior NDC depths, including orthographic and finite/infinite perspective projections.
  float referenceW = 0, referenceDepthDirection = 0;
  for (xiiUInt32 y = 0; y < 2; ++y)
    for (xiiUInt32 x = 0; x < 2; ++x)
    {
      auto a = matrix * xiiVec4(x ? 1.0f : -1.0f, y ? 1.0f : -1.0f, 0.25f, 1);
      auto b = matrix * xiiVec4(x ? 1.0f : -1.0f, y ? 1.0f : -1.0f, 0.75f, 1);
      if (!a.IsValid() || !b.IsValid() || xiiMath::Abs(a.w) < 1e-12f || xiiMath::Abs(b.w) < 1e-12f || xiiMath::Abs(forward.Dot(b.GetAsVec3() / b.w - a.GetAsVec3() / a.w)) < 1e-8f)
        return context.ReportFailure("Cluster projection rays must intersect the supplied camera-depth planes.");
      const auto origin = a.GetAsVec3() / a.w, direction = b.GetAsVec3() / b.w - origin;
      const auto denominator = forward.Dot(direction);
      if (referenceW == 0)
      {
        referenceW              = a.w;
        referenceDepthDirection = denominator;
      }
      if ((a.w > 0) != (referenceW > 0) || (b.w > 0) != (referenceW > 0) || (denominator > 0) != (referenceDepthDirection > 0))
        return context.ReportFailure("Cluster projection crosses a horizon inside the viewport.");
      for (const auto distance : {nearDistance, farDistance})
        if (!(origin + direction * ((distance - forward.Dot(origin - position)) / denominator)).IsValid())
          return context.ReportFailure("Cluster depth planes exceed finite world-space bounds.");
    }
  const xiiUInt64 columns = (width + m_uiTileSize - 1U) / m_uiTileSize, rows = (height + m_uiTileSize - 1U) / m_uiTileSize;
  const xiiUInt64 count = columns * rows * m_uiDepthSlices, indices = count * m_uiMaxLightsPerCluster;
  if (count > m_uiMaximumClusters || indices > m_uiMaximumIndices)
    return context.ReportFailure("Cluster layout exceeds the explicitly configured cluster or index budget.");
  xiiLightClusterConstants constants;
  constants.InverseViewProjection = matrix;
  constants.ViewProjection        = projection;
  constants.ClusterCounts         = xiiVec4U32(xiiUInt32(columns), xiiUInt32(rows), m_uiDepthSlices, xiiUInt32(count));
  constants.Viewport              = xiiVec4U32(width, height, m_uiTileSize, m_uiMaxLightsPerCluster);
  constants.DepthRange            = xiiVec4(nearDistance, farDistance, xiiMath::Log2(farDistance / nearDistance), m_bLogarithmicDepth ? 1.0f : 0.0f);
  constants.CameraPosition        = xiiVec4(position, 1);
  constants.CameraForward         = xiiVec4(forward, 0);
  constants.Work                  = xiiVec4U32(0, xiiUInt32(indices), 0, 0);
  return context.SetOutput(m_Layout, XII_NEW(xiiFoundation::GetAlignedAllocator(), xiiLightClusterFrameData, context.m_Graph.GetSetupSerial(), constants));
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_LightClusterViewPass);
