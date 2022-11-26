#include <Foundation/FoundationPCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>

xiiResult xiiGraphicsUtils::ConvertWorldPosToScreenPos(const xiiMat4& ModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3& vPoint, xiiVec3& out_vScreenPos, xiiClipSpaceDepthRange::Enum DepthRange)
{
  const xiiVec4 vToProject = vPoint.GetAsVec4(1.0f);

  xiiVec4 vClipSpace = ModelViewProjection * vToProject;

  if (vClipSpace.w == 0.0f)
    return XII_FAILURE;

  xiiVec3 vProjected = vClipSpace.GetAsVec3() / vClipSpace.w;
  if (vClipSpace.w < 0.0f)
    vProjected.z = -vProjected.z;

  out_vScreenPos.x = uiViewportX + uiViewportWidth * ((vProjected.x * 0.5f) + 0.5f);
  out_vScreenPos.y = uiViewportY + uiViewportHeight * ((vProjected.y * 0.5f) + 0.5f);

  // normalize the output z value to always be in [0; 1] range
  // That means when the projection matrix spits out values between -1 and +1, rescale those values
  if (DepthRange == xiiClipSpaceDepthRange::MinusOneToOne)
    out_vScreenPos.z = vProjected.z * 0.5f + 0.5f;
  else
    out_vScreenPos.z = vProjected.z;

  return XII_SUCCESS;
}

xiiResult xiiGraphicsUtils::ConvertScreenPosToWorldPos(
  const xiiMat4&               InverseModelViewProjection,
  const xiiUInt32              uiViewportX,
  const xiiUInt32              uiViewportY,
  const xiiUInt32              uiViewportWidth,
  const xiiUInt32              uiViewportHeight,
  const xiiVec3&               vScreenPos,
  xiiVec3&                     out_vPoint,
  xiiVec3*                     out_vDirection,
  xiiClipSpaceDepthRange::Enum DepthRange)
{
  xiiVec3 vClipSpace = vScreenPos;

  // From window coordinates to [0; 1] range
  vClipSpace.x = (vClipSpace.x - uiViewportX) / uiViewportWidth;
  vClipSpace.y = (vClipSpace.y - uiViewportY) / uiViewportHeight;

  // Map to range [-1; 1]
  vClipSpace.x = vClipSpace.x * 2.0f - 1.0f;
  vClipSpace.y = vClipSpace.y * 2.0f - 1.0f;

  // The OpenGL matrix expects the z values to be between -1 and +1, so rescale the incoming value to that range
  if (DepthRange == xiiClipSpaceDepthRange::MinusOneToOne)
    vClipSpace.z = vClipSpace.z * 2.0f - 1.0f;

  xiiVec4 vToUnProject = vClipSpace.GetAsVec4(1.0f);

  xiiVec4 vWorldSpacePoint = InverseModelViewProjection * vToUnProject;

  if (vWorldSpacePoint.w == 0.0f)
    return XII_FAILURE;

  out_vPoint = vWorldSpacePoint.GetAsVec3() / vWorldSpacePoint.w;

  if (out_vDirection != nullptr)
  {
    vToUnProject.z += 0.1f; // a point that is a bit further away

    const xiiVec4 vWorldSpacePoint2 = InverseModelViewProjection * vToUnProject;

    XII_ASSERT_DEV(vWorldSpacePoint2.w != 0.0f, "It should not be possible that the first projected point has a w other than zero, but the second one has!");

    const xiiVec3 vPoint2 = vWorldSpacePoint2.GetAsVec3() / vWorldSpacePoint2.w;

    *out_vDirection = (vPoint2 - out_vPoint).GetNormalized();
  }

  return XII_SUCCESS;
}

xiiResult xiiGraphicsUtils::ConvertScreenPosToWorldPos(const xiiMat4d& InverseModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3& vScreenPos, xiiVec3& out_vPoint, xiiVec3* out_vDirection /*= nullptr*/, xiiClipSpaceDepthRange::Enum DepthRange /*= xiiClipSpaceDepthRange::Default*/)
{
  xiiVec3 vClipSpace = vScreenPos;

  // From window coordinates to [0; 1] range
  vClipSpace.x = (vClipSpace.x - uiViewportX) / uiViewportWidth;
  vClipSpace.y = (vClipSpace.y - uiViewportY) / uiViewportHeight;

  // Map to range [-1; 1]
  vClipSpace.x = vClipSpace.x * 2.0f - 1.0f;
  vClipSpace.y = vClipSpace.y * 2.0f - 1.0f;

  // The OpenGL matrix expects the z values to be between -1 and +1, so rescale the incoming value to that range
  if (DepthRange == xiiClipSpaceDepthRange::MinusOneToOne)
    vClipSpace.z = vClipSpace.z * 2.0f - 1.0f;

  xiiVec4d vToUnProject = xiiVec4d(vClipSpace.x, vClipSpace.y, vClipSpace.z, 1.0);

  xiiVec4d vWorldSpacePoint = InverseModelViewProjection * vToUnProject;

  if (vWorldSpacePoint.w == 0.0)
    return XII_FAILURE;

  xiiVec3d outTemp = vWorldSpacePoint.GetAsVec3() / vWorldSpacePoint.w;
  out_vPoint.Set((float)outTemp.x, (float)outTemp.y, (float)outTemp.z);

  if (out_vDirection != nullptr)
  {
    vToUnProject.z += 0.1f; // a point that is a bit further away

    const xiiVec4d vWorldSpacePoint2 = InverseModelViewProjection * vToUnProject;

    XII_ASSERT_DEV(vWorldSpacePoint2.w != 0.0, "It should not be possible that the first projected point has a w other than zero, but the second one has!");

    const xiiVec3d vPoint2 = vWorldSpacePoint2.GetAsVec3() / vWorldSpacePoint2.w;

    xiiVec3d outDir = (vPoint2 - outTemp).GetNormalized();
    out_vDirection->Set((float)outDir.x, (float)outDir.y, (float)outDir.z);
  }

  return XII_SUCCESS;
}

bool xiiGraphicsUtils::IsTriangleFlipRequired(const xiiMat3& mTransformation)
{
  return (mTransformation.GetColumn(0).CrossRH(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);
}

void xiiGraphicsUtils::ConvertProjectionMatrixDepthRange(xiiMat4& inout_Matrix, xiiClipSpaceDepthRange::Enum SrcDepthRange, xiiClipSpaceDepthRange::Enum DstDepthRange)
{
  // exclude identity transformations
  if (SrcDepthRange == DstDepthRange)
    return;

  xiiVec4 row2 = inout_Matrix.GetRow(2);
  xiiVec4 row3 = inout_Matrix.GetRow(3);

  // only need to check SrcDepthRange, the rest is the logical conclusion from being not equal
  if (SrcDepthRange == xiiClipSpaceDepthRange::MinusOneToOne /*&& DstDepthRange == xiiClipSpaceDepthRange::ZeroToOne*/)
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


  inout_Matrix.SetRow(2, row2);
  inout_Matrix.SetRow(3, row3);
}

void xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const xiiMat4& ProjectionMatrix, xiiAngle& out_fFovX, xiiAngle& out_fFovY)
{

  const xiiVec3 row0 = ProjectionMatrix.GetRow(0).GetAsVec3();
  const xiiVec3 row1 = ProjectionMatrix.GetRow(1).GetAsVec3();
  const xiiVec3 row3 = ProjectionMatrix.GetRow(3).GetAsVec3();

  const xiiVec3 leftPlane   = (row3 + row0).GetNormalized();
  const xiiVec3 rightPlane  = (row3 - row0).GetNormalized();
  const xiiVec3 bottomPlane = (row3 + row1).GetNormalized();
  const xiiVec3 topPlane    = (row3 - row1).GetNormalized();

  out_fFovX = xiiAngle::Radian(xiiMath::Pi<float>()) - xiiMath::ACos(leftPlane.Dot(rightPlane));
  out_fFovY = xiiAngle::Radian(xiiMath::Pi<float>()) - xiiMath::ACos(topPlane.Dot(bottomPlane));
}

void xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const xiiMat4& ProjectionMatrix, xiiAngle& out_fFovLeft, xiiAngle& out_fFovRight, xiiAngle& out_fFovBottom, xiiAngle& out_fFovTop, xiiClipSpaceYMode::Enum yRange)
{
  const xiiVec3 row0 = ProjectionMatrix.GetRow(0).GetAsVec3();
  const xiiVec3 row1 = ProjectionMatrix.GetRow(1).GetAsVec3();
  const xiiVec3 row3 = ProjectionMatrix.GetRow(3).GetAsVec3();

  const xiiVec3 leftPlane   = (row3 + row0).GetNormalized();
  const xiiVec3 rightPlane  = (row3 - row0).GetNormalized();
  const xiiVec3 bottomPlane = (row3 + row1).GetNormalized();
  const xiiVec3 topPlane    = (row3 - row1).GetNormalized();

  out_fFovLeft   = -xiiMath::ACos(leftPlane.Dot(xiiVec3(1.0f, 0, 0)));
  out_fFovRight  = xiiAngle::Radian(xiiMath::Pi<float>()) - xiiMath::ACos(rightPlane.Dot(xiiVec3(1.0f, 0, 0)));
  out_fFovBottom = -xiiMath::ACos(bottomPlane.Dot(xiiVec3(0, 1.0f, 0)));
  out_fFovTop    = xiiAngle::Radian(xiiMath::Pi<float>()) - xiiMath::ACos(topPlane.Dot(xiiVec3(0, 1.0f, 0)));

  if (yRange == xiiClipSpaceYMode::Flipped)
    xiiMath::Swap(out_fFovBottom, out_fFovTop);
}

xiiResult xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const xiiMat4& ProjectionMatrix, float& out_fLeft, float& out_fRight, float& out_fBottom, float& out_fTop, xiiClipSpaceDepthRange::Enum DepthRange, xiiClipSpaceYMode::Enum yRange)
{
  float fNear, fFar;
  XII_SUCCEED_OR_RETURN(ExtractNearAndFarClipPlaneDistances(fNear, fFar, ProjectionMatrix, DepthRange));
  // Compensate for inverse-Z.
  const float fMinDepth = xiiMath::Min(fNear, fFar);

  xiiAngle fFovLeft;
  xiiAngle fFovRight;
  xiiAngle fFovBottom;
  xiiAngle fFovTop;
  ExtractPerspectiveMatrixFieldOfView(ProjectionMatrix, fFovLeft, fFovRight, fFovBottom, fFovTop, yRange);

  out_fLeft   = xiiMath::Tan(fFovLeft) * fMinDepth;
  out_fRight  = xiiMath::Tan(fFovRight) * fMinDepth;
  out_fBottom = xiiMath::Tan(fFovBottom) * fMinDepth;
  out_fTop    = xiiMath::Tan(fFovTop) * fMinDepth;
  return XII_SUCCESS;
}

xiiResult xiiGraphicsUtils::ExtractNearAndFarClipPlaneDistances(float& out_fNear, float& out_fFar, const xiiMat4& ProjectionMatrix, xiiClipSpaceDepthRange::Enum DepthRange)
{
  const xiiVec4 row2 = ProjectionMatrix.GetRow(2);
  const xiiVec4 row3 = ProjectionMatrix.GetRow(3);

  xiiVec4 nearPlane = row2;

  if (DepthRange == xiiClipSpaceDepthRange::MinusOneToOne)
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

xiiPlane xiiGraphicsUtils::ComputeInterpolatedFrustumPlane(FrustumPlaneInterpolation direction, float fLerpFactor, const xiiMat4& ProjectionMatrix, xiiClipSpaceDepthRange::Enum DepthRange)
{
  xiiVec4     rowA;
  xiiVec4     rowB            = ProjectionMatrix.GetRow(3);
  const float factorMinus1to1 = (fLerpFactor - 0.5f) * 2.0f; // bring into [-1; +1] range

  switch (direction)
  {
    case FrustumPlaneInterpolation::LeftToRight:
    {
      rowA = ProjectionMatrix.GetRow(0);
      rowB *= factorMinus1to1;
      break;
    }

    case FrustumPlaneInterpolation::BottomToTop:
    {
      rowA = ProjectionMatrix.GetRow(1);
      rowB *= factorMinus1to1;
      break;
    }

    case FrustumPlaneInterpolation::NearToFar:
      rowA = ProjectionMatrix.GetRow(2);

      if (DepthRange == xiiClipSpaceDepthRange::ZeroToOne)
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

xiiMat4 xiiGraphicsUtils::CreatePerspectiveProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum DepthRange, xiiClipSpaceYMode::Enum yRange, xiiHandedness::Enum handedness)
{
  const float vw = fViewWidth * 0.5f;
  const float vh = fViewHeight * 0.5f;

  return CreatePerspectiveProjectionMatrix(-vw, vw, -vh, vh, fNearZ, fFarZ, DepthRange, yRange, handedness);
}

xiiMat4 xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(xiiAngle fieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum DepthRange, xiiClipSpaceYMode::Enum yRange, xiiHandedness::Enum handedness)
{
  // Taking the minimum allows the function to be used to create
  // inverse z matrices (fNearZ > fFarZ) as well.
  const float xm = xiiMath::Min(fNearZ, fFarZ) * xiiMath::Tan(fieldOfViewX * 0.5f);
  const float ym = xm / fAspectRatioWidthDivHeight;

  return CreatePerspectiveProjectionMatrix(-xm, xm, -ym, ym, fNearZ, fFarZ, DepthRange, yRange, handedness);
}

xiiMat4 xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(xiiAngle fieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum DepthRange, xiiClipSpaceYMode::Enum yRange, xiiHandedness::Enum handedness)
{
  // Taking the minimum allows the function to be used to create
  // inverse z matrices (fNearZ > fFarZ) as well.
  const float ym = xiiMath::Min(fNearZ, fFarZ) * xiiMath::Tan(fieldOfViewY * 0.5);
  const float xm = ym * fAspectRatioWidthDivHeight;

  return CreatePerspectiveProjectionMatrix(-xm, xm, -ym, ym, fNearZ, fFarZ, DepthRange, yRange, handedness);
}

xiiMat4 xiiGraphicsUtils::CreateOrthographicProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange, xiiClipSpaceYMode::Enum yRange, xiiHandedness::Enum handedness)
{
  return CreateOrthographicProjectionMatrix(-fViewWidth * 0.5f, fViewWidth * 0.5f, -fViewHeight * 0.5f, fViewHeight * 0.5f, fNearZ, fFarZ, depthRange, yRange, handedness);
}

xiiMat4 xiiGraphicsUtils::CreateOrthographicProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum DepthRange, xiiClipSpaceYMode::Enum yRange, xiiHandedness::Enum handedness)
{
  XII_ASSERT_DEBUG(xiiMath::IsFinite(fNearZ) && xiiMath::IsFinite(fFarZ), "Infinite plane values are not supported for orthographic projections!");

  xiiMat4 res;
  res.SetIdentity();

  if (yRange == xiiClipSpaceYMode::Flipped)
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


  if (DepthRange == xiiClipSpaceDepthRange::MinusOneToOne)
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

xiiMat4 xiiGraphicsUtils::CreatePerspectiveProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum DepthRange, xiiClipSpaceYMode::Enum yRange, xiiHandedness::Enum handedness)
{
  XII_ASSERT_DEBUG(xiiMath::IsFinite(fNearZ) || xiiMath::IsFinite(fFarZ), "fNearZ and fFarZ cannot both be infinite at the same time!");

  xiiMat4 res;
  res.SetZero();

  if (yRange == xiiClipSpaceYMode::Flipped)
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
  if (DepthRange == xiiClipSpaceDepthRange::MinusOneToOne)
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

xiiMat4 xiiGraphicsUtils::CreateLookAtViewMatrix(const xiiVec3& vEyePos, const xiiVec3& vLookAtPos, const xiiVec3& vUpDir, xiiHandedness::Enum handedness)
{
  const xiiMat3 rotation = CreateLookAtViewMatrix(vLookAtPos - vEyePos, vUpDir, handedness);

  xiiMat4 res;
  res.SetRotationalPart(rotation);
  res.SetTranslationVector(rotation * -vEyePos);
  res.SetRow(3, xiiVec4(0, 0, 0, 1));
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

void xiiGraphicsUtils::DecomposeViewMatrix(xiiVec3& vPosition, xiiVec3& vForwardDir, xiiVec3& vRightDir, xiiVec3& vUpDir, const xiiMat4& viewMatrix, xiiHandedness::Enum handedness)
{
  const xiiMat3 rotation = viewMatrix.GetRotationalPart();

  if (handedness == xiiHandedness::LeftHanded)
  {
    vRightDir   = rotation.GetRow(0);
    vUpDir      = rotation.GetRow(1);
    vForwardDir = rotation.GetRow(2);
  }
  else
  {
    vRightDir   = rotation.GetRow(0);
    vUpDir      = rotation.GetRow(1);
    vForwardDir = -rotation.GetRow(2);
  }

  vPosition = rotation.GetTranspose() * -viewMatrix.GetTranslationVector();
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

XII_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_GraphicsUtils);
