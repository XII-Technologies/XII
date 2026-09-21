/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Declarations.h>

class xiiStreamWriter;
class xiiStreamReader;

/// A color curve for animating colors.
///
/// The gradient consists of a number of control points, for rgb, alpha and intensity.
/// One can evaluate the curve at any x coordinate.
class XII_FOUNDATION_DLL xiiColorGradient
{
public:
  /// Color control point. Stores red, green and blue in gamma space.
  struct ColorCP
  {
    XII_DECLARE_POD_TYPE();

    double   m_PosX;
    xiiUInt8 m_GammaRed;
    xiiUInt8 m_GammaGreen;
    xiiUInt8 m_GammaBlue;
    float    m_fInvDistToNextCp; /// Internal: Optimization for Evaluate to not recalculate 1/distance to the next control point

    XII_ALWAYS_INLINE bool operator<(const ColorCP& rhs) const { return m_PosX < rhs.m_PosX; }
  };

  /// Alpha control point.
  struct AlphaCP
  {
    XII_DECLARE_POD_TYPE();

    double   m_PosX;
    xiiUInt8 m_Alpha;
    float    m_fInvDistToNextCp; /// Internal: Optimization for Evaluate to not recalculate 1/distance to the next control point

    XII_ALWAYS_INLINE bool operator<(const AlphaCP& rhs) const { return m_PosX < rhs.m_PosX; }
  };

  /// Intensity control point. Used to scale rgb for high-dynamic range values.
  struct IntensityCP
  {
    XII_DECLARE_POD_TYPE();

    double m_PosX;
    float  m_Intensity;
    float  m_fInvDistToNextCp; /// Internal: Optimization for Evaluate to not recalculate 1/distance to the next control point

    XII_ALWAYS_INLINE bool operator<(const IntensityCP& rhs) const { return m_PosX < rhs.m_PosX; }
  };

public:
  xiiColorGradient();

  /// Removes all control points.
  void Clear();

  /// Checks whether the curve has any control point.
  bool IsEmpty() const;

  /// Appends a color control point. SortControlPoints() must be called to before evaluating the curve.
  void AddColorControlPoint(double x, const xiiColorGammaUB& rgb);

  /// Appends an alpha control point. SortControlPoints() must be called to before evaluating the curve.
  void AddAlphaControlPoint(double x, xiiUInt8 uiAlpha);

  /// Appends an intensity control point. SortControlPoints() must be called to before evaluating the curve.
  void AddIntensityControlPoint(double x, float fIntensity);

  /// Determines the min and max x-coordinate value across all control points.
  bool GetExtents(double& ref_fMinx, double& ref_fMaxx) const;

  /// Returns the number of control points of each type.
  void GetNumControlPoints(xiiUInt32& ref_uiRgb, xiiUInt32& ref_uiAlpha, xiiUInt32& ref_uiIntensity) const;

  /// Const access to a control point.
  const ColorCP& GetColorControlPoint(xiiUInt32 uiIdx) const { return m_ColorCPs[uiIdx]; }
  /// Const access to a control point.
  const AlphaCP& GetAlphaControlPoint(xiiUInt32 uiIdx) const { return m_AlphaCPs[uiIdx]; }
  /// Const access to a control point.
  const IntensityCP& GetIntensityControlPoint(xiiUInt32 uiIdx) const { return m_IntensityCPs[uiIdx]; }

  /// Non-const access to a control point. If you modify the x coordinate, SortControlPoints() has to be called before evaluating the
  /// curve.
  ColorCP& ModifyColorControlPoint(xiiUInt32 uiIdx) { return m_ColorCPs[uiIdx]; }
  /// Non-const access to a control point. If you modify the x coordinate, SortControlPoints() has to be called before evaluating the
  /// curve.
  AlphaCP& ModifyAlphaControlPoint(xiiUInt32 uiIdx) { return m_AlphaCPs[uiIdx]; }
  /// Non-const access to a control point. If you modify the x coordinate, SortControlPoints() has to be called before evaluating the
  /// curve.
  IntensityCP& ModifyIntensityControlPoint(xiiUInt32 uiIdx) { return m_IntensityCPs[uiIdx]; }

  /// Sorts the control point arrays by their x-coordinate. The CPs have to be sorted before calling Evaluate(), otherwise the result
  /// will be wrong.
  void SortControlPoints();

  /// Evaluates the curve at the given x-coordinate and returns RGBA and intensity separately.
  ///
  /// The control points have to be sorted, so call SortControlPoints() before, if any modifications where done.
  void Evaluate(double x, xiiColorGammaUB& ref_rgba, float& ref_fIntensity) const;

  /// Evaluates the curve and returns RGBA and intensity in one combined xiiColor value.
  void Evaluate(double x, xiiColor& ref_hdr) const;

  /// Evaluates only the color curve.
  void EvaluateColor(double x, xiiColorGammaUB& ref_rgb) const;
  /// Evaluates only the color curve.
  void EvaluateColor(double x, xiiColor& ref_rgb) const;
  /// Evaluates only the alpha curve.
  void EvaluateAlpha(double x, xiiUInt8& ref_uiAlpha) const;
  /// Evaluates only the intensity curve.
  void EvaluateIntensity(double x, float& ref_fIntensity) const;

  /// How much heap memory the curve uses.
  xiiUInt64 GetHeapMemoryUsage() const;

  /// Stores the current state in a stream.
  void Save(xiiStreamWriter& ref_stream) const;

  /// Restores the state from a stream.
  void Load(xiiStreamReader& ref_stream);

private:
  void PrecomputeLerpNormalizer();

  xiiHybridArray<ColorCP, 8>     m_ColorCPs;
  xiiHybridArray<AlphaCP, 8>     m_AlphaCPs;
  xiiHybridArray<IntensityCP, 8> m_IntensityCPs;
};
