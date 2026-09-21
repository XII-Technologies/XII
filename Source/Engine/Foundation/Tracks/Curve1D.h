/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Types/Enum.h>

class xiiStreamWriter;
class xiiStreamReader;

struct XII_FOUNDATION_DLL xiiCurveTangentMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Bezier,
    FixedLength,
    Linear,
    // Constant,
    Auto,
    Default = Auto
  };
};

/// A 1D curve for animating a single value over time.
class XII_FOUNDATION_DLL xiiCurve1D
{
public:
  /// Stores position and tangents to control spline interpolation
  struct ControlPoint
  {
    XII_DECLARE_POD_TYPE();

    ControlPoint();

    /// The position (x,y) of the control point
    xiiVec2d m_Position;

    /// The tangent for the curve segment to the left that affects the spline interpolation
    xiiVec2 m_LeftTangent;
    /// The tangent for the curve segment to the right that affects the spline interpolation
    xiiVec2 m_RightTangent;

    xiiEnum<xiiCurveTangentMode> m_TangentModeLeft;
    xiiEnum<xiiCurveTangentMode> m_TangentModeRight;

    xiiUInt16 m_uiOriginalIndex;

    XII_ALWAYS_INLINE bool operator<(const ControlPoint& rhs) const { return m_Position.x < rhs.m_Position.x; }
  };

public:
  xiiCurve1D();

  /// Removes all control points.
  void Clear();

  /// Checks whether the curve has any control point.
  bool IsEmpty() const;

  /// Appends a control point. SortControlPoints() must be called to before evaluating the curve.
  ControlPoint& AddControlPoint(double fPos);

  /// Updates the min/max X value that can be retrieved through GetExtents().
  ///
  /// This is automatically done when SortControlPoints() is called. It can be called manually, if the information is required without
  /// sorting.
  void RecomputeExtents();

  /// returns the min and max position value across all control points.
  ///
  /// The returned values are only up to date if either SortControlPoints() or RecomputeExtents() was called before.
  /// Otherwise they will contain stale values.
  void QueryExtents(double& ref_fMinx, double& ref_fMaxx) const;

  /// Returns the min and max Y value across the curve.
  /// For this information to be available, the linear approximation of the curve must have been computed, otherwise stale values will be
  /// returned.
  void QueryExtremeValues(double& ref_fMinVal, double& ref_fMaxVal) const;

  /// Returns the number of control points.
  xiiUInt32 GetNumControlPoints() const;

  /// Const access to a control point.
  const ControlPoint& GetControlPoint(xiiUInt32 uiIdx) const { return m_ControlPoints[uiIdx]; }

  /// Non-const access to a control point. If you modify the position, SortControlPoints() has to be called before evaluating the
  /// curve.
  ControlPoint& ModifyControlPoint(xiiUInt32 uiIdx) { return m_ControlPoints[uiIdx]; }

  /// Sorts the control point arrays by their position. The CPs have to be sorted before calling Evaluate(), otherwise the result
  /// will be wrong.
  void SortControlPoints();

  /// Evaluates the curve at the given position (x coordinate) and returns the value Y value at that point.
  ///
  /// This uses the linear approximation of the curve, so CreateLinearApproximation() must have been called first.
  ///
  /// \sa CreateLinearApproximation
  double Evaluate(double fPosition) const;

  /// Takes the normalized x coordinate [0;1] and converts it into a valid position on the curve
  ///
  /// \note This only works when the curve extents are available. See QueryExtents() and RecomputeExtents().
  ///
  /// \sa RecomputeExtents
  /// \sa QueryExtents
  double ConvertNormalizedPos(double fPos) const;

  /// Takes a value (typically returned by Evaluate()) and normalizes it into [0;1] range
  ///
  /// \note This only works when the linear approximation of the curve has been computed first.
  double NormalizeValue(double value) const;

  /// How much heap memory the curve uses.
  xiiUInt64 GetHeapMemoryUsage() const;

  /// Stores the current state in a stream.
  void Save(xiiStreamWriter& ref_stream) const;

  /// Restores the state from a stream.
  void Load(xiiStreamReader& ref_stream);

  /// Pre-computes sample points for linear interpolation that approximate the curve within the allowed error threshold.
  ///
  /// \note All control points must already be in sorted order, so call SortControlPoints() first if necessary.
  void CreateLinearApproximation(double fMaxError = 0.01, xiiUInt8 uiMaxSubDivs = 8);

  const xiiHybridArray<xiiVec2d, 24>& GetLinearApproximation() const { return m_LinearApproximation; }

  /// Adjusts the tangents such that the curve cannot make loopings
  void ClampTangents();

  /// Adjusts the tangents in accordance to the specified tangent modes at each control point
  ///
  /// \note All control points must already be in sorted order, so call SortControlPoints() first if necessary.
  void ApplyTangentModes();

  /// Typically called by ApplyTangentModes() for specific control points. Control points must be in sorted order.
  void MakeFixedLengthTangentLeft(xiiUInt32 uiCpIdx);
  /// Typically called by ApplyTangentModes() for specific control points. Control points must be in sorted order.
  void MakeFixedLengthTangentRight(xiiUInt32 uiCpIdx);
  /// Typically called by ApplyTangentModes() for specific control points. Control points must be in sorted order.
  void MakeLinearTangentLeft(xiiUInt32 uiCpIdx);
  /// Typically called by ApplyTangentModes() for specific control points. Control points must be in sorted order.
  void MakeLinearTangentRight(xiiUInt32 uiCpIdx);

  void MakeAutoTangentLeft(xiiUInt32 uiCpIdx);
  void MakeAutoTangentRight(xiiUInt32 uiCpIdx);

private:
  void     RecomputeLinearApproxExtremes();
  void     ApproximateMinMaxValues(const ControlPoint& lhs, const ControlPoint& rhs, double& fMinY, double& fMaxY);
  void     ApproximateCurve(const xiiVec2d& p0, const xiiVec2d& p1, const xiiVec2d& p2, const xiiVec2d& p3, double fMaxErrorX, double fMaxErrorY, xiiInt32 iSubDivLeft);
  void     ApproximateCurvePiece(const xiiVec2d& p0, const xiiVec2d& p1, const xiiVec2d& p2, const xiiVec2d& p3, double tLeft, const xiiVec2d& pLeft, double tRight, const xiiVec2d& pRight, double fMaxErrorX, double fMaxErrorY, xiiInt32 iSubDivLeft);
  xiiInt32 FindApproxControlPoint(double x) const;

  double                          m_fMinX, m_fMaxX;
  double                          m_fMinY, m_fMaxY;
  xiiHybridArray<ControlPoint, 8> m_ControlPoints;
  xiiHybridArray<xiiVec2d, 24>    m_LinearApproximation;
};
