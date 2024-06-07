#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>

XII_CREATE_SIMPLE_TEST(Utility, GraphicsUtils)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Perspective (-1/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    xiiMat4 mProj, mProjInv;

    mProj    = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(xiiAngle::MakeFromDegree(85.0f), 2.0f, 1.0f, 1000.0f, xiiClipSpaceDepthRange::MinusOneToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (xiiUInt32 y = 0; y < 25; ++y)
    {
      for (xiiUInt32 x = 0; x < 50; ++x)
      {
        xiiVec3 vPoint, vDir;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertScreenPosToWorldPos(mProjInv, 0, 0, 50, 25, xiiVec3((float)x, (float)y, 0.5f), vPoint, &vDir, xiiClipSpaceDepthRange::MinusOneToOne).Succeeded());

        XII_TEST_VEC3(vDir, vPoint.GetNormalized(), 0.01f);

        xiiVec3 vScreen;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, xiiClipSpaceDepthRange::MinusOneToOne).Succeeded());

        XII_TEST_VEC3(vScreen, xiiVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Perspective (-1/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos (Double)")
  {
    xiiMat4d mProj, mProjInv;

    mProj    = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixDoubleFromFovX(xiiAngled::MakeFromDegree(85.0), 2.0, 1.0, 1000.0, xiiClipSpaceDepthRange::MinusOneToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (xiiUInt32 y = 0; y < 25; ++y)
    {
      for (xiiUInt32 x = 0; x < 50; ++x)
      {
        xiiVec3d vPoint, vDir;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertScreenPosToWorldPos(mProjInv, 0, 0, 50, 25, xiiVec3d((double)x, (double)y, 0.5), vPoint, &vDir, xiiClipSpaceDepthRange::MinusOneToOne).Succeeded());

        XII_TEST_VEC3(vDir, vPoint.GetNormalized(), 0.01);

        xiiVec3d vScreen;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, xiiClipSpaceDepthRange::MinusOneToOne).Succeeded());

        XII_TEST_VEC3(vScreen, xiiVec3((double)x, (double)y, 0.5), 0.01);
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Perspective (0/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    xiiMat4 mProj, mProjInv;
    mProj    = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(xiiAngle::MakeFromDegree(85.0f), 2.0f, 1.0f, 1000.0f, xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (xiiUInt32 y = 0; y < 25; ++y)
    {
      for (xiiUInt32 x = 0; x < 50; ++x)
      {
        xiiVec3 vPoint, vDir;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertScreenPosToWorldPos(mProjInv, 0, 0, 50, 25, xiiVec3((float)x, (float)y, 0.5f), vPoint, &vDir, xiiClipSpaceDepthRange::ZeroToOne).Succeeded());

        XII_TEST_VEC3(vDir, vPoint.GetNormalized(), 0.01f);

        xiiVec3 vScreen;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, xiiClipSpaceDepthRange::ZeroToOne).Succeeded());

        XII_TEST_VEC3(vScreen, xiiVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Perspective (0/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos (Double)")
  {
    xiiMat4d mProj, mProjInv;
    mProj    = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixDoubleFromFovX(xiiAngled::MakeFromDegree(85.0), 2.0, 1.0, 1000.0, xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (xiiUInt32 y = 0; y < 25; ++y)
    {
      for (xiiUInt32 x = 0; x < 50; ++x)
      {
        xiiVec3d vPoint, vDir;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertScreenPosToWorldPos(mProjInv, 0, 0, 50, 25, xiiVec3d((double)x, (double)y, 0.5), vPoint, &vDir, xiiClipSpaceDepthRange::ZeroToOne).Succeeded());

        XII_TEST_VEC3(vDir, vPoint.GetNormalized(), 0.01);

        xiiVec3d vScreen;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, xiiClipSpaceDepthRange::ZeroToOne).Succeeded());

        XII_TEST_VEC3(vScreen, xiiVec3d((double)x, (double)y, 0.5), 0.01);
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Ortho (-1/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    xiiMat4 mProj, mProjInv;
    mProj = xiiGraphicsUtils::CreateOrthographicProjectionMatrix(50, 25, 1.0f, 1000.0f, xiiClipSpaceDepthRange::MinusOneToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);

    mProjInv = mProj.GetInverse();

    for (xiiUInt32 y = 0; y < 25; ++y)
    {
      for (xiiUInt32 x = 0; x < 50; ++x)
      {
        xiiVec3 vPoint, vDir;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertScreenPosToWorldPos(mProjInv, 0, 0, 50, 25, xiiVec3((float)x, (float)y, 0.5f), vPoint, &vDir, xiiClipSpaceDepthRange::MinusOneToOne).Succeeded());

        XII_TEST_VEC3(vDir, xiiVec3(0, 0, 1.0f), 0.01f);

        xiiVec3 vScreen;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, xiiClipSpaceDepthRange::MinusOneToOne).Succeeded());

        XII_TEST_VEC3(vScreen, xiiVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Ortho (-1/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos (Double)")
  {
    xiiMat4d mProj, mProjInv;
    mProj = xiiGraphicsUtils::CreateOrthographicProjectionMatrixDouble(50, 25, 1.0, 1000.0, xiiClipSpaceDepthRange::MinusOneToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);

    mProjInv = mProj.GetInverse();

    for (xiiUInt32 y = 0; y < 25; ++y)
    {
      for (xiiUInt32 x = 0; x < 50; ++x)
      {
        xiiVec3d vPoint, vDir;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertScreenPosToWorldPos(mProjInv, 0, 0, 50, 25, xiiVec3d((double)x, (double)y, 0.5), vPoint, &vDir, xiiClipSpaceDepthRange::MinusOneToOne).Succeeded());

        XII_TEST_VEC3(vDir, xiiVec3d(0, 0, 1.0), 0.01);

        xiiVec3d vScreen;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, xiiClipSpaceDepthRange::MinusOneToOne).Succeeded());

        XII_TEST_VEC3(vScreen, xiiVec3d((double)x, (double)y, 0.5), 0.01);
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Ortho (0/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    xiiMat4 mProj, mProjInv;
    mProj    = xiiGraphicsUtils::CreateOrthographicProjectionMatrix(50, 25, 1.0f, 1000.0f, xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (xiiUInt32 y = 0; y < 25; ++y)
    {
      for (xiiUInt32 x = 0; x < 50; ++x)
      {
        xiiVec3 vPoint, vDir;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertScreenPosToWorldPos(mProjInv, 0, 0, 50, 25, xiiVec3((float)x, (float)y, 0.5f), vPoint, &vDir, xiiClipSpaceDepthRange::ZeroToOne).Succeeded());

        XII_TEST_VEC3(vDir, xiiVec3(0, 0, 1.0f), 0.01f);

        xiiVec3 vScreen;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, xiiClipSpaceDepthRange::ZeroToOne).Succeeded());

        XII_TEST_VEC3(vScreen, xiiVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Ortho (0/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos (Double)")
  {
    xiiMat4d mProj, mProjInv;
    mProj    = xiiGraphicsUtils::CreateOrthographicProjectionMatrixDouble(50, 25, 1.0, 1000.0, xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (xiiUInt32 y = 0; y < 25; ++y)
    {
      for (xiiUInt32 x = 0; x < 50; ++x)
      {
        xiiVec3d vPoint, vDir;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertScreenPosToWorldPos(mProjInv, 0, 0, 50, 25, xiiVec3d((double)x, (double)y, 0.5), vPoint, &vDir, xiiClipSpaceDepthRange::ZeroToOne).Succeeded());

        XII_TEST_VEC3(vDir, xiiVec3d(0, 0, 1.0), 0.01);

        xiiVec3d vScreen;
        XII_TEST_BOOL(xiiGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, xiiClipSpaceDepthRange::ZeroToOne).Succeeded());

        XII_TEST_VEC3(vScreen, xiiVec3d((double)x, (double)y, 0.5), 0.01);
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ConvertProjectionMatrixDepthRange")
  {
    xiiMat4 mProj1, mProj2;
    mProj1 = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(xiiAngle::MakeFromDegree(85.0f), 2.0f, 1.0f, 1000.0f, xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
    mProj2 = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(xiiAngle::MakeFromDegree(85.0f), 2.0f, 1.0f, 1000.0f, xiiClipSpaceDepthRange::MinusOneToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);

    xiiMat4 mProj1b = mProj1;
    xiiMat4 mProj2b = mProj2;
    xiiGraphicsUtils::ConvertProjectionMatrixDepthRange(mProj1b, xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceDepthRange::MinusOneToOne);
    xiiGraphicsUtils::ConvertProjectionMatrixDepthRange(mProj2b, xiiClipSpaceDepthRange::MinusOneToOne, xiiClipSpaceDepthRange::ZeroToOne);

    XII_TEST_BOOL(mProj1.IsEqual(mProj2b, 0.001f));
    XII_TEST_BOOL(mProj2.IsEqual(mProj1b, 0.001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ConvertProjectionMatrixDepthRangeDouble")
  {
    xiiMat4d mProj1, mProj2;
    mProj1 = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixDoubleFromFovX(xiiAngled::MakeFromDegree(85.0), 2.0, 1.0, 1000.0, xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
    mProj2 = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixDoubleFromFovX(xiiAngled::MakeFromDegree(85.0), 2.0, 1.0, 1000.0, xiiClipSpaceDepthRange::MinusOneToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);

    xiiMat4d mProj1b = mProj1;
    xiiMat4d mProj2b = mProj2;
    xiiGraphicsUtils::ConvertProjectionMatrixDepthRange(mProj1b, xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceDepthRange::MinusOneToOne);
    xiiGraphicsUtils::ConvertProjectionMatrixDepthRange(mProj2b, xiiClipSpaceDepthRange::MinusOneToOne, xiiClipSpaceDepthRange::ZeroToOne);

    XII_TEST_BOOL(mProj1.IsEqual(mProj2b, 0.001));
    XII_TEST_BOOL(mProj2.IsEqual(mProj1b, 0.001));
  }

  struct DepthRange
  {
    float fNear = 0.0f;
    float fFar  = 0.0f;
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExtractPerspectiveMatrixFieldOfView")
  {
    DepthRange                   depthRanges[]     = {{1.0f, 1000.0f}, {1000.0f, 1.0f}, {0.5f, 20.0f}, {20.0f, 0.5f}};
    xiiClipSpaceDepthRange::Enum clipRanges[]      = {xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceDepthRange::MinusOneToOne};
    xiiHandedness::Enum          handednesses[]    = {xiiHandedness::LeftHanded, xiiHandedness::RightHanded};
    xiiClipSpaceYMode::Enum      clipSpaceYModes[] = {xiiClipSpaceYMode::Regular, xiiClipSpaceYMode::Flipped};

    for (auto clipSpaceYMode : clipSpaceYModes)
    {
      for (auto handedness : handednesses)
      {
        for (auto depthRange : depthRanges)
        {
          for (auto clipRange : clipRanges)
          {
            for (xiiUInt32 angle = 10; angle < 180; angle += 10)
            {
              {
                xiiMat4 mProj;
                mProj = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(xiiAngle::MakeFromDegree((float)angle), 2.0f, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                xiiAngle fovx, fovy;
                xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fovx, fovy);

                XII_TEST_FLOAT(fovx.GetDegree(), (float)angle, 0.5f);
              }

              {
                xiiMat4 mProj;
                mProj = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(xiiAngle::MakeFromDegree((float)angle), 1.0f / 3.0f, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                xiiAngle fovx, fovy;
                xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fovx, fovy);

                XII_TEST_FLOAT(fovy.GetDegree(), (float)angle, 0.5f);
              }

              {
                const float    fMinDepth = xiiMath::Min(depthRange.fNear, depthRange.fFar);
                const xiiAngle right     = xiiAngle::MakeFromDegree((float)angle) / 2.0f;
                const xiiAngle top       = xiiAngle::MakeFromDegree((float)angle) / 2.0f;
                const float    fLeft     = xiiMath::Tan(-right) * fMinDepth;
                const float    fRight    = xiiMath::Tan(right) * fMinDepth * 0.8f;
                const float    fBottom   = xiiMath::Tan(-top) * fMinDepth;
                const float    fTop      = xiiMath::Tan(top) * fMinDepth * 0.7f;

                xiiMat4 mProj;
                mProj = xiiGraphicsUtils::CreatePerspectiveProjectionMatrix(fLeft, fRight, fBottom, fTop, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                float fNearOut, fFarOut;
                XII_TEST_BOOL(xiiGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, clipRange).Succeeded());
                XII_TEST_FLOAT(depthRange.fNear, fNearOut, 0.1f);
                XII_TEST_FLOAT(depthRange.fFar, fFarOut, 0.1f);

                float fLeftOut, fRightOut, fBottomOut, fTopOut;
                XII_TEST_BOOL(xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fLeftOut, fRightOut, fBottomOut, fTopOut, clipRange, clipSpaceYMode).Succeeded());
                XII_TEST_FLOAT(fLeft, fLeftOut, xiiMath::LargeEpsilon<float>());
                XII_TEST_FLOAT(fRight, fRightOut, xiiMath::LargeEpsilon<float>());
                XII_TEST_FLOAT(fBottom, fBottomOut, xiiMath::LargeEpsilon<float>());
                XII_TEST_FLOAT(fTop, fTopOut, xiiMath::LargeEpsilon<float>());

                xiiAngle fFovLeft;
                xiiAngle fFovRight;
                xiiAngle fFovBottom;
                xiiAngle fFovTop;
                xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop, clipSpaceYMode);

                XII_TEST_FLOAT(fLeft, xiiMath::Tan(fFovLeft) * fMinDepth, xiiMath::LargeEpsilon<float>());
                XII_TEST_FLOAT(fRight, xiiMath::Tan(fFovRight) * fMinDepth, xiiMath::LargeEpsilon<float>());
                XII_TEST_FLOAT(fBottom, xiiMath::Tan(fFovBottom) * fMinDepth, xiiMath::LargeEpsilon<float>());
                XII_TEST_FLOAT(fTop, xiiMath::Tan(fFovTop) * fMinDepth, xiiMath::LargeEpsilon<float>());
              }
            }
          }
        }
      }
    }
  }

  struct DoubleDepthRange
  {
    double fNear = 0.0;
    double fFar  = 0.0;
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExtractPerspectiveMatrixFieldOfView (Double)")
  {
    DepthRange                   depthRanges[]     = {{1.0, 1000.0}, {1000.0, 1.0}, {0.5, 20.0}, {20.0, 0.5}};
    xiiClipSpaceDepthRange::Enum clipRanges[]      = {xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceDepthRange::MinusOneToOne};
    xiiHandedness::Enum          handednesses[]    = {xiiHandedness::LeftHanded, xiiHandedness::RightHanded};
    xiiClipSpaceYMode::Enum      clipSpaceYModes[] = {xiiClipSpaceYMode::Regular, xiiClipSpaceYMode::Flipped};

    for (auto clipSpaceYMode : clipSpaceYModes)
    {
      for (auto handedness : handednesses)
      {
        for (auto depthRange : depthRanges)
        {
          for (auto clipRange : clipRanges)
          {
            for (xiiUInt32 angle = 10; angle < 180; angle += 10)
            {
              {
                xiiMat4d mProj;
                mProj = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixDoubleFromFovX(xiiAngled::MakeFromDegree((double)angle), 2.0, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                xiiAngled fovx, fovy;
                xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fovx, fovy);

                XII_TEST_DOUBLE(fovx.GetDegree(), (double)angle, 0.5f);
              }

              {
                xiiMat4d mProj;
                mProj = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixDoubleFromFovY(xiiAngled::MakeFromDegree((double)angle), 1.0 / 3.0, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                xiiAngled fovx, fovy;
                xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fovx, fovy);

                XII_TEST_DOUBLE(fovy.GetDegree(), (double)angle, 0.5);
              }

              {
                const double    fMinDepth = xiiMath::Min(depthRange.fNear, depthRange.fFar);
                const xiiAngled right     = xiiAngled::MakeFromDegree((double)angle) / 2.0;
                const xiiAngled top       = xiiAngled::MakeFromDegree((double)angle) / 2.0;
                const double    fLeft     = xiiMath::Tan(-right) * fMinDepth;
                const double    fRight    = xiiMath::Tan(right) * fMinDepth * 0.8;
                const double    fBottom   = xiiMath::Tan(-top) * fMinDepth;
                const double    fTop      = xiiMath::Tan(top) * fMinDepth * 0.7;

                xiiMat4d mProj;
                mProj = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixDouble(fLeft, fRight, fBottom, fTop, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                double fNearOut, fFarOut;
                XII_TEST_BOOL(xiiGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, clipRange).Succeeded());
                XII_TEST_DOUBLE(depthRange.fNear, fNearOut, 0.1f);
                XII_TEST_DOUBLE(depthRange.fFar, fFarOut, 0.1f);

                double fLeftOut, fRightOut, fBottomOut, fTopOut;
                XII_TEST_BOOL(xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fLeftOut, fRightOut, fBottomOut, fTopOut, clipRange, clipSpaceYMode).Succeeded());
                XII_TEST_DOUBLE(fLeft, fLeftOut, xiiMath::LargeEpsilon<double>());
                XII_TEST_DOUBLE(fRight, fRightOut, xiiMath::LargeEpsilon<double>());
                XII_TEST_DOUBLE(fBottom, fBottomOut, xiiMath::LargeEpsilon<double>());
                XII_TEST_DOUBLE(fTop, fTopOut, xiiMath::LargeEpsilon<double>());

                xiiAngled fFovLeft;
                xiiAngled fFovRight;
                xiiAngled fFovBottom;
                xiiAngled fFovTop;
                xiiGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop, clipSpaceYMode);

                XII_TEST_DOUBLE(fLeft, xiiMath::Tan(fFovLeft) * fMinDepth, xiiMath::LargeEpsilon<double>());
                XII_TEST_DOUBLE(fRight, xiiMath::Tan(fFovRight) * fMinDepth, xiiMath::LargeEpsilon<double>());
                XII_TEST_DOUBLE(fBottom, xiiMath::Tan(fFovBottom) * fMinDepth, xiiMath::LargeEpsilon<double>());
                XII_TEST_DOUBLE(fTop, xiiMath::Tan(fFovTop) * fMinDepth, xiiMath::LargeEpsilon<double>());
              }
            }
          }
        }
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExtractNearAndFarClipPlaneDistances")
  {
    DepthRange                   depthRanges[]     = {{0.001f, 100.0f}, {0.01f, 10.0f}, {10.0f, 0.01f}, {1.01f, 110.0f}, {110.0f, 1.01f}};
    xiiClipSpaceDepthRange::Enum clipRanges[]      = {xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceDepthRange::MinusOneToOne};
    xiiHandedness::Enum          handednesses[]    = {xiiHandedness::LeftHanded, xiiHandedness::RightHanded};
    xiiClipSpaceYMode::Enum      clipSpaceYModes[] = {xiiClipSpaceYMode::Regular, xiiClipSpaceYMode::Flipped};
    xiiAngle                     fovs[]            = {xiiAngle::MakeFromDegree(10.0f), xiiAngle::MakeFromDegree(70.0f)};

    for (auto clipSpaceYMode : clipSpaceYModes)
    {
      for (auto handedness : handednesses)
      {
        for (auto depthRange : depthRanges)
        {
          for (auto clipRange : clipRanges)
          {
            for (auto fov : fovs)
            {
              xiiMat4 mProj;
              mProj = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(fov, 0.7f, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

              float fNearOut, fFarOut;
              XII_TEST_BOOL(xiiGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, clipRange).Succeeded());

              XII_TEST_FLOAT(depthRange.fNear, fNearOut, 0.1f);
              XII_TEST_FLOAT(depthRange.fFar, fFarOut, 0.2f);
            }
          }
        }
      }
    }

    { // Test failure on broken projection matrix
      // This matrix has a 0 in the w-component of the third column (invalid perspective divide)
      float   vals[] = {0.770734549f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, 1.73205078f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, -1.00000000f, 0.00000000f, 0.000000000, 0.000000000f, -0.100000001f, 0.000000000f};
      xiiMat4 mProj;
      memcpy(mProj.m_fElementsCM, vals, 16 * sizeof(float));
      float fNearOut = 0.f, fFarOut = 0.f;
      XII_TEST_BOOL(xiiGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, xiiClipSpaceDepthRange::MinusOneToOne).Failed());
      XII_TEST_BOOL(fNearOut == 0.0f);
      XII_TEST_BOOL(fFarOut == 0.0f);
    }

    { // Test failure on broken projection matrix
      // This matrix has a 0 in the z-component of the fourth column (one or both projection planes are zero)
      float   vals[] = {0.770734549f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, 1.73205078f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, -1.00000000f, -1.00000000f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f};
      xiiMat4 mProj;
      memcpy(mProj.m_fElementsCM, vals, 16 * sizeof(float));
      float fNearOut = 0.f, fFarOut = 0.f;
      XII_TEST_BOOL(xiiGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, xiiClipSpaceDepthRange::MinusOneToOne).Failed());
      XII_TEST_BOOL(fNearOut == 0.0f);
      XII_TEST_BOOL(fFarOut == 0.0f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExtractNearAndFarClipPlaneDistances (Double)")
  {
    DepthRange                   depthRanges[]     = {{0.001, 100.0}, {0.01, 10.0}, {10.0, 0.01}, {1.01, 110.0}, {110.0, 1.01}};
    xiiClipSpaceDepthRange::Enum clipRanges[]      = {xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceDepthRange::MinusOneToOne};
    xiiHandedness::Enum          handednesses[]    = {xiiHandedness::LeftHanded, xiiHandedness::RightHanded};
    xiiClipSpaceYMode::Enum      clipSpaceYModes[] = {xiiClipSpaceYMode::Regular, xiiClipSpaceYMode::Flipped};
    xiiAngled                    fovs[]            = {xiiAngled::MakeFromDegree(10.0), xiiAngled::MakeFromDegree(70.0)};

    for (auto clipSpaceYMode : clipSpaceYModes)
    {
      for (auto handedness : handednesses)
      {
        for (auto depthRange : depthRanges)
        {
          for (auto clipRange : clipRanges)
          {
            for (auto fov : fovs)
            {
              xiiMat4d mProj;
              mProj = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixDoubleFromFovY(fov, 0.7, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

              double fNearOut, fFarOut;
              XII_TEST_BOOL(xiiGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, clipRange).Succeeded());

              XII_TEST_DOUBLE(depthRange.fNear, fNearOut, 0.1);
              XII_TEST_DOUBLE(depthRange.fFar, fFarOut, 0.2);
            }
          }
        }
      }
    }

    {
      // Test failure on broken projection matrix
      // This matrix has a 0 in the w-component of the third column (invalid perspective divide)
      double   vals[] = {0.770734549, 0.000000000, 0.000000000, 0.000000000, 0.000000000, 1.73205078, 0.000000000, 0.000000000, 0.000000000, 0.000000000, -1.00000000, 0.00000000, 0.000000000, 0.000000000, -0.100000001, 0.000000000};
      xiiMat4d mProj;
      memcpy(mProj.m_fElementsCM, vals, 16 * sizeof(double));
      double fNearOut = 0.0, fFarOut = 0.0;
      XII_TEST_BOOL(xiiGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, xiiClipSpaceDepthRange::MinusOneToOne).Failed());
      XII_TEST_BOOL(fNearOut == 0.0);
      XII_TEST_BOOL(fFarOut == 0.0);
    }

    {
      // Test failure on broken projection matrix
      // This matrix has a 0 in the z-component of the fourth column (one or both projection planes are zero)
      double   vals[] = {0.770734549, 0.000000000, 0.000000000, 0.000000000, 0.000000000, 1.73205078, 0.000000000, 0.000000000, 0.000000000, 0.000000000, -1.00000000, -1.00000000, 0.000000000, 0.000000000, 0.000000000, 0.000000000};
      xiiMat4d mProj;
      memcpy(mProj.m_fElementsCM, vals, 16 * sizeof(double));
      double fNearOut = 0.0, fFarOut = 0.0;
      XII_TEST_BOOL(xiiGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, xiiClipSpaceDepthRange::MinusOneToOne).Failed());
      XII_TEST_BOOL(fNearOut == 0.0);
      XII_TEST_BOOL(fFarOut == 0.0);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ComputeInterpolatedFrustumPlane")
  {
    for (xiiUInt32 i = 0; i <= 10; ++i)
    {
      float nearPlane = 1.0f;
      float farPlane  = 1000.0f;

      xiiMat4 mProj;
      mProj = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(xiiAngle::MakeFromDegree(90.0f), 1.0f, nearPlane, farPlane, xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);

      const xiiPlane horz = xiiGraphicsUtils::ComputeInterpolatedFrustumPlane(xiiGraphicsUtils::FrustumPlaneInterpolation::LeftToRight, i * 0.1f, mProj, xiiClipSpaceDepthRange::ZeroToOne);
      const xiiPlane vert = xiiGraphicsUtils::ComputeInterpolatedFrustumPlane(xiiGraphicsUtils::FrustumPlaneInterpolation::BottomToTop, i * 0.1f, mProj, xiiClipSpaceDepthRange::ZeroToOne);
      const xiiPlane forw = xiiGraphicsUtils::ComputeInterpolatedFrustumPlane(xiiGraphicsUtils::FrustumPlaneInterpolation::NearToFar, i * 0.1f, mProj, xiiClipSpaceDepthRange::ZeroToOne);

      // Generate clip space point at intersection of the 3 planes and project to worldspace
      xiiVec4 clipSpacePoint = xiiVec4(0.1f * i * 2 - 1, 0.1f * i * 2 - 1, 0.1f * i, 1);

      xiiVec4 worldSpacePoint = mProj.GetInverse() * clipSpacePoint;
      worldSpacePoint /= worldSpacePoint.w;

      XII_TEST_FLOAT(horz.GetDistanceTo(xiiVec3::MakeZero()), 0.0f, 0.01f);
      XII_TEST_FLOAT(vert.GetDistanceTo(xiiVec3::MakeZero()), 0.0f, 0.01f);

      if (i == 0)
      {
        XII_TEST_FLOAT(forw.GetDistanceTo(xiiVec3::MakeZero()), -nearPlane, 0.01f);
      }
      else if (i == 10)
      {
        XII_TEST_FLOAT(forw.GetDistanceTo(xiiVec3::MakeZero()), -farPlane, 0.01f);
      }

      XII_TEST_FLOAT(horz.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0f, 0.02);
      XII_TEST_FLOAT(vert.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0f, 0.02f);
      XII_TEST_FLOAT(forw.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0f, 0.02f);

      // this isn't interpolated linearly across the angle (rotated), so the epsilon has to be very large (just an approx test)
      XII_TEST_FLOAT(horz.m_vNormal.GetAngleBetween(xiiVec3(1, 0, 0)).GetDegree(), xiiMath::Abs(-45.0f + 90.0f * i * 0.1f), 4.0f);
      XII_TEST_FLOAT(vert.m_vNormal.GetAngleBetween(xiiVec3(0, 1, 0)).GetDegree(), xiiMath::Abs(-45.0f + 90.0f * i * 0.1f), 4.0f);
      XII_TEST_VEC3(forw.m_vNormal, xiiVec3(0, 0, 1), 0.01f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ComputeInterpolatedFrustumPlane (Double)")
  {
    for (xiiUInt32 i = 0; i <= 10; ++i)
    {
      double nearPlane = 1.0;
      double farPlane  = 1000.0;

      xiiMat4d mProj;
      mProj = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixDoubleFromFovY(xiiAngled::MakeFromDegree(90.0), 1.0, nearPlane, farPlane, xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);

      const xiiPlaned horz = xiiGraphicsUtils::ComputeInterpolatedFrustumPlane(xiiGraphicsUtils::FrustumPlaneInterpolation::LeftToRight, i * 0.1, mProj, xiiClipSpaceDepthRange::ZeroToOne);
      const xiiPlaned vert = xiiGraphicsUtils::ComputeInterpolatedFrustumPlane(xiiGraphicsUtils::FrustumPlaneInterpolation::BottomToTop, i * 0.1, mProj, xiiClipSpaceDepthRange::ZeroToOne);
      const xiiPlaned forw = xiiGraphicsUtils::ComputeInterpolatedFrustumPlane(xiiGraphicsUtils::FrustumPlaneInterpolation::NearToFar, i * 0.1, mProj, xiiClipSpaceDepthRange::ZeroToOne);

      // Generate clip space point at intersection of the 3 planes and project to worldspace
      xiiVec4d clipSpacePoint = xiiVec4d(0.1 * i * 2 - 1, 0.1 * i * 2 - 1, 0.1 * i, 1);

      xiiVec4d worldSpacePoint = mProj.GetInverse() * clipSpacePoint;
      worldSpacePoint /= worldSpacePoint.w;

      XII_TEST_DOUBLE(horz.GetDistanceTo(xiiVec3d::MakeZero()), 0.0, 0.01);
      XII_TEST_DOUBLE(vert.GetDistanceTo(xiiVec3d::MakeZero()), 0.0, 0.01);

      if (i == 0)
      {
        XII_TEST_DOUBLE(forw.GetDistanceTo(xiiVec3d::MakeZero()), -nearPlane, 0.01);
      }
      else if (i == 10)
      {
        XII_TEST_DOUBLE(forw.GetDistanceTo(xiiVec3d::MakeZero()), -farPlane, 0.01);
      }

      XII_TEST_DOUBLE(horz.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0, 0.02);
      XII_TEST_DOUBLE(vert.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0, 0.02);
      XII_TEST_DOUBLE(forw.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0, 0.02);

      // this isn't interpolated linearly across the angle (rotated), so the epsilon has to be very large (just an approx test)
      XII_TEST_DOUBLE(horz.m_vNormal.GetAngleBetween(xiiVec3d(1, 0, 0)).GetDegree(), xiiMath::Abs(-45.0f + 90.0 * i * 0.1), 4.0);
      XII_TEST_DOUBLE(vert.m_vNormal.GetAngleBetween(xiiVec3d(0, 1, 0)).GetDegree(), xiiMath::Abs(-45.0f + 90.0 * i * 0.1), 4.0);
      XII_TEST_VEC3(forw.m_vNormal, xiiVec3d(0, 0, 1), 0.01);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CreateLookAtViewMatrix / CreateInverseLookAtViewMatrix")
  {
    for (int h = 0; h < 2; ++h)
    {
      const xiiHandedness::Enum handedness = (h == 0) ? xiiHandedness::LeftHanded : xiiHandedness::RightHanded;

      {
        xiiMat3 mLook3    = xiiGraphicsUtils::CreateLookAtViewMatrix(xiiVec3(1, 0, 0), xiiVec3(0, 0, 1), handedness);
        xiiMat3 mLookInv3 = xiiGraphicsUtils::CreateInverseLookAtViewMatrix(xiiVec3(1, 0, 0), xiiVec3(0, 0, 1), handedness);

        XII_TEST_BOOL((mLook3 * mLookInv3).IsIdentity(0.01f));

        xiiMat4 mLook4    = xiiGraphicsUtils::CreateLookAtViewMatrix(xiiVec3(0), xiiVec3(1, 0, 0), xiiVec3(0, 0, 1), handedness);
        xiiMat4 mLookInv4 = xiiGraphicsUtils::CreateInverseLookAtViewMatrix(xiiVec3(0), xiiVec3(1, 0, 0), xiiVec3(0, 0, 1), handedness);

        XII_TEST_BOOL((mLook4 * mLookInv4).IsIdentity(0.01f));

        XII_TEST_BOOL(mLook3.IsEqual(mLook4.GetRotationalPart(), 0.01f));
        XII_TEST_BOOL(mLookInv3.IsEqual(mLookInv4.GetRotationalPart(), 0.01f));
      }

      {
        xiiMat4 mLook4    = xiiGraphicsUtils::CreateLookAtViewMatrix(xiiVec3(1, 2, 0), xiiVec3(4, 5, 0), xiiVec3(0, 0, 1), handedness);
        xiiMat4 mLookInv4 = xiiGraphicsUtils::CreateInverseLookAtViewMatrix(xiiVec3(1, 2, 0), xiiVec3(4, 5, 0), xiiVec3(0, 0, 1), handedness);

        XII_TEST_BOOL((mLook4 * mLookInv4).IsIdentity(0.01f));
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CreateLookAtViewMatrix / CreateInverseLookAtViewMatrix (Double)")
  {
    for (int h = 0; h < 2; ++h)
    {
      const xiiHandedness::Enum handedness = (h == 0) ? xiiHandedness::LeftHanded : xiiHandedness::RightHanded;

      {
        xiiMat3d mLook3    = xiiGraphicsUtils::CreateLookAtViewMatrix(xiiVec3d(1, 0, 0), xiiVec3d(0, 0, 1), handedness);
        xiiMat3d mLookInv3 = xiiGraphicsUtils::CreateInverseLookAtViewMatrix(xiiVec3d(1, 0, 0), xiiVec3d(0, 0, 1), handedness);

        XII_TEST_BOOL((mLook3 * mLookInv3).IsIdentity(0.01));

        xiiMat4d mLook4    = xiiGraphicsUtils::CreateLookAtViewMatrix(xiiVec3d(0), xiiVec3d(1, 0, 0), xiiVec3d(0, 0, 1), handedness);
        xiiMat4d mLookInv4 = xiiGraphicsUtils::CreateInverseLookAtViewMatrix(xiiVec3d(0), xiiVec3d(1, 0, 0), xiiVec3d(0, 0, 1), handedness);

        XII_TEST_BOOL((mLook4 * mLookInv4).IsIdentity(0.01));

        XII_TEST_BOOL(mLook3.IsEqual(mLook4.GetRotationalPart(), 0.01));
        XII_TEST_BOOL(mLookInv3.IsEqual(mLookInv4.GetRotationalPart(), 0.01));
      }

      {
        xiiMat4d mLook4    = xiiGraphicsUtils::CreateLookAtViewMatrix(xiiVec3d(1, 2, 0), xiiVec3d(4, 5, 0), xiiVec3d(0, 0, 1), handedness);
        xiiMat4d mLookInv4 = xiiGraphicsUtils::CreateInverseLookAtViewMatrix(xiiVec3d(1, 2, 0), xiiVec3d(4, 5, 0), xiiVec3d(0, 0, 1), handedness);

        XII_TEST_BOOL((mLook4 * mLookInv4).IsIdentity(0.01));
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CreateViewMatrix / DecomposeViewMatrix / CreateInverseViewMatrix")
  {
    for (int h = 0; h < 2; ++h)
    {
      const xiiHandedness::Enum handedness = (h == 0) ? xiiHandedness::LeftHanded : xiiHandedness::RightHanded;

      const xiiVec3 vEye(0);
      const xiiVec3 vTarget(0, 0, 1);
      const xiiVec3 vUp0(0, 1, 0);
      const xiiVec3 vFwd   = (vTarget - vEye).GetNormalized();
      xiiVec3       vRight = vUp0.CrossRH(vFwd).GetNormalized();
      const xiiVec3 vUp    = vFwd.CrossRH(vRight).GetNormalized();

      if (handedness == xiiHandedness::RightHanded)
        vRight = -vRight;

      const xiiMat4 mLookAt = xiiGraphicsUtils::CreateLookAtViewMatrix(vEye, vTarget, vUp0, handedness);

      xiiVec3 decFwd, decRight, decUp, decPos;
      xiiGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, mLookAt, handedness);

      XII_TEST_VEC3(decPos, vEye, 0.01f);
      XII_TEST_VEC3(decFwd, vFwd, 0.01f);
      XII_TEST_VEC3(decUp, vUp, 0.01f);
      XII_TEST_VEC3(decRight, vRight, 0.01f);

      const xiiMat4 mView    = xiiGraphicsUtils::CreateViewMatrix(decPos, decFwd, decRight, decUp, handedness);
      const xiiMat4 mViewInv = xiiGraphicsUtils::CreateInverseViewMatrix(decPos, decFwd, decRight, decUp, handedness);

      XII_TEST_BOOL(mLookAt.IsEqual(mView, 0.01f));

      XII_TEST_BOOL((mLookAt * mViewInv).IsIdentity());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CreateViewMatrix / DecomposeViewMatrix / CreateInverseViewMatrix (Double)")
  {
    for (int h = 0; h < 2; ++h)
    {
      const xiiHandedness::Enum handedness = (h == 0) ? xiiHandedness::LeftHanded : xiiHandedness::RightHanded;

      const xiiVec3d vEye(0);
      const xiiVec3d vTarget(0, 0, 1);
      const xiiVec3d vUp0(0, 1, 0);
      const xiiVec3d vFwd   = (vTarget - vEye).GetNormalized();
      xiiVec3d       vRight = vUp0.CrossRH(vFwd).GetNormalized();
      const xiiVec3d vUp    = vFwd.CrossRH(vRight).GetNormalized();

      if (handedness == xiiHandedness::RightHanded)
        vRight = -vRight;

      const xiiMat4d mLookAt = xiiGraphicsUtils::CreateLookAtViewMatrix(vEye, vTarget, vUp0, handedness);

      xiiVec3d decFwd, decRight, decUp, decPos;
      xiiGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, mLookAt, handedness);

      XII_TEST_VEC3(decPos, vEye, 0.01);
      XII_TEST_VEC3(decFwd, vFwd, 0.01);
      XII_TEST_VEC3(decUp, vUp, 0.01);
      XII_TEST_VEC3(decRight, vRight, 0.01);

      const xiiMat4d mView    = xiiGraphicsUtils::CreateViewMatrix(decPos, decFwd, decRight, decUp, handedness);
      const xiiMat4d mViewInv = xiiGraphicsUtils::CreateInverseViewMatrix(decPos, decFwd, decRight, decUp, handedness);

      XII_TEST_BOOL(mLookAt.IsEqual(mView, 0.01));

      XII_TEST_BOOL((mLookAt * mViewInv).IsIdentity());
    }
  }
}
