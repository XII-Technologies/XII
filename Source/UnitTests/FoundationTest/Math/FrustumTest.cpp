#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Frustum.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Utilities/GraphicsUtils.h>

XII_CREATE_SIMPLE_TEST(Math, Frustum)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeFromPlanes")
  {
    xiiFrustum f;

    xiiPlane p[6];
    p[xiiFrustum::PlaneType::LeftPlane]   = xiiPlane::MakeFromNormalAndPoint(xiiVec3(-1, 0, 0), xiiVec3(-2, 0, 0));
    p[xiiFrustum::PlaneType::RightPlane]  = xiiPlane::MakeFromNormalAndPoint(xiiVec3(+1, 0, 0), xiiVec3(+2, 0, 0));
    p[xiiFrustum::PlaneType::BottomPlane] = xiiPlane::MakeFromNormalAndPoint(xiiVec3(0, -1, 0), xiiVec3(0, -2, 0));
    p[xiiFrustum::PlaneType::TopPlane]    = xiiPlane::MakeFromNormalAndPoint(xiiVec3(0, +1, 0), xiiVec3(0, +2, 0));
    p[xiiFrustum::PlaneType::NearPlane]   = xiiPlane::MakeFromNormalAndPoint(xiiVec3(0, 0, -1), xiiVec3(0, 0, 0));
    p[xiiFrustum::PlaneType::FarPlane]    = xiiPlane::MakeFromNormalAndPoint(xiiVec3(0, 0, 1), xiiVec3(0, 0, 100));

    f = xiiFrustum::MakeFromPlanes(p);

    XII_TEST_BOOL(f.GetPlane(0) == p[0]);
    XII_TEST_BOOL(f.GetPlane(1) == p[1]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformFrustum/GetTransformedFrustum")
  {
    xiiFrustum f;

    xiiPlane p[6];
    p[xiiFrustum::PlaneType::LeftPlane]   = xiiPlane::MakeFromNormalAndPoint(xiiVec3(-1, 0, 0), xiiVec3(-2, 0, 0));
    p[xiiFrustum::PlaneType::RightPlane]  = xiiPlane::MakeFromNormalAndPoint(xiiVec3(+1, 0, 0), xiiVec3(+2, 0, 0));
    p[xiiFrustum::PlaneType::BottomPlane] = xiiPlane::MakeFromNormalAndPoint(xiiVec3(0, -1, 0), xiiVec3(0, -2, 0));
    p[xiiFrustum::PlaneType::TopPlane]    = xiiPlane::MakeFromNormalAndPoint(xiiVec3(0, +1, 0), xiiVec3(0, +2, 0));
    p[xiiFrustum::PlaneType::NearPlane]   = xiiPlane::MakeFromNormalAndPoint(xiiVec3(0, 0, -1), xiiVec3(0, 0, 0));
    p[xiiFrustum::PlaneType::FarPlane]    = xiiPlane::MakeFromNormalAndPoint(xiiVec3(0, 0, 1), xiiVec3(0, 0, 100));

    f = xiiFrustum::MakeFromPlanes(p);

    xiiMat4 mTransform;
    mTransform = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(90.0f));
    mTransform.SetTranslationVector(xiiVec3(2, 3, 4));

    xiiFrustum tf = f;
    tf.TransformFrustum(mTransform);

    p[0].Transform(mTransform);
    p[1].Transform(mTransform);

    for (int planeIndex = 0; planeIndex < 6; ++planeIndex)
    {
      XII_TEST_BOOL(f.GetTransformedFrustum(mTransform).GetPlane(planeIndex) == tf.GetPlane(planeIndex));
    }

    XII_TEST_BOOL(tf.GetPlane(0).IsEqual(p[0], 0.001f));
    XII_TEST_BOOL(tf.GetPlane(1).IsEqual(p[1], 0.001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "InvertFrustum")
  {
    xiiFrustum f;

    xiiPlane p[6];
    p[xiiFrustum::PlaneType::LeftPlane]   = xiiPlane::MakeFromNormalAndPoint(xiiVec3(-1, 0, 0), xiiVec3(-2, 0, 0));
    p[xiiFrustum::PlaneType::RightPlane]  = xiiPlane::MakeFromNormalAndPoint(xiiVec3(+1, 0, 0), xiiVec3(+2, 0, 0));
    p[xiiFrustum::PlaneType::BottomPlane] = xiiPlane::MakeFromNormalAndPoint(xiiVec3(0, -1, 0), xiiVec3(0, -2, 0));
    p[xiiFrustum::PlaneType::TopPlane]    = xiiPlane::MakeFromNormalAndPoint(xiiVec3(0, +1, 0), xiiVec3(0, +2, 0));
    p[xiiFrustum::PlaneType::NearPlane]   = xiiPlane::MakeFromNormalAndPoint(xiiVec3(0, 0, -1), xiiVec3(0, 0, 0));
    p[xiiFrustum::PlaneType::FarPlane]    = xiiPlane::MakeFromNormalAndPoint(xiiVec3(0, 0, 1), xiiVec3(0, 0, 100));

    f = xiiFrustum::MakeFromPlanes(p);

    f.InvertFrustum();

    p[0].Flip();
    p[1].Flip();

    XII_TEST_BOOL(f.GetPlane(0) == p[0]);
    XII_TEST_BOOL(f.GetPlane(1) == p[1]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFrustum")
  {
    // check that the extracted frustum planes are always the same, no matter the handedness or depth-range

    // test the different depth ranges
    for (int r = 0; r < 2; ++r)
    {
      const xiiClipSpaceDepthRange::Enum range = (r == 0) ? xiiClipSpaceDepthRange::MinusOneToOne : xiiClipSpaceDepthRange::ZeroToOne;

      // test rotated model-view matrices
      for (int rot = 0; rot < 360; rot += 45)
      {
        xiiVec3 vLookDir;
        vLookDir.Set(xiiMath::Sin(xiiAngle::MakeFromDegree((float)rot)), 0, -xiiMath::Cos(xiiAngle::MakeFromDegree((float)rot)));

        xiiVec3 vRightDir;
        vRightDir.Set(xiiMath::Sin(xiiAngle::MakeFromDegree(rot + 90.0f)), 0, -xiiMath::Cos(xiiAngle::MakeFromDegree(rot + 90.0f)));

        const xiiVec3 vCamPos(rot * 1.0f, rot * 0.5f, rot * -0.3f);

        // const xiiMat4 mViewLH = xiiGraphicsUtils::CreateViewMatrix(vCamPos, vLookDir, -vRightDir, xiiVec3(0, 1, 0), xiiHandedness::LeftHanded);
        // const xiiMat4 mViewRH = xiiGraphicsUtils::CreateViewMatrix(vCamPos, vLookDir, vRightDir, xiiVec3(0, 1, 0), xiiHandedness::RightHanded);
        const xiiMat4 mViewLH = xiiGraphicsUtils::CreateLookAtViewMatrix(vCamPos, vCamPos + vLookDir, xiiVec3(0, 1, 0), xiiHandedness::LeftHanded);
        const xiiMat4 mViewRH = xiiGraphicsUtils::CreateLookAtViewMatrix(vCamPos, vCamPos + vLookDir, xiiVec3(0, 1, 0), xiiHandedness::RightHanded);

        const xiiMat4 mProjLH = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(xiiAngle::MakeFromDegree(90), 1.0f, 1.0f, 100.0f, range, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
        const xiiMat4 mProjRH = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(xiiAngle::MakeFromDegree(90), 1.0f, 1.0f, 100.0f, range, xiiClipSpaceYMode::Regular, xiiHandedness::RightHanded);

        const xiiMat4 mViewProjLH = mProjLH * mViewLH;
        const xiiMat4 mViewProjRH = mProjRH * mViewRH;

        xiiFrustum       fB;
        const xiiFrustum fLH = xiiFrustum::MakeFromMVP(mViewProjLH, range, xiiHandedness::LeftHanded);
        const xiiFrustum fRH = xiiFrustum::MakeFromMVP(mViewProjRH, range, xiiHandedness::RightHanded);

        fB = xiiFrustum::MakeFromFOV(vCamPos, vLookDir, xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(90), xiiAngle::MakeFromDegree(90), 1.0f, 100.0f);

        XII_TEST_BOOL(fRH.GetPlane(xiiFrustum::NearPlane).IsEqual(fB.GetPlane(xiiFrustum::NearPlane), 0.1f));
        XII_TEST_BOOL(fRH.GetPlane(xiiFrustum::LeftPlane).IsEqual(fB.GetPlane(xiiFrustum::LeftPlane), 0.1f));
        XII_TEST_BOOL(fRH.GetPlane(xiiFrustum::RightPlane).IsEqual(fB.GetPlane(xiiFrustum::RightPlane), 0.1f));
        XII_TEST_BOOL(fRH.GetPlane(xiiFrustum::FarPlane).IsEqual(fB.GetPlane(xiiFrustum::FarPlane), 0.1f));
        XII_TEST_BOOL(fRH.GetPlane(xiiFrustum::BottomPlane).IsEqual(fB.GetPlane(xiiFrustum::BottomPlane), 0.1f));
        XII_TEST_BOOL(fRH.GetPlane(xiiFrustum::TopPlane).IsEqual(fB.GetPlane(xiiFrustum::TopPlane), 0.1f));

        XII_TEST_BOOL(fLH.GetPlane(xiiFrustum::NearPlane).IsEqual(fB.GetPlane(xiiFrustum::NearPlane), 0.1f));
        XII_TEST_BOOL(fLH.GetPlane(xiiFrustum::LeftPlane).IsEqual(fB.GetPlane(xiiFrustum::LeftPlane), 0.1f));
        XII_TEST_BOOL(fLH.GetPlane(xiiFrustum::RightPlane).IsEqual(fB.GetPlane(xiiFrustum::RightPlane), 0.1f));
        XII_TEST_BOOL(fLH.GetPlane(xiiFrustum::FarPlane).IsEqual(fB.GetPlane(xiiFrustum::FarPlane), 0.1f));
        XII_TEST_BOOL(fLH.GetPlane(xiiFrustum::BottomPlane).IsEqual(fB.GetPlane(xiiFrustum::BottomPlane), 0.1f));
        XII_TEST_BOOL(fLH.GetPlane(xiiFrustum::TopPlane).IsEqual(fB.GetPlane(xiiFrustum::TopPlane), 0.1f));
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Culling")
  {
    const xiiVec3 offsetPos(23, 17, -9);
    const xiiVec3 camDir[6] = {xiiVec3(-1, 0, 0), xiiVec3(1, 0, 0), xiiVec3(0, -1, 0), xiiVec3(0, 1, 0), xiiVec3(0, 0, -1), xiiVec3(0, 0, 1)};
    const xiiVec3 objPos[6] = {xiiVec3(-9, 0, 0), xiiVec3(9, 0, 0), xiiVec3(0, -9, 0), xiiVec3(0, 9, 0), xiiVec3(0, 0, -9), xiiVec3(0, 0, 9)};

    for (xiiUInt32 dir = 0; dir < 6; ++dir)
    {
      xiiFrustum fDir;
      fDir = xiiFrustum::MakeFromFOV(offsetPos, camDir[dir], camDir[dir].GetOrthogonalVector() /*arbitrary*/, xiiAngle::MakeFromDegree(90), xiiAngle::MakeFromDegree(90), 1.0f, 100.0f);

      for (xiiUInt32 obj = 0; obj < 6; ++obj)
      {
        // box
        {
          xiiBoundingBox boundingObj;
          boundingObj = xiiBoundingBox::MakeFromCenterAndHalfExtents(offsetPos + objPos[obj], xiiVec3(1.0f));

          const xiiVolumePosition::Enum res = fDir.GetObjectPosition(boundingObj);

          if (obj == dir)
            XII_TEST_BOOL(res == xiiVolumePosition::Inside);
          else
            XII_TEST_BOOL(res == xiiVolumePosition::Outside);
        }

        // sphere
        {
          xiiBoundingSphere boundingObj = xiiBoundingSphere::MakeFromCenterAndRadius(offsetPos + objPos[obj], 0.93f);

          const xiiVolumePosition::Enum res = fDir.GetObjectPosition(boundingObj);

          if (obj == dir)
            XII_TEST_BOOL(res == xiiVolumePosition::Inside);
          else
            XII_TEST_BOOL(res == xiiVolumePosition::Outside);
        }

        // vertices
        {
          xiiBoundingBox boundingObj;
          boundingObj = xiiBoundingBox::MakeFromCenterAndHalfExtents(offsetPos + objPos[obj], xiiVec3(1.0f));

          xiiVec3 vertices[8];
          boundingObj.GetCorners(vertices);

          const xiiVolumePosition::Enum res = fDir.GetObjectPosition(vertices, 8);

          if (obj == dir)
            XII_TEST_BOOL(res == xiiVolumePosition::Inside);
          else
            XII_TEST_BOOL(res == xiiVolumePosition::Outside);
        }

        // vertices + transform
        {
          xiiBoundingBox boundingObj;
          boundingObj = xiiBoundingBox::MakeFromCenterAndHalfExtents(objPos[obj], xiiVec3(1.0f));

          xiiVec3 vertices[8];
          boundingObj.GetCorners(vertices);

          xiiMat4 transform = xiiMat4::MakeTranslation(offsetPos);

          const xiiVolumePosition::Enum res = fDir.GetObjectPosition(vertices, 8, transform);

          if (obj == dir)
            XII_TEST_BOOL(res == xiiVolumePosition::Inside);
          else
            XII_TEST_BOOL(res == xiiVolumePosition::Outside);
        }

        // SIMD box
        {
          xiiBoundingBox boundingObj;
          boundingObj = xiiBoundingBox::MakeFromCenterAndHalfExtents(offsetPos + objPos[obj], xiiVec3(1.0f));

          const bool res = fDir.Overlaps(xiiSimdConversion::ToBBox(boundingObj));

          if (obj == dir)
            XII_TEST_BOOL(res == true);
          else
            XII_TEST_BOOL(res == false);
        }

        // SIMD sphere
        {
          xiiBoundingSphere boundingObj = xiiBoundingSphere::MakeFromCenterAndRadius(offsetPos + objPos[obj], 0.93f);

          const bool res = fDir.Overlaps(xiiSimdConversion::ToBSphere(boundingObj));

          if (obj == dir)
            XII_TEST_BOOL(res == true);
          else
            XII_TEST_BOOL(res == false);
        }
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ComputeCornerPoints")
  {
    const xiiMat4 mProj = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(xiiAngle::MakeFromDegree(90), 1.0f, 1.0f, 10.0f, xiiClipSpaceDepthRange::MinusOneToOne, xiiClipSpaceYMode::Regular, xiiHandedness::RightHanded);

    xiiFrustum frustum[2];
    frustum[0] = xiiFrustum::MakeFromMVP(mProj, xiiClipSpaceDepthRange::MinusOneToOne, xiiHandedness::RightHanded);
    frustum[1] = xiiFrustum::MakeFromFOV(xiiVec3::MakeZero(), xiiVec3(0, 0, -1), xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(90), xiiAngle::MakeFromDegree(90), 1.0f, 10.0f);

    for (int f = 0; f < 2; ++f)
    {
      xiiVec3 corner[8];
      frustum[f].ComputeCornerPoints(corner).AssertSuccess();

      xiiPositionOnPlane::Enum results[8][6];

      for (int c = 0; c < 8; ++c)
      {
        for (int p = 0; p < 6; ++p)
        {
          results[c][p] = xiiPositionOnPlane::Back;
        }
      }

      results[xiiFrustum::FrustumCorner::NearTopLeft][xiiFrustum::PlaneType::NearPlane] = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::NearTopLeft][xiiFrustum::PlaneType::TopPlane]  = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::NearTopLeft][xiiFrustum::PlaneType::LeftPlane] = xiiPositionOnPlane::OnPlane;

      results[xiiFrustum::FrustumCorner::NearTopRight][xiiFrustum::PlaneType::NearPlane]  = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::NearTopRight][xiiFrustum::PlaneType::TopPlane]   = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::NearTopRight][xiiFrustum::PlaneType::RightPlane] = xiiPositionOnPlane::OnPlane;

      results[xiiFrustum::FrustumCorner::NearBottomLeft][xiiFrustum::PlaneType::NearPlane]   = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::NearBottomLeft][xiiFrustum::PlaneType::BottomPlane] = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::NearBottomLeft][xiiFrustum::PlaneType::LeftPlane]   = xiiPositionOnPlane::OnPlane;

      results[xiiFrustum::FrustumCorner::NearBottomRight][xiiFrustum::PlaneType::NearPlane]   = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::NearBottomRight][xiiFrustum::PlaneType::BottomPlane] = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::NearBottomRight][xiiFrustum::PlaneType::RightPlane]  = xiiPositionOnPlane::OnPlane;

      results[xiiFrustum::FrustumCorner::FarTopLeft][xiiFrustum::PlaneType::FarPlane]  = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::FarTopLeft][xiiFrustum::PlaneType::TopPlane]  = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::FarTopLeft][xiiFrustum::PlaneType::LeftPlane] = xiiPositionOnPlane::OnPlane;

      results[xiiFrustum::FrustumCorner::FarTopRight][xiiFrustum::PlaneType::FarPlane]   = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::FarTopRight][xiiFrustum::PlaneType::TopPlane]   = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::FarTopRight][xiiFrustum::PlaneType::RightPlane] = xiiPositionOnPlane::OnPlane;

      results[xiiFrustum::FrustumCorner::FarBottomLeft][xiiFrustum::PlaneType::FarPlane]    = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::FarBottomLeft][xiiFrustum::PlaneType::BottomPlane] = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::FarBottomLeft][xiiFrustum::PlaneType::LeftPlane]   = xiiPositionOnPlane::OnPlane;

      results[xiiFrustum::FrustumCorner::FarBottomRight][xiiFrustum::PlaneType::FarPlane]    = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::FarBottomRight][xiiFrustum::PlaneType::BottomPlane] = xiiPositionOnPlane::OnPlane;
      results[xiiFrustum::FrustumCorner::FarBottomRight][xiiFrustum::PlaneType::RightPlane]  = xiiPositionOnPlane::OnPlane;

      for (int c = 0; c < 8; ++c)
      {
        xiiFrustum::FrustumCorner cornerName = (xiiFrustum::FrustumCorner)c;

        for (int p = 0; p < 6; ++p)
        {
          xiiFrustum::PlaneType planeName = (xiiFrustum::PlaneType)p;

          xiiPlane                 plane    = frustum[f].GetPlane(planeName);
          xiiPositionOnPlane::Enum expected = results[cornerName][planeName];
          xiiPositionOnPlane::Enum result   = plane.GetPointPosition(corner[cornerName], 0.1f);
          // float fDistToPlane = plane.GetDistanceTo(corner[cornerName]);
          XII_TEST_BOOL(result == expected);
        }
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeFromCorners")
  {
    const xiiFrustum fOrg = xiiFrustum::MakeFromFOV(xiiVec3(1, 2, 3), xiiVec3(1, 1, 0).GetNormalized(), xiiVec3(0, 0, 1).GetNormalized(), xiiAngle::MakeFromDegree(110), xiiAngle::MakeFromDegree(70), 0.1f, 100.0f);

    xiiVec3 corners[8];
    fOrg.ComputeCornerPoints(corners).AssertSuccess();

    const xiiFrustum fNew = xiiFrustum::MakeFromCorners(corners);

    for (xiiUInt32 i = 0; i < 6; ++i)
    {
      xiiPlane p1 = fOrg.GetPlane(i);
      xiiPlane p2 = fNew.GetPlane(i);

      XII_TEST_BOOL(p1.IsEqual(p2, xiiMath::LargeEpsilon<float>()));
    }

    xiiVec3 corners2[8];
    fNew.ComputeCornerPoints(corners2).AssertSuccess();

    for (xiiUInt32 i = 0; i < 8; ++i)
    {
      XII_TEST_BOOL(corners[i].IsEqual(corners2[i], 0.01f));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeFromMVPInfiniteFarPlane")
  {
    xiiMat4 perspective = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(xiiAngle::MakeFromDegree(90), 1.0f, xiiMath::Infinity<float>(), 100.0f, xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::RightHanded);

    auto frustum = xiiFrustum::MakeFromMVP(perspective);
    XII_TEST_BOOL(frustum.IsValid());
  }
}
