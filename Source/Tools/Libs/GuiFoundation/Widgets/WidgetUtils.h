/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

class QRectF;
class QScreen;

namespace xiiWidgetUtils
{
  /// Contrary to QApplication::screenAt() this function will always succeed with a valid cursor positions
  /// and also with out of bounds cursor positions.
  XII_GUIFOUNDATION_DLL QScreen& GetClosestScreen(const QPoint& point);

  XII_GUIFOUNDATION_DLL void AdjustGridDensity(double& ref_fFinestDensity, double& ref_fRoughDensity, xiiUInt32 uiWindowWidth, double fViewportSceneWidth, xiiUInt32 uiMinPixelsForStep);

  XII_GUIFOUNDATION_DLL void ComputeGridExtentsX(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinX, double& out_fMaxX);

  XII_GUIFOUNDATION_DLL void ComputeGridExtentsY(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinY, double& out_fMaxY);
} // namespace xiiWidgetUtils
