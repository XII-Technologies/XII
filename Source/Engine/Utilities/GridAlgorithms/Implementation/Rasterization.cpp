#include <Utilities/UtilitiesPCH.h>

#include <Utilities/GridAlgorithms/Rasterization.h>

xiiRasterizationResult::Enum xii2DGridUtils::ComputePointsOnLine(xiiInt32 iStartX, xiiInt32 iStartY, xiiInt32 iEndX, xiiInt32 iEndY, XII_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */)
{
  // Implements Bresenham's line algorithm:
  // http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm

  xiiInt32 dx = xiiMath::Abs(iEndX - iStartX);
  xiiInt32 dy = xiiMath::Abs(iEndY - iStartY);

  xiiInt32 sx = (iStartX < iEndX) ? 1 : -1;
  xiiInt32 sy = (iStartY < iEndY) ? 1 : -1;

  xiiInt32 err = dx - dy;

  while (true)
  {
    // The user callback can stop the algorithm at any point, if no further points on the line are required
    if (callback(iStartX, iStartY, pPassThrough) == xiiCallbackResult::Stop)
      return xiiRasterizationResult::Aborted;

    if ((iStartX == iEndX) && (iStartY == iEndY))
      return xiiRasterizationResult::Finished;

    xiiInt32 e2 = 2 * err;

    if (e2 > -dy)
    {
      err     = err - dy;
      iStartX = iStartX + sx;
    }

    if (e2 < dx)
    {
      err     = err + dx;
      iStartY = iStartY + sy;
    }
  }
}

xiiRasterizationResult::Enum xii2DGridUtils::ComputePointsOnLineConservative(xiiInt32 iStartX, xiiInt32 iStartY, xiiInt32 iEndX, xiiInt32 iEndY, XII_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */, bool bVisitBothNeighbors /* = false */)
{
  xiiInt32 dx = xiiMath::Abs(iEndX - iStartX);
  xiiInt32 dy = xiiMath::Abs(iEndY - iStartY);

  xiiInt32 sx = (iStartX < iEndX) ? 1 : -1;
  xiiInt32 sy = (iStartY < iEndY) ? 1 : -1;

  xiiInt32 err = dx - dy;

  xiiInt32 iLastX = iStartX;
  xiiInt32 iLastY = iStartY;

  while (true)
  {
    // if this is going to be a diagonal step, make sure to insert horizontal/vertical steps

    if ((xiiMath::Abs(iLastX - iStartX) + xiiMath::Abs(iLastY - iStartY)) == 2)
    {
      // This part is the difference to the non-conservative line algorithm

      if (callback(iLastX, iStartY, pPassThrough) == xiiCallbackResult::Continue)
      {
        // first one succeeded, going to continue

        // if this is true, the user still wants a callback for the alternative, even though it does not change the outcome anymore
        if (bVisitBothNeighbors)
          callback(iStartX, iLastY, pPassThrough);
      }
      else
      {
        // first one failed, try the second
        if (callback(iStartX, iLastY, pPassThrough) == xiiCallbackResult::Stop)
          return xiiRasterizationResult::Aborted;
      }
    }

    iLastX = iStartX;
    iLastY = iStartY;

    // The user callback can stop the algorithm at any point, if no further points on the line are required
    if (callback(iStartX, iStartY, pPassThrough) == xiiCallbackResult::Stop)
      return xiiRasterizationResult::Aborted;

    if ((iStartX == iEndX) && (iStartY == iEndY))
      return xiiRasterizationResult::Finished;

    xiiInt32 e2 = 2 * err;

    if (e2 > -dy)
    {
      err     = err - dy;
      iStartX = iStartX + sx;
    }

    if (e2 < dx)
    {
      err     = err + dx;
      iStartY = iStartY + sy;
    }
  }
}


xiiRasterizationResult::Enum xii2DGridUtils::ComputePointsOnCircle(xiiInt32 iStartX, xiiInt32 iStartY, xiiUInt32 uiRadius, XII_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */)
{
  int f     = 1 - uiRadius;
  int ddF_x = 1;
  int ddF_y = -2 * uiRadius;
  int x     = 0;
  int y     = uiRadius;

  // report the four extremes
  if (callback(iStartX, iStartY + uiRadius, pPassThrough) == xiiCallbackResult::Stop)
    return xiiRasterizationResult::Aborted;
  if (callback(iStartX, iStartY - uiRadius, pPassThrough) == xiiCallbackResult::Stop)
    return xiiRasterizationResult::Aborted;
  if (callback(iStartX + uiRadius, iStartY, pPassThrough) == xiiCallbackResult::Stop)
    return xiiRasterizationResult::Aborted;
  if (callback(iStartX - uiRadius, iStartY, pPassThrough) == xiiCallbackResult::Stop)
    return xiiRasterizationResult::Aborted;

  // the loop iterates over an eighth of the circle (a 45 degree segment) and then mirrors each point 8 times to fill the entire circle
  while (x < y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    if (callback(iStartX + x, iStartY + y, pPassThrough) == xiiCallbackResult::Stop)
      return xiiRasterizationResult::Aborted;
    if (callback(iStartX - x, iStartY + y, pPassThrough) == xiiCallbackResult::Stop)
      return xiiRasterizationResult::Aborted;
    if (callback(iStartX + x, iStartY - y, pPassThrough) == xiiCallbackResult::Stop)
      return xiiRasterizationResult::Aborted;
    if (callback(iStartX - x, iStartY - y, pPassThrough) == xiiCallbackResult::Stop)
      return xiiRasterizationResult::Aborted;
    if (callback(iStartX + y, iStartY + x, pPassThrough) == xiiCallbackResult::Stop)
      return xiiRasterizationResult::Aborted;
    if (callback(iStartX - y, iStartY + x, pPassThrough) == xiiCallbackResult::Stop)
      return xiiRasterizationResult::Aborted;
    if (callback(iStartX + y, iStartY - x, pPassThrough) == xiiCallbackResult::Stop)
      return xiiRasterizationResult::Aborted;
    if (callback(iStartX - y, iStartY - x, pPassThrough) == xiiCallbackResult::Stop)
      return xiiRasterizationResult::Aborted;
  }

  return xiiRasterizationResult::Finished;
}

xiiUInt32 xii2DGridUtils::FloodFill(xiiInt32 iStartX, xiiInt32 iStartY, XII_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */, xiiDeque<xiiVec2I32>* pTempArray /* = nullptr */)
{
  xiiUInt32 uiFilled = 0;

  xiiDeque<xiiVec2I32> FallbackQueue;

  if (pTempArray == nullptr)
    pTempArray = &FallbackQueue;

  pTempArray->Clear();
  pTempArray->PushBack(xiiVec2I32(iStartX, iStartY));

  while (!pTempArray->IsEmpty())
  {
    xiiVec2I32 v = pTempArray->PeekBack();
    pTempArray->PopBack();

    if (callback(v.x, v.y, pPassThrough) == xiiCallbackResult::Continue)
    {
      ++uiFilled;

      // put the four neighbors into the queue
      pTempArray->PushBack(xiiVec2I32(v.x - 1, v.y));
      pTempArray->PushBack(xiiVec2I32(v.x + 1, v.y));
      pTempArray->PushBack(xiiVec2I32(v.x, v.y - 1));
      pTempArray->PushBack(xiiVec2I32(v.x, v.y + 1));
    }
  }

  return uiFilled;
}

xiiUInt32 xii2DGridUtils::FloodFillDiag(xiiInt32 iStartX, xiiInt32 iStartY, XII_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /*= nullptr*/, xiiDeque<xiiVec2I32>* pTempArray /*= nullptr*/)
{
  xiiUInt32 uiFilled = 0;

  xiiDeque<xiiVec2I32> FallbackQueue;

  if (pTempArray == nullptr)
    pTempArray = &FallbackQueue;

  pTempArray->Clear();
  pTempArray->PushBack(xiiVec2I32(iStartX, iStartY));

  while (!pTempArray->IsEmpty())
  {
    xiiVec2I32 v = pTempArray->PeekBack();
    pTempArray->PopBack();

    if (callback(v.x, v.y, pPassThrough) == xiiCallbackResult::Continue)
    {
      ++uiFilled;

      // put the eight neighbors into the queue
      pTempArray->PushBack(xiiVec2I32(v.x - 1, v.y));
      pTempArray->PushBack(xiiVec2I32(v.x + 1, v.y));
      pTempArray->PushBack(xiiVec2I32(v.x, v.y - 1));
      pTempArray->PushBack(xiiVec2I32(v.x, v.y + 1));

      pTempArray->PushBack(xiiVec2I32(v.x - 1, v.y - 1));
      pTempArray->PushBack(xiiVec2I32(v.x + 1, v.y - 1));
      pTempArray->PushBack(xiiVec2I32(v.x + 1, v.y + 1));
      pTempArray->PushBack(xiiVec2I32(v.x - 1, v.y + 1));
    }
  }

  return uiFilled;
}

// Lookup table that describes the shape of the circle
// When rasterizing circles with few pixels algorithms usually don't give nice shapes
// so this lookup table is handcrafted for better results
static const xiiUInt8 OverlapCircle[15][15] = {{9, 9, 9, 9, 9, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9}, {9, 9, 9, 8, 8, 7, 7, 7, 7, 7, 8, 8, 9, 9, 9}, {9, 9, 8, 8, 7, 6, 6, 6, 6, 6, 7, 8, 8, 9, 9}, {9, 8, 8, 7, 6, 6, 5, 5, 5, 6, 6, 7, 8, 8, 9}, {9, 8, 7, 6, 6, 5, 4, 4, 4, 5, 6, 6, 7, 8, 9}, {8, 7, 6, 6, 5, 4, 3, 3, 3, 4, 5, 6, 6, 7, 8}, {8, 7, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 7, 8}, {8, 7, 6, 5, 4, 3, 1, 0, 1, 3, 4, 5, 6, 7, 8}, {8, 7, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 7, 8}, {8, 7, 6, 6, 5, 4, 3, 3, 3, 4, 5, 6, 6, 7, 8}, {9, 8, 7, 6, 6, 5, 4, 4, 4, 5, 6, 6, 7, 8, 9}, {9, 8, 8, 7, 6, 6, 5, 5, 5, 6, 6, 7, 8, 8, 9}, {9, 9, 8, 8, 7, 6, 6, 6, 6, 6, 7, 8, 8, 9, 9}, {9, 9, 9, 8, 8, 7, 7, 7, 7, 7, 8, 8, 9, 9, 9}, {9, 9, 9, 9, 9, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9}};

static const xiiInt32 CircleCenter     = 7;
static const xiiUInt8 CircleAreaMin[9] = {7, 6, 6, 5, 4, 3, 2, 1, 0};
static const xiiUInt8 CircleAreaMax[9] = {7, 8, 8, 9, 10, 11, 12, 13, 14};

xiiRasterizationResult::Enum xii2DGridUtils::RasterizeBlob(xiiInt32 iPosX, xiiInt32 iPosY, xiiBlobType type, XII_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */)
{
  const xiiUInt8 uiCircleType = xiiMath::Clamp<xiiUInt8>(type, 0, 8);

  const xiiInt32 iAreaMin = CircleAreaMin[uiCircleType];
  const xiiInt32 iAreaMax = CircleAreaMax[uiCircleType];

  iPosX -= CircleCenter;
  iPosY -= CircleCenter;

  for (xiiInt32 y = iAreaMin; y <= iAreaMax; ++y)
  {
    for (xiiInt32 x = iAreaMin; x <= iAreaMax; ++x)
    {
      if (OverlapCircle[y][x] <= uiCircleType)
      {
        if (callback(iPosX + x, iPosY + y, pPassThrough) == xiiCallbackResult::Stop)
          return xiiRasterizationResult::Aborted;
      }
    }
  }

  return xiiRasterizationResult::Finished;
}

xiiRasterizationResult::Enum xii2DGridUtils::RasterizeBlobWithDistance(xiiInt32 iPosX, xiiInt32 iPosY, xiiBlobType type, XII_RASTERIZED_BLOB_CALLBACK callback, void* pPassThrough /*= nullptr*/)
{
  const xiiUInt8 uiCircleType = xiiMath::Clamp<xiiUInt8>(type, 0, 8);

  const xiiInt32 iAreaMin = CircleAreaMin[uiCircleType];
  const xiiInt32 iAreaMax = CircleAreaMax[uiCircleType];

  iPosX -= CircleCenter;
  iPosY -= CircleCenter;

  for (xiiInt32 y = iAreaMin; y <= iAreaMax; ++y)
  {
    for (xiiInt32 x = iAreaMin; x <= iAreaMax; ++x)
    {
      const xiiUInt8 uiDistance = OverlapCircle[y][x];

      if (uiDistance <= uiCircleType)
      {
        if (callback(iPosX + x, iPosY + y, pPassThrough, uiDistance) == xiiCallbackResult::Stop)
          return xiiRasterizationResult::Aborted;
      }
    }
  }

  return xiiRasterizationResult::Finished;
}

xiiRasterizationResult::Enum xii2DGridUtils::RasterizeCircle(xiiInt32 iPosX, xiiInt32 iPosY, float fRadius, XII_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */)
{
  const xiiVec2 vCenter((float)iPosX, (float)iPosY);

  const xiiInt32 iRadius    = (xiiInt32)fRadius;
  const float    fRadiusSqr = xiiMath::Square(fRadius);

  for (xiiInt32 y = iPosY - iRadius; y <= iPosY + iRadius; ++y)
  {
    for (xiiInt32 x = iPosX - iRadius; x <= iPosX + iRadius; ++x)
    {
      const xiiVec2 v((float)x, (float)y);

      if ((v - vCenter).GetLengthSquared() > fRadiusSqr)
        continue;

      if (callback(x, y, pPassThrough) == xiiCallbackResult::Stop)
        return xiiRasterizationResult::Aborted;
    }
  }

  return xiiRasterizationResult::Finished;
}


struct VisibilityLine
{
  xiiDynamicArray<xiiUInt8>*                    m_pVisible;
  xiiUInt32                                     m_uiSize;
  xiiUInt32                                     m_uiRadius;
  xiiInt32                                      m_iCenterX;
  xiiInt32                                      m_iCenterY;
  xii2DGridUtils::XII_RASTERIZED_POINT_CALLBACK m_VisCallback;
  void*                                         m_pUserPassThrough;
  xiiUInt32                                     m_uiWidth;
  xiiUInt32                                     m_uiHeight;
  xiiVec2                                       m_vDirection;
  xiiAngle                                      m_ConeAngle;
};

struct CellFlags
{
  enum Enum
  {
    NotVisited = 0,
    Visited    = XII_BIT(0),
    Visible    = Visited | XII_BIT(1),
    Invisible  = Visited,
  };
};

static xiiCallbackResult::Enum MarkPointsOnLineVisible(xiiInt32 x, xiiInt32 y, void* pPassThrough)
{
  VisibilityLine* VisLine = (VisibilityLine*)pPassThrough;

  // if the reported point is outside the playing field, don't continue
  if (x < 0 || y < 0 || x >= (xiiInt32)VisLine->m_uiWidth || y >= (xiiInt32)VisLine->m_uiHeight)
    return xiiCallbackResult::Stop;

  // compute the point position inside our virtual grid (where the start position is at the center)
  const xiiUInt32 VisX = x - VisLine->m_iCenterX + VisLine->m_uiRadius;
  const xiiUInt32 VisY = y - VisLine->m_iCenterY + VisLine->m_uiRadius;

  // if we are outside our virtual grid, stop
  if (VisX >= (xiiInt32)VisLine->m_uiSize || VisY >= (xiiInt32)VisLine->m_uiSize)
    return xiiCallbackResult::Stop;

  // We actually only need two bits for each cell (visited + visible)
  // so we pack the information for four cells into one byte
  const xiiUInt32 uiCellIndex       = VisY * VisLine->m_uiSize + VisX;
  const xiiUInt32 uiBitfieldByte    = uiCellIndex >> 2;                      // division by four
  const xiiUInt32 uiBitfieldBiteOff = uiBitfieldByte << 2;                   // modulo to determine where in the byte this cell is stored
  const xiiUInt32 uiMaskShift       = (uiCellIndex - uiBitfieldBiteOff) * 2; // times two because we use two bits

  xiiUInt8&      CellFlagsRef   = (*VisLine->m_pVisible)[uiBitfieldByte]; // for writing into the byte later
  const xiiUInt8 ThisCellsFlags = (CellFlagsRef >> uiMaskShift) & 3U;     // the decoded flags value for reading (3U == lower two bits)

  // if this point on the line was already visited and determined to be invisible, don't continue
  if (ThisCellsFlags == CellFlags::Invisible)
    return xiiCallbackResult::Stop;

  // this point has been visited already and the point was determined to be visible, so just continue
  if (ThisCellsFlags == CellFlags::Visible)
    return xiiCallbackResult::Continue;

  // apparently this cell has not been visited yet, so ask the user callback what to do
  if (VisLine->m_VisCallback(x, y, VisLine->m_pUserPassThrough) == xiiCallbackResult::Continue)
  {
    // the callback reported this cell as visible, so flag it and continue
    CellFlagsRef |= ((xiiUInt8)CellFlags::Visible) << uiMaskShift;
    return xiiCallbackResult::Continue;
  }

  // the callback reported this flag as invisible, flag it and stop the line
  CellFlagsRef |= ((xiiUInt8)CellFlags::Invisible) << uiMaskShift;
  return xiiCallbackResult::Stop;
}

static xiiCallbackResult::Enum MarkPointsInCircleVisible(xiiInt32 x, xiiInt32 y, void* pPassThrough)
{
  VisibilityLine* ld = (VisibilityLine*)pPassThrough;

  xii2DGridUtils::ComputePointsOnLineConservative(ld->m_iCenterX, ld->m_iCenterY, x, y, MarkPointsOnLineVisible, pPassThrough, false);

  return xiiCallbackResult::Continue;
}

void xii2DGridUtils::ComputeVisibleArea(xiiInt32 iPosX, xiiInt32 iPosY, xiiUInt16 uiRadius, xiiUInt32 uiWidth, xiiUInt32 uiHeight, XII_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */, xiiDynamicArray<xiiUInt8>* pTempArray /* = nullptr */)
{
  const xiiUInt32 uiSize = uiRadius * 2 + 1;

  xiiDynamicArray<xiiUInt8> VisiblityFlags;

  // if we don't get a temp array, use our own array, with blackjack etc.
  if (pTempArray == nullptr)
    pTempArray = &VisiblityFlags;

  pTempArray->Clear();
  pTempArray->SetCount(xiiMath::Square(uiSize) / 4); // we store only two bits per cell, so we can pack four values into each byte

  VisibilityLine ld;
  ld.m_uiSize           = uiSize;
  ld.m_uiRadius         = uiRadius;
  ld.m_pVisible         = pTempArray;
  ld.m_iCenterX         = iPosX;
  ld.m_iCenterY         = iPosY;
  ld.m_VisCallback      = callback;
  ld.m_pUserPassThrough = pPassThrough;
  ld.m_uiWidth          = uiWidth;
  ld.m_uiHeight         = uiHeight;

  // from the center, trace lines to all points on the circle around it
  // each line determines for each cell whether it is visible
  // once an invisible cell is encountered, a line will stop further tracing
  // no cell is ever reported twice to the user callback
  xii2DGridUtils::ComputePointsOnCircle(iPosX, iPosY, uiRadius, MarkPointsInCircleVisible, &ld);
}

static xiiCallbackResult::Enum MarkPointsInConeVisible(xiiInt32 x, xiiInt32 y, void* pPassThrough)
{
  VisibilityLine* ld = (VisibilityLine*)pPassThrough;

  const xiiVec2 vPos((float)x, (float)y);
  const xiiVec2 vDirToPos = (vPos - xiiVec2((float)ld->m_iCenterX, (float)ld->m_iCenterY)).GetNormalized();

  const xiiAngle angle = xiiMath::ACos(vDirToPos.Dot(ld->m_vDirection));

  if (angle.GetRadian() < ld->m_ConeAngle.GetRadian())
    xii2DGridUtils::ComputePointsOnLineConservative(ld->m_iCenterX, ld->m_iCenterY, x, y, MarkPointsOnLineVisible, pPassThrough, false);

  return xiiCallbackResult::Continue;
}

void xii2DGridUtils::ComputeVisibleAreaInCone(xiiInt32 iPosX, xiiInt32 iPosY, xiiUInt16 uiRadius, const xiiVec2& vDirection, xiiAngle coneAngle, xiiUInt32 uiWidth, xiiUInt32 uiHeight, XII_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */, xiiDynamicArray<xiiUInt8>* pTempArray /* = nullptr */)
{
  const xiiUInt32 uiSize = uiRadius * 2 + 1;

  xiiDynamicArray<xiiUInt8> VisiblityFlags;

  // if we don't get a temp array, use our own array, with blackjack etc.
  if (pTempArray == nullptr)
    pTempArray = &VisiblityFlags;

  pTempArray->Clear();
  pTempArray->SetCount(xiiMath::Square(uiSize) / 4); // we store only two bits per cell, so we can pack four values into each byte


  VisibilityLine ld;
  ld.m_uiSize           = uiSize;
  ld.m_uiRadius         = uiRadius;
  ld.m_pVisible         = pTempArray;
  ld.m_iCenterX         = iPosX;
  ld.m_iCenterY         = iPosY;
  ld.m_VisCallback      = callback;
  ld.m_pUserPassThrough = pPassThrough;
  ld.m_uiWidth          = uiWidth;
  ld.m_uiHeight         = uiHeight;
  ld.m_vDirection       = vDirection;
  ld.m_ConeAngle        = coneAngle;

  xii2DGridUtils::ComputePointsOnCircle(iPosX, iPosY, uiRadius, MarkPointsInConeVisible, &ld);
}



XII_STATICLINK_FILE(Utilities, Utilities_GridAlgorithms_Implementation_Rasterization);
