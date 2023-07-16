#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Frustum.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Utilities/GraphicsUtils.h>

XII_CREATE_SIMPLE_TEST(Math, Frustum)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFrustum (planes)")
  {
    xiiFrustum f;

    xiiPlane p[6];
    p[0].SetFromNormalAndPoint(xiiVec3(1, 0, 0), xiiVec3(1, 2, 3));
    p[1].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));
    p[2].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));
    p[3].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));
    p[4].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));
    p[5].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));

    f.SetFrustum(p);

    XII_TEST_BOOL(f.GetPlane(0) == p[0]);
    XII_TEST_BOOL(f.GetPlane(1) == p[1]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformFrustum")
  {
    xiiFrustum f;

    xiiPlane p[6];
    p[0].SetFromNormalAndPoint(xiiVec3(1, 0, 0), xiiVec3(1, 2, 3));
    p[1].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));
    p[2].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));
    p[3].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));
    p[4].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));
    p[5].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));

    f.SetFrustum(p);

    xiiMat4 mTransform;
    mTransform.SetRotationMatrixY(xiiAngle::Degree(90.0f));
    mTransform.SetTranslationVector(xiiVec3(2, 3, 4));

    f.TransformFrustum(mTransform);

    p[0].Transform(mTransform);
    p[1].Transform(mTransform);

    XII_TEST_BOOL(f.GetPlane(0).IsEqual(p[0], 0.001f));
    XII_TEST_BOOL(f.GetPlane(1).IsEqual(p[1], 0.001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "InvertFrustum")
  {
    xiiFrustum f;

    xiiPlane p[6];
    p[0].SetFromNormalAndPoint(xiiVec3(1, 0, 0), xiiVec3(1, 2, 3));
    p[1].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));
    p[2].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));
    p[3].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));
    p[4].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));
    p[5].SetFromNormalAndPoint(xiiVec3(0, 1, 0), xiiVec3(2, 3, 4));

    f.SetFrustum(p);

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
        vLookDir.Set(xiiMath::Sin(xiiAngle::Degree((float)rot)), 0, -xiiMath::Cos(xiiAngle::Degree((float)rot)));

        xiiVec3 vRightDir;
        vRightDir.Set(xiiMath::Sin(xiiAngle::Degree(rot + 90.0f)), 0, -xiiMath::Cos(xiiAngle::Degree(rot + 90.0f)));

        const xiiVec3 vCamPos(rot * 1.0f, rot * 0.5f, rot * -0.3f);

        // const xiiMat4 mViewLH = xiiGraphicsUtils::CreateViewMatrix(vCamPos, vLookDir, -vRightDir, xiiVec3(0, 1, 0), xiiHandedness::LeftHanded);
        // const xiiMat4 mViewRH = xiiGraphicsUtils::CreateViewMatrix(vCamPos, vLookDir, vRightDir, xiiVec3(0, 1, 0), xiiHandedness::RightHanded);
        const xiiMat4 mViewLH = xiiGraphicsUtils::CreateLookAtViewMatrix(vCamPos, vCamPos + vLookDir, xiiVec3(0, 1, 0), xiiHandedness::LeftHanded);
        const xiiMat4 mViewRH = xiiGraphicsUtils::CreateLookAtViewMatrix(vCamPos, vCamPos + vLookDir, xiiVec3(0, 1, 0), xiiHandedness::RightHanded);

        const xiiMat4 mProjLH = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
          xiiAngle::Degree(90), 1.0f, 1.0f, 100.0f, range, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
        const xiiMat4 mProjRH = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
          xiiAngle::Degree(90), 1.0f, 1.0f, 100.0f, range, xiiClipSpaceYMode::Regular, xiiHandedness::RightHanded);

        const xiiMat4 mViewProjLH = mProjLH * mViewLH;
        const xiiMat4 mViewProjRH = mProjRH * mViewRH;

        xiiFrustum fLH, fRH, fB;
        fLH.SetFrustum(mViewProjLH, range, xiiHandedness::LeftHanded);
        fRH.SetFrustum(mViewProjRH, range, xiiHandedness::RightHanded);

        fB.SetFrustum(vCamPos, vLookDir, xiiVec3(0, 1, 0), xiiAngle::Degree(90), xiiAngle::Degree(90), 1.0f, 100.0f);

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
      fDir.SetFrustum(
        offsetPos, camDir[dir], camDir[dir].GetOrthogonalVector() /*arbitrary*/, xiiAngle::Degree(90), xiiAngle::Degree(90), 1.0f, 100.0f);

      for (xiiUInt32 obj = 0; obj < 6; ++obj)
      {
        // box
        {
          xiiBoundingBox boundingObj;
          boundingObj.SetCenterAndHalfExtents(offsetPos + objPos[obj], xiiVec3(1.0f));

          const xiiVolumePosition::Enum res = fDir.GetObjectPosition(boundingObj);

          if (obj == dir)
            XII_TEST_BOOL(res == xiiVolumePosition::Inside);
          else
            XII_TEST_BOOL(res == xiiVolumePosition::Outside);
        }

        // sphere
        {
          xiiBoundingSphere boundingObj;
          boundingObj.SetElements(offsetPos + objPos[obj], 0.93f);

          const xiiVolumePosition::Enum res = fDir.GetObjectPosition(boundingObj);

          if (obj == dir)
            XII_TEST_BOOL(res == xiiVolumePosition::Inside);
          else
            XII_TEST_BOOL(res == xiiVolumePosition::Outside);
        }

        // vertices
        {
          xiiBoundingBox boundingObj;
          boundingObj.SetCenterAndHalfExtents(offsetPos + objPos[obj], xiiVec3(1.0f));

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
          boundingObj.SetCenterAndHalfExtents(objPos[obj], xiiVec3(1.0f));

          xiiVec3 vertices[8];
          boundingObj.GetCorners(vertices);

          xiiMat4 transform;
          transform.SetTranslationMatrix(offsetPos);

          const xiiVolumePosition::Enum res = fDir.GetObjectPosition(vertices, 8, transform);

          if (obj == dir)
            XII_TEST_BOOL(res == xiiVolumePosition::Inside);
          else
            XII_TEST_BOOL(res == xiiVolumePosition::Outside);
        }

        // SIMD box
        {
          xiiBoundingBox boundingObj;
          boundingObj.SetCenterAndHalfExtents(offsetPos + objPos[obj], xiiVec3(1.0f));

          const bool res = fDir.Overlaps(xiiSimdConversion::ToBBox(boundingObj));

          if (obj == dir)
            XII_TEST_BOOL(res == true);
          else
            XII_TEST_BOOL(res == false);
        }

        // SIMD sphere
        {
          xiiBoundingSphere boundingObj;
          boundingObj.SetElements(offsetPos + objPos[obj], 0.93f);

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
    const xiiMat4 mProj = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
      xiiAngle::Degree(90), 1.0f, 1.0f, 10.0f, xiiClipSpaceDepthRange::MinusOneToOne, xiiClipSpaceYMode::Regular, xiiHandedness::RightHanded);

    xiiFrustum frustum[2];
    frustum[0].SetFrustum(mProj, xiiClipSpaceDepthRange::MinusOneToOne, xiiHandedness::RightHanded);
    frustum[1].SetFrustum(xiiVec3::ZeroVector(), xiiVec3(0, 0, -1), xiiVec3(0, 1, 0), xiiAngle::Degree(90), xiiAngle::Degree(90), 1.0f, 10.0f);

    for (int f = 0; f < 2; ++f)
    {
      xiiVec3 corner[8];
      frustum[f].ComputeCornerPoints(corner);

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
}
