/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ModelImporter/ModelImporterPCH.h>

#include <ModelImporter/Implementation/AnimationImportInternal.h>
#include <cmath>

namespace
{
  using namespace xiiModelImportInternal;

  xiiVec4 ConvertValue(const aiVector3D& v) { return xiiVec4(v.x, v.y, v.z, 0); }
  xiiVec4 ConvertValue(const aiQuaternion& q) { return xiiVec4(q.x, q.y, q.z, q.w); }
  xiiQuat ConvertQuaternion(const xiiVec4& v)
  {
    auto q = xiiQuat::MakeFromElements(v.x, v.y, v.z, v.w);
    q.Normalize();
    return q;
  }

  template <typename Key>
  class SourceCurve
  {
  public:
    xiiResult Initialize(const Key* keys, xiiUInt32 count, const AnimationCurveSettings& settings, bool rotation, xiiStringBuilder* error)
    {
      m_Keys        = keys;
      m_Rotation    = rotation;
      m_PreDefault  = settings.m_PreState == aiAnimBehaviour_DEFAULT && !settings.m_bClampDefault;
      m_PostDefault = settings.m_PostState == aiAnimBehaviour_DEFAULT && !settings.m_bClampDefault;
      if ((settings.m_PreState != aiAnimBehaviour_DEFAULT && settings.m_PreState != aiAnimBehaviour_CONSTANT) || (settings.m_PostState != aiAnimBehaviour_DEFAULT && settings.m_PostState != aiAnimBehaviour_CONSTANT))
        return Fail(error, "Native animation import does not support extrapolated or repeated per-channel boundary behavior.");
      if (count == 0)
        return XII_SUCCESS;
      if (keys == nullptr || count > 1000000)
        return Fail(error, "Animation has missing or excessive source keys.");
      m_Cubic  = keys[0].mInterpolation == aiAnimInterpolation_Cubic_Spline;
      m_Stride = m_Cubic ? 3U : 1U;
      if (count % m_Stride != 0)
        return Fail(error, "Cubic animation keys must contain tangent/value/tangent triplets.");
      const xiiUInt32 logicalCount = count / m_Stride;
      m_Times.Reserve(logicalCount);
      for (xiiUInt32 i = 0; i < logicalCount; ++i)
      {
        const auto& key  = keys[i * m_Stride];
        const float time = static_cast<float>(key.mTime / settings.m_fTicksPerSecond - settings.m_fBegin);
        if (!xiiMath::IsFinite(key.mTime) || key.mTime < 0 || !xiiMath::IsFinite(time) || (!m_Times.IsEmpty() && time <= m_Times.PeekBack()))
          return Fail(error, "Animation key times are invalid or cannot be distinguished at native float precision.");
        m_Times.PushBack(time);
        for (xiiUInt32 j = 0; j < m_Stride; ++j)
        {
          const auto& part  = keys[i * m_Stride + j];
          const auto  value = ConvertValue(part.mValue);
          if (part.mTime != key.mTime || !value.IsValid() || (m_Cubic && part.mInterpolation != aiAnimInterpolation_Cubic_Spline) || (!m_Cubic && part.mInterpolation != aiAnimInterpolation_Step && part.mInterpolation != aiAnimInterpolation_Linear && part.mInterpolation != aiAnimInterpolation_Spherical_Linear) || (rotation && (!m_Cubic || j == 1) && (!xiiMath::IsFinite(value.Dot(value)) || value.Dot(value) < 1e-12f)))
            return Fail(error, "Animation contains invalid values or unsupported interpolation.");
        }
      }
      return XII_SUCCESS;
    }

    xiiResult GetSampleTimes(float duration, float sampleRate, xiiDynamicArray<float>& times, xiiStringBuilder* error) const
    {
      times.PushBack(0);
      if (duration > 0)
        times.PushBack(duration);
      if (m_Cubic)
      {
        const double samples = xiiMath::Ceil(double(duration) * sampleRate);
        if (samples > 1000000)
          return Fail(error, "Animation exceeds one million baked samples per track.");
        for (xiiUInt32 i = 1; i < static_cast<xiiUInt32>(samples); ++i)
          times.PushBack(static_cast<float>(i / double(sampleRate)));
      }
      for (xiiUInt32 i = 0; i < m_Times.GetCount(); ++i)
      {
        const float time = m_Times[i];
        if (time <= 0 || time > duration)
          continue;
        times.PushBack(time);
        // Linear runtime interpolation cannot interpolate across a step. No float
        // sample time exists between these two boundary keys.
        if (i == 0 && m_PreDefault)
          times.PushBack(std::nextafter(time, 0.0f));
        if (i > 0 && !m_Cubic && m_Keys[(i - 1) * m_Stride].mInterpolation == aiAnimInterpolation_Step)
          times.PushBack(std::nextafter(time, 0.0f));
      }
      if (m_PostDefault && !m_Times.IsEmpty() && m_Times.PeekBack() >= 0 && m_Times.PeekBack() < duration)
        times.PushBack(std::nextafter(m_Times.PeekBack(), duration));
      if (times.GetCount() > 1000000)
        return Fail(error, "Animation exceeds one million baked samples per track.");
      times.Sort();
      xiiUInt32 unique = 0;
      for (xiiUInt32 i = 0; i < times.GetCount(); ++i)
        if (unique == 0 || times[i] != times[unique - 1])
          times[unique++] = times[i];
      times.SetCount(unique);
      return XII_SUCCESS;
    }

    xiiVec4 Sample(float time, const xiiVec4& fallback) const
    {
      if (m_Times.IsEmpty())
        return fallback;
      if (time < m_Times[0])
        return m_PreDefault ? fallback : Value(0);
      if (time == m_Times[0])
        return Value(0);
      if (time > m_Times.PeekBack())
        return m_PostDefault ? fallback : Value(m_Times.GetCount() - 1);
      if (time == m_Times.PeekBack())
        return Value(m_Times.GetCount() - 1);
      xiiUInt32 low = 0, high = m_Times.GetCount() - 1;
      while (low + 1 < high)
      {
        const auto middle = low + (high - low) / 2;
        if (m_Times[middle] <= time)
          low = middle;
        else
          high = middle;
      }
      const auto  a = Value(low), b = Value(high);
      const float range = m_Times[high] - m_Times[low];
      const float t     = (time - m_Times[low]) / range;
      if (!m_Cubic && m_Keys[low].mInterpolation == aiAnimInterpolation_Step)
        return a;
      if (m_Cubic)
      {
        // glTF tangents are derivatives per second; Assimp keeps their values
        // while converting key times to ticks.
        const auto  outTangent = ConvertValue(m_Keys[low * 3 + 2].mValue);
        const auto  inTangent  = ConvertValue(m_Keys[high * 3].mValue);
        const float t2 = t * t, t3 = t2 * t;
        return a * (2 * t3 - 3 * t2 + 1) + outTangent * ((t3 - 2 * t2 + t) * range) +
          b * (-2 * t3 + 3 * t2) + inTangent * ((t3 - t2) * range);
      }
      if (m_Rotation)
      {
        const auto q = xiiQuat::MakeSlerp(ConvertQuaternion(a), ConvertQuaternion(b), t);
        return xiiVec4(q.x, q.y, q.z, q.w);
      }
      return a + (b - a) * t;
    }

  private:
    xiiVec4                Value(xiiUInt32 index) const { return ConvertValue(m_Keys[index * m_Stride + (m_Cubic ? 1 : 0)].mValue); }
    const Key*             m_Keys = nullptr;
    xiiDynamicArray<float> m_Times;
    xiiUInt32              m_Stride      = 1;
    bool                   m_PreDefault  = false;
    bool                   m_PostDefault = false;
    bool                   m_Cubic       = false;
    bool                   m_Rotation    = false;
  };
} // namespace

xiiResult xiiModelImportInternal::BakeVectorKeys(const aiVectorKey* keys, xiiUInt32 count, const AnimationCurveSettings& settings, const xiiVec3& fallback, bool additive, bool scale, xiiHybridArray<xiiAnimationKeyVec3, 4>& out_keys, xiiStringBuilder* error)
{
  SourceCurve<aiVectorKey> curve;
  XII_SUCCEED_OR_RETURN(curve.Initialize(keys, count, settings, false, error));
  xiiDynamicArray<float> times;
  XII_SUCCEED_OR_RETURN(curve.GetSampleTimes(static_cast<float>(settings.m_fEnd - settings.m_fBegin), settings.m_fSampleRate, times, error));
  const xiiVec4 defaultValue(fallback.x, fallback.y, fallback.z, 0);
  const auto    reference = curve.Sample(0, defaultValue).GetAsVec3();
  if (additive && scale && (reference.x == 0 || reference.y == 0 || reference.z == 0))
    return Fail(error, "Additive animation requires a nonzero reference scale.");
  out_keys.Reserve(times.GetCount());
  for (const float time : times)
  {
    auto value = curve.Sample(time, defaultValue).GetAsVec3();
    if (additive)
      value = scale ? value.CompDiv(reference) : value - reference;
    if (!value.IsValid())
      return Fail(error, "Baked animation contains non-finite transform values.");
    out_keys.PushBack({time, value});
  }
  return XII_SUCCESS;
}

xiiResult xiiModelImportInternal::BakeRotationKeys(const aiQuatKey* keys, xiiUInt32 count, const AnimationCurveSettings& settings, const xiiQuat& fallback, bool additive, xiiHybridArray<xiiAnimationKeyQuat, 4>& out_keys, xiiStringBuilder* error)
{
  SourceCurve<aiQuatKey> curve;
  XII_SUCCEED_OR_RETURN(curve.Initialize(keys, count, settings, true, error));
  xiiDynamicArray<float> times;
  XII_SUCCEED_OR_RETURN(curve.GetSampleTimes(static_cast<float>(settings.m_fEnd - settings.m_fBegin), settings.m_fSampleRate, times, error));
  const xiiVec4 defaultValue(fallback.x, fallback.y, fallback.z, fallback.w);
  const auto    reference = curve.Sample(0, defaultValue);
  if (!reference.IsValid() || !xiiMath::IsFinite(reference.Dot(reference)) || reference.Dot(reference) < 1e-12f)
    return Fail(error, "Animation has an invalid reference rotation.");
  const auto inverseReference = ConvertQuaternion(reference).GetInverse();
  out_keys.Reserve(times.GetCount());
  for (const float time : times)
  {
    const auto value = curve.Sample(time, defaultValue);
    if (!value.IsValid() || !xiiMath::IsFinite(value.Dot(value)) || value.Dot(value) < 1e-12f)
      return Fail(error, "Baked animation contains invalid rotations.");
    auto rotation = ConvertQuaternion(value);
    if (additive)
      rotation = rotation * inverseReference;
    out_keys.PushBack({time, rotation});
  }
  return XII_SUCCESS;
}
