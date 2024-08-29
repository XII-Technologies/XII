#pragma once

template <class CellData>
xiiGameGrid<CellData>::xiiGameGrid()
{
  m_uiGridSizeX = 0;
  m_uiGridSizeY = 0;

  m_mRotateToWorldspace.SetIdentity();
  m_mRotateToGridspace.SetIdentity();

  m_vWorldSpaceOrigin.SetZero();
  m_vLocalSpaceCellSize.Set(1.0f);
  m_vInverseLocalSpaceCellSize.Set(1.0f);
}

template <class CellData>
void xiiGameGrid<CellData>::CreateGrid(xiiUInt16 uiSizeX, xiiUInt16 uiSizeY)
{
  m_Cells.Clear();

  m_uiGridSizeX = uiSizeX;
  m_uiGridSizeY = uiSizeY;

  m_Cells.SetCount(m_uiGridSizeX * m_uiGridSizeY);
}

template <class CellData>
void xiiGameGrid<CellData>::SetWorldSpaceDimensions(const xiiVec3& vLowerLeftCorner, const xiiVec3& vCellSize, Orientation ori)
{
  xiiMat3 mRot;

  switch (ori)
  {
    case InPlaneXY:
      mRot.SetIdentity();
      break;
    case InPlaneXZ:
      mRot = xiiMat3::MakeAxisRotation(xiiVec3(1, 0, 0), xiiAngle::MakeFromDegree(90.0f));
      break;
    case InPlaneXminusZ:
      mRot = xiiMat3::MakeAxisRotation(xiiVec3(1, 0, 0), xiiAngle::MakeFromDegree(-90.0f));
      break;
  }

  SetWorldSpaceDimensions(vLowerLeftCorner, vCellSize, mRot);
}

template <class CellData>
void xiiGameGrid<CellData>::SetWorldSpaceDimensions(const xiiVec3& vLowerLeftCorner, const xiiVec3& vCellSize, const xiiMat3& mRotation)
{
  m_vWorldSpaceOrigin          = vLowerLeftCorner;
  m_vLocalSpaceCellSize        = vCellSize;
  m_vInverseLocalSpaceCellSize = xiiVec3(1.0f).CompDiv(vCellSize);

  m_mRotateToWorldspace = mRotation;
  m_mRotateToGridspace  = mRotation.GetInverse();
}

template <class CellData>
xiiVec2I32 xiiGameGrid<CellData>::GetCellAtWorldPosition(const xiiVec3& vWorldSpacePos) const
{
  const xiiVec3 vCell = (m_mRotateToGridspace * ((vWorldSpacePos - m_vWorldSpaceOrigin)).CompMul(m_vInverseLocalSpaceCellSize));

  // Without the Floor, the border case when the position is outside (-1 / -1) is not immediately detected
  return xiiVec2I32((xiiInt32)xiiMath::Floor(vCell.x), (xiiInt32)xiiMath::Floor(vCell.y));
}

template <class CellData>
xiiVec3 xiiGameGrid<CellData>::GetCellWorldSpaceOrigin(const xiiVec2I32& vCoord) const
{
  return m_vWorldSpaceOrigin + m_mRotateToWorldspace * GetCellLocalSpaceOrigin(vCoord);
}

template <class CellData>
xiiVec3 xiiGameGrid<CellData>::GetCellLocalSpaceOrigin(const xiiVec2I32& vCoord) const
{
  return m_vLocalSpaceCellSize.CompMul(xiiVec3((float)vCoord.x, (float)vCoord.y, 0.0f));
}

template <class CellData>
xiiVec3 xiiGameGrid<CellData>::GetCellWorldSpaceCenter(const xiiVec2I32& vCoord) const
{
  return m_vWorldSpaceOrigin + m_mRotateToWorldspace * GetCellLocalSpaceCenter(vCoord);
}

template <class CellData>
xiiVec3 xiiGameGrid<CellData>::GetCellLocalSpaceCenter(const xiiVec2I32& vCoord) const
{
  return m_vLocalSpaceCellSize.CompMul(xiiVec3((float)vCoord.x + 0.5f, (float)vCoord.y + 0.5f, 0.5f));
}

template <class CellData>
bool xiiGameGrid<CellData>::IsValidCellCoordinate(const xiiVec2I32& vCoord) const
{
  return (vCoord.x >= 0 && vCoord.x < m_uiGridSizeX && vCoord.y >= 0 && vCoord.y < m_uiGridSizeY);
}

template <class CellData>
bool xiiGameGrid<CellData>::PickCell(const xiiVec3& vRayStartPos, const xiiVec3& vRayDirNorm, xiiVec2I32* out_pCellCoord, xiiVec3* out_pIntersection) const
{
  xiiPlane p = xiiPlane::MakeFromNormalAndPoint(m_mRotateToWorldspace * xiiVec3(0, 0, -1), m_vWorldSpaceOrigin);

  xiiVec3 vPos;

  if (!p.GetRayIntersection(vRayStartPos, vRayDirNorm, nullptr, &vPos))
    return false;

  if (out_pIntersection)
    *out_pIntersection = vPos;

  if (out_pCellCoord)
    *out_pCellCoord = GetCellAtWorldPosition(vPos);

  return true;
}

template <class CellData>
xiiBoundingBox xiiGameGrid<CellData>::GetWorldBoundingBox() const
{
  xiiVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  vGridBox = m_mRotateToWorldspace * m_vLocalSpaceCellSize.CompMul(vGridBox);

  return xiiBoundingBox(m_vWorldSpaceOrigin, m_vWorldSpaceOrigin + vGridBox);
}

template <class CellData>
bool xiiGameGrid<CellData>::GetRayIntersection(const xiiVec3& vRayStartWorldSpace, const xiiVec3& vRayDirNormalizedWorldSpace, float fMaxLength, float& out_fIntersection, xiiVec2I32& out_vCellCoord) const
{
  const xiiVec3 vRayStart = m_mRotateToGridspace * (vRayStartWorldSpace - m_vWorldSpaceOrigin);
  const xiiVec3 vRayDir   = m_mRotateToGridspace * vRayDirNormalizedWorldSpace;

  xiiVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  const xiiBoundingBox localBox(xiiVec3(0.0f), m_vLocalSpaceCellSize.CompMul(vGridBox));

  if (localBox.Contains(vRayStart))
  {
    // if the ray is already inside the box, we know that a cell is hit
    out_fIntersection = 0.0f;
  }
  else
  {
    if (!localBox.GetRayIntersection(vRayStart, vRayDir, &out_fIntersection, nullptr))
      return false;

    if (out_fIntersection > fMaxLength)
      return false;
  }

  const xiiVec3 vEnterPos = vRayStart + vRayDir * out_fIntersection;

  const xiiVec3 vCell = vEnterPos.CompMul(m_vInverseLocalSpaceCellSize);

  // Without the Floor, the border case when the position is outside (-1 / -1) is not immediately detected
  out_vCellCoord   = xiiVec2I32((xiiInt32)xiiMath::Floor(vCell.x), (xiiInt32)xiiMath::Floor(vCell.y));
  out_vCellCoord.x = xiiMath::Clamp(out_vCellCoord.x, 0, m_uiGridSizeX - 1);
  out_vCellCoord.y = xiiMath::Clamp(out_vCellCoord.y, 0, m_uiGridSizeY - 1);

  return true;
}

template <class CellData>
bool xiiGameGrid<CellData>::GetRayIntersectionExpandedBBox(const xiiVec3& vRayStartWorldSpace, const xiiVec3& vRayDirNormalizedWorldSpace, float fMaxLength, float& out_fIntersection, const xiiVec3& vExpandBBoxByThis) const
{
  const xiiVec3 vRayStart = m_mRotateToGridspace * (vRayStartWorldSpace - m_vWorldSpaceOrigin);
  const xiiVec3 vRayDir   = m_mRotateToGridspace * vRayDirNormalizedWorldSpace;

  xiiVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  xiiBoundingBox localBox(xiiVec3(0.0f), m_vLocalSpaceCellSize.CompMul(vGridBox));
  localBox.Grow(vExpandBBoxByThis);

  if (localBox.Contains(vRayStart))
  {
    // if the ray is already inside the box, we know that a cell is hit
    out_fIntersection = 0.0f;
  }
  else
  {
    if (!localBox.GetRayIntersection(vRayStart, vRayDir, &out_fIntersection, nullptr))
      return false;

    if (out_fIntersection > fMaxLength)
      return false;
  }

  return true;
}

template <class CellData>
xiiResult xiiGameGrid<CellData>::Serialize(xiiStreamWriter& ref_stream) const
{
  ref_stream.WriteVersion(1);

  ref_stream << m_uiGridSizeX;
  ref_stream << m_uiGridSizeY;
  ref_stream << m_mRotateToWorldspace;
  ref_stream << m_mRotateToGridspace;
  ref_stream << m_vWorldSpaceOrigin;
  ref_stream << m_vLocalSpaceCellSize;
  ref_stream << m_vInverseLocalSpaceCellSize;
  XII_SUCCEED_OR_RETURN(ref_stream.WriteArray(m_Cells));

  return XII_SUCCESS;
}

template <class CellData>
xiiResult xiiGameGrid<CellData>::Deserialize(xiiStreamReader& ref_stream)
{
  const xiiTypeVersion version = ref_stream.ReadVersion(1);
  XII_IGNORE_UNUSED(version);

  ref_stream >> m_uiGridSizeX;
  ref_stream >> m_uiGridSizeY;
  ref_stream >> m_mRotateToWorldspace;
  ref_stream >> m_mRotateToGridspace;
  ref_stream >> m_vWorldSpaceOrigin;
  ref_stream >> m_vLocalSpaceCellSize;
  ref_stream >> m_vInverseLocalSpaceCellSize;
  XII_SUCCEED_OR_RETURN(ref_stream.ReadArray(m_Cells));

  return XII_SUCCESS;
}
