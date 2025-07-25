#pragma once

#include <GraphicsCore/Decals/DecalComponent.h>
#include <GraphicsCore/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Lights/Implementation/ReflectionProbeData.h>
#include <GraphicsCore/Lights/PointLightComponent.h>
#include <GraphicsCore/Lights/SpotLightComponent.h>
#include <GraphicsFoundation/Shader/ShaderUtils.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Common/LightData.h>
XII_DEFINE_AS_POD_TYPE(xiiPerLightData);
XII_DEFINE_AS_POD_TYPE(xiiPerDecalData);
XII_DEFINE_AS_POD_TYPE(xiiPerReflectionProbeData);
XII_DEFINE_AS_POD_TYPE(xiiPerClusterData);

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Utilities/GraphicsUtils.h>

namespace
{
  ///\todo Make this configurable.
  static float s_fMinLightDistance = 5.0f;
  static float s_fMaxLightDistance = 500.0f;

  static float s_fDepthSliceScale = (NUM_CLUSTERS_Z - 1) / (xiiMath::Log2(s_fMaxLightDistance) - xiiMath::Log2(s_fMinLightDistance));
  static float s_fDepthSliceBias  = -s_fDepthSliceScale * xiiMath::Log2(s_fMinLightDistance) + 1.0f;

  XII_ALWAYS_INLINE float GetDepthFromSliceIndex(xiiUInt32 uiSliceIndex)
  {
    return xiiMath::Pow(2.0f, (uiSliceIndex - s_fDepthSliceBias + 1.0f) / s_fDepthSliceScale);
  }

  XII_ALWAYS_INLINE xiiUInt32 GetSliceIndexFromDepth(float fLinearDepth)
  {
    return xiiMath::Clamp((xiiInt32)(xiiMath::Log2(fLinearDepth) * s_fDepthSliceScale + s_fDepthSliceBias), 0, NUM_CLUSTERS_Z - 1);
  }

  XII_ALWAYS_INLINE xiiUInt32 GetClusterIndexFromCoord(xiiUInt32 x, xiiUInt32 y, xiiUInt32 z)
  {
    return z * NUM_CLUSTERS_XY + y * NUM_CLUSTERS_X + x;
  }

  // in order: tlf, trf, blf, brf, tln, trn, bln, brn
  XII_FORCE_INLINE void GetClusterCornerPoints(const xiiCamera& camera, float fZf, float fZn, float fTanLeft, float fTanRight, float fTanBottom, float fTanTop, xiiInt32 x, xiiInt32 y, xiiInt32 z, xiiVec3* out_pCorners)
  {
    const xiiVec3& pos        = camera.GetPosition();
    const xiiVec3& dirForward = camera.GetDirForwards();
    const xiiVec3& dirRight   = camera.GetDirRight();
    const xiiVec3& dirUp      = camera.GetDirUp();

    const float fStartXf = fZf * fTanLeft;
    const float fStartYf = fZf * fTanBottom;
    const float fEndXf   = fZf * fTanRight;
    const float fEndYf   = fZf * fTanTop;

    float fStepXf = (fEndXf - fStartXf) / NUM_CLUSTERS_X;
    float fStepYf = (fEndYf - fStartYf) / NUM_CLUSTERS_Y;

    float fXf = fStartXf + x * fStepXf;
    float fYf = fStartYf + y * fStepYf;

    out_pCorners[0] = pos + dirForward * fZf + dirRight * fXf - dirUp * fYf;
    out_pCorners[1] = out_pCorners[0] + dirRight * fStepXf;
    out_pCorners[2] = out_pCorners[0] - dirUp * fStepYf;
    out_pCorners[3] = out_pCorners[2] + dirRight * fStepXf;

    const float fStartXn = fZn * fTanLeft;
    const float fStartYn = fZn * fTanBottom;
    const float fEndXn   = fZn * fTanRight;
    const float fEndYn   = fZn * fTanTop;

    float fStepXn = (fEndXn - fStartXn) / NUM_CLUSTERS_X;
    float fStepYn = (fEndYn - fStartYn) / NUM_CLUSTERS_Y;
    float fXn     = fStartXn + x * fStepXn;
    float fYn     = fStartYn + y * fStepYn;

    out_pCorners[4] = pos + dirForward * fZn + dirRight * fXn - dirUp * fYn;
    out_pCorners[5] = out_pCorners[4] + dirRight * fStepXn;
    out_pCorners[6] = out_pCorners[4] - dirUp * fStepYn;
    out_pCorners[7] = out_pCorners[6] + dirRight * fStepXn;
  }

  void FillClusterBoundingSpheres(const xiiCamera& camera, const xiiMat4& mProj, xiiArrayPtr<xiiSimdBSphere> clusterBoundingSpheres)
  {
    XII_PROFILE_SCOPE("FillClusterBoundingSpheres");

    ///\todo proper implementation for orthographic views
    if (camera.IsOrthographic())
      return;

    xiiSimdVec4f stepScale;
    xiiSimdVec4f tanLBLB;
    {
      xiiAngle fFovLeft;
      xiiAngle fFovRight;
      xiiAngle fFovBottom;
      xiiAngle fFovTop;
      xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop);

      const float fTanLeft   = xiiMath::Tan(fFovLeft);
      const float fTanRight  = xiiMath::Tan(fFovRight);
      const float fTanBottom = xiiMath::Tan(fFovBottom);
      const float fTanTop    = xiiMath::Tan(fFovTop);

      float fStepXf = (fTanRight - fTanLeft) / NUM_CLUSTERS_X;
      float fStepYf = (fTanTop - fTanBottom) / NUM_CLUSTERS_Y;

      stepScale = xiiSimdVec4f(fStepXf, fStepYf, fStepXf, fStepYf);
      tanLBLB   = xiiSimdVec4f(fTanLeft, fTanBottom, fTanLeft, fTanBottom);
    }

    const xiiSimdVec4f dirForward = xiiSimdVec4f(0, 0, 1, 0);
    const xiiSimdVec4f dirRight   = xiiSimdVec4f(1, 0, 0, 0);
    const xiiSimdVec4f dirUp      = xiiSimdVec4f(0, 1, 0, 0);

    xiiSimdVec4f fZn = xiiSimdVec4f::MakeZero();
    xiiSimdVec4f cc[8];

    for (xiiInt32 z = 0; z < NUM_CLUSTERS_Z; z++)
    {
      xiiSimdVec4f fZf     = xiiSimdVec4f(GetDepthFromSliceIndex(z));
      xiiSimdVec4f zff_znn = fZf.GetCombined<xiiSwizzle::XXXX>(fZn);
      xiiSimdVec4f steps   = zff_znn.CompMul(stepScale);

      xiiSimdVec4f depthF = dirForward * fZf.x();
      xiiSimdVec4f depthN = dirForward * fZn.x();

      xiiSimdVec4f startLBLB = zff_znn.CompMul(tanLBLB);

      for (xiiInt32 y = 0; y < NUM_CLUSTERS_Y; y++)
      {
        for (xiiInt32 x = 0; x < NUM_CLUSTERS_X; x++)
        {
          xiiSimdVec4f xyxy = xiiSimdVec4i(x, y, x, y).ToFloat();
          xiiSimdVec4f xfyf = startLBLB + (xyxy).CompMul(steps);

          cc[0] = depthF + dirRight * xfyf.x() - dirUp * xfyf.y();
          cc[1] = cc[0] + dirRight * steps.x();
          cc[2] = cc[0] - dirUp * steps.y();
          cc[3] = cc[2] + dirRight * steps.x();

          cc[4] = depthN + dirRight * xfyf.z() - dirUp * xfyf.w();
          cc[5] = cc[4] + dirRight * steps.z();
          cc[6] = cc[4] - dirUp * steps.w();
          cc[7] = cc[6] + dirRight * steps.z();

          clusterBoundingSpheres[GetClusterIndexFromCoord(x, y, z)] = xiiSimdBSphere::MakeFromPoints(cc, 8);
        }
      }

      fZn = fZf;
    }
  }

  XII_ALWAYS_INLINE void FillLightData(xiiPerLightData& ref_perLightData, const xiiLightRenderData* pLightRenderData, xiiUInt8 uiType)
  {
    xiiMemoryUtils::ZeroFill(&ref_perLightData, 1);

    xiiColorLinearUB lightColor = pLightRenderData->m_LightColor;
    lightColor.a                = uiType;

    ref_perLightData.colorAndType     = *reinterpret_cast<xiiUInt32*>(&lightColor.r);
    ref_perLightData.intensity        = pLightRenderData->m_fIntensity;
    ref_perLightData.shadowDataOffset = pLightRenderData->m_uiShadowDataOffset;
  }

  void FillPointLightData(xiiPerLightData& ref_perLightData, const xiiPointLightRenderData* pPointLightRenderData)
  {
    FillLightData(ref_perLightData, pPointLightRenderData, LIGHT_TYPE_POINT);

    ref_perLightData.position        = pPointLightRenderData->m_GlobalTransform.m_vPosition;
    ref_perLightData.invSqrAttRadius = 1.0f / (pPointLightRenderData->m_fRange * pPointLightRenderData->m_fRange);
  }

  void FillSpotLightData(xiiPerLightData& ref_perLightData, const xiiSpotLightRenderData* pSpotLightRenderData)
  {
    FillLightData(ref_perLightData, pSpotLightRenderData, LIGHT_TYPE_SPOT);

    ref_perLightData.direction       = xiiGALShaderUtilities::Float3ToRGB10(pSpotLightRenderData->m_GlobalTransform.m_qRotation * xiiVec3(-1, 0, 0));
    ref_perLightData.position        = pSpotLightRenderData->m_GlobalTransform.m_vPosition;
    ref_perLightData.invSqrAttRadius = 1.0f / (pSpotLightRenderData->m_fRange * pSpotLightRenderData->m_fRange);

    const float fCosInner        = xiiMath::Cos(pSpotLightRenderData->m_InnerSpotAngle * 0.5f);
    const float fCosOuter        = xiiMath::Cos(pSpotLightRenderData->m_OuterSpotAngle * 0.5f);
    const float fSpotParamScale  = 1.0f / xiiMath::Max(0.001f, (fCosInner - fCosOuter));
    const float fSpotParamOffset = -fCosOuter * fSpotParamScale;
    ref_perLightData.spotParams  = xiiGALShaderUtilities::Float2ToRG16F(xiiVec2(fSpotParamScale, fSpotParamOffset));
  }

  void FillDirLightData(xiiPerLightData& ref_perLightData, const xiiDirectionalLightRenderData* pDirLightRenderData)
  {
    FillLightData(ref_perLightData, pDirLightRenderData, LIGHT_TYPE_DIR);

    ref_perLightData.direction = xiiGALShaderUtilities::Float3ToRGB10(pDirLightRenderData->m_GlobalTransform.m_qRotation * xiiVec3(-1, 0, 0));
  }

  void FillDecalData(xiiPerDecalData& ref_perDecalData, const xiiDecalRenderData* pDecalRenderData)
  {
    xiiVec3 position    = pDecalRenderData->m_GlobalTransform.m_vPosition;
    xiiVec3 dirForwards = pDecalRenderData->m_GlobalTransform.m_qRotation * xiiVec3(1.0f, 0.0, 0.0f);
    xiiVec3 dirUp       = pDecalRenderData->m_GlobalTransform.m_qRotation * xiiVec3(0.0f, 0.0, 1.0f);
    xiiVec3 scale       = pDecalRenderData->m_GlobalTransform.m_vScale;

    // the CompMax prevents division by zero (thus inf, thus NaN later, then crash)
    // if negative scaling should be allowed, this would need to be changed
    scale = xiiVec3(1.0f).CompDiv(scale.CompMax(xiiVec3(0.00001f)));

    const xiiMat4 lookAt   = xiiGraphicsUtils::CreateLookAtViewMatrix(position, position + dirForwards, dirUp);
    xiiMat4       scaleMat = xiiMat4::MakeScaling(xiiVec3(scale.y, -scale.z, scale.x));

    ref_perDecalData.worldToDecalMatrix   = scaleMat * lookAt;
    ref_perDecalData.applyOnlyToId        = pDecalRenderData->m_uiApplyOnlyToId;
    ref_perDecalData.decalFlags           = pDecalRenderData->m_uiFlags;
    ref_perDecalData.angleFadeParams      = pDecalRenderData->m_uiAngleFadeParams;
    ref_perDecalData.baseColor            = *reinterpret_cast<const xiiUInt32*>(&pDecalRenderData->m_BaseColor.r);
    ref_perDecalData.emissiveColorRG      = xiiGALShaderUtilities::PackFloat16intoUint(pDecalRenderData->m_EmissiveColor.r, pDecalRenderData->m_EmissiveColor.g);
    ref_perDecalData.emissiveColorBA      = xiiGALShaderUtilities::PackFloat16intoUint(pDecalRenderData->m_EmissiveColor.b, pDecalRenderData->m_EmissiveColor.a);
    ref_perDecalData.baseColorAtlasScale  = pDecalRenderData->m_uiBaseColorAtlasScale;
    ref_perDecalData.baseColorAtlasOffset = pDecalRenderData->m_uiBaseColorAtlasOffset;
    ref_perDecalData.normalAtlasScale     = pDecalRenderData->m_uiNormalAtlasScale;
    ref_perDecalData.normalAtlasOffset    = pDecalRenderData->m_uiNormalAtlasOffset;
    ref_perDecalData.ormAtlasScale        = pDecalRenderData->m_uiORMAtlasScale;
    ref_perDecalData.ormAtlasOffset       = pDecalRenderData->m_uiORMAtlasOffset;
  }

  void FillReflectionProbeData(xiiPerReflectionProbeData& ref_perReflectionProbeData, const xiiReflectionProbeRenderData* pReflectionProbeRenderData)
  {
    xiiVec3 position = pReflectionProbeRenderData->m_GlobalTransform.m_vPosition;
    xiiVec3 scale    = pReflectionProbeRenderData->m_GlobalTransform.m_vScale.CompMul(pReflectionProbeRenderData->m_vHalfExtents);

    // We store scale separately so we easily transform into probe projection space (with scale), influence space (scale + offset) and cube map space (no scale).
    auto trans     = pReflectionProbeRenderData->m_GlobalTransform;
    trans.m_vScale = xiiVec3(1.0f, 1.0f, 1.0f);
    auto inverse   = trans.GetAsMat4().GetInverse();

    // the CompMax prevents division by zero (thus inf, thus NaN later, then crash)
    // if negative scaling should be allowed, this would need to be changed
    scale                                                   = xiiVec3(1.0f).CompDiv(scale.CompMax(xiiVec3(0.00001f)));
    ref_perReflectionProbeData.WorldToProbeProjectionMatrix = inverse;

    ref_perReflectionProbeData.ProbePosition = pReflectionProbeRenderData->m_vProbePosition.GetAsVec4(1.0f); // W isn't used.
    ref_perReflectionProbeData.Scale         = scale.GetAsVec4(0.0f);                                        // W isn't used.

    ref_perReflectionProbeData.InfluenceScale = pReflectionProbeRenderData->m_vInfluenceScale.GetAsVec4(0.0f);
    ref_perReflectionProbeData.InfluenceShift = pReflectionProbeRenderData->m_vInfluenceShift.CompMul(xiiVec3(1.0f) - pReflectionProbeRenderData->m_vInfluenceScale).GetAsVec4(0.0f);

    ref_perReflectionProbeData.PositiveFalloff = pReflectionProbeRenderData->m_vPositiveFalloff.GetAsVec4(0.0f);
    ref_perReflectionProbeData.NegativeFalloff = pReflectionProbeRenderData->m_vNegativeFalloff.GetAsVec4(0.0f);
    ref_perReflectionProbeData.Index           = pReflectionProbeRenderData->m_uiIndex;
  }


  XII_FORCE_INLINE xiiSimdBBox GetScreenSpaceBounds(const xiiSimdBSphere& viewSpaceSphere, const xiiSimdMat4f& mProjectionMatrix)
  {
    xiiSimdVec4f viewSpaceCenter = viewSpaceSphere.GetCenter();
    xiiSimdFloat depth           = viewSpaceCenter.z();
    xiiSimdFloat radius          = viewSpaceSphere.GetRadius();

    xiiSimdVec4f mi;
    xiiSimdVec4f ma;

    if (viewSpaceCenter.GetLength<3>() > radius && depth > radius)
    {
      xiiSimdVec4f one       = xiiSimdVec4f(1.0f);
      xiiSimdVec4f oneNegOne = xiiSimdVec4f(1.0f, -1.0f, 1.0f, -1.0f);

      xiiSimdVec4f pRadius  = xiiSimdVec4f(radius / depth);
      xiiSimdVec4f pRadius2 = pRadius.CompMul(pRadius);

      xiiSimdVec4f xy    = viewSpaceCenter / depth;
      xiiSimdVec4f xxyy  = xy.Get<xiiSwizzle::XXYY>();
      xiiSimdVec4f nom   = (pRadius2.CompMul(xxyy.CompMul(xxyy) - pRadius2 + one)).GetSqrt() - xxyy.CompMul(oneNegOne);
      xiiSimdVec4f denom = pRadius2 - one;

      xiiSimdVec4f projection        = mProjectionMatrix.m_col0.GetCombined<xiiSwizzle::XXYY>(mProjectionMatrix.m_col1);
      xiiSimdVec4f minXmaxX_minYmaxY = nom.CompDiv(denom).CompMul(oneNegOne).CompMul(projection);

      mi = minXmaxX_minYmaxY.Get<xiiSwizzle::XZXX>();
      ma = minXmaxX_minYmaxY.Get<xiiSwizzle::YWYY>();
    }
    else
    {
      mi = xiiSimdVec4f(-1.0f);
      ma = xiiSimdVec4f(1.0f);
    }

    mi.SetZ(depth - radius);
    ma.SetZ(depth + radius);

    return xiiSimdBBox(mi, ma);
  }

  template <typename Cluster, typename IntersectionFunc>
  XII_FORCE_INLINE void FillCluster(const xiiSimdBBox& screenSpaceBounds, xiiUInt32 uiBlockIndex, xiiUInt32 uiMask, Cluster* pClusters, IntersectionFunc func)
  {
    xiiSimdVec4f scale = xiiSimdVec4f(0.5f * NUM_CLUSTERS_X, -0.5f * NUM_CLUSTERS_Y, 1.0f, 1.0f);
    xiiSimdVec4f bias  = xiiSimdVec4f(0.5f * NUM_CLUSTERS_X, 0.5f * NUM_CLUSTERS_Y, 0.0f, 0.0f);

    xiiSimdVec4f mi = xiiSimdVec4f::MulAdd(screenSpaceBounds.m_Min, scale, bias);
    xiiSimdVec4f ma = xiiSimdVec4f::MulAdd(screenSpaceBounds.m_Max, scale, bias);

    xiiSimdVec4i minXY_maxXY = xiiSimdVec4i::Truncate(mi.GetCombined<xiiSwizzle::XYXY>(ma));

    xiiSimdVec4i maxClusterIndex = xiiSimdVec4i(NUM_CLUSTERS_X, NUM_CLUSTERS_Y, NUM_CLUSTERS_X, NUM_CLUSTERS_Y);
    minXY_maxXY                  = minXY_maxXY.CompMin(maxClusterIndex - xiiSimdVec4i(1));
    minXY_maxXY                  = minXY_maxXY.CompMax(xiiSimdVec4i::MakeZero());

    xiiUInt32 xMin = minXY_maxXY.x();
    xiiUInt32 yMin = minXY_maxXY.w();

    xiiUInt32 xMax = minXY_maxXY.z();
    xiiUInt32 yMax = minXY_maxXY.y();

    xiiUInt32 zMin = GetSliceIndexFromDepth(screenSpaceBounds.m_Min.z());
    xiiUInt32 zMax = GetSliceIndexFromDepth(screenSpaceBounds.m_Max.z());

    for (xiiUInt32 z = zMin; z <= zMax; ++z)
    {
      for (xiiUInt32 y = yMin; y <= yMax; ++y)
      {
        for (xiiUInt32 x = xMin; x <= xMax; ++x)
        {
          xiiUInt32 uiClusterIndex = GetClusterIndexFromCoord(x, y, z);
          if (func(uiClusterIndex))
          {
            pClusters[uiClusterIndex].m_BitMask[uiBlockIndex] |= uiMask;
          }
        }
      }
    }
  }

  template <typename Cluster>
  void RasterizeSphere(const xiiSimdBSphere& pointLightSphere, xiiUInt32 uiLightIndex, const xiiSimdMat4f& mViewMatrix, const xiiSimdMat4f& mProjectionMatrix, Cluster* pClusters, xiiSimdBSphere* pClusterBoundingSpheres)
  {
    xiiSimdBSphere viewSpaceSphere(mViewMatrix.TransformPosition(pointLightSphere.GetCenter()), pointLightSphere.GetRadius());

    xiiSimdBBox screenSpaceBounds = GetScreenSpaceBounds(viewSpaceSphere, mProjectionMatrix);

    const xiiUInt32 uiBlockIndex = uiLightIndex / 32;
    const xiiUInt32 uiMask       = 1 << (uiLightIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters, [&](xiiUInt32 uiClusterIndex) { return viewSpaceSphere.Overlaps(pClusterBoundingSpheres[uiClusterIndex]); });
  }

  struct BoundingCone
  {
    xiiSimdBSphere m_BoundingSphere;
    xiiSimdVec4f   m_PositionAndRange;
    xiiSimdVec4f   m_ForwardDir;
    xiiSimdVec4f   m_SinCosAngle;
  };

  template <typename Cluster>
  void RasterizeSpotLight(const BoundingCone& spotLightCone, xiiUInt32 uiLightIndex, const xiiSimdMat4f& mViewMatrix, const xiiSimdMat4f& mProjectionMatrix, Cluster* pClusters, xiiSimdBSphere* pClusterBoundingSpheres)
  {
    xiiSimdVec4f position   = mViewMatrix.TransformPosition(spotLightCone.m_PositionAndRange);
    xiiSimdFloat range      = spotLightCone.m_PositionAndRange.w();
    xiiSimdVec4f forwardDir = mViewMatrix.TransformDirection(spotLightCone.m_ForwardDir);
    xiiSimdFloat sinAngle   = spotLightCone.m_SinCosAngle.x();
    xiiSimdFloat cosAngle   = spotLightCone.m_SinCosAngle.y();

    // First calculate a bounding sphere around the cone to get min and max bounds
    xiiSimdVec4f bSphereCenter;
    xiiSimdFloat bSphereRadius;
    if (sinAngle > 0.707107f) // sin(45)
    {
      bSphereCenter = position + forwardDir * cosAngle * range;
      bSphereRadius = sinAngle * range;
    }
    else
    {
      bSphereRadius = range / (cosAngle + cosAngle);
      bSphereCenter = position + forwardDir * bSphereRadius;
    }

    xiiSimdBSphere spotLightSphere(bSphereCenter, bSphereRadius);
    xiiSimdBBox    screenSpaceBounds = GetScreenSpaceBounds(spotLightSphere, mProjectionMatrix);

    const xiiUInt32 uiBlockIndex = uiLightIndex / 32;
    const xiiUInt32 uiMask       = 1 << (uiLightIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters, [&](xiiUInt32 uiClusterIndex) {
      xiiSimdBSphere clusterSphere = pClusterBoundingSpheres[uiClusterIndex];
      xiiSimdFloat   clusterRadius = clusterSphere.GetRadius();

      xiiSimdVec4f toConePos    = clusterSphere.m_CenterAndRadius - position;
      xiiSimdFloat projected    = forwardDir.Dot<3>(toConePos);
      xiiSimdFloat distToConeSq = toConePos.Dot<3>(toConePos);
      xiiSimdFloat distClosestP = cosAngle * (distToConeSq - projected * projected).GetSqrt() - projected * sinAngle;

      bool angleCull = distClosestP > clusterRadius;
      bool frontCull = projected > clusterRadius + range;
      bool backCull  = projected < -clusterRadius;

      return !(angleCull || frontCull || backCull);
    });
  }

  template <typename Cluster>
  void RasterizeDirLight(const xiiDirectionalLightRenderData* pDirLightRenderData, xiiUInt32 uiLightIndex, xiiArrayPtr<Cluster> clusters)
  {
    const xiiUInt32 uiBlockIndex = uiLightIndex / 32;
    const xiiUInt32 uiMask       = 1 << (uiLightIndex - uiBlockIndex * 32);

    for (xiiUInt32 i = 0; i < clusters.GetCount(); ++i)
    {
      clusters[i].m_BitMask[uiBlockIndex] |= uiMask;
    }
  }

  template <typename Cluster>
  void RasterizeBox(const xiiTransform& transform, xiiUInt32 uiDecalIndex, const xiiSimdMat4f& mInvView, const xiiSimdMat4f& mViewProjection, Cluster* pClusters, xiiSimdBSphere* pClusterBoundingSpheres)
  {
    xiiSimdMat4f boxToWorld = xiiSimdConversion::ToTransform(transform).GetAsMat4();
    xiiSimdMat4f viewToBox  = boxToWorld.GetInverse() * mInvView;

    xiiVec3 corners[8];
    xiiBoundingBox(xiiVec3(-1), xiiVec3(1)).GetCorners(corners);

    xiiSimdMat4f decalToScreen     = mViewProjection * boxToWorld;
    bool         bInsideBox        = false;
    xiiSimdBBox  screenSpaceBounds = xiiSimdBBox::MakeInvalid();

    for (xiiUInt32 i = 0; i < 8; ++i)
    {
      xiiSimdVec4f corner            = xiiSimdConversion::ToVec3(corners[i]);
      xiiSimdVec4f screenSpaceCorner = decalToScreen.TransformPosition(corner);
      xiiSimdFloat depth             = screenSpaceCorner.w();
      bInsideBox |= depth < xiiSimdFloat::MakeZero();

      screenSpaceCorner /= depth;
      screenSpaceCorner = screenSpaceCorner.GetCombined<xiiSwizzle::XYZW>(xiiSimdVec4f(depth));

      screenSpaceBounds.m_Min = screenSpaceBounds.m_Min.CompMin(screenSpaceCorner);
      screenSpaceBounds.m_Max = screenSpaceBounds.m_Max.CompMax(screenSpaceCorner);
    }

    if (bInsideBox)
    {
      screenSpaceBounds.m_Min = xiiSimdVec4f(-1.0f).GetCombined<xiiSwizzle::XYZW>(screenSpaceBounds.m_Min);
      screenSpaceBounds.m_Max = xiiSimdVec4f(1.0f).GetCombined<xiiSwizzle::XYZW>(screenSpaceBounds.m_Max);
    }

    xiiSimdVec4f decalHalfExtents = xiiSimdVec4f(1.0f);
    xiiSimdBBox  localDecalBounds = xiiSimdBBox(-decalHalfExtents, decalHalfExtents);

    const xiiUInt32 uiBlockIndex = uiDecalIndex / 32;
    const xiiUInt32 uiMask       = 1 << (uiDecalIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters, [&](xiiUInt32 uiClusterIndex) {
      xiiSimdBSphere clusterSphere = pClusterBoundingSpheres[uiClusterIndex];
      clusterSphere.Transform(viewToBox);

      return localDecalBounds.Overlaps(clusterSphere);
    });
  }
} // namespace
