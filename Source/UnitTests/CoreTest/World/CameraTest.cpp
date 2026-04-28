/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>

XII_CREATE_SIMPLE_TEST(World, Camera)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "LookAt")
  {
    xiiCamera camera;

    camera.LookAt(xiiVec3(0, 0, 0), xiiVec3(1, 0, 0), xiiVec3(0, 0, 1));
    XII_TEST_VEC3(camera.GetPosition(), xiiVec3(0, 0, 0), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirForwards(), xiiVec3(1, 0, 0), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirRight(), xiiVec3(0, 1, 0), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirUp(), xiiVec3(0, 0, 1), xiiMath::DefaultEpsilon<float>());

    camera.LookAt(xiiVec3(0, 0, 0), xiiVec3(-1, 0, 0), xiiVec3(0, 0, 1));
    XII_TEST_VEC3(camera.GetPosition(), xiiVec3(0, 0, 0), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirForwards(), xiiVec3(-1, 0, 0), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirRight(), xiiVec3(0, -1, 0), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirUp(), xiiVec3(0, 0, 1), xiiMath::DefaultEpsilon<float>());

    camera.LookAt(xiiVec3(0, 0, 0), xiiVec3(0, 0, 1), xiiVec3(0, 1, 0));
    XII_TEST_VEC3(camera.GetPosition(), xiiVec3(0, 0, 0), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirForwards(), xiiVec3(0, 0, 1), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirRight(), xiiVec3(1, 0, 0), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirUp(), xiiVec3(0, 1, 0), xiiMath::DefaultEpsilon<float>());

    camera.LookAt(xiiVec3(0, 0, 0), xiiVec3(0, 0, -1), xiiVec3(0, 1, 0));
    XII_TEST_VEC3(camera.GetPosition(), xiiVec3(0, 0, 0), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirForwards(), xiiVec3(0, 0, -1), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirRight(), xiiVec3(-1, 0, 0), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirUp(), xiiVec3(0, 1, 0), xiiMath::DefaultEpsilon<float>());

    const xiiMat4 mLookAt = xiiGraphicsUtils::CreateLookAtViewMatrix(xiiVec3(2, 3, 4), xiiVec3(3, 3, 4), xiiVec3(0, 0, 1), xiiHandedness::LeftHanded);
    camera.SetViewMatrix(mLookAt);

    XII_TEST_VEC3(camera.GetPosition(), xiiVec3(2, 3, 4), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirForwards(), xiiVec3(1, 0, 0), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirRight(), xiiVec3(0, 1, 0), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirUp(), xiiVec3(0, 0, 1), xiiMath::DefaultEpsilon<float>());

    // look at with dir == up vector
    camera.LookAt(xiiVec3(2, 3, 4), xiiVec3(2, 3, 5), xiiVec3(0, 0, 1));
    XII_TEST_VEC3(camera.GetPosition(), xiiVec3(2, 3, 4), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirForwards(), xiiVec3(0, 0, 1), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirRight(), xiiVec3(0, 1, 0), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirUp(), xiiVec3(-1, 0, 0), xiiMath::DefaultEpsilon<float>());

    camera.LookAt(xiiVec3(2, 3, 4), xiiVec3(2, 3, 3), xiiVec3(0, 0, 1));
    XII_TEST_VEC3(camera.GetPosition(), xiiVec3(2, 3, 4), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirForwards(), xiiVec3(0, 0, -1), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirRight(), xiiVec3(0, 1, 0), xiiMath::DefaultEpsilon<float>());
    XII_TEST_VEC3(camera.GetDirUp(), xiiVec3(1, 0, 0), xiiMath::DefaultEpsilon<float>());
  }
}
