#pragma once

template <class CellData>
void xiiGridNavmesh::CreateFromGrid(
  const xiiGameGrid<CellData>& grid,
  CellComparator               isSameCellType,
  void*                        pPassThrough,
  CellBlocked                  isCellBlocked,
  void*                        pPassThrough2)
{
  m_NodesGrid.CreateGrid(Grid.GetGridSizeX(), Grid.GetGridSizeY());

  UpdateRegion(xiiRectU32(Grid.GetGridSizeX(), Grid.GetGridSizeY()), IsSameCellType, pPassThrough, IsCellBlocked, pPassThrough2);

  CreateGraphEdges();
}
