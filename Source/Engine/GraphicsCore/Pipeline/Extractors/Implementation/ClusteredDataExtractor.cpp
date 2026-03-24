#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Profiling/Profiling.h>
#include <GraphicsCore/Components/FogComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Lights/AmbientLightComponent.h>
#include <GraphicsCore/Pipeline/Extractors/ClusteredDataExtractor.h>
#include <GraphicsCore/Lights/Implementation/ClusteredDataUtils.h>
#include <GraphicsCore/Pipeline/RenderData/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/View.h>

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
xiiCVarBool cvar_RenderingLightingVisClusterData("Rendering.Lighting.VisClusterData", false, xiiCVarFlags::Default, "Enables debug visualization of clustered light data");
xiiCVarInt  cvar_RenderingLightingVisClusterDepthSlice("Rendering.Lighting.VisClusterDepthSlice", -1, xiiCVarFlags::Default, "Show the debug visualization only for the given depth slice");

namespace
{
  void VisualizeClusteredData(const xiiView& view, const xiiClusteredDataCPU* pData, xiiArrayPtr<xiiSimdBSphere> boundingSpheres)
  {
    if (!cvar_RenderingLightingVisClusterData)
      return;

    const xiiCamera* pCamera = view.GetCullingCamera();

    if (pCamera->IsOrthographic())
      return;

    float fAspectRatio = view.GetViewport().width / view.GetViewport().height;

    xiiMat4 mProj;
    pCamera->GetProjectionMatrix(fAspectRatio, mProj);

    const xiiMat4& mInvView = pCamera->GetViewMatrix().GetInverse();

    xiiAngle fFovLeft;
    xiiAngle fFovRight;
    xiiAngle fFovBottom;
    xiiAngle fFovTop;
    xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop);

    const float fTanLeft   = xiiMath::Tan(fFovLeft);
    const float fTanRight  = xiiMath::Tan(fFovRight);
    const float fTanBottom = xiiMath::Tan(fFovBottom);
    const float fTanTop    = xiiMath::Tan(fFovTop);

    xiiColor lineColor = xiiColor(1.0f, 1.0f, 1.0f, 0.1f);

    const xiiInt32  debugSlice    = cvar_RenderingLightingVisClusterDepthSlice;
    const bool      bOnlyOneSlice = debugSlice >= 0;
    const xiiUInt32 maxSlice      = bOnlyOneSlice ? debugSlice + 1 : NUM_CLUSTERS_Z;
    const xiiUInt32 minSlice      = bOnlyOneSlice ? debugSlice : 0;

    bool             bDrawBoundingSphere = false;
    xiiStringBuilder sb;

    for (xiiUInt32 z = maxSlice; z-- > minSlice;)
    {
      float fZf = GetDepthFromSliceIndex(z);
      float fZn = (z > 0) ? GetDepthFromSliceIndex(z - 1) : 0.0f;
      for (xiiInt32 y = 0; y < NUM_CLUSTERS_Y; ++y)
      {
        for (xiiInt32 x = 0; x < NUM_CLUSTERS_X; ++x)
        {
          xiiUInt32 clusterIndex = GetClusterIndexFromCoord(x, y, z);
          auto&     clusterData  = pData->m_ClusterData[clusterIndex];

          if (clusterData.counts > 0)
          {
            if (bDrawBoundingSphere)
            {
              xiiBoundingSphere s = xiiSimdConversion::ToBSphere(boundingSpheres[clusterIndex]);
              s.TransformFromOrigin(mInvView);
              xiiDebugRenderer::DrawLineSphere(view.GetHandle(), s, lineColor);
            }
            else
            {
              xiiVec3 cc[8];
              GetClusterCornerPoints(*pCamera, fZf, fZn, fTanLeft, fTanRight, fTanBottom, fTanTop, x, y, z, cc);

              const float    lightCount = (float)GET_LIGHT_INDEX(clusterData.counts);
              const float    decalCount = (float)GET_DECAL_INDEX(clusterData.counts);
              const float    probeCount = (float)GET_PROBE_INDEX(clusterData.counts);
              const float    r          = xiiMath::Clamp(lightCount / 16.0f, 0.0f, 1.0f);
              const float    g          = xiiMath::Clamp(decalCount / 16.0f, 0.0f, 1.0f);
              const float    b          = xiiMath::Clamp(probeCount / 16.0f, 0.0f, 1.0f);
              const xiiColor color(r, g, b);

              xiiDebugRendererTriangle tris[12];
              // back
              tris[0] = xiiDebugRendererTriangle(cc[0], cc[2], cc[1]);
              tris[1] = xiiDebugRendererTriangle(cc[2], cc[3], cc[1]);
              // front
              tris[2] = xiiDebugRendererTriangle(cc[4], cc[5], cc[6]);
              tris[3] = xiiDebugRendererTriangle(cc[6], cc[5], cc[7]);
              // top
              tris[4] = xiiDebugRendererTriangle(cc[4], cc[0], cc[5]);
              tris[5] = xiiDebugRendererTriangle(cc[0], cc[1], cc[5]);
              // bottom
              tris[6] = xiiDebugRendererTriangle(cc[6], cc[7], cc[2]);
              tris[7] = xiiDebugRendererTriangle(cc[2], cc[7], cc[3]);
              // left
              tris[8] = xiiDebugRendererTriangle(cc[4], cc[6], cc[0]);
              tris[9] = xiiDebugRendererTriangle(cc[0], cc[6], cc[2]);
              // right
              tris[10] = xiiDebugRendererTriangle(cc[5], cc[1], cc[7]);
              tris[11] = xiiDebugRendererTriangle(cc[1], cc[3], cc[7]);

              xiiDebugRenderer::DrawSolidTriangles(view.GetHandle(), tris, color.WithAlpha(0.1f));

              xiiDebugRendererLine lines[12];
              lines[0] = xiiDebugRendererLine(cc[4], cc[5]);
              lines[1] = xiiDebugRendererLine(cc[5], cc[7]);
              lines[2] = xiiDebugRendererLine(cc[7], cc[6]);
              lines[3] = xiiDebugRendererLine(cc[6], cc[4]);

              lines[4] = xiiDebugRendererLine(cc[0], cc[1]);
              lines[5] = xiiDebugRendererLine(cc[1], cc[3]);
              lines[6] = xiiDebugRendererLine(cc[3], cc[2]);
              lines[7] = xiiDebugRendererLine(cc[2], cc[0]);

              lines[8]  = xiiDebugRendererLine(cc[4], cc[0]);
              lines[9]  = xiiDebugRendererLine(cc[5], cc[1]);
              lines[10] = xiiDebugRendererLine(cc[7], cc[3]);
              lines[11] = xiiDebugRendererLine(cc[6], cc[2]);

              xiiDebugRenderer::DrawLines(view.GetHandle(), lines, color);

              if (bOnlyOneSlice)
              {
                sb.SetFormat("L:{}\nD:{}\nR:{}", (xiiUInt32)lightCount, (xiiUInt32)decalCount, (xiiUInt32)probeCount);
                xiiVec3 textPos = (cc[0] + cc[1] + cc[2] + cc[3] + cc[4] + cc[5] + cc[6] + cc[7]) / 8.0f;
                xiiDebugRenderer::Draw3DText(view.GetHandle(), sb, textPos, color * 4.0f, 16u, xiiDebugTextHAlign::Center, xiiDebugTextVAlign::Center);
              }
            }
          }
        }
      }

      {
        xiiVec3 leftWidth    = pCamera->GetDirRight() * fZf * fTanLeft;
        xiiVec3 rightWidth   = pCamera->GetDirRight() * fZf * fTanRight;
        xiiVec3 bottomHeight = pCamera->GetDirUp() * fZf * fTanBottom;
        xiiVec3 topHeight    = pCamera->GetDirUp() * fZf * fTanTop;

        xiiVec3 depthFar = pCamera->GetPosition() + pCamera->GetDirForwards() * fZf;
        xiiVec3 p0       = depthFar + rightWidth + topHeight;
        xiiVec3 p1       = depthFar + rightWidth + bottomHeight;
        xiiVec3 p2       = depthFar + leftWidth + bottomHeight;
        xiiVec3 p3       = depthFar + leftWidth + topHeight;

        xiiDebugRendererLine lines[4];
        lines[0] = xiiDebugRendererLine(p0, p1);
        lines[1] = xiiDebugRendererLine(p1, p2);
        lines[2] = xiiDebugRendererLine(p2, p3);
        lines[3] = xiiDebugRendererLine(p3, p0);

        xiiDebugRenderer::DrawLines(view.GetHandle(), lines, lineColor);
      }
    }
  }
} // namespace
#endif

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClusteredDataCPU, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiClusteredDataCPU::xiiClusteredDataCPU()  = default;
xiiClusteredDataCPU::~xiiClusteredDataCPU() = default;

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClusteredDataExtractor, 1, xiiRTTIDefaultAllocator<xiiClusteredDataExtractor>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiClusteredDataExtractor::xiiClusteredDataExtractor(xiiStringView sName) :
  xiiExtractor(sName)
{
  m_DependsOn.PushBack(xiiMakeHashedString("xiiVisibleObjectsExtractor"));

  m_TempLightsClusters.SetCountUninitialized(NUM_CLUSTERS);
  m_TempDecalsClusters.SetCountUninitialized(NUM_CLUSTERS);
  m_TempReflectionProbeClusters.SetCountUninitialized(NUM_CLUSTERS);

  xiiMemoryUtils::ZeroFill(m_TempLightsClusters.GetData(), NUM_CLUSTERS);
  xiiMemoryUtils::ZeroFill(m_TempDecalsClusters.GetData(), NUM_CLUSTERS);
  xiiMemoryUtils::ZeroFill(m_TempReflectionProbeClusters.GetData(), NUM_CLUSTERS);

  m_ClusterBoundingSpheres.SetCountUninitialized(NUM_CLUSTERS);
}

xiiClusteredDataExtractor::~xiiClusteredDataExtractor() = default;

void xiiClusteredDataExtractor::PostSortAndBatch(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData)
{
  XII_PROFILE_SCOPE("PostSortAndBatch");

  const xiiCamera* pCamera      = view.GetCullingCamera();
  const float      fAspectRatio = view.GetViewport().width / view.GetViewport().height;

  xiiMat4 mProj;
  pCamera->GetProjectionMatrix(fAspectRatio, mProj);
  if (m_mProjection != mProj)
  {
    m_mProjection = mProj;

    FillClusterBoundingSpheres(*pCamera, mProj, m_ClusterBoundingSpheres);
  }

  xiiClusteredDataCPU* pData = XII_NEW(xiiFrameAllocator::GetCurrentAllocator(), xiiClusteredDataCPU);
  pData->m_ClusterData       = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiPerClusterData, NUM_CLUSTERS);

  xiiMat4      tmp        = pCamera->GetViewMatrix();
  xiiSimdMat4f viewMatrix = xiiSimdConversion::ToMat4(tmp);

  pCamera->GetProjectionMatrix(fAspectRatio, tmp);
  xiiSimdMat4f projectionMatrix = xiiSimdConversion::ToMat4(tmp);

  xiiSimdMat4f invViewMatrix        = viewMatrix.GetInverse();
  xiiSimdMat4f viewProjectionMatrix = projectionMatrix * viewMatrix;

  // Lights
  {
    XII_PROFILE_SCOPE("Lights");
    m_TempLightData.Clear();

    auto            batchList    = ref_extractedRenderData.GetRenderDataBatchesWithCategory(xiiDefaultRenderDataCategories::Light);
    const xiiUInt32 uiBatchCount = batchList.GetBatchCount();
    for (xiiUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const xiiRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<xiiRenderData>(); it.IsValid(); ++it)
      {
        const xiiUInt32 uiLightIndex = m_TempLightData.GetCount();

        if (uiLightIndex == xiiClusteredDataCPU::MAX_LIGHT_DATA)
        {
          xiiLog::Warning("Maximum number of lights reached ({0}). Further lights will be discarded.", xiiClusteredDataCPU::MAX_LIGHT_DATA);
          break;
        }

        if (auto pPointLightRenderData = xiiDynamicCast<const xiiPointLightRenderData*>(it))
        {
          FillPointLightData(m_TempLightData.ExpandAndGetRef(), pPointLightRenderData);

          xiiSimdBSphere pointLightSphere = xiiSimdBSphere(xiiSimdConversion::ToVec3(pPointLightRenderData->m_GlobalTransform.m_vPosition), pPointLightRenderData->m_fRange);
          RasterizeSphere(pointLightSphere, uiLightIndex, viewMatrix, projectionMatrix, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheres.GetData());

          if (false)
          {
            xiiSimdBSphere viewSpaceSphere(viewMatrix.TransformPosition(pointLightSphere.GetCenter()), pointLightSphere.GetRadius());
            xiiSimdBBox    ssb  = GetScreenSpaceBounds(viewSpaceSphere, projectionMatrix);
            float          minX = ((float)ssb.m_Min.x() * 0.5f + 0.5f) * view.GetViewport().width;
            float          maxX = ((float)ssb.m_Max.x() * 0.5f + 0.5f) * view.GetViewport().width;
            float          minY = ((float)ssb.m_Max.y() * -0.5f + 0.5f) * view.GetViewport().height;
            float          maxY = ((float)ssb.m_Min.y() * -0.5f + 0.5f) * view.GetViewport().height;

            xiiRectFloat rect(minX, minY, maxX - minX, maxY - minY);
            xiiDebugRenderer::Draw2DRectangle(view.GetHandle(), rect, 0.0f, xiiColor::Blue.WithAlpha(0.3f));
          }
        }
        else if (auto pSpotLightRenderData = xiiDynamicCast<const xiiSpotLightRenderData*>(it))
        {
          FillSpotLightData(m_TempLightData.ExpandAndGetRef(), pSpotLightRenderData);

          xiiAngle halfAngle = pSpotLightRenderData->m_OuterSpotAngle / 2.0f;

          BoundingCone cone;
          cone.m_PositionAndRange = xiiSimdConversion::ToVec3(pSpotLightRenderData->m_GlobalTransform.m_vPosition);
          cone.m_PositionAndRange.SetW(pSpotLightRenderData->m_fRange);
          cone.m_ForwardDir  = xiiSimdConversion::ToVec3(pSpotLightRenderData->m_GlobalTransform.m_qRotation * xiiVec3(1.0f, 0.0f, 0.0f));
          cone.m_SinCosAngle = xiiSimdVec4f(xiiMath::Sin(halfAngle), xiiMath::Cos(halfAngle), 0.0f);
          RasterizeSpotLight(cone, uiLightIndex, viewMatrix, projectionMatrix, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheres.GetData());
        }
        else if (auto pDirLightRenderData = xiiDynamicCast<const xiiDirectionalLightRenderData*>(it))
        {
          FillDirLightData(m_TempLightData.ExpandAndGetRef(), pDirLightRenderData);

          RasterizeDirLight(pDirLightRenderData, uiLightIndex, m_TempLightsClusters.GetArrayPtr());
        }
        else if (auto pFogRenderData = xiiDynamicCast<const xiiFogRenderData*>(it))
        {
          float fogBaseHeight    = pFogRenderData->m_GlobalTransform.m_vPosition.z;
          float fogHeightFalloff = pFogRenderData->m_fHeightFalloff > 0.0f ? xiiMath::Ln(0.0001f) / pFogRenderData->m_fHeightFalloff : 0.0f;

          float fogAtCameraPos = fogHeightFalloff * (pCamera->GetPosition().z - fogBaseHeight);
          if (fogAtCameraPos >= 80.0f) // Prevent infs
          {
            fogHeightFalloff = 0.0f;
          }

          pData->m_fFogHeight             = -fogHeightFalloff * fogBaseHeight;
          pData->m_fFogHeightFalloff      = fogHeightFalloff;
          pData->m_fFogDensityAtCameraPos = xiiMath::Exp(xiiMath::Clamp(fogAtCameraPos, -80.0f, 80.0f)); // Prevent infs
          pData->m_fFogDensity            = pFogRenderData->m_fDensity;
          pData->m_fFogInvSkyDistance     = pFogRenderData->m_fInvSkyDistance;

          pData->m_FogColor = pFogRenderData->m_Color;
        }
        else
        {
          xiiLog::Warning("Unhandled render data type '{}' in 'Light' category", it->GetDynamicRTTI()->GetTypeName());
        }
      }
    }

    pData->m_LightData = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiPerLightData, m_TempLightData.GetCount());
    pData->m_LightData.CopyFrom(m_TempLightData);

    pData->m_uiSkyIrradianceIndex = view.GetWorld()->GetIndex();
    pData->m_cameraUsageHint      = view.GetCameraUsageHint();
  }

  // Decals
  {
    XII_PROFILE_SCOPE("Decals");
    m_TempDecalData.Clear();

    auto            batchList    = ref_extractedRenderData.GetRenderDataBatchesWithCategory(xiiDefaultRenderDataCategories::Decal);
    const xiiUInt32 uiBatchCount = batchList.GetBatchCount();
    for (xiiUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const xiiRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<xiiRenderData>(); it.IsValid(); ++it)
      {
        const xiiUInt32 uiDecalIndex = m_TempDecalData.GetCount();

        if (uiDecalIndex == xiiClusteredDataCPU::MAX_DECAL_DATA)
        {
          xiiLog::Warning("Maximum number of decals reached ({0}). Further decals will be discarded.", xiiClusteredDataCPU::MAX_DECAL_DATA);
          break;
        }

        if (auto pDecalRenderData = xiiDynamicCast<const xiiDecalRenderData*>(it))
        {
          FillDecalData(m_TempDecalData.ExpandAndGetRef(), pDecalRenderData);

          RasterizeBox(pDecalRenderData->m_GlobalTransform, uiDecalIndex, invViewMatrix, viewProjectionMatrix, m_TempDecalsClusters.GetData(), m_ClusterBoundingSpheres.GetData());
        }
        else
        {
          xiiLog::Warning("Unhandled render data type '{}' in 'Decal' category", it->GetDynamicRTTI()->GetTypeName());
        }
      }
    }

    pData->m_DecalData = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiPerDecalData, m_TempDecalData.GetCount());
    pData->m_DecalData.CopyFrom(m_TempDecalData);
  }

  // Reflection Probes
  {
    XII_PROFILE_SCOPE("Probes");
    m_TempReflectionProbeData.Clear();

    auto            batchList    = ref_extractedRenderData.GetRenderDataBatchesWithCategory(xiiDefaultRenderDataCategories::ReflectionProbe);
    const xiiUInt32 uiBatchCount = batchList.GetBatchCount();
    for (xiiUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const xiiRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<xiiRenderData>(); it.IsValid(); ++it)
      {
        const xiiUInt32 uiProbeIndex = m_TempReflectionProbeData.GetCount();

        if (uiProbeIndex == xiiClusteredDataCPU::MAX_REFLECTION_PROBE_DATA)
        {
          xiiLog::Warning("Maximum number of reflection probes reached ({0}). Further reflection probes will be discarded.", xiiClusteredDataCPU::MAX_REFLECTION_PROBE_DATA);
          break;
        }

        if (auto pReflectionProbeRenderData = xiiDynamicCast<const xiiReflectionProbeRenderData*>(it))
        {
          auto& probeData = m_TempReflectionProbeData.ExpandAndGetRef();
          FillReflectionProbeData(probeData, pReflectionProbeRenderData);

          const xiiVec3 vFullScale = pReflectionProbeRenderData->m_vHalfExtents.CompMul(pReflectionProbeRenderData->m_GlobalTransform.m_vScale);

          bool  bRasterizeSphere = false;
          float fMaxRadius       = 0.0f;
          if (pReflectionProbeRenderData->m_uiIndex & REFLECTION_PROBE_IS_SPHERE)
          {
            constexpr float fSphereConstant = (4.0f / 3.0f) * xiiMath::Pi<float>();
            fMaxRadius                      = xiiMath::Max(xiiMath::Max(xiiMath::Abs(vFullScale.x), xiiMath::Abs(vFullScale.y)), xiiMath::Abs(vFullScale.z));
            const float fSphereVolume       = fSphereConstant * xiiMath::Pow(fMaxRadius, 3.0f);
            const float fBoxVolume          = xiiMath::Abs(vFullScale.x * vFullScale.y * vFullScale.z * 8);
            if (fSphereVolume < fBoxVolume)
            {
              bRasterizeSphere = true;
            }
          }

          if (bRasterizeSphere)
          {
            xiiSimdBSphere pointLightSphere = xiiSimdBSphere(xiiSimdConversion::ToVec3(pReflectionProbeRenderData->m_GlobalTransform.m_vPosition), fMaxRadius);
            RasterizeSphere(pointLightSphere, uiProbeIndex, viewMatrix, projectionMatrix, m_TempReflectionProbeClusters.GetData(), m_ClusterBoundingSpheres.GetData());
          }
          else
          {
            xiiTransform transform = pReflectionProbeRenderData->m_GlobalTransform;
            transform.m_vScale     = vFullScale.CompMul(probeData.InfluenceScale.GetAsVec3());
            transform.m_vPosition += transform.m_qRotation * vFullScale.CompMul(probeData.InfluenceShift.GetAsVec3());

            // const xiiBoundingBox aabb(xiiVec3(-1.0f), xiiVec3(1.0f));
            // xiiDebugRenderer::DrawLineBox(view.GetHandle(), aabb, xiiColor::DarkBlue, transform);

            RasterizeBox(transform, uiProbeIndex, invViewMatrix, viewProjectionMatrix, m_TempReflectionProbeClusters.GetData(), m_ClusterBoundingSpheres.GetData());
          }
        }
        else
        {
          xiiLog::Warning("Unhandled render data type '{}' in 'ReflectionProbe' category", it->GetDynamicRTTI()->GetTypeName());
        }
      }
    }

    pData->m_ReflectionProbeData = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiPerReflectionProbeData, m_TempReflectionProbeData.GetCount());
    pData->m_ReflectionProbeData.CopyFrom(m_TempReflectionProbeData);
  }

  FillItemListAndClusterData(pData);

  ref_extractedRenderData.AddFrameData(pData);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  VisualizeClusteredData(view, pData, m_ClusterBoundingSpheres);
#endif
}

xiiResult xiiClusteredDataExtractor::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return XII_SUCCESS;
}

xiiResult xiiClusteredDataExtractor::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  return XII_SUCCESS;
}

namespace
{
  XII_FORCE_INLINE xiiUInt32 MakeDecalIndex(xiiUInt32 uiDecalIndex)
  {
    return uiDecalIndex << DECAL_SHIFT;
  }

  XII_FORCE_INLINE xiiUInt32 MakeProbeIndex(xiiUInt32 uiReflectionProbeIndex)
  {
    return uiReflectionProbeIndex << PROBE_SHIFT;
  }
} // namespace

void xiiClusteredDataExtractor::FillItemListAndClusterData(xiiClusteredDataCPU* pData)
{
  XII_PROFILE_SCOPE("FillItemListAndClusterData");
  m_TempClusterItemList.Clear();

  const xiiUInt32 uiNumLights          = m_TempLightData.GetCount();
  const xiiUInt32 uiMaxLightBlockIndex = (uiNumLights + 31) / 32;

  const xiiUInt32 uiNumDecals          = m_TempDecalData.GetCount();
  const xiiUInt32 uiMaxDecalBlockIndex = (uiNumDecals + 31) / 32;

  const xiiUInt32 uiNumReflectionProbes          = m_TempReflectionProbeData.GetCount();
  const xiiUInt32 uiMaxReflectionProbeBlockIndex = (uiNumReflectionProbes + 31) / 32;

  const xiiUInt32 uiWorstCase = xiiMath::Max(uiNumLights, uiNumDecals, uiNumReflectionProbes);
  for (xiiUInt32 i = 0; i < NUM_CLUSTERS; ++i)
  {
    const xiiUInt32 uiOffset     = m_TempClusterItemList.GetCount();
    xiiUInt32       uiLightCount = 0;

    // We expand m_TempClusterItemList by the worst case this loop can produce and then cut it down again to the actual size once we have filled the data. This makes sure we do not waste time on boundary checks or potential out of line calls like PushBack or PushBackUnchecked.
    m_TempClusterItemList.SetCountUninitialized(uiOffset + uiWorstCase);
    xiiUInt32* pTempClusterItemListRange = m_TempClusterItemList.GetData() + uiOffset;

    // Lights
    {
      auto& tempCluster = m_TempLightsClusters[i];
      for (xiiUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxLightBlockIndex; ++uiBlockIndex)
      {
        xiiUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          xiiUInt32 uiLightIndex = xiiMath::FirstBitLow(mask);
          mask &= mask - 1;

          uiLightIndex += uiBlockIndex * 32;
          pTempClusterItemListRange[uiLightCount] = uiLightIndex;
          ++uiLightCount;
        }

        tempCluster.m_BitMask[uiBlockIndex] = 0;
      }
    }

    xiiUInt32 uiDecalCount = 0;

    // Decals
    {
      auto& tempCluster = m_TempDecalsClusters[i];
      for (xiiUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxDecalBlockIndex; ++uiBlockIndex)
      {
        xiiUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          xiiUInt32 uiDecalIndex = xiiMath::FirstBitLow(mask);
          mask &= mask - 1;

          uiDecalIndex += uiBlockIndex * 32;

          const xiiUInt32 item                    = pTempClusterItemListRange[uiDecalCount];
          pTempClusterItemListRange[uiDecalCount] = (uiDecalCount < uiLightCount ? item : 0) | MakeDecalIndex(uiDecalIndex);

          ++uiDecalCount;
        }

        tempCluster.m_BitMask[uiBlockIndex] = 0;
      }
    }

    xiiUInt32       uiReflectionProbeCount = 0;
    const xiiUInt32 uiMaxUsed              = xiiMath::Max(uiLightCount, uiDecalCount);
    // Reflection Probes
    {
      auto& tempCluster = m_TempReflectionProbeClusters[i];
      for (xiiUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxReflectionProbeBlockIndex; ++uiBlockIndex)
      {
        xiiUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          xiiUInt32 uiReflectionProbeIndex = xiiMath::FirstBitLow(mask);
          mask &= mask - 1;

          uiReflectionProbeIndex += uiBlockIndex * 32;

          const xiiUInt32 item                              = pTempClusterItemListRange[uiReflectionProbeCount];
          pTempClusterItemListRange[uiReflectionProbeCount] = (uiReflectionProbeCount < uiMaxUsed ? item : 0) | MakeProbeIndex(uiReflectionProbeIndex);

          ++uiReflectionProbeCount;
        }

        tempCluster.m_BitMask[uiBlockIndex] = 0;
      }
    }

    // Cut down the array to the actual number of elements we have written.
    const xiiUInt32 uiActualCase = xiiMath::Max(uiLightCount, uiDecalCount, uiReflectionProbeCount);
    m_TempClusterItemList.SetCountUninitialized(uiOffset + uiActualCase);

    auto& clusterData  = pData->m_ClusterData[i];
    clusterData.offset = uiOffset;
    clusterData.counts = uiLightCount | MakeDecalIndex(uiDecalCount) | MakeProbeIndex(uiReflectionProbeCount);
  }

  pData->m_ClusterItemList = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiUInt32, m_TempClusterItemList.GetCount());
  pData->m_ClusterItemList.CopyFrom(m_TempClusterItemList);
}
