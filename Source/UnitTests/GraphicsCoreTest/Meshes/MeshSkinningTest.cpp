/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Meshes/MeshSkinning.h>

XII_CREATE_SIMPLE_TEST(Meshes, Skinning)
{
  const xiiVec3    position(2, 3, 4);
  const xiiVec4U16 joints(0, 1, 65535, 65535);
  xiiMat4          palette[] = {xiiMat4::MakeTranslation(xiiVec3(10, 0, 0)), xiiMat4::MakeTranslation(xiiVec3(0, 20, 0))};

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Bind pose, zero influences and normalized byte weights")
  {
    xiiVec3 result(99);
    XII_TEST_RESULT(xiiMeshSkinning::TransformPosition(position, joints, xiiColorLinearUB(255, 255, 255, 255), {}, result));
    XII_TEST_VEC3(result, position, 0);
    XII_TEST_RESULT(xiiMeshSkinning::TransformPosition(position, joints, xiiColorLinearUB(0, 0, 0, 0), palette, result));
    XII_TEST_VEC3(result, position, 0);
    XII_TEST_RESULT(xiiMeshSkinning::TransformPosition(position, joints, xiiColorLinearUB(60, 20, 0, 0), palette, result));
    XII_TEST_VEC3(result, xiiVec3(9.5f, 8, 4), 0.00001f);
    XII_TEST_RESULT(xiiMeshSkinning::TransformPosition(position, joints, xiiColorLinearUB(255, 255, 0, 0), palette, result));
    XII_TEST_VEC3(result, xiiVec3(7, 13, 4), 0.00001f);
    // Output may alias the bind position; ignored influences need no valid joint index.
    result = position;
    XII_TEST_RESULT(xiiMeshSkinning::TransformPosition(result, joints, xiiColorLinearUB(255, 0, 0, 0), palette, result));
    XII_TEST_VEC3(result, xiiVec3(12, 3, 4), 0.00001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid inputs leave output unchanged")
  {
    const xiiVec3 sentinel(99);
    xiiVec3       result = sentinel;
    XII_TEST_BOOL(xiiMeshSkinning::TransformPosition(position, joints, xiiColorLinearUB(0, 0, 1, 0), palette, result).Failed());
    XII_TEST_BOOL(result == sentinel);
    XII_TEST_BOOL(xiiMeshSkinning::TransformPosition(xiiVec3::MakeNaN(), joints, xiiColorLinearUB(0, 0, 0, 0), {}, result).Failed());
    XII_TEST_BOOL(result == sentinel);
    auto invalid          = xiiMat4::MakeIdentity();
    invalid.Element(0, 0) = xiiMath::Infinity<float>();
    XII_TEST_BOOL(xiiMeshSkinning::ValidatePalette(xiiMakeArrayPtr(&invalid, 1)).Failed());
    invalid               = xiiMat4::MakeIdentity();
    invalid.Element(0, 3) = 0.1f;
    XII_TEST_BOOL(xiiMeshSkinning::ValidatePalette(xiiMakeArrayPtr(&invalid, 1)).Failed());
    XII_TEST_BOOL(xiiMeshSkinning::TransformPosition(position, xiiVec4U16::MakeZero(), xiiColorLinearUB(255, 0, 0, 0), xiiMakeArrayPtr(&invalid, 1), result).Failed());
    XII_TEST_BOOL(result == sentinel);
    invalid = xiiMat4::MakeScaling(xiiVec3(xiiMath::MaxValue<float>()));
    XII_TEST_BOOL(xiiMeshSkinning::TransformPosition(position, xiiVec4U16::MakeZero(), xiiColorLinearUB(255, 0, 0, 0), xiiMakeArrayPtr(&invalid, 1), result).Failed());
    XII_TEST_BOOL(result == sentinel);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Conservative bounds enclose blends, shear, reflection and unweighted vertices")
  {
    const auto bind             = xiiBoundingBoxSphere::MakeFromBox(xiiBoundingBox::MakeFromMinMax(xiiVec3(-2, -3, -4), xiiVec3(2, 3, 4)));
    xiiMat4    transforms[]     = {xiiMat4::MakeScaling(xiiVec3(-2, 1, 0)), xiiMat4::MakeTranslation(xiiVec3(10, 20, 30))};
    transforms[0].Element(1, 0) = 3; // Shear; a sphere transformed by max column length is insufficient.
    XII_TEST_RESULT(xiiMeshSkinning::ValidatePalette(transforms));
    xiiBoundingBoxSphere bounds;
    XII_TEST_RESULT(xiiMeshSkinning::ComputeBounds(bind, transforms, bounds));
    const auto box = bounds.GetBox();
    XII_TEST_BOOL(box.Contains(bind.GetBox()));
    xiiVec3 corners[8];
    bind.GetBox().GetCorners(corners);
    for (const auto& corner : corners)
    {
      for (xiiUInt32 weight = 0; weight <= 255; ++weight)
      {
        xiiVec3 deformed;
        XII_TEST_RESULT(xiiMeshSkinning::TransformPosition(corner, joints, xiiColorLinearUB(xiiUInt8(weight), xiiUInt8(255 - weight), 0, 0), transforms, deformed));
        XII_TEST_BOOL(box.Contains(deformed));
        XII_TEST_BOOL(bounds.GetSphere().Contains(deformed));
      }
    }
    XII_TEST_RESULT(xiiMeshSkinning::ComputeBounds(bind, {}, bounds));
    XII_TEST_BOOL(bounds == bind);
    XII_TEST_RESULT(xiiMeshSkinning::ComputeBounds(bounds, transforms, bounds));
    XII_TEST_BOOL(bounds.GetBox().Contains(box));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid bounds, palettes and overflow are rejected atomically")
  {
    const auto sentinel = xiiBoundingBoxSphere::MakeZero();
    auto       bounds   = sentinel;
    XII_TEST_BOOL(xiiMeshSkinning::ComputeBounds(xiiBoundingBoxSphere::MakeInvalid(), {}, bounds).Failed());
    XII_TEST_BOOL(bounds == sentinel);
    auto bind = xiiBoundingBoxSphere::MakeFromCenterExtents(xiiVec3::MakeZero(), xiiVec3(1), xiiMath::Infinity<float>());
    XII_TEST_BOOL(xiiMeshSkinning::ComputeBounds(bind, {}, bounds).Failed());
    XII_TEST_BOOL(bounds == sentinel);
    bind                 = xiiBoundingBoxSphere::MakeFromBox(xiiBoundingBox::MakeFromMinMax(xiiVec3(-1), xiiVec3(1)));
    auto matrix          = xiiMat4::MakeIdentity();
    matrix.Element(3, 3) = 2;
    XII_TEST_BOOL(xiiMeshSkinning::ComputeBounds(bind, xiiMakeArrayPtr(&matrix, 1), bounds).Failed());
    XII_TEST_BOOL(bounds == sentinel);
    matrix = xiiMat4::MakeScaling(xiiVec3(xiiMath::MaxValue<float>() / 4));
    XII_TEST_BOOL(xiiMeshSkinning::ComputeBounds(bind, xiiMakeArrayPtr(&matrix, 1), bounds).Failed());
    XII_TEST_BOOL(bounds == sentinel);
  }
}
