/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Rect.h>
#include <Utilities/DataStructures/GameGrid.h>

/// Takes a xiiGameGrid and creates an optimized navmesh structure from it, that is more efficient for path searches.
class XII_UTILITIES_DLL xiiGridNavmesh
{
public:
  struct ConvexArea
  {
    XII_DECLARE_POD_TYPE();

    /// The space that is enclosed by this convex area.
    xiiRectU32 m_Rect;

    /// The first AreaEdge that belongs to this ConvexArea.
    xiiUInt32 m_uiFirstEdge;

    /// The number of AreaEdge's that belong to this ConvexArea.
    xiiUInt32 m_uiNumEdges;
  };

  struct AreaEdge
  {
    XII_DECLARE_POD_TYPE();

    /// The 'area' of the edge. This is a one cell wide line that is always WITHIN the ConvexArea from where the edge connects to a neighbor
    /// area.
    xiiRectU16 m_EdgeRect;

    /// The index of the area that can be reached over this edge. This is always a valid index.
    xiiInt32 m_iNeighborArea;
  };

  /// Callback that determines whether the cell with index \a uiCell1 and the cell with index \a uiCell2 represent the same type of
  /// terrain.
  using CellComparator = bool (*)(xiiUInt32, xiiUInt32, void*);

  /// Callback that determines whether the cell with index \a uiCell is blocked entirely (for every type of unit) and therefore can
  /// be optimized away.
  using CellBlocked = bool (*)(xiiUInt32, void*);

  /// Creates the navmesh from the given xiiGameGrid.
  template <class CellData>
  void CreateFromGrid(const xiiGameGrid<CellData>& grid, CellComparator isSameCellType, void* pPassThroughSame, CellBlocked isCellBlocked, void* pPassThroughBlocked);

  /// Returns the index of the ConvexArea at the given cell coordinates. Negative, if the cell is blocked.
  xiiInt32 GetAreaAt(const xiiVec2I32& vCoord) const { return m_NodesGrid.GetCell(vCoord); }

  /// Returns the number of convex areas that this navmesh consists of.
  xiiUInt32 GetNumConvexAreas() const { return m_ConvexAreas.GetCount(); }

  /// Returns the given convex area by index.
  const ConvexArea& GetConvexArea(xiiInt32 iArea) const { return m_ConvexAreas[iArea]; }

  /// Returns the number of edges between convex areas.
  xiiUInt32 GetNumAreaEdges() const { return m_GraphEdges.GetCount(); }

  /// Returns the given area edge by index.
  const AreaEdge& GetAreaEdge(xiiInt32 iAreaEdge) const { return m_GraphEdges[iAreaEdge]; }

private:
  void UpdateRegion(xiiRectU32 region, CellComparator IsSameCellType, void* pPassThrough1, CellBlocked IsCellBlocked, void* pPassThrough2);

  void Optimize(xiiRectU32 region, CellComparator IsSameCellType, void* pPassThrough);
  bool OptimizeBoxes(xiiRectU32 region, CellComparator IsSameCellType, void* pPassThrough, xiiUInt32 uiIntervalX, xiiUInt32 uiIntervalY, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiOffsetX = 0, xiiUInt32 uiOffsetY = 0);
  bool CanCreateArea(xiiRectU32 region, CellComparator IsSameCellType, void* pPassThrough) const;

  bool CanMergeRight(xiiInt32 x, xiiInt32 y, CellComparator IsSameCellType, void* pPassThrough, xiiRectU32& out_Result) const;
  bool CanMergeDown(xiiInt32 x, xiiInt32 y, CellComparator IsSameCellType, void* pPassThrough, xiiRectU32& out_Result) const;
  bool MergeBestFit(xiiRectU32 region, CellComparator IsSameCellType, void* pPassThrough);

  void CreateGraphEdges();
  void CreateGraphEdges(ConvexArea& Area);

  xiiRectU32 GetCellBBox(xiiInt32 x, xiiInt32 y) const;
  void       Merge(const xiiRectU32& rect);
  void       CreateNodes(xiiRectU32 region, CellBlocked IsCellBlocked, void* pPassThrough);

  xiiGameGrid<xiiInt32>       m_NodesGrid;
  xiiDynamicArray<ConvexArea> m_ConvexAreas;
  xiiDeque<AreaEdge>          m_GraphEdges;
};

#include <Utilities/PathFinding/Implementation/GridNavmesh_inl.h>
