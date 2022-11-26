#pragma once

#include <Foundation/SimdMath/SimdTransform.h>
#include <JoltPlugin/JoltPluginDLL.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Jolt/Core/Color.h>
#include <Jolt/Math/Float3.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Vec4.h>

namespace xiiJoltConversionUtils
{
  XII_ALWAYS_INLINE xiiVec3 ToVec3(const JPH::Vec3& v) { return xiiVec3(v.mF32[0], v.mF32[1], v.mF32[2]); }

  XII_ALWAYS_INLINE xiiVec3 ToVec3(const JPH::Float3& v) { return reinterpret_cast<const xiiVec3&>(v); }

  XII_ALWAYS_INLINE xiiColor ToColor(const JPH::ColorArg& c)
  {
    const JPH::Vec4 v4 = c.ToVec4();
    return reinterpret_cast<const xiiColor&>(v4);
  }

  XII_ALWAYS_INLINE xiiSimdVec4f ToSimdVec3(const JPH::Vec3& v) { return xiiSimdVec4f(v.mF32[0], v.mF32[1], v.mF32[2], v.mF32[3]); }

  XII_ALWAYS_INLINE JPH::Vec3 ToVec3(const xiiVec3& v) { return JPH::Vec3(v.x, v.y, v.z); }

  XII_ALWAYS_INLINE JPH::Float3 ToFloat3(const xiiVec3& v) { return reinterpret_cast<const JPH::Float3&>(v); }

  XII_ALWAYS_INLINE JPH::Vec3 ToVec3(const xiiSimdVec4f& v) { return reinterpret_cast<const JPH::Vec3&>(v); }

  XII_ALWAYS_INLINE xiiQuat ToQuat(const JPH::Quat& q) { return reinterpret_cast<const xiiQuat&>(q); }

  XII_ALWAYS_INLINE xiiSimdQuat ToSimdQuat(const JPH::Quat& q) { return reinterpret_cast<const xiiSimdQuat&>(q); }

  XII_ALWAYS_INLINE JPH::Quat ToQuat(const xiiQuat& q) { return JPH::Quat(q.v.x, q.v.y, q.v.z, q.w); }

  XII_ALWAYS_INLINE JPH::Quat ToQuat(const xiiSimdQuat& q) { return reinterpret_cast<const JPH::Quat&>(q); }

  XII_ALWAYS_INLINE xiiTransform ToTransform(const JPH::Vec3& pos, const JPH::Quat& rot) { return xiiTransform(ToVec3(pos), ToQuat(rot)); }

  XII_ALWAYS_INLINE xiiTransform ToTransform(const JPH::Vec3& pos) { return xiiTransform(ToVec3(pos)); }

} // namespace xiiJoltConversionUtils
