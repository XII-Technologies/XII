/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/PhotometricUnits.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiPhotometricUnit, 1)
  XII_ENUM_CONSTANTS(xiiPhotometricUnit::Lumen, xiiPhotometricUnit::Candela, xiiPhotometricUnit::Lux, xiiPhotometricUnit::Nit)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

namespace
{
  constexpr float s_fMinimumPhysicalExtent = 1.0e-6f;

  float Sanitize(float fValue)
  {
    return xiiMath::IsFinite(fValue) ? xiiMath::Max(fValue, 0.0f) : 0.0f;
  }

  float SafeDivisor(float fValue)
  {
    return xiiMath::Max(Sanitize(fValue), s_fMinimumPhysicalExtent);
  }
} // namespace

float xiiPhotometricUtils::ConeSolidAngle(xiiAngle fullConeAngle)
{
  const xiiAngle halfAngle = xiiMath::Clamp(fullConeAngle * 0.5f, xiiAngle::MakeZero(), xiiAngle::MakeFromRadian(xiiMath::Pi<float>()));
  return 2.0f * xiiMath::Pi<float>() * (1.0f - xiiMath::Cos(halfAngle));
}

float xiiPhotometricUtils::LuminousFluxToIntensity(float fLumens, float fSolidAngleSteradians)
{
  return Sanitize(fLumens) / SafeDivisor(fSolidAngleSteradians);
}

float xiiPhotometricUtils::LuminousIntensityToFlux(float fCandela, float fSolidAngleSteradians)
{
  return Sanitize(fCandela) * Sanitize(fSolidAngleSteradians);
}

float xiiPhotometricUtils::LuminousIntensityToIlluminance(float fCandela, float fDistanceMeters)
{
  const float fDistance = SafeDivisor(fDistanceMeters);
  return Sanitize(fCandela) / (fDistance * fDistance);
}

float xiiPhotometricUtils::IlluminanceToLuminousIntensity(float fLux, float fDistanceMeters)
{
  const float fDistance = Sanitize(fDistanceMeters);
  return Sanitize(fLux) * fDistance * fDistance;
}

float xiiPhotometricUtils::LuminanceToLuminousIntensity(float fNits, float fProjectedAreaSquareMeters)
{
  return Sanitize(fNits) * Sanitize(fProjectedAreaSquareMeters);
}

float xiiPhotometricUtils::LuminousIntensityToLuminance(float fCandela, float fProjectedAreaSquareMeters)
{
  const float fArea = Sanitize(fProjectedAreaSquareMeters);
  return fArea > 0.0f ? Sanitize(fCandela) / SafeDivisor(fArea) : 0.0f;
}

float xiiPhotometricUtils::LuminanceToLuminousFlux(float fNits, float fEmittingAreaSquareMeters)
{
  return Sanitize(fNits) * Sanitize(fEmittingAreaSquareMeters) * xiiMath::Pi<float>();
}

float xiiPhotometricUtils::LuminousFluxToLuminance(float fLumens, float fEmittingAreaSquareMeters)
{
  const float fArea = Sanitize(fEmittingAreaSquareMeters);
  return fArea > 0.0f ? Sanitize(fLumens) / (xiiMath::Pi<float>() * SafeDivisor(fArea)) : 0.0f;
}

float xiiPhotometricUtils::ToLuminousIntensity(float fValue, xiiEnum<xiiPhotometricUnit> unit, float fEmissionSolidAngleSteradians, float fProjectedAreaSquareMeters)
{
  switch (unit.GetValue())
  {
    case xiiPhotometricUnit::Lumen:
      return LuminousFluxToIntensity(fValue, fEmissionSolidAngleSteradians);
    case xiiPhotometricUnit::Candela:
      return Sanitize(fValue);
    case xiiPhotometricUnit::Lux:
      return IlluminanceToLuminousIntensity(fValue, 1.0f);
    case xiiPhotometricUnit::Nit:
      return LuminanceToLuminousIntensity(fValue, fProjectedAreaSquareMeters);
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return 0.0f;
  }
}

float xiiPhotometricUtils::ToIlluminance(float fValue, xiiEnum<xiiPhotometricUnit> unit, float fSourceSolidAngleSteradians)
{
  switch (unit.GetValue())
  {
    case xiiPhotometricUnit::Lumen:
      return LuminousIntensityToIlluminance(LuminousFluxToIntensity(fValue, fSourceSolidAngleSteradians), 1.0f);
    case xiiPhotometricUnit::Candela:
      return LuminousIntensityToIlluminance(fValue, 1.0f);
    case xiiPhotometricUnit::Lux:
      return Sanitize(fValue);
    case xiiPhotometricUnit::Nit:
      return Sanitize(fValue) * Sanitize(fSourceSolidAngleSteradians);
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return 0.0f;
  }
}

float xiiPhotometricUtils::ToLuminance(float fValue, xiiEnum<xiiPhotometricUnit> unit, float fEmittingAreaSquareMeters, float fProjectedAreaSquareMeters)
{
  switch (unit.GetValue())
  {
    case xiiPhotometricUnit::Lumen:
      return LuminousFluxToLuminance(fValue, fEmittingAreaSquareMeters);
    case xiiPhotometricUnit::Candela:
      return LuminousIntensityToLuminance(fValue, fProjectedAreaSquareMeters);
    case xiiPhotometricUnit::Lux:
      return Sanitize(fValue) / xiiMath::Pi<float>();
    case xiiPhotometricUnit::Nit:
      return Sanitize(fValue);
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return 0.0f;
  }
}
