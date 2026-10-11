/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Transform.h>
#include <Foundation/Strings/StringBuilder.h>
#include <assimp/scene.h>

namespace xiiModelImportInternal
{
  inline xiiResult Fail(xiiStringBuilder* error, xiiStringView message)
  {
    if (error != nullptr)
      *error = message;
    return XII_FAILURE;
  }

  inline xiiMat4 ConvertMatrix(const aiMatrix4x4& matrix)
  {
    const float values[] = {matrix.a1, matrix.a2, matrix.a3, matrix.a4,
                            matrix.b1, matrix.b2, matrix.b3, matrix.b4,
                            matrix.c1, matrix.c2, matrix.c3, matrix.c4,
                            matrix.d1, matrix.d2, matrix.d3, matrix.d4};
    return xiiMat4::MakeFromRowMajorArray(values);
  }

  inline xiiResult ConvertPose(const aiMatrix4x4& matrix, xiiTransform& out_pose)
  {
    aiVector3D   scale, translation;
    aiQuaternion rotation;
    matrix.Decompose(scale, rotation, translation);
    out_pose = xiiTransform::Make(xiiVec3(translation.x, translation.y, translation.z),
                                  xiiQuat::MakeFromElements(rotation.x, rotation.y, rotation.z, rotation.w),
                                  xiiVec3(scale.x, scale.y, scale.z));
    if (!out_pose.IsValid() || scale.x == 0 || scale.y == 0 || scale.z == 0 ||
        !out_pose.GetAsMat4().IsEqual(ConvertMatrix(matrix), 0.001f))
      return XII_FAILURE;
    return XII_SUCCESS;
  }
} // namespace xiiModelImportInternal
