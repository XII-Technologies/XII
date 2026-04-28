/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Declarations.h>
#include <GuiFoundation/Widgets/WidgetUtils.h>
#include <QApplication>
#include <QRect>

QScreen& xiiWidgetUtils::GetClosestScreen(const QPoint& point)
{
  QScreen* pClosestScreen = QApplication::screenAt(point);
  if (pClosestScreen == nullptr)
  {
    QList<QScreen*> screens           = QApplication::screens();
    float           fShortestDistance = xiiMath::Infinity<float>();
    for (QScreen* pScreen : screens)
    {
      const QRect    geom    = pScreen->geometry();
      xiiBoundingBox xiiGeom = xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3(geom.center().x(), geom.center().y(), 0), xiiVec3(geom.width() / 2.0f, geom.height() / 2.0f, 0));
      const xiiVec3  xiiPoint(point.x(), point.y(), 0);
      if (xiiGeom.Contains(xiiPoint))
      {
        return *pScreen;
      }
      float fDistance = xiiGeom.GetDistanceSquaredTo(xiiPoint);
      if (fDistance < fShortestDistance)
      {
        fShortestDistance = fDistance;
        pClosestScreen    = pScreen;
      }
    }
    XII_ASSERT_DEV(pClosestScreen != nullptr, "There are no screens connected, UI cannot function.");
  }
  return *pClosestScreen;
}

void xiiWidgetUtils::AdjustGridDensity(double& ref_fFinestDensity, double& ref_fRoughDensity, xiiUInt32 uiWindowWidth, double fViewportSceneWidth, xiiUInt32 uiMinPixelsForStep)
{
  const double fMaxStepsFitInWindow = (double)uiWindowWidth / (double)uiMinPixelsForStep;

  const double fStartDensity = ref_fFinestDensity;

  xiiInt32 iFactor     = 1;
  double   fNewDensity = ref_fFinestDensity;
  xiiInt32 iFactors[2] = {5, 2};
  xiiInt32 iLastFactor = 0;

  while (true)
  {
    const double fStepsAtDensity = fViewportSceneWidth / fNewDensity;

    if (fStepsAtDensity < fMaxStepsFitInWindow)
      break;

    iFactor *= iFactors[iLastFactor];
    fNewDensity = fStartDensity * iFactor;

    iLastFactor = (iLastFactor + 1) % 2;
  }

  ref_fFinestDensity = fStartDensity * iFactor;

  iFactor *= iFactors[iLastFactor];
  ref_fRoughDensity = fStartDensity * iFactor;
}

void xiiWidgetUtils::ComputeGridExtentsX(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinX, double& out_fMaxX)
{
  out_fMinX = xiiMath::RoundDown((double)viewportSceneRect.left(), fGridStops);
  out_fMaxX = xiiMath::RoundUp((double)viewportSceneRect.right(), fGridStops);
}

void xiiWidgetUtils::ComputeGridExtentsY(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinY, double& out_fMaxY)
{
  out_fMinY = xiiMath::RoundDown((double)viewportSceneRect.top(), fGridStops);
  out_fMaxY = xiiMath::RoundUp((double)viewportSceneRect.bottom(), fGridStops);
}
