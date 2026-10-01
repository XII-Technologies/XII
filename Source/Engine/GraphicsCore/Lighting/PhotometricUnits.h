/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/Types/Enum.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// Physical quantity used to author the intensity of a light source.
///
/// The renderer converts authored values to the canonical quantity required by
/// each emitter: candela for local lights, lux for directional lights, and nits
/// for finite area emitters.
struct XII_GRAPHICSCORE_DLL xiiPhotometricUnit
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Lumen,   ///< Luminous flux (lm).
    Candela, ///< Luminous intensity (cd = lm / sr).
    Lux,     ///< Illuminance or luminous exitance (lx = lm / m^2).
    Nit,     ///< Luminance (nt = cd / m^2).

    Default = Lumen
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiPhotometricUnit);

/// Unit-safe photometric conversions used by components, GPU extraction, and
/// sensor rendering. All inputs are clamped to finite, non-negative values.
class XII_GRAPHICSCORE_DLL xiiPhotometricUtils
{
public:
  /// Solid angle of a circular cone described by its full apex angle.
  [[nodiscard]] static float ConeSolidAngle(xiiAngle fullConeAngle);

  /// Converts luminous flux to luminous intensity for a uniform emitter.
  [[nodiscard]] static float LuminousFluxToIntensity(float fLumens, float fSolidAngleSteradians);
  [[nodiscard]] static float LuminousIntensityToFlux(float fCandela, float fSolidAngleSteradians);

  /// Inverse-square conversion between luminous intensity and illuminance.
  [[nodiscard]] static float LuminousIntensityToIlluminance(float fCandela, float fDistanceMeters);
  [[nodiscard]] static float IlluminanceToLuminousIntensity(float fLux, float fDistanceMeters);

  /// Conversion between luminance and on-axis luminous intensity.
  [[nodiscard]] static float LuminanceToLuminousIntensity(float fNits, float fProjectedAreaSquareMeters);
  [[nodiscard]] static float LuminousIntensityToLuminance(float fCandela, float fProjectedAreaSquareMeters);

  /// Total flux emitted by a one-sided, ideal Lambertian surface.
  [[nodiscard]] static float LuminanceToLuminousFlux(float fNits, float fEmittingAreaSquareMeters);
  [[nodiscard]] static float LuminousFluxToLuminance(float fLumens, float fEmittingAreaSquareMeters);

  /// Converts an authored local-light value to canonical luminous intensity.
  /// Lux is interpreted as illuminance measured at one metre. Nits require the
  /// projected emitter area; a zero-area emitter therefore produces zero cd.
  [[nodiscard]] static float ToLuminousIntensity(float fValue, xiiEnum<xiiPhotometricUnit> unit, float fEmissionSolidAngleSteradians, float fProjectedAreaSquareMeters = 0.0f);

  /// Converts an authored distant-light value to canonical illuminance.
  /// Nits are integrated over the apparent source solid angle. Candela and
  /// lumen inputs are interpreted at a calibration distance of one metre.
  [[nodiscard]] static float ToIlluminance(float fValue, xiiEnum<xiiPhotometricUnit> unit, float fSourceSolidAngleSteradians);

  /// Converts an authored finite-area value to canonical luminance.
  /// Lux is interpreted as luminous exitance for the emitting surface.
  [[nodiscard]] static float ToLuminance(float fValue, xiiEnum<xiiPhotometricUnit> unit, float fEmittingAreaSquareMeters, float fProjectedAreaSquareMeters);
};
