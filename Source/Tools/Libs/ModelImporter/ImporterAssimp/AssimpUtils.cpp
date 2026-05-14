/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ModelImporter/ModelImporterPCH.h>

#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/types.h>
#include <assimp/vector3.h>

namespace xiiModelImporter
{
  xiiColor ConvertAssimpType(const aiColor4D& value, bool bInvert /*= false*/)
  {
    if (bInvert)
      return xiiColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b, 1.0f - value.a);
    else
      return xiiColor(value.r, value.g, value.b, value.a);
  }

  xiiColor ConvertAssimpType(const aiColor3D& value, bool bInvert /*= false*/)
  {
    if (bInvert)
      return xiiColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b);
    else
      return xiiColor(value.r, value.g, value.b);
  }

  xiiMat4 ConvertAssimpType(const aiMatrix4x4& value, bool bDummy /*= false*/)
  {
    XII_ASSERT_DEBUG(!bDummy, "not implemented");

    return xiiMat4::MakeFromRowMajorArray(&value.a1);
  }

  xiiVec3 ConvertAssimpType(const aiVector3D& value, bool bDummy /*= false*/)
  {
    XII_ASSERT_DEBUG(!bDummy, "not implemented");

    return xiiVec3(value.x, value.y, value.z);
  }

  xiiQuat ConvertAssimpType(const aiQuaternion& value, bool bDummy /*= false*/)
  {
    XII_ASSERT_DEBUG(!bDummy, "not implemented");

    return xiiQuat(value.x, value.y, value.z, value.w);
  }

  float ConvertAssimpType(float value, bool bDummy /*= false*/)
  {
    XII_ASSERT_DEBUG(!bDummy, "not implemented");

    return value;
  }

  int ConvertAssimpType(int value, bool bDummy /*= false*/)
  {
    XII_ASSERT_DEBUG(!bDummy, "not implemented");

    return value;
  }

} // namespace xiiModelImporter
