#include <Foundation/FoundationPCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>

xiiResult xiiGraphicsUtils::ConvertWorldPosToScreenPos(const xiiMat4& mModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3& vPoint, xiiVec3& out_vScreenPos, xiiClipSpaceDepthRange::Enum depthRange)
{
  const xiiVec4 vToProject = vPoint.GetAsVec4(1.0f);

  xiiVec4 vClipSpace = mModelViewProjection * vToProject;

  if (vClipSpace.w == 0.0f)
    return XII_FAILURE;

  xiiVec3 vProjected = vClipSpace.GetAsVec3() / vClipSpace.w;
  if (vClipSpace.w < 0.0f)
    vProjected.z = -vProjected.z;

  out_vScreenPos.x = uiViewportX + uiViewportWidth * ((vProjected.x * 0.5f) + 0.5f);
  out_vScreenPos.y = uiViewportY + uiViewportHeight * ((vProjected.y * 0.5f) + 0.5f);

  // normalize the output z value to always be in [0; 1] range
  // That means when the projection matrix spits out values between -1 and +1, rescale those values
  if (depthRange == xiiClipSpaceDepthRange::MinusOneToOne)
    out_vScreenPos.z = vProjected.z * 0.5f + 0.5f;
  else
    out_vScreenPos.z = vProjected.z;

  return XII_SUCCESS;
}

xiiResult xiiGraphicsUtils::ConvertWorldPosToScreenPos(const xiiMat4d& mModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3d& vPoint, xiiVec3d& out_vScreenPos, xiiClipSpaceDepthRange::Enum depthRange)
{
  const xiiVec4d vToProject = vPoint.GetAsVec4(1.0);

  xiiVec4d vClipSpace = mModelViewProjection * vToProject;

  if (vClipSpace.w == 0.0)
    return XII_FAILURE;

  xiiVec3d vProjected = vClipSpace.GetAsVec3() / vClipSpace.w;
  if (vClipSpace.w < 0.0)
    vProjected.z = -vProjected.z;

  out_vScreenPos.x = uiViewportX + uiViewportWidth * ((vProjected.x * 0.5) + 0.5);
  out_vScreenPos.y = uiViewportY + uiViewportHeight * ((vProjected.y * 0.5) + 0.5);

  // normalize the output z value to always be in [0; 1] range
  // That means when the projection matrix spits out values between -1 and +1, rescale those values
  if (depthRange == xiiClipSpaceDepthRange::MinusOneToOne)
    out_vScreenPos.z = vProjected.z * 0.5 + 0.5;
  else
    out_vScreenPos.z = vProjected.z;

  return XII_SUCCESS;
}

xiiResult xiiGraphicsUtils::ConvertScreenPosToWorldPos(const xiiMat4& mInverseModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3& vScreenPos, xiiVec3& out_vPoint, xiiVec3* out_pDirection, xiiClipSpaceDepthRange::Enum depthRange)
{
  xiiVec3 vClipSpace = vScreenPos;

  // From window coordinates to [0; 1] range
  vClipSpace.x = (vClipSpace.x - uiViewportX) / uiViewportWidth;
  vClipSpace.y = (vClipSpace.y - uiViewportY) / uiViewportHeight;

  // Map to range [-1; 1]
  vClipSpace.x = vClipSpace.x * 2.0f - 1.0f;
  vClipSpace.y = vClipSpace.y * 2.0f - 1.0f;

  // The OpenGL matrix expects the z values to be between -1 and +1, so rescale the incoming value to that range
  if (depthRange == xiiClipSpaceDepthRange::MinusOneToOne)
    vClipSpace.z = vClipSpace.z * 2.0f - 1.0f;

  xiiVec4 vToUnProject = vClipSpace.GetAsVec4(1.0f);

  xiiVec4 vWorldSpacePoint = mInverseModelViewProjection * vToUnProject;

  if (vWorldSpacePoint.w == 0.0f)
    return XII_FAILURE;

  out_vPoint = vWorldSpacePoint.GetAsVec3() / vWorldSpacePoint.w;

  if (out_pDirection != nullptr)
  {
    vToUnProject.z += 0.1f; // a point that is a bit further away

    const xiiVec4 vWorldSpacePoint2 = mInverseModelViewProjection * vToUnProject;

    XII_ASSERT_DEV(vWorldSpacePoint2.w != 0.0f, "It should not be possible that the first projected point has a w other than zero, but the second one has!");

    const xiiVec3 vPoint2 = vWorldSpacePoint2.GetAsVec3() / vWorldSpacePoint2.w;

    *out_pDirection = (vPoint2 - out_vPoint).GetNormalized();
  }

  return XII_SUCCESS;
}

xiiResult xiiGraphicsUtils::ConvertScreenPosToWorldPos(const xiiMat4d& mInverseModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3d& vScreenPos, xiiVec3d& out_vPoint, xiiVec3d* out_pDirection, xiiClipSpaceDepthRange::Enum depthRange)
{
  xiiVec3d vClipSpace = vScreenPos;

  // From window coordinates to [0; 1] range
  vClipSpace.x = (vClipSpace.x - uiViewportX) / uiViewportWidth;
  vClipSpace.y = (vClipSpace.y - uiViewportY) / uiViewportHeight;

  // Map to range [-1; 1]
  vClipSpace.x = vClipSpace.x * 2.0 - 1.0;
  vClipSpace.y = vClipSpace.y * 2.0 - 1.0;

  // The OpenGL matrix expects the z values to be between -1 and +1, so rescale the incoming value to that range
  if (depthRange == xiiClipSpaceDepthRange::MinusOneToOne)
    vClipSpace.z = vClipSpace.z * 2.0 - 1.0;

  xiiVec4d vToUnProject = vClipSpace.GetAsVec4(1.0);

  xiiVec4d vWorldSpacePoint = mInverseModelViewProjection * vToUnProject;

  if (vWorldSpacePoint.w == 0.0)
    return XII_FAILURE;

  out_vPoint = vWorldSpacePoint.GetAsVec3() / vWorldSpacePoint.w;

  if (out_pDirection != nullptr)
  {
    vToUnProject.z += 0.1; // a point that is a bit further away

    const xiiVec4d vWorldSpacePoint2 = mInverseModelViewProjection * vToUnProject;

    XII_ASSERT_DEV(vWorldSpacePoint2.w != 0.0, "It should not be possible that the first projected point has a w other than zero, but the second one has!");

    const xiiVec3d vPoint2 = vWorldSpacePoint2.GetAsVec3() / vWorldSpacePoint2.w;

    *out_pDirection = (vPoint2 - out_vPoint).GetNormalized();
  }

  return XII_SUCCESS;
}

bool xiiGraphicsUtils::IsTriangleFlipRequired(const xiiMat3& mTransformation)
{
  return (mTransformation.GetColumn(0).CrossRH(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);
}

bool xiiGraphicsUtils::IsTriangleFlipRequired(const xiiMat3d& mTransformation)
{
  return (mTransformation.GetColumn(0).CrossRH(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0);
}

void xiiGraphicsUtils::ConvertProjectionMatrixDepthRange(xiiMat4& inout_mMatrix, xiiClipSpaceDepthRange::Enum srcDepthRange, xiiClipSpaceDepthRange::Enum dstDepthRange)
{
  // exclude identity transformations
  if (srcDepthRange == dstDepthRange)
    return;

  xiiVec4 row2 = inout_mMatrix.GetRow(2);
  xiiVec4 row3 = inout_mMatrix.GetRow(3);

  // only need to check SrcDepthRange, the rest is the logical conclusion from being not equal
  if (srcDepthRange == xiiClipSpaceDepthRange::MinusOneToOne /*&& DstDepthRange == xiiClipSpaceDepthRange::ZeroToOne*/)
  {
    // map z => (z + w)/2
    row2 += row3;
    row2 *= 0.5f;
  }
  else // if (SrcDepthRange == xiiClipSpaceDepthRange::ZeroToOne && DstDepthRange == xiiClipSpaceDepthRange::MinusOneToOne)
  {
    // map z => 2z - w
    row2 += row2;
    row2 -= row3;
  }


  inout_mMatrix.SetRow(2, row2);
  inout_mMatrix.SetRow(3, row3);
}

void xiiGraphicsUtils::ConvertProjectionMatrixDepthRange(xiiMat4d& inout_mMatrix, xiiClipSpaceDepthRange::Enum srcDepthRange, xiiClipSpaceDepthRange::Enum dstDepthRange)
{
  // exclude identity transformations
  if (srcDepthRange == dstDepthRange)
    return;

  xiiVec4d row2 = inout_mMatrix.GetRow(2);
  xiiVec4d row3 = inout_mMatrix.GetRow(3);

  // only need to check SrcDepthRange, the rest is the logical conclusion from being not equal
  if (srcDepthRange == xiiClipSpaceDepthRange::MinusOneToOne /*&& DstDepthRange == xiiClipSpaceDepthRange::ZeroToOne*/)
  {
    // map z => (z + w)/2
    row2 += row3;
    row2 *= 0.5f;
  }
  else // if (SrcDepthRange == xiiClipSpaceDepthRange::ZeroToOne && DstDepthRange == xiiClipSpaceDepthRange::MinusOneToOne)
  {
    // map z => 2z - w
    row2 += row2;
    row2 -= row3;
  }


  inout_mMatrix.SetRow(2, row2);
  inout_mMatrix.SetRow(3, row3);
}

void xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const xiiMat4& mProjectionMatrix, xiiAngle& out_fovX, xiiAngle& out_fovY)
{

  const xiiVec3 row0 = mProjectionMatrix.GetRow(0).GetAsVec3();
  const xiiVec3 row1 = mProjectionMatrix.GetRow(1).GetAsVec3();
  const xiiVec3 row3 = mProjectionMatrix.GetRow(3).GetAsVec3();

  const xiiVec3 leftPlane   = (row3 + row0).GetNormalized();
  const xiiVec3 rightPlane  = (row3 - row0).GetNormalized();
  const xiiVec3 bottomPlane = (row3 + row1).GetNormalized();
  const xiiVec3 topPlane    = (row3 - row1).GetNormalized();

  out_fovX = xiiAngle::Radian(xiiMath::Pi<float>()) - xiiMath::ACos(leftPlane.Dot(rightPlane));
  out_fovY = xiiAngle::Radian(xiiMath::Pi<float>()) - xiiMath::ACos(topPlane.Dot(bottomPlane));
}

void xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const xiiMat4d& mProjectionMatrix, xiiAngle& out_fovX, xiiAngle& out_fovY)
{

  const xiiVec3d row0 = mProjectionMatrix.GetRow(0).GetAsVec3();
  const xiiVec3d row1 = mProjectionMatrix.GetRow(1).GetAsVec3();
  const xiiVec3d row3 = mProjectionMatrix.GetRow(3).GetAsVec3();

  const xiiVec3d leftPlane   = (row3 + row0).GetNormalized();
  const xiiVec3d rightPlane  = (row3 - row0).GetNormalized();
  const xiiVec3d bottomPlane = (row3 + row1).GetNormalized();
  const xiiVec3d topPlane    = (row3 - row1).GetNormalized();

  xiiAngled fovX = xiiAngled::Radian(xiiMath::Pi<double>()) - xiiMath::ACos<double>(leftPlane.Dot(rightPlane));
  xiiAngled fovY = xiiAngled::Radian(xiiMath::Pi<double>()) - xiiMath::ACos<double>(topPlane.Dot(bottomPlane));

  out_fovX = xiiAngle::Radian((float)fovX.GetRadian());
  out_fovY = xiiAngle::Radian((float)fovY.GetRadian());
}

void xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const xiiMat4& mProjectionMatrix, xiiAngle& out_fovLeft, xiiAngle& out_fovRight, xiiAngle& out_fovBottom, xiiAngle& out_fovTop, xiiClipSpaceYMode::Enum range)
{
  const xiiVec3 row0 = mProjectionMatrix.GetRow(0).GetAsVec3();
  const xiiVec3 row1 = mProjectionMatrix.GetRow(1).GetAsVec3();
  const xiiVec3 row3 = mProjectionMatrix.GetRow(3).GetAsVec3();

  const xiiVec3 leftPlane   = (row3 + row0).GetNormalized();
  const xiiVec3 rightPlane  = (row3 - row0).GetNormalized();
  const xiiVec3 bottomPlane = (row3 + row1).GetNormalized();
  const xiiVec3 topPlane    = (row3 - row1).GetNormalized();

  out_fovLeft   = -xiiMath::ACos(leftPlane.Dot(xiiVec3(1.0f, 0, 0)));
  out_fovRight  = xiiAngle::Radian(xiiMath::Pi<float>()) - xiiMath::ACos(rightPlane.Dot(xiiVec3(1.0f, 0, 0)));
  out_fovBottom = -xiiMath::ACos(bottomPlane.Dot(xiiVec3(0, 1.0f, 0)));
  out_fovTop    = xiiAngle::Radian(xiiMath::Pi<float>()) - xiiMath::ACos(topPlane.Dot(xiiVec3(0, 1.0f, 0)));

  if (range == xiiClipSpaceYMode::Flipped)
    xiiMath::Swap(out_fovBottom, out_fovTop);
}

void xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const xiiMat4d& mProjectionMatrix, xiiAngle& out_fovLeft, xiiAngle& out_fovRight, xiiAngle& out_fovBottom, xiiAngle& out_fovTop, xiiClipSpaceYMode::Enum range)
{
  const xiiVec3d row0 = mProjectionMatrix.GetRow(0).GetAsVec3();
  const xiiVec3d row1 = mProjectionMatrix.GetRow(1).GetAsVec3();
  const xiiVec3d row3 = mProjectionMatrix.GetRow(3).GetAsVec3();

  const xiiVec3d leftPlane   = (row3 + row0).GetNormalized();
  const xiiVec3d rightPlane  = (row3 - row0).GetNormalized();
  const xiiVec3d bottomPlane = (row3 + row1).GetNormalized();
  const xiiVec3d topPlane    = (row3 - row1).GetNormalized();

  out_fovLeft   = -xiiMath::ACos<float>((float)leftPlane.Dot(xiiVec3d(1.0, 0, 0)));
  out_fovRight  = xiiAngle::Radian(xiiMath::Pi<float>()) - xiiMath::ACos<float>((float)rightPlane.Dot(xiiVec3d(1.0, 0, 0)));
  out_fovBottom = -xiiMath::ACos<float>((float)bottomPlane.Dot(xiiVec3d(0, 1.0, 0)));
  out_fovTop    = xiiAngle::Radian(xiiMath::Pi<float>()) - xiiMath::ACos<float>((float)topPlane.Dot(xiiVec3d(0, 1.0, 0)));

  if (range == xiiClipSpaceYMode::Flipped)
    xiiMath::Swap(out_fovBottom, out_fovTop);
}

xiiResult xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const xiiMat4& mProjectionMatrix, float& out_fLeft, float& out_fRight, float& out_fBottom, float& out_fTop, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum range)
{
  float fNear, fFar;
  XII_SUCCEED_OR_RETURN(ExtractNearAndFarClipPlaneDistances(fNear, fFar, mProjectionMatrix, depthRange));
  // Compensate for inverse-Z.
  const float fMinDepth = xiiMath::Min(fNear, fFar);

  xiiAngle fFovLeft;
  xiiAngle fFovRight;
  xiiAngle fFovBottom;
  xiiAngle fFovTop;
  ExtractPerspectiveMatrixFieldOfView(mProjectionMatrix, fFovLeft, fFovRight, fFovBottom, fFovTop, range);

  out_fLeft   = xiiMath::Tan(fFovLeft) * fMinDepth;
  out_fRight  = xiiMath::Tan(fFovRight) * fMinDepth;
  out_fBottom = xiiMath::Tan(fFovBottom) * fMinDepth;
  out_fTop    = xiiMath::Tan(fFovTop) * fMinDepth;
  return XII_SUCCESS;
}

xiiResult xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const xiiMat4d& mProjectionMatrix, float& out_fLeft, float& out_fRight, float& out_fBottom, float& out_fTop, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum range)
{
  float fNear, fFar;
  XII_SUCCEED_OR_RETURN(ExtractNearAndFarClipPlaneDistances(fNear, fFar, mProjectionMatrix, depthRange));
  // Compensate for inverse-Z.
  const float fMinDepth = xiiMath::Min(fNear, fFar);

  xiiAngle fFovLeft;
  xiiAngle fFovRight;
  xiiAngle fFovBottom;
  xiiAngle fFovTop;
  ExtractPerspectiveMatrixFieldOfView(mProjectionMatrix, fFovLeft, fFovRight, fFovBottom, fFovTop, range);

  out_fLeft   = xiiMath::Tan(fFovLeft) * fMinDepth;
  out_fRight  = xiiMath::Tan(fFovRight) * fMinDepth;
  out_fBottom = xiiMath::Tan(fFovBottom) * fMinDepth;
  out_fTop    = xiiMath::Tan(fFovTop) * fMinDepth;
  return XII_SUCCESS;
}

xiiResult xiiGraphicsUtils::ExtractNearAndFarClipPlaneDistances(float& out_fNear, float& out_fFar, const xiiMat4& mProjectionMatrix, xiiClipSpaceDepthRange::Enum depthRange)
{
  const xiiVec4 row2 = mProjectionMatrix.GetRow(2);
  const xiiVec4 row3 = mProjectionMatrix.GetRow(3);

  xiiVec4 nearPlane = row2;

  if (depthRange == xiiClipSpaceDepthRange::MinusOneToOne)
  {
    nearPlane += row3;
  }

  const xiiVec4 farPlane = row3 - row2;

  const float nearLength = nearPlane.GetAsVec3().GetLength();
  const float farLength  = farPlane.GetAsVec3().GetLength();

  const float nearW = xiiMath::Abs(nearPlane.w);
  const float farW  = xiiMath::Abs(farPlane.w);

  if ((nearLength < xiiMath::SmallEpsilon<float>() && farLength < xiiMath::SmallEpsilon<float>()) ||
      nearW < xiiMath::SmallEpsilon<float>() || farW < xiiMath::SmallEpsilon<float>())
  {
    return XII_FAILURE;
  }

  const float fNear = nearW / nearLength;
  const float fFar  = farW / farLength;

  if (xiiMath::IsEqual(fNear, fFar, xiiMath::SmallEpsilon<float>()))
  {
    return XII_FAILURE;
  }

  out_fNear = fNear;
  out_fFar  = fFar;

  return XII_SUCCESS;
}

xiiResult xiiGraphicsUtils::ExtractNearAndFarClipPlaneDistances(float& out_fNear, float& out_fFar, const xiiMat4d& mProjectionMatrix, xiiClipSpaceDepthRange::Enum depthRange)
{
  const xiiVec4d row2 = mProjectionMatrix.GetRow(2);
  const xiiVec4d row3 = mProjectionMatrix.GetRow(3);

  xiiVec4d nearPlane = row2;

  if (depthRange == xiiClipSpaceDepthRange::MinusOneToOne)
  {
    nearPlane += row3;
  }

  const xiiVec4d farPlane = row3 - row2;

  const double nearLength = nearPlane.GetAsVec3().GetLength();
  const double farLength  = farPlane.GetAsVec3().GetLength();

  const double nearW = xiiMath::Abs(nearPlane.w);
  const double farW  = xiiMath::Abs(farPlane.w);

  if ((nearLength < xiiMath::SmallEpsilon<double>() && farLength < xiiMath::SmallEpsilon<double>()) ||
      nearW < xiiMath::SmallEpsilon<double>() || farW < xiiMath::SmallEpsilon<double>())
  {
    return XII_FAILURE;
  }

  const double fNear = nearW / nearLength;
  const double fFar  = farW / farLength;

  if (xiiMath::IsEqual(fNear, fFar, xiiMath::SmallEpsilon<double>()))
  {
    return XII_FAILURE;
  }

  out_fNear = (float)fNear;
  out_fFar  = (float)fFar;

  return XII_SUCCESS;
}

xiiPlane xiiGraphicsUtils::ComputeInterpolatedFrustumPlane(FrustumPlaneInterpolation direction, float fLerpFactor, const xiiMat4& mProjectionMatrix, xiiClipSpaceDepthRange::Enum depthRange)
{
  xiiVec4     rowA;
  xiiVec4     rowB            = mProjectionMatrix.GetRow(3);
  const float factorMinus1to1 = (fLerpFactor - 0.5f) * 2.0f; // bring into [-1; +1] range

  switch (direction)
  {
    case FrustumPlaneInterpolation::LeftToRight:
    {
      rowA = mProjectionMatrix.GetRow(0);
      rowB *= factorMinus1to1;
      break;
    }

    case FrustumPlaneInterpolation::BottomToTop:
    {
      rowA = mProjectionMatrix.GetRow(1);
      rowB *= factorMinus1to1;
      break;
    }

    case FrustumPlaneInterpolation::NearToFar:
      rowA = mProjectionMatrix.GetRow(2);

      if (depthRange == xiiClipSpaceDepthRange::ZeroToOne)
        rowB *= fLerpFactor; // [0; 1] range
      else
        rowB *= factorMinus1to1;
      break;
  }

  xiiPlane res;
  res.m_vNormal      = rowA.GetAsVec3() - rowB.GetAsVec3();
  res.m_fNegDistance = (rowA.w - rowB.w) / res.m_vNormal.GetLengthAndNormalize();

  return res;
}

xiiPlaned xiiGraphicsUtils::ComputeInterpolatedFrustumPlane(FrustumPlaneInterpolation direction, float fLerpFactor, const xiiMat4d& mProjectionMatrix, xiiClipSpaceDepthRange::Enum depthRange)
{
  xiiVec4d    rowA;
  xiiVec4d    rowB            = mProjectionMatrix.GetRow(3);
  const float factorMinus1to1 = (fLerpFactor - 0.5f) * 2.0f; // bring into [-1; +1] range

  switch (direction)
  {
    case FrustumPlaneInterpolation::LeftToRight:
    {
      rowA = mProjectionMatrix.GetRow(0);
      rowB *= factorMinus1to1;
      break;
    }

    case FrustumPlaneInterpolation::BottomToTop:
    {
      rowA = mProjectionMatrix.GetRow(1);
      rowB *= factorMinus1to1;
      break;
    }

    case FrustumPlaneInterpolation::NearToFar:
      rowA = mProjectionMatrix.GetRow(2);

      if (depthRange == xiiClipSpaceDepthRange::ZeroToOne)
        rowB *= fLerpFactor; // [0; 1] range
      else
        rowB *= factorMinus1to1;
      break;
  }

  xiiPlaned res;
  res.m_vNormal      = rowA.GetAsVec3() - rowB.GetAsVec3();
  res.m_fNegDistance = (rowA.w - rowB.w) / res.m_vNormal.GetLengthAndNormalize();

  return res;
}

xiiMat4 xiiGraphicsUtils::CreatePerspectiveProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum range, xiiHandedness::Enum handedness)
{
  const float vw = fViewWidth * 0.5f;
  const float vh = fViewHeight * 0.5f;

  return CreatePerspectiveProjectionMatrix(-vw, vw, -vh, vh, fNearZ, fFarZ, depthRange, range, handedness);
}

xiiMat4d xiiGraphicsUtils::CreatePerspectiveProjectionMatrixDouble(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum range, xiiHandedness::Enum handedness)
{
  const float vw = fViewWidth * 0.5f;
  const float vh = fViewHeight * 0.5f;

  return CreatePerspectiveProjectionMatrixDouble(-vw, vw, -vh, vh, fNearZ, fFarZ, depthRange, range, handedness);
}

xiiMat4 xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(xiiAngle fieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum range, xiiHandedness::Enum handedness)
{
  // Taking the minimum allows the function to be used to create
  // inverse z matrices (fNearZ > fFarZ) as well.
  const float xm = xiiMath::Min(fNearZ, fFarZ) * xiiMath::Tan(fieldOfViewX * 0.5f);
  const float ym = xm / fAspectRatioWidthDivHeight;

  return CreatePerspectiveProjectionMatrix(-xm, xm, -ym, ym, fNearZ, fFarZ, depthRange, range, handedness);
}

xiiMat4d xiiGraphicsUtils::CreatePerspectiveProjectionMatrixDoubleFromFovX(xiiAngle fieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum range, xiiHandedness::Enum handedness)
{
  // Taking the minimum allows the function to be used to create
  // inverse z matrices (fNearZ > fFarZ) as well.
  const float xm = xiiMath::Min(fNearZ, fFarZ) * xiiMath::Tan(fieldOfViewX * 0.5f);
  const float ym = xm / fAspectRatioWidthDivHeight;

  return CreatePerspectiveProjectionMatrixDouble(-xm, xm, -ym, ym, fNearZ, fFarZ, depthRange, range, handedness);
}

xiiMat4 xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(xiiAngle fieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum range, xiiHandedness::Enum handedness)
{
  // Taking the minimum allows the function to be used to create
  // inverse z matrices (fNearZ > fFarZ) as well.
  const float ym = xiiMath::Min(fNearZ, fFarZ) * xiiMath::Tan(fieldOfViewY * 0.5f);
  const float xm = ym * fAspectRatioWidthDivHeight;

  return CreatePerspectiveProjectionMatrix(-xm, xm, -ym, ym, fNearZ, fFarZ, depthRange, range, handedness);
}

xiiMat4d xiiGraphicsUtils::CreatePerspectiveProjectionMatrixDoubleFromFovY(xiiAngle fieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum range, xiiHandedness::Enum handedness)
{
  // Taking the minimum allows the function to be used to create
  // inverse z matrices (fNearZ > fFarZ) as well.
  const float ym = xiiMath::Min(fNearZ, fFarZ) * xiiMath::Tan(fieldOfViewY * 0.5f);
  const float xm = ym * fAspectRatioWidthDivHeight;

  return CreatePerspectiveProjectionMatrixDouble(-xm, xm, -ym, ym, fNearZ, fFarZ, depthRange, range, handedness);
}

xiiMat4 xiiGraphicsUtils::CreateOrthographicProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum range, xiiHandedness::Enum handedness)
{
  return CreateOrthographicProjectionMatrix(-fViewWidth * 0.5f, fViewWidth * 0.5f, -fViewHeight * 0.5f, fViewHeight * 0.5f, fNearZ, fFarZ, depthRange, range, handedness);
}

xiiMat4d xiiGraphicsUtils::CreateOrthographicProjectionMatrixDouble(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum range, xiiHandedness::Enum handedness)
{
  return CreateOrthographicProjectionMatrixDouble(-fViewWidth * 0.5f, fViewWidth * 0.5f, -fViewHeight * 0.5f, fViewHeight * 0.5f, fNearZ, fFarZ, depthRange, range, handedness);
}

xiiMat4 xiiGraphicsUtils::CreateOrthographicProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum range, xiiHandedness::Enum handedness)
{
  XII_ASSERT_DEBUG(xiiMath::IsFinite(fNearZ) && xiiMath::IsFinite(fFarZ), "Infinite plane values are not supported for orthographic projections!");

  xiiMat4 res;
  res.SetIdentity();

  if (range == xiiClipSpaceYMode::Flipped)
  {
    xiiMath::Swap(fBottom, fTop);
  }

  const float fOneDivFarMinusNear   = 1.0f / (fFarZ - fNearZ);
  const float fOneDivRightMinusLeft = 1.0f / (fRight - fLeft);
  const float fOneDivTopMinusBottom = 1.0f / (fTop - fBottom);

  res.Element(0, 0) = 2.0f / (fRight - fLeft);

  res.Element(1, 1) = 2.0f / (fTop - fBottom);

  res.Element(3, 0) = -(fLeft + fRight) * fOneDivRightMinusLeft;
  res.Element(3, 1) = -(fTop + fBottom) * fOneDivTopMinusBottom;


  if (depthRange == xiiClipSpaceDepthRange::MinusOneToOne)
  {
    // The OpenGL Way: http://wiki.delphigl.com/index.php/glFrustum
    res.Element(2, 2) = -2.0f * fOneDivFarMinusNear;
    res.Element(3, 2) = -(fFarZ + fNearZ) * fOneDivFarMinusNear;
  }
  else
  {
    // The Left-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixorthooffcenterlh
    // The Right-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixorthooffcenterrh

    res.Element(2, 2) = -1.0f * fOneDivFarMinusNear;
    res.Element(3, 2) = -fNearZ * fOneDivFarMinusNear;
  }

  if (handedness == xiiHandedness::LeftHanded)
  {
    res.SetColumn(2, -res.GetColumn(2));
  }

  return res;
}

xiiMat4d xiiGraphicsUtils::CreateOrthographicProjectionMatrixDouble(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum range, xiiHandedness::Enum handedness)
{
  XII_ASSERT_DEBUG(xiiMath::IsFinite(fNearZ) && xiiMath::IsFinite(fFarZ), "Infinite plane values are not supported for orthographic projections!");

  xiiMat4d res;
  res.SetIdentity();

  if (range == xiiClipSpaceYMode::Flipped)
  {
    xiiMath::Swap(fBottom, fTop);
  }

  const double fOneDivFarMinusNear   = 1.0 / (fFarZ - fNearZ);
  const double fOneDivRightMinusLeft = 1.0 / (fRight - fLeft);
  const double fOneDivTopMinusBottom = 1.0 / (fTop - fBottom);

  res.Element(0, 0) = 2.0 / (fRight - fLeft);

  res.Element(1, 1) = 2.0 / (fTop - fBottom);

  res.Element(3, 0) = -(fLeft + fRight) * fOneDivRightMinusLeft;
  res.Element(3, 1) = -(fTop + fBottom) * fOneDivTopMinusBottom;


  if (depthRange == xiiClipSpaceDepthRange::MinusOneToOne)
  {
    // The OpenGL Way: http://wiki.delphigl.com/index.php/glFrustum
    res.Element(2, 2) = -2.0 * fOneDivFarMinusNear;
    res.Element(3, 2) = -(fFarZ + fNearZ) * fOneDivFarMinusNear;
  }
  else
  {
    // The Left-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixorthooffcenterlh
    // The Right-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixorthooffcenterrh

    res.Element(2, 2) = -1.0 * fOneDivFarMinusNear;
    res.Element(3, 2) = -fNearZ * fOneDivFarMinusNear;
  }

  if (handedness == xiiHandedness::LeftHanded)
  {
    res.SetColumn(2, -res.GetColumn(2));
  }

  return res;
}

xiiMat4 xiiGraphicsUtils::CreatePerspectiveProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum range, xiiHandedness::Enum handedness)
{
  XII_ASSERT_DEBUG(xiiMath::IsFinite(fNearZ) || xiiMath::IsFinite(fFarZ), "fNearZ and fFarZ cannot both be infinite at the same time!");

  xiiMat4 res;
  res.SetZero();

  if (range == xiiClipSpaceYMode::Flipped)
  {
    xiiMath::Swap(fBottom, fTop);
  }

  // Taking the minimum of the two plane values allows
  // this function to also be used to create inverse-z
  // matrices by specifying values of fNearZ > fFarZ.
  // Otherwise the x and y scaling values will be wrong
  // in the final matrix.
  const float fMinPlane             = xiiMath::Min(fNearZ, fFarZ);
  const float fTwoNearZ             = fMinPlane + fMinPlane;
  const float fOneDivRightMinusLeft = 1.0f / (fRight - fLeft);
  const float fOneDivTopMinusBottom = 1.0f / (fTop - fBottom);

  res.Element(0, 0) = fTwoNearZ * fOneDivRightMinusLeft;

  res.Element(1, 1) = fTwoNearZ * fOneDivTopMinusBottom;

  res.Element(2, 0) = (fLeft + fRight) * fOneDivRightMinusLeft;
  res.Element(2, 1) = (fTop + fBottom) * fOneDivTopMinusBottom;
  res.Element(2, 3) = -1.0f;

  // If either fNearZ or fFarZ is infinite, one can derive the resulting z-transformation by using limit math
  // and letting the respective variable approach infinity in the original expressions for P(2, 2) and P(3, 2).
  // The result is that a couple of terms from the original fraction get reduced to 0 by being divided by infinity,
  // which fortunately yields 1) finite and 2) much simpler expressions for P(2, 2) and P(3, 2).
  if (depthRange == xiiClipSpaceDepthRange::MinusOneToOne)
  {
    // The OpenGL Way: http://wiki.delphigl.com/index.php/glFrustum
    // Algebraically reordering the z-row fractions from the above source in a way so infinite fNearZ or fFarZ will zero out
    // instead of producing NaNs due to inf/inf divisions will yield these generalized formulas which could be used instead
    // of the branching below. Insert infinity for either fNearZ or fFarZ to see that these will yield exactly these simplifications:
    //res.Element(2, 2) = 1.f / (fNearZ / fFarZ - 1.f) + 1.f / (1.f - fFarZ / fNearZ);
    //res.Element(3, 2) = 2.f / (1.f / fFarZ - 1.f / fNearZ);
    if (!xiiMath::IsFinite(fNearZ))
    {
      res.Element(2, 2) = 1.f;
      res.Element(3, 2) = 2.f * fFarZ;
    }
    else if (!xiiMath::IsFinite(fFarZ))
    {
      res.Element(2, 2) = -1.f;
      res.Element(3, 2) = -2.f * fNearZ;
    }
    else
    {
      const float fOneDivNearMinusFar = 1.0f / (fNearZ - fFarZ);
      res.Element(2, 2)               = (fFarZ + fNearZ) * fOneDivNearMinusFar;
      res.Element(3, 2)               = 2 * fFarZ * fNearZ * fOneDivNearMinusFar;
    }
  }
  else
  {
    // The Left-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixperspectiveoffcenterlh
    // The Right-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixperspectiveoffcenterrh
    // Algebraically reordering the z-row fractions from the above source in a way so infinite fNearZ or fFarZ will zero out
    // instead of producing NaNs due to inf/inf divisions will yield these generalized formulas which could be used instead
    // of the branching below. Insert infinity for either fNearZ or fFarZ to see that these will yield exactly these simplifications:
    //res.Element(2, 2) = 1.f / (fNearZ / fFarZ - 1.f);
    //res.Element(3, 2) = 1.f / (1.f / fFarZ - 1.f / fNearZ);
    if (!xiiMath::IsFinite(fNearZ))
    {
      res.Element(2, 2) = 0.f;
      res.Element(3, 2) = fFarZ;
    }
    else if (!xiiMath::IsFinite(fFarZ))
    {
      res.Element(2, 2) = -1.f;
      res.Element(3, 2) = -fNearZ;
    }
    else
    {
      const float fOneDivNearMinusFar = 1.0f / (fNearZ - fFarZ);
      res.Element(2, 2)               = fFarZ * fOneDivNearMinusFar;
      res.Element(3, 2)               = fFarZ * fNearZ * fOneDivNearMinusFar;
    }
  }

  if (handedness == xiiHandedness::LeftHanded)
  {
    res.SetColumn(2, -res.GetColumn(2));
  }

  return res;
}

xiiMat4d xiiGraphicsUtils::CreatePerspectiveProjectionMatrixDouble(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum range, xiiHandedness::Enum handedness)
{
  XII_ASSERT_DEBUG(xiiMath::IsFinite(fNearZ) || xiiMath::IsFinite(fFarZ), "fNearZ and fFarZ cannot both be infinite at the same time!");

  xiiMat4d res;
  res.SetZero();

  if (range == xiiClipSpaceYMode::Flipped)
  {
    xiiMath::Swap(fBottom, fTop);
  }

  // Taking the minimum of the two plane values allows
  // this function to also be used to create inverse-z
  // matrices by specifying values of fNearZ > fFarZ.
  // Otherwise the x and y scaling values will be wrong
  // in the final matrix.
  const double fMinPlane             = xiiMath::Min(fNearZ, fFarZ);
  const double fTwoNearZ             = fMinPlane + fMinPlane;
  const double fOneDivRightMinusLeft = 1.0 / (fRight - fLeft);
  const double fOneDivTopMinusBottom = 1.0 / (fTop - fBottom);

  res.Element(0, 0) = fTwoNearZ * fOneDivRightMinusLeft;

  res.Element(1, 1) = fTwoNearZ * fOneDivTopMinusBottom;

  res.Element(2, 0) = (fLeft + fRight) * fOneDivRightMinusLeft;
  res.Element(2, 1) = (fTop + fBottom) * fOneDivTopMinusBottom;
  res.Element(2, 3) = -1.0f;

  // If either fNearZ or fFarZ is infinite, one can derive the resulting z-transformation by using limit math
  // and letting the respective variable approach infinity in the original expressions for P(2, 2) and P(3, 2).
  // The result is that a couple of terms from the original fraction get reduced to 0 by being divided by infinity,
  // which fortunately yields 1) finite and 2) much simpler expressions for P(2, 2) and P(3, 2).
  if (depthRange == xiiClipSpaceDepthRange::MinusOneToOne)
  {
    // The OpenGL Way: http://wiki.delphigl.com/index.php/glFrustum
    // Algebraically reordering the z-row fractions from the above source in a way so infinite fNearZ or fFarZ will zero out
    // instead of producing NaNs due to inf/inf divisions will yield these generalized formulas which could be used instead
    // of the branching below. Insert infinity for either fNearZ or fFarZ to see that these will yield exactly these simplifications:
    //res.Element(2, 2) = 1.f / (fNearZ / fFarZ - 1.f) + 1.f / (1.f - fFarZ / fNearZ);
    //res.Element(3, 2) = 2.f / (1.f / fFarZ - 1.f / fNearZ);
    if (!xiiMath::IsFinite(fNearZ))
    {
      res.Element(2, 2) = 1.f;
      res.Element(3, 2) = 2.f * fFarZ;
    }
    else if (!xiiMath::IsFinite(fFarZ))
    {
      res.Element(2, 2) = -1.f;
      res.Element(3, 2) = -2.f * fNearZ;
    }
    else
    {
      const double fOneDivNearMinusFar = 1.0 / (fNearZ - fFarZ);
      res.Element(2, 2)                = (fFarZ + fNearZ) * fOneDivNearMinusFar;
      res.Element(3, 2)                = 2 * fFarZ * fNearZ * fOneDivNearMinusFar;
    }
  }
  else
  {
    // The Left-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixperspectiveoffcenterlh
    // The Right-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixperspectiveoffcenterrh
    // Algebraically reordering the z-row fractions from the above source in a way so infinite fNearZ or fFarZ will zero out
    // instead of producing NaNs due to inf/inf divisions will yield these generalized formulas which could be used instead
    // of the branching below. Insert infinity for either fNearZ or fFarZ to see that these will yield exactly these simplifications:
    //res.Element(2, 2) = 1.f / (fNearZ / fFarZ - 1.f);
    //res.Element(3, 2) = 1.f / (1.f / fFarZ - 1.f / fNearZ);
    if (!xiiMath::IsFinite(fNearZ))
    {
      res.Element(2, 2) = 0.0;
      res.Element(3, 2) = fFarZ;
    }
    else if (!xiiMath::IsFinite(fFarZ))
    {
      res.Element(2, 2) = -1.0;
      res.Element(3, 2) = -fNearZ;
    }
    else
    {
      const double fOneDivNearMinusFar = 1.0 / (fNearZ - fFarZ);
      res.Element(2, 2)                = fFarZ * fOneDivNearMinusFar;
      res.Element(3, 2)                = fFarZ * fNearZ * fOneDivNearMinusFar;
    }
  }

  if (handedness == xiiHandedness::LeftHanded)
  {
    res.SetColumn(2, -res.GetColumn(2));
  }

  return res;
}

xiiMat3 xiiGraphicsUtils::CreateLookAtViewMatrix(const xiiVec3& vTarget, const xiiVec3& vUpDir, xiiHandedness::Enum handedness)
{
  XII_ASSERT_DEBUG(!vTarget.IsZero(), "The target must not be at the origin.");

  xiiVec3 vLookDir = vTarget;
  vLookDir.NormalizeIfNotZero(xiiVec3::UnitXAxis()).IgnoreResult();

  xiiVec3 vNormalizedUpDir = vUpDir.GetNormalized();

  if (xiiMath::Abs(vLookDir.Dot(vNormalizedUpDir)) > 0.9999f) // less than 1 degree difference -> problem
  {
    // use some arbitrary other orthogonal vector as UP
    vNormalizedUpDir = vLookDir.GetOrthogonalVector();
  }

  xiiMat3 res;

  const xiiVec3 zaxis = (handedness == xiiHandedness::RightHanded) ? -vLookDir : vLookDir;
  const xiiVec3 xaxis = vNormalizedUpDir.CrossRH(zaxis).GetNormalized();
  const xiiVec3 yaxis = zaxis.CrossRH(xaxis);

  res.SetRow(0, xaxis);
  res.SetRow(1, yaxis);
  res.SetRow(2, zaxis);

  return res;
}

xiiMat3d xiiGraphicsUtils::CreateLookAtViewMatrix(const xiiVec3d& vTarget, const xiiVec3d& vUpDir, xiiHandedness::Enum handedness)
{
  XII_ASSERT_DEBUG(!vTarget.IsZero(), "The target must not be at the origin.");

  xiiVec3d vLookDir = vTarget;
  vLookDir.NormalizeIfNotZero(xiiVec3d::UnitXAxis()).IgnoreResult();

  xiiVec3d vNormalizedUpDir = vUpDir.GetNormalized();

  if (xiiMath::Abs(vLookDir.Dot(vNormalizedUpDir)) > 0.99999999) // less than 1 degree difference -> problem
  {
    // use some arbitrary other orthogonal vector as UP
    vNormalizedUpDir = vLookDir.GetOrthogonalVector();
  }

  xiiMat3d res;

  const xiiVec3d zaxis = (handedness == xiiHandedness::RightHanded) ? -vLookDir : vLookDir;
  const xiiVec3d xaxis = vNormalizedUpDir.CrossRH(zaxis).GetNormalized();
  const xiiVec3d yaxis = zaxis.CrossRH(xaxis);

  res.SetRow(0, xaxis);
  res.SetRow(1, yaxis);
  res.SetRow(2, zaxis);

  return res;
}

xiiMat3 xiiGraphicsUtils::CreateInverseLookAtViewMatrix(const xiiVec3& vTarget, const xiiVec3& vUpDir, xiiHandedness::Enum handedness)
{
  XII_ASSERT_DEBUG(!vTarget.IsZero(), "The target must not be at the origin.");

  xiiVec3 vLookDir = vTarget;
  vLookDir.NormalizeIfNotZero(xiiVec3::UnitXAxis()).IgnoreResult();

  xiiVec3 vNormalizedUpDir = vUpDir.GetNormalized();

  if (xiiMath::Abs(vLookDir.Dot(vNormalizedUpDir)) > 0.9999f) // less than 1 degree difference -> problem
  {
    // use some arbitrary other orthogonal vector as UP
    vNormalizedUpDir = vLookDir.GetOrthogonalVector();
  }

  xiiMat3 res;

  const xiiVec3 zaxis = (handedness == xiiHandedness::RightHanded) ? -vLookDir : vLookDir;
  const xiiVec3 xaxis = vNormalizedUpDir.CrossRH(zaxis).GetNormalized();
  const xiiVec3 yaxis = zaxis.CrossRH(xaxis);

  res.SetColumn(0, xaxis);
  res.SetColumn(1, yaxis);
  res.SetColumn(2, zaxis);

  return res;
}

xiiMat3d xiiGraphicsUtils::CreateInverseLookAtViewMatrix(const xiiVec3d& vTarget, const xiiVec3d& vUpDir, xiiHandedness::Enum handedness)
{
  XII_ASSERT_DEBUG(!vTarget.IsZero(), "The target must not be at the origin.");

  xiiVec3d vLookDir = vTarget;
  vLookDir.NormalizeIfNotZero(xiiVec3d::UnitXAxis()).IgnoreResult();

  xiiVec3d vNormalizedUpDir = vUpDir.GetNormalized();

  if (xiiMath::Abs(vLookDir.Dot(vNormalizedUpDir)) > 0.99999999) // less than 1 degree difference -> problem
  {
    // use some arbitrary other orthogonal vector as UP
    vNormalizedUpDir = vLookDir.GetOrthogonalVector();
  }

  xiiMat3d res;

  const xiiVec3d zaxis = (handedness == xiiHandedness::RightHanded) ? -vLookDir : vLookDir;
  const xiiVec3d xaxis = vNormalizedUpDir.CrossRH(zaxis).GetNormalized();
  const xiiVec3d yaxis = zaxis.CrossRH(xaxis);

  res.SetColumn(0, xaxis);
  res.SetColumn(1, yaxis);
  res.SetColumn(2, zaxis);

  return res;
}

xiiMat4 xiiGraphicsUtils::CreateLookAtViewMatrix(const xiiVec3& vEyePos, const xiiVec3& vLookAtPos, const xiiVec3& vUpDir, xiiHandedness::Enum handedness)
{
  const xiiMat3 rotation = CreateLookAtViewMatrix(vLookAtPos - vEyePos, vUpDir, handedness);

  xiiMat4 res;
  res.SetRotationalPart(rotation);
  res.SetTranslationVector(rotation * -vEyePos);
  res.SetRow(3, xiiVec4(0, 0, 0, 1));
  return res;
}

xiiMat4d xiiGraphicsUtils::CreateLookAtViewMatrix(const xiiVec3d& vEyePos, const xiiVec3d& vLookAtPos, const xiiVec3d& vUpDir, xiiHandedness::Enum handedness)
{
  const xiiMat3d rotation = CreateLookAtViewMatrix(vLookAtPos - vEyePos, vUpDir, handedness);

  xiiMat4d res;
  res.SetRotationalPart(rotation);
  res.SetTranslationVector(rotation * -vEyePos);
  res.SetRow(3, xiiVec4d(0, 0, 0, 1));
  return res;
}

xiiMat4 xiiGraphicsUtils::CreateInverseLookAtViewMatrix(const xiiVec3& vEyePos, const xiiVec3& vLookAtPos, const xiiVec3& vUpDir, xiiHandedness::Enum handedness)
{
  const xiiMat3 rotation = CreateInverseLookAtViewMatrix(vLookAtPos - vEyePos, vUpDir, handedness);

  xiiMat4 res;
  res.SetRotationalPart(rotation);
  res.SetTranslationVector(vEyePos);
  res.SetRow(3, xiiVec4(0, 0, 0, 1));
  return res;
}

xiiMat4d xiiGraphicsUtils::CreateInverseLookAtViewMatrix(const xiiVec3d& vEyePos, const xiiVec3d& vLookAtPos, const xiiVec3d& vUpDir, xiiHandedness::Enum handedness)
{
  const xiiMat3d rotation = CreateInverseLookAtViewMatrix(vLookAtPos - vEyePos, vUpDir, handedness);

  xiiMat4d res;
  res.SetRotationalPart(rotation);
  res.SetTranslationVector(vEyePos);
  res.SetRow(3, xiiVec4d(0, 0, 0, 1));
  return res;
}

xiiMat4 xiiGraphicsUtils::CreateViewMatrix(const xiiVec3& vPosition, const xiiVec3& vForwardDir, const xiiVec3& vRightDir, const xiiVec3& vUpDir, xiiHandedness::Enum handedness)
{
  xiiMat4 res;
  res.SetIdentity();

  xiiVec3 xaxis, yaxis, zaxis;

  if (handedness == xiiHandedness::LeftHanded)
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = vForwardDir;
  }
  else
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = -vForwardDir;
  }

  res.SetRow(0, xaxis.GetAsVec4(0));
  res.SetRow(1, yaxis.GetAsVec4(0));
  res.SetRow(2, zaxis.GetAsVec4(0));
  res.SetTranslationVector(xiiVec3(-xaxis.Dot(vPosition), -yaxis.Dot(vPosition), -zaxis.Dot(vPosition)));

  return res;
}

xiiMat4d xiiGraphicsUtils::CreateViewMatrix(const xiiVec3d& vPosition, const xiiVec3d& vForwardDir, const xiiVec3d& vRightDir, const xiiVec3d& vUpDir, xiiHandedness::Enum handedness)
{
  xiiMat4d res;
  res.SetIdentity();

  xiiVec3d xaxis, yaxis, zaxis;

  if (handedness == xiiHandedness::LeftHanded)
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = vForwardDir;
  }
  else
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = -vForwardDir;
  }

  res.SetRow(0, xaxis.GetAsVec4(0));
  res.SetRow(1, yaxis.GetAsVec4(0));
  res.SetRow(2, zaxis.GetAsVec4(0));
  res.SetTranslationVector(xiiVec3d(-xaxis.Dot(vPosition), -yaxis.Dot(vPosition), -zaxis.Dot(vPosition)));

  return res;
}

xiiMat4 xiiGraphicsUtils::CreateInverseViewMatrix(const xiiVec3& vPosition, const xiiVec3& vForwardDir, const xiiVec3& vRightDir, const xiiVec3& vUpDir, xiiHandedness::Enum handedness)
{
  xiiMat4 res;
  res.SetIdentity();

  xiiVec3 xaxis, yaxis, zaxis;

  if (handedness == xiiHandedness::LeftHanded)
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = vForwardDir;
  }
  else
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = -vForwardDir;
  }

  res.SetColumn(0, xaxis.GetAsVec4(0));
  res.SetColumn(1, yaxis.GetAsVec4(0));
  res.SetColumn(2, zaxis.GetAsVec4(0));
  res.SetTranslationVector(vPosition);

  return res;
}

xiiMat4d xiiGraphicsUtils::CreateInverseViewMatrix(const xiiVec3d& vPosition, const xiiVec3d& vForwardDir, const xiiVec3d& vRightDir, const xiiVec3d& vUpDir, xiiHandedness::Enum handedness)
{
  xiiMat4d res;
  res.SetIdentity();

  xiiVec3d xaxis, yaxis, zaxis;

  if (handedness == xiiHandedness::LeftHanded)
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = vForwardDir;
  }
  else
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = -vForwardDir;
  }

  res.SetColumn(0, xaxis.GetAsVec4(0));
  res.SetColumn(1, yaxis.GetAsVec4(0));
  res.SetColumn(2, zaxis.GetAsVec4(0));
  res.SetTranslationVector(vPosition);

  return res;
}

void xiiGraphicsUtils::DecomposeViewMatrix(xiiVec3& ref_vPosition, xiiVec3& ref_vForwardDir, xiiVec3& ref_vRightDir, xiiVec3& ref_vUpDir, const xiiMat4& mViewMatrix, xiiHandedness::Enum handedness)
{
  const xiiMat3 rotation = mViewMatrix.GetRotationalPart();

  if (handedness == xiiHandedness::LeftHanded)
  {
    ref_vRightDir   = rotation.GetRow(0);
    ref_vUpDir      = rotation.GetRow(1);
    ref_vForwardDir = rotation.GetRow(2);
  }
  else
  {
    ref_vRightDir   = rotation.GetRow(0);
    ref_vUpDir      = rotation.GetRow(1);
    ref_vForwardDir = -rotation.GetRow(2);
  }

  ref_vPosition = rotation.GetTranspose() * -mViewMatrix.GetTranslationVector();
}

void xiiGraphicsUtils::DecomposeViewMatrix(xiiVec3d& ref_vPosition, xiiVec3d& ref_vForwardDir, xiiVec3d& ref_vRightDir, xiiVec3d& ref_vUpDir, const xiiMat4d& mViewMatrix, xiiHandedness::Enum handedness)
{
  const xiiMat3d rotation = mViewMatrix.GetRotationalPart();

  if (handedness == xiiHandedness::LeftHanded)
  {
    ref_vRightDir   = rotation.GetRow(0);
    ref_vUpDir      = rotation.GetRow(1);
    ref_vForwardDir = rotation.GetRow(2);
  }
  else
  {
    ref_vRightDir   = rotation.GetRow(0);
    ref_vUpDir      = rotation.GetRow(1);
    ref_vForwardDir = -rotation.GetRow(2);
  }

  ref_vPosition = rotation.GetTranspose() * -mViewMatrix.GetTranslationVector();
}

xiiResult xiiGraphicsUtils::ComputeBarycentricCoordinates(xiiVec3& out_vCoordinates, const xiiVec3& a, const xiiVec3& b, const xiiVec3& c, const xiiVec3& p)
{
  // implementation copied from https://gamedev.stackexchange.com/a/49370

  const xiiVec3 v0 = b - a;
  const xiiVec3 v1 = c - a;
  const xiiVec3 v2 = p - a;

  const float d00   = v0.Dot(v0);
  const float d01   = v0.Dot(v1);
  const float d11   = v1.Dot(v1);
  const float d20   = v2.Dot(v0);
  const float d21   = v2.Dot(v1);
  const float denom = d00 * d11 - d01 * d01;

  if (xiiMath::IsZero(denom, xiiMath::SmallEpsilon<float>()))
    return XII_FAILURE;

  const float invDenom = 1.0f / denom;

  const float v = (d11 * d20 - d01 * d21) * invDenom;
  const float w = (d00 * d21 - d01 * d20) * invDenom;
  const float u = 1.0f - v - w;

  out_vCoordinates.Set(u, v, w);

  return XII_SUCCESS;
}

xiiResult xiiGraphicsUtils::ComputeBarycentricCoordinates(xiiVec3d& out_vCoordinates, const xiiVec3d& a, const xiiVec3d& b, const xiiVec3d& c, const xiiVec3d& p)
{
  // implementation copied from https://gamedev.stackexchange.com/a/49370

  const xiiVec3d v0 = b - a;
  const xiiVec3d v1 = c - a;
  const xiiVec3d v2 = p - a;

  const double d00   = v0.Dot(v0);
  const double d01   = v0.Dot(v1);
  const double d11   = v1.Dot(v1);
  const double d20   = v2.Dot(v0);
  const double d21   = v2.Dot(v1);
  const double denom = d00 * d11 - d01 * d01;

  if (xiiMath::IsZero(denom, xiiMath::SmallEpsilon<double>()))
    return XII_FAILURE;

  const double invDenom = 1.0 / denom;

  const double v = (d11 * d20 - d01 * d21) * invDenom;
  const double w = (d00 * d21 - d01 * d20) * invDenom;
  const double u = 1.0f - v - w;

  out_vCoordinates.Set(u, v, w);

  return XII_SUCCESS;
}

xiiResult xiiGraphicsUtils::ComputeBarycentricCoordinates(xiiVec3& out_vCoordinates, const xiiVec2& a, const xiiVec2& b, const xiiVec2& c, const xiiVec2& p)
{
  // implementation copied from https://gamedev.stackexchange.com/a/63203

  const xiiVec2 v0 = b - a;
  const xiiVec2 v1 = c - a;
  const xiiVec2 v2 = p - a;

  const float denom = v0.x * v1.y - v1.x * v0.y;

  if (xiiMath::IsZero(denom, xiiMath::SmallEpsilon<float>()))
    return XII_FAILURE;

  const float invDenom = 1.0f / denom;
  const float v        = (v2.x * v1.y - v1.x * v2.y) * invDenom;
  const float w        = (v0.x * v2.y - v2.x * v0.y) * invDenom;
  const float u        = 1.0f - v - w;

  out_vCoordinates.Set(u, v, w);

  return XII_SUCCESS;
}

xiiResult xiiGraphicsUtils::ComputeBarycentricCoordinates(xiiVec3d& out_vCoordinates, const xiiVec2d& a, const xiiVec2d& b, const xiiVec2d& c, const xiiVec2d& p)
{
  // implementation copied from https://gamedev.stackexchange.com/a/63203

  const xiiVec2d v0 = b - a;
  const xiiVec2d v1 = c - a;
  const xiiVec2d v2 = p - a;

  const double denom = v0.x * v1.y - v1.x * v0.y;

  if (xiiMath::IsZero(denom, xiiMath::SmallEpsilon<double>()))
    return XII_FAILURE;

  const double invDenom = 1.0f / denom;
  const double v        = (v2.x * v1.y - v1.x * v2.y) * invDenom;
  const double w        = (v0.x * v2.y - v2.x * v0.y) * invDenom;
  const double u        = 1.0f - v - w;

  out_vCoordinates.Set(u, v, w);

  return XII_SUCCESS;
}



XII_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_GraphicsUtils);
