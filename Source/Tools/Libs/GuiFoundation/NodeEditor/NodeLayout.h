/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec2.h>
#include <QRectF>

/// A directed connection, with pin ordinates measured from each node's bounding-box top.
struct xiiNodeLayoutConnection
{
  XII_DECLARE_POD_TYPE();
  xiiUInt32 m_uiSource;
  xiiUInt32 m_uiTarget;
  double    m_fSourceY;
  double    m_fTargetY;
};

/// Size-aware left-to-right layout shared by all node editors. No document or widget mutation.
class XII_GUIFOUNDATION_DLL xiiNodeLayout
{
public:
  static constexpr double HorizontalSpacing = 120.0;
  static constexpr double VerticalSpacing   = 60.0;
  static constexpr double ComponentSpacing  = 120.0;

  /// Returns bounding-box top-left positions in input order. Input order is the stable tie-breaker;
  /// callers should sort nodes by persistent identity. Original positions only determine the anchor.
  /// Cycles share a column; disconnected components are packed separately. Invalid input fails
  /// with empty output. Repeated layout with unchanged sizes/connectivity is idempotent.
  static xiiResult Compute(xiiArrayPtr<const QRectF> bounds, xiiArrayPtr<const xiiNodeLayoutConnection> connections, xiiDynamicArray<xiiVec2>& out_positions);
};
