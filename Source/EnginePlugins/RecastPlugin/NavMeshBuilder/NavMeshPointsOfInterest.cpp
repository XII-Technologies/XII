#include <RecastPlugin/RecastPluginPCH.h>

#include <Foundation/Containers/StaticArray.h>
#include <Recast/Recast.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshPointsOfInterest.h>

xiiNavMeshPointOfInterestGraph::xiiNavMeshPointOfInterestGraph()  = default;
xiiNavMeshPointOfInterestGraph::~xiiNavMeshPointOfInterestGraph() = default;

void xiiNavMeshPointOfInterestGraph::IncreaseCheckVisibiblityTimeStamp(xiiTime tNow)
{
  if (tNow - m_LastTimeStampStep < xiiTime::Seconds(0.5f))
    return;

  m_LastTimeStampStep = tNow;
  m_uiCheckVisibilityTimeStamp += 4;
}

XII_ALWAYS_INLINE static xiiVec3 GetNavMeshVertex(const rcPolyMesh* pMesh, xiiUInt16 uiVertex, const xiiVec3& vMeshOrigin, float fCellSize, float fCellHeight)
{
  const xiiUInt16* v = &pMesh->verts[uiVertex * 3];
  const float      x = vMeshOrigin.x + v[0] * fCellSize;
  const float      y = vMeshOrigin.y + v[2] * fCellSize;
  const float      z = vMeshOrigin.z + v[1] * fCellHeight;

  return xiiVec3(x, y, z);
}

const float toCenterOffset  = 0.1f;
const float alongLineOffset = 0.5f;

struct PotentialPoI
{
  bool    m_bUsed;
  xiiVec3 m_vVertexPos;
  xiiVec3 m_vPosition;
  xiiVec3 m_vLineDir;
};

XII_ALWAYS_INLINE static void AddToInterestPoints(xiiDeque<PotentialPoI>& interestPoints, xiiInt32 iVertexIdx, const xiiVec3& pos, const xiiVec3& vPolyCenter, xiiVec3 vLineDir)
{
  xiiVec3 toCenter = vPolyCenter - pos;
  toCenter.SetLength(toCenterOffset).IgnoreResult();

  const xiiVec3 posWithOffset = pos + toCenter + vLineDir * alongLineOffset;

  if (iVertexIdx < 0)
  {
    auto& poi       = interestPoints.ExpandAndGetRef();
    poi.m_bUsed     = true;
    poi.m_vPosition = posWithOffset;
    poi.m_vLineDir  = vLineDir;
  }
  else if (!interestPoints[iVertexIdx].m_bUsed)
  {
    auto& poi       = interestPoints[iVertexIdx];
    poi.m_bUsed     = true;
    poi.m_vPosition = posWithOffset;
    poi.m_vLineDir  = vLineDir;
  }
  else
  {
    auto& poi = interestPoints[iVertexIdx];

    xiiPlane plane;
    plane.SetFromPoints(poi.m_vVertexPos, poi.m_vVertexPos + poi.m_vLineDir, poi.m_vVertexPos + xiiVec3(0, 0, 1.0f)).IgnoreResult();

    xiiPositionOnPlane::Enum side1 = plane.GetPointPosition(poi.m_vPosition);
    xiiPositionOnPlane::Enum side2 = plane.GetPointPosition(posWithOffset);

    if (side1 == side2)
    {
      // same side, collapse points to average position

      poi.m_vPosition = xiiMath::Lerp(poi.m_vPosition, posWithOffset, 0.5f);
    }
    else
    {
      // different sides, keep both points

      auto& poi2       = interestPoints.ExpandAndGetRef();
      poi2.m_bUsed     = true;
      poi2.m_vPosition = posWithOffset;
      poi2.m_vLineDir  = vLineDir;
    }
  }
}

void xiiNavMeshPointOfInterestGraph::ExtractInterestPointsFromMesh(const rcPolyMesh& mesh, bool bReinitialize)
{
  XII_LOG_BLOCK("Extract NavMesh Points of Interest");

  const xiiInt32 iMaxNumVertInPoly = mesh.nvp;
  const float    fCellSize         = mesh.cs;
  const float    fCellHeight       = mesh.ch;

  const xiiVec3 vMeshOrigin(mesh.bmin[0], mesh.bmin[2], mesh.bmin[1]);

  xiiDeque<PotentialPoI> interestPoints;
  interestPoints.SetCount(mesh.nverts);

  for (xiiInt32 i = 0; i < mesh.nverts; ++i)
  {
    interestPoints[i].m_bUsed      = false;
    interestPoints[i].m_vVertexPos = GetNavMeshVertex(&mesh, static_cast<xiiUInt16>(i), vMeshOrigin, fCellSize, fCellHeight);
  }

  xiiStaticArray<xiiVec3, 16>   polyVertices;
  xiiStaticArray<xiiUInt16, 16> polyVertexIndices;
  xiiStaticArray<bool, 16>      isContourEdge;

  for (xiiInt32 i = 0; i < mesh.npolys; ++i)
  {
    const xiiUInt32  uiBaseIndex    = i * (iMaxNumVertInPoly * 2);
    const xiiUInt16* polyVtxIndices = &mesh.polys[uiBaseIndex];
    const xiiUInt16* neighborData   = &mesh.polys[uiBaseIndex + iMaxNumVertInPoly];

    bool hasAnyContour = false;
    polyVertices.Clear();
    polyVertexIndices.Clear();
    isContourEdge.Clear();

    xiiVec3 vPolyCenter;
    vPolyCenter.SetZero();

    for (xiiInt32 j = 0; j < iMaxNumVertInPoly; ++j)
    {
      if (polyVtxIndices[j] == RC_MESH_NULL_IDX)
        break;

      const bool isDisconnected = neighborData[j] == 0xffff;
      hasAnyContour |= isDisconnected;

      const xiiVec3 pos = GetNavMeshVertex(&mesh, polyVtxIndices[j], vMeshOrigin, fCellSize, fCellHeight);
      vPolyCenter += pos;

      polyVertices.PushBack(pos);
      polyVertexIndices.PushBack(polyVtxIndices[j]);
      isContourEdge.PushBack(isDisconnected);
    }

    if (!hasAnyContour)
      continue;

    vPolyCenter /= (float)polyVertices.GetCount();

    // filter out too short edges
    {
      xiiUInt32 uiPrevEdgeIdx = isContourEdge.GetCount() - 2;
      xiiUInt32 uiCurEdgeIdx  = isContourEdge.GetCount() - 1;

      for (xiiUInt32 uiNextEdgeIdx = 0; uiNextEdgeIdx < isContourEdge.GetCount(); ++uiNextEdgeIdx)
      {
        if (isContourEdge[uiCurEdgeIdx])
        {
          const xiiVec3 start      = polyVertices[uiCurEdgeIdx];
          const xiiVec3 end        = polyVertices[uiNextEdgeIdx];
          const xiiVec3 startToEnd = end - start;
          const float   distSqr    = startToEnd.GetLengthSquared();

          if (distSqr < xiiMath::Square(0.5f))
          {
            isContourEdge[uiCurEdgeIdx] = false;
          }
        }

        uiPrevEdgeIdx = uiCurEdgeIdx;
        uiCurEdgeIdx  = uiNextEdgeIdx;
      }
    }

    // filter out medium edges with neighbors
    {
      xiiUInt32 uiPrevEdgeIdx = isContourEdge.GetCount() - 2;
      xiiUInt32 uiCurEdgeIdx  = isContourEdge.GetCount() - 1;

      for (xiiUInt32 uiNextEdgeIdx = 0; uiNextEdgeIdx < isContourEdge.GetCount(); ++uiNextEdgeIdx)
      {
        if (isContourEdge[uiCurEdgeIdx])
        {
          const xiiVec3 start      = polyVertices[uiCurEdgeIdx];
          const xiiVec3 end        = polyVertices[uiNextEdgeIdx];
          const xiiVec3 startToEnd = end - start;
          const float   distSqr    = startToEnd.GetLengthSquared();

          if (distSqr < xiiMath::Square(2.0f))
          {
            // if we have neighbor edges, let them try again
            if (isContourEdge[uiPrevEdgeIdx] || isContourEdge[uiNextEdgeIdx])
            {
              // nothing inserted, so treat this as connected edge
              isContourEdge[uiCurEdgeIdx] = false;
            }
          }
        }

        uiPrevEdgeIdx = uiCurEdgeIdx;
        uiCurEdgeIdx  = uiNextEdgeIdx;
      }
    }

    // now insert points of interests along contour edges
    {
      xiiUInt32 uiPrevEdgeIdx = isContourEdge.GetCount() - 2;
      xiiUInt32 uiCurEdgeIdx  = isContourEdge.GetCount() - 1;

      for (xiiUInt32 uiNextEdgeIdx = 0; uiNextEdgeIdx < isContourEdge.GetCount(); ++uiNextEdgeIdx)
      {
        if (isContourEdge[uiCurEdgeIdx])
        {
          const xiiInt32 startIdx = polyVertexIndices[uiCurEdgeIdx];
          const xiiInt32 endIdx   = polyVertexIndices[uiNextEdgeIdx];

          const xiiVec3 start      = polyVertices[uiCurEdgeIdx];
          const xiiVec3 end        = polyVertices[uiNextEdgeIdx];
          const xiiVec3 startToEnd = end - start;
          const float   distSqr    = startToEnd.GetLengthSquared();

          if (distSqr < xiiMath::Square(2.0f))
          {
            AddToInterestPoints(interestPoints, -1, xiiMath::Lerp(start, end, 0.5f), vPolyCenter, xiiVec3::ZeroVector());
          }
          else
          {
            // to prevent inserting the same point multiple times, only the edge with the larger index is allowed to insert
            // points at connected edges
            // due to the index wrap around, there is no guarantee that uiNextEdgeIdx is always larger than uiCurEdgeIdx etc.

            if (isContourEdge[uiPrevEdgeIdx])
            {
              if (uiCurEdgeIdx > uiPrevEdgeIdx)
              {
                AddToInterestPoints(interestPoints, startIdx, start, vPolyCenter, xiiVec3::ZeroVector());
              }
            }
            else
            {
              AddToInterestPoints(interestPoints, startIdx, start, vPolyCenter, startToEnd.GetNormalized());
            }

            if (isContourEdge[uiNextEdgeIdx])
            {
              if (uiCurEdgeIdx > uiNextEdgeIdx)
              {
                AddToInterestPoints(interestPoints, endIdx, end, vPolyCenter, xiiVec3::ZeroVector());
              }
            }
            else
            {
              AddToInterestPoints(interestPoints, endIdx, end, vPolyCenter, -startToEnd.GetNormalized());
            }
          }
        }

        uiPrevEdgeIdx = uiCurEdgeIdx;
        uiCurEdgeIdx  = uiNextEdgeIdx;
      }
    }
  }

  if (bReinitialize)
  {
    xiiBoundingBox box;
    box.SetInvalid();

    // compute bounding box
    {
      for (auto potPoi : interestPoints)
      {
        if (potPoi.m_bUsed)
        {
          box.ExpandToInclude(potPoi.m_vPosition);
        }
      }

      box.Grow(xiiVec3(1.0f));
    }

    m_NavMeshPointGraph.Initialize(box.GetCenter(), box.GetHalfExtents());
  }



  // add all points
  {
    xiiUInt32 uiNumPoints = 0;

    for (auto potPoi : interestPoints)
    {
      if (potPoi.m_bUsed)
      {
        ++uiNumPoints;

        auto& poi            = m_NavMeshPointGraph.AddPoint(potPoi.m_vPosition);
        poi.m_vFloorPosition = potPoi.m_vPosition;
      }
    }

    xiiLog::Dev("Num Points of Interest: {0}", uiNumPoints);
  }
}
