#include <ModelImporter2/ModelImporterPCH.h>

#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/types.h>
#include <assimp/vector3.h>

namespace xiiModelImporter2
{
  xiiColor ConvertAssimpType(const aiColor4D& value, bool invert /*= false*/)
  {
    if (invert)
      return xiiColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b, 1.0f - value.a);
    else
      return xiiColor(value.r, value.g, value.b, value.a);
  }

  xiiColor ConvertAssimpType(const aiColor3D& value, bool invert /*= false*/)
  {
    if (invert)
      return xiiColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b);
    else
      return xiiColor(value.r, value.g, value.b);
  }

  xiiMat4 ConvertAssimpType(const aiMatrix4x4& value, bool dummy /*= false*/)
  {
    XII_ASSERT_DEBUG(!dummy, "not implemented");

    xiiMat4 mTransformation;
    mTransformation.SetFromArray(&value.a1, xiiMatrixLayout::RowMajor);
    return mTransformation;
  }

  xiiVec3 ConvertAssimpType(const aiVector3D& value, bool dummy /*= false*/)
  {
    XII_ASSERT_DEBUG(!dummy, "not implemented");

    return xiiVec3(value.x, value.y, value.z);
  }

  xiiQuat ConvertAssimpType(const aiQuaternion& value, bool dummy /*= false*/)
  {
    XII_ASSERT_DEBUG(!dummy, "not implemented");

    return xiiQuat(value.x, value.y, value.z, value.w);
  }

  float ConvertAssimpType(float value, bool dummy /*= false*/)
  {
    XII_ASSERT_DEBUG(!dummy, "not implemented");

    return value;
  }

  int ConvertAssimpType(int value, bool dummy /*= false*/)
  {
    XII_ASSERT_DEBUG(!dummy, "not implemented");

    return value;
  }

} // namespace xiiModelImporter2
