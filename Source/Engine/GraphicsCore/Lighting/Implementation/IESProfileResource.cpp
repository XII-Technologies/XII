/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Math.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/Lighting/IESProfileResource.h>

namespace
{
  class NumericTokenReader
  {
  public:
    explicit NumericTokenReader(xiiStringView source) :
      m_pCurrent(source.GetStartPointer()), m_pEnd(source.GetEndPointer())
    {
    }

    bool Read(double& out_value)
    {
      while (m_pCurrent < m_pEnd && (xiiStringUtils::IsWhiteSpace(*m_pCurrent) || *m_pCurrent == ','))
        ++m_pCurrent;

      if (m_pCurrent >= m_pEnd)
        return false;

      const char* pLast = nullptr;
      if (xiiConversionUtils::StringToFloat(xiiStringView(m_pCurrent, m_pEnd), out_value, &pLast).Failed() || pLast == nullptr || pLast <= m_pCurrent)
        return false;

      m_pCurrent = pLast;
      return true;
    }

    bool ReadCount(xiiUInt32& out_value)
    {
      double value = 0.0;
      if (!Read(value) || value < 0.0 || value > static_cast<double>(xiiMath::MaxValue<xiiUInt32>()))
        return false;

      const xiiUInt32 rounded = static_cast<xiiUInt32>(value + 0.5);
      if (xiiMath::Abs(value - static_cast<double>(rounded)) > 0.0001)
        return false;

      out_value = rounded;
      return true;
    }

  private:
    const char* m_pCurrent;
    const char* m_pEnd;
  };

  void SetError(xiiStringBuilder* pError, xiiStringView text)
  {
    if (pError != nullptr)
      pError->Set(text);
  }

  xiiUInt32 FindLowerSample(xiiArrayPtr<const float> angles, float value)
  {
    if (angles.GetCount() < 2U || value <= angles[0])
      return 0U;

    for (xiiUInt32 i = 1U; i < angles.GetCount(); ++i)
    {
      if (value <= angles[i])
        return i - 1U;
    }

    return angles.GetCount() - 2U;
  }

  float InterpolateSource(const xiiDynamicArray<float>& verticalAngles, const xiiDynamicArray<float>& horizontalAngles,
    const xiiDynamicArray<float>& candela, float verticalDegrees, float horizontalDegrees)
  {
    verticalDegrees = xiiMath::Clamp(verticalDegrees, verticalAngles[0], verticalAngles.PeekBack());

    if (horizontalAngles.GetCount() == 1U)
    {
      horizontalDegrees = horizontalAngles[0];
    }
    else
    {
      horizontalDegrees = xiiMath::Mod(horizontalDegrees, 360.0f);
      if (horizontalDegrees < 0.0f)
        horizontalDegrees += 360.0f;

      const float maximumHorizontal = horizontalAngles.PeekBack();
      if (maximumHorizontal <= 90.001f)
      {
        horizontalDegrees = xiiMath::Mod(horizontalDegrees, 180.0f);
        if (horizontalDegrees > 90.0f)
          horizontalDegrees = 180.0f - horizontalDegrees;
      }
      else if (maximumHorizontal <= 180.001f && horizontalDegrees > 180.0f)
      {
        horizontalDegrees = 360.0f - horizontalDegrees;
      }

      horizontalDegrees = xiiMath::Clamp(horizontalDegrees, horizontalAngles[0], maximumHorizontal);
    }

    const xiiUInt32 v0 = FindLowerSample(verticalAngles, verticalDegrees);
    const xiiUInt32 v1 = xiiMath::Min(v0 + 1U, verticalAngles.GetCount() - 1U);
    const float verticalSpan = verticalAngles[v1] - verticalAngles[v0];
    const float verticalWeight = verticalSpan > 0.0f ? (verticalDegrees - verticalAngles[v0]) / verticalSpan : 0.0f;

    const xiiUInt32 h0 = horizontalAngles.GetCount() > 1U ? FindLowerSample(horizontalAngles, horizontalDegrees) : 0U;
    const xiiUInt32 h1 = xiiMath::Min(h0 + 1U, horizontalAngles.GetCount() - 1U);
    const float horizontalSpan = horizontalAngles[h1] - horizontalAngles[h0];
    const float horizontalWeight = horizontalSpan > 0.0f ? (horizontalDegrees - horizontalAngles[h0]) / horizontalSpan : 0.0f;

    const xiiUInt32 verticalCount = verticalAngles.GetCount();
    const float c00 = candela[h0 * verticalCount + v0];
    const float c01 = candela[h0 * verticalCount + v1];
    const float c10 = candela[h1 * verticalCount + v0];
    const float c11 = candela[h1 * verticalCount + v1];
    return xiiMath::Lerp(xiiMath::Lerp(c00, c01, verticalWeight), xiiMath::Lerp(c10, c11, verticalWeight), horizontalWeight);
  }
} // namespace

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiIESProfileResourceDescriptor, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiIESProfileResourceDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MaximumCandela", m_fMaximumCandela)->AddAttributes(new xiiSuffixAttribute(" cd"), new xiiReadOnlyAttribute()),
    XII_MEMBER_PROPERTY("ReportedLumens", m_fReportedLumens)->AddAttributes(new xiiSuffixAttribute(" lm"), new xiiReadOnlyAttribute()),
    XII_ARRAY_MEMBER_PROPERTY("NormalizedCandela", m_NormalizedCandela)->AddAttributes(new xiiReadOnlyAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiIESProfileResource, 1, xiiRTTIDefaultAllocator<xiiIESProfileResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiIESProfileResource);

bool xiiIESProfileResourceDescriptor::IsValid() const
{
  return m_fMaximumCandela > 0.0f && xiiMath::IsFinite(m_fMaximumCandela) && m_NormalizedCandela.GetCount() == s_uiSampleCount;
}

xiiResult xiiIESProfileResourceDescriptor::ParseLM63(xiiStringView sSource, xiiStringBuilder* pError)
{
  const char* pTilt = sSource.FindSubString_NoCase("TILT=");
  if (pTilt == nullptr)
  {
    SetError(pError, "IES profile does not contain a TILT directive.");
    return XII_FAILURE;
  }

  const char* pLineEnd = pTilt;
  while (pLineEnd < sSource.GetEndPointer() && *pLineEnd != '\r' && *pLineEnd != '\n')
    ++pLineEnd;

  xiiStringView tiltValue(pTilt + 5, pLineEnd);
  tiltValue.Trim(" \t");
  if (!tiltValue.IsEqual_NoCase("NONE"))
  {
    SetError(pError, "Only LM-63 profiles with TILT=NONE are supported.");
    return XII_FAILURE;
  }

  NumericTokenReader reader(xiiStringView(pLineEnd, sSource.GetEndPointer()));
  xiiUInt32 lampCount = 0U;
  xiiUInt32 verticalCount = 0U;
  xiiUInt32 horizontalCount = 0U;
  xiiUInt32 photometricType = 0U;
  double lumensPerLamp = 0.0;
  double candelaMultiplier = 0.0;
  double ignored = 0.0;

  if (!reader.ReadCount(lampCount) || !reader.Read(lumensPerLamp) || !reader.Read(candelaMultiplier) ||
      !reader.ReadCount(verticalCount) || !reader.ReadCount(horizontalCount) || !reader.ReadCount(photometricType))
  {
    SetError(pError, "IES photometric header is incomplete.");
    return XII_FAILURE;
  }

  // Units, width, length, height, ballast factor, future use, and input watts.
  for (xiiUInt32 i = 0U; i < 7U; ++i)
  {
    if (!reader.Read(ignored))
    {
      SetError(pError, "IES luminaire dimensions or electrical data are incomplete.");
      return XII_FAILURE;
    }
  }

  if (photometricType != 1U)
  {
    SetError(pError, "Only IES Type-C photometry is supported.");
    return XII_FAILURE;
  }
  if (verticalCount < 2U || verticalCount > 4096U || horizontalCount == 0U || horizontalCount > 4096U || lampCount == 0U || candelaMultiplier <= 0.0)
  {
    SetError(pError, "IES profile contains invalid sample counts or intensity metadata.");
    return XII_FAILURE;
  }

  xiiDynamicArray<float> verticalAngles;
  xiiDynamicArray<float> horizontalAngles;
  xiiDynamicArray<float> sourceCandela;
  verticalAngles.SetCountUninitialized(verticalCount);
  horizontalAngles.SetCountUninitialized(horizontalCount);
  sourceCandela.SetCountUninitialized(verticalCount * horizontalCount);

  auto ReadFiniteNonDecreasingAngles = [&reader](xiiDynamicArray<float>& angles) -> bool {
    float previous = -xiiMath::Infinity<float>();
    for (float& angle : angles)
    {
      double value = 0.0;
      if (!reader.Read(value) || !xiiMath::IsFinite(value) || value < previous)
        return false;
      angle = static_cast<float>(value);
      previous = angle;
    }
    return true;
  };

  if (!ReadFiniteNonDecreasingAngles(verticalAngles) || !ReadFiniteNonDecreasingAngles(horizontalAngles))
  {
    SetError(pError, "IES angles must be finite and monotonically increasing.");
    return XII_FAILURE;
  }

  float maximumCandela = 0.0f;
  for (float& sample : sourceCandela)
  {
    double value = 0.0;
    if (!reader.Read(value) || !xiiMath::IsFinite(value) || value < 0.0)
    {
      SetError(pError, "IES candela table is incomplete or contains invalid values.");
      return XII_FAILURE;
    }
    sample = static_cast<float>(value * candelaMultiplier);
    maximumCandela = xiiMath::Max(maximumCandela, sample);
  }

  if (!(maximumCandela > 0.0f) || verticalAngles[0] < 0.0f || verticalAngles.PeekBack() > 180.001f ||
      horizontalAngles[0] < 0.0f || horizontalAngles.PeekBack() > 360.001f)
  {
    SetError(pError, "IES angular domain or candela distribution is invalid.");
    return XII_FAILURE;
  }

  xiiIESProfileResourceDescriptor parsed;
  parsed.m_fMaximumCandela = maximumCandela;
  parsed.m_fReportedLumens = static_cast<float>(xiiMath::Max(lumensPerLamp, 0.0) * static_cast<double>(lampCount));
  parsed.m_NormalizedCandela.SetCountUninitialized(s_uiSampleCount);

  for (xiiUInt32 h = 0U; h < s_uiHorizontalSampleCount; ++h)
  {
    const float horizontalDegrees = 360.0f * static_cast<float>(h) / static_cast<float>(s_uiHorizontalSampleCount);
    for (xiiUInt32 v = 0U; v < s_uiVerticalSampleCount; ++v)
    {
      const float verticalDegrees = 180.0f * static_cast<float>(v) / static_cast<float>(s_uiVerticalSampleCount - 1U);
      const float candela = InterpolateSource(verticalAngles, horizontalAngles, sourceCandela, verticalDegrees, horizontalDegrees);
      parsed.m_NormalizedCandela[h * s_uiVerticalSampleCount + v] = xiiMath::Saturate(candela / maximumCandela);
    }
  }

  *this = std::move(parsed);
  if (pError != nullptr)
    pError->Clear();
  return XII_SUCCESS;
}

float xiiIESProfileResourceDescriptor::Sample(xiiAngle verticalAngle, xiiAngle horizontalAngle) const
{
  if (!IsValid())
    return 1.0f;

  const float v = xiiMath::Clamp(verticalAngle.GetDegree() / 180.0f, 0.0f, 1.0f) * static_cast<float>(s_uiVerticalSampleCount - 1U);
  float hNormalized = xiiMath::Mod(horizontalAngle.GetDegree(), 360.0f) / 360.0f;
  if (hNormalized < 0.0f)
    hNormalized += 1.0f;
  const float h = hNormalized * static_cast<float>(s_uiHorizontalSampleCount);

  const xiiUInt32 v0 = xiiMath::Min(static_cast<xiiUInt32>(xiiMath::Floor(v)), s_uiVerticalSampleCount - 1U);
  const xiiUInt32 v1 = xiiMath::Min(v0 + 1U, s_uiVerticalSampleCount - 1U);
  const xiiUInt32 h0 = static_cast<xiiUInt32>(xiiMath::Floor(h)) % s_uiHorizontalSampleCount;
  const xiiUInt32 h1 = (h0 + 1U) % s_uiHorizontalSampleCount;
  const float vWeight = v - static_cast<float>(v0);
  const float hWeight = h - xiiMath::Floor(h);

  const float c00 = m_NormalizedCandela[h0 * s_uiVerticalSampleCount + v0];
  const float c01 = m_NormalizedCandela[h0 * s_uiVerticalSampleCount + v1];
  const float c10 = m_NormalizedCandela[h1 * s_uiVerticalSampleCount + v0];
  const float c11 = m_NormalizedCandela[h1 * s_uiVerticalSampleCount + v1];
  return xiiMath::Lerp(xiiMath::Lerp(c00, c01, vWeight), xiiMath::Lerp(c10, c11, vWeight), hWeight);
}

void xiiIESProfileResourceDescriptor::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1U);
  inout_stream << m_fMaximumCandela;
  inout_stream << m_fReportedLumens;
  inout_stream.WriteArray(m_NormalizedCandela).IgnoreResult();
}

xiiResult xiiIESProfileResourceDescriptor::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream.ReadVersion(1U);
  inout_stream >> m_fMaximumCandela;
  inout_stream >> m_fReportedLumens;
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_NormalizedCandela));
  return IsValid() ? XII_SUCCESS : XII_FAILURE;
}

xiiIESProfileResource::xiiIESProfileResource() :
  xiiResource(DoUpdate::OnAnyThread, 1U)
{
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiIESProfileResource, xiiIESProfileResourceDescriptor)
{
  m_Descriptor = std::move(descriptor);

  xiiResourceLoadDescription result;
  result.m_uiQualityLevelsDiscardable = 0U;
  result.m_uiQualityLevelsLoadable = 0U;
  result.m_State = m_Descriptor.IsValid() ? xiiResourceState::Loaded : xiiResourceState::LoadedResourceMissing;
  return result;
}

xiiResourceLoadDescription xiiIESProfileResource::UnloadData(Unload whatToUnload)
{
  XII_IGNORE_UNUSED(whatToUnload);
  m_Descriptor = {};

  xiiResourceLoadDescription result;
  result.m_uiQualityLevelsDiscardable = 0U;
  result.m_uiQualityLevelsLoadable = 0U;
  result.m_State = xiiResourceState::Unloaded;
  return result;
}

xiiResourceLoadDescription xiiIESProfileResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiResourceLoadDescription result;
  result.m_uiQualityLevelsDiscardable = 0U;
  result.m_uiQualityLevelsLoadable = 0U;
  result.m_State = xiiResourceState::LoadedResourceMissing;
  m_Descriptor = {};

  if (pStream == nullptr)
    return result;

  xiiStringBuilder absolutePath;
  *pStream >> absolutePath;

  xiiString source;
  source.ReadAll(*pStream);
  xiiStringBuilder error;
  if (m_Descriptor.ParseLM63(source.GetView(), &error).Failed())
  {
    xiiLog::Error("Failed to parse IES profile '{}': {}", GetResourceID(), error);
    return result;
  }

  result.m_State = xiiResourceState::Loaded;
  return result;
}

void xiiIESProfileResource::UpdateMemoryUsage(MemoryUsage& out_memoryUsage)
{
  out_memoryUsage.m_uiMemoryCPU = sizeof(*this) + static_cast<xiiUInt32>(m_Descriptor.m_NormalizedCandela.GetHeapMemoryUsage());
  out_memoryUsage.m_uiMemoryGPU = 0U;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_IESProfileResource);

