/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Utilities/UtilitiesDLL.h>

/// xiiGameGrid is a general purpose 2D grid structure that has several convenience functions which are often required when working
/// with a grid.
template <class CellData>
class xiiGameGrid
{
public:
  enum Orientation
  {
    InPlaneXY,      ///< The grid is expected to lie in the XY plane in world-space (when Y is up, this is similar to a 2D side scroller)
    InPlaneXZ,      ///< The grid is expected to lie in the XZ plane in world-space (when Y is up, this is similar to a top down RTS game)
    InPlaneXminusZ, ///< The grid is expected to lie in the XZ plane in world-space (when Y is up, this is similar to a top down RTS game)
  };

  xiiGameGrid();

  /// Clears all data and reallocates the grid with the given dimensions.
  void CreateGrid(xiiUInt16 uiSizeX, xiiUInt16 uiSizeY);

  /// Sets the lower left position of the grid in world space coordinates and the cell size.
  ///
  /// Together with the grid size, these values determine the final world space dimensions.
  /// The rotation defines how the grid is rotated in world space. An identity rotation means that grid cell coordinates (X, Y)
  /// map directly to world space coordinates (X, Y). So the grid is 'standing up' in world space (considering that Y is 'up').
  /// Other rotations allow to rotate the grid into other planes, such as XZ, if that is more convenient.
  void SetWorldSpaceDimensions(const xiiVec3& vLowerLeftCorner, const xiiVec3& vCellSize, Orientation ori = InPlaneXZ);

  /// Sets the lower left position of the grid in world space coordinates and the cell size.
  ///
  /// Together with the grid size, these values determine the final world space dimensions.
  /// The rotation defines how the grid is rotated in world space. An identity rotation means that grid cell coordinates (X, Y)
  /// map directly to world space coordinates (X, Y). So the grid is 'standing up' in world space (considering that Y is 'up').
  /// Other rotations allow to rotate the grid into other planes, such as XZ, if that is more convenient.
  void SetWorldSpaceDimensions(const xiiVec3& vLowerLeftCorner, const xiiVec3& vCellSize, const xiiMat3& mRotation);

  /// Returns the size of each cell.
  xiiVec3 GetCellSize() const { return m_vLocalSpaceCellSize; }

  /// Returns the coordinate of the cell at the given world-space position. The world space dimension must be set for this to work.
  /// The indices might be outside valid ranges (negative, larger than the maximum size).
  xiiVec2I32 GetCellAtWorldPosition(const xiiVec3& vWorldSpacePos) const;

  /// Returns the number of cells along the X axis.
  xiiUInt16 GetGridSizeX() const { return m_uiGridSizeX; }

  /// Returns the number of cells along the Y axis.
  xiiUInt16 GetGridSizeY() const { return m_uiGridSizeY; }

  /// Returns the world-space bounding box of the grid, as specified via SetWorldDimensions.
  xiiBoundingBox GetWorldBoundingBox() const;

  /// Returns the total number of cells.
  xiiUInt32 GetNumCells() const { return m_uiGridSizeX * m_uiGridSizeY; }

  /// Gives access to a cell by cell index.
  CellData& GetCell(xiiUInt32 uiIndex) { return m_Cells[uiIndex]; }

  /// Gives access to a cell by cell index.
  const CellData& GetCell(xiiUInt32 uiIndex) const { return m_Cells[uiIndex]; }

  /// Gives access to a cell by cell coordinates.
  CellData& GetCell(const xiiVec2I32& vCoord) { return m_Cells[ConvertCellCoordinateToIndex(vCoord)]; }

  /// Gives access to a cell by cell coordinates.
  const CellData& GetCell(const xiiVec2I32& vCoord) const { return m_Cells[ConvertCellCoordinateToIndex(vCoord)]; }

  /// Converts a cell index into a 2D cell coordinate.
  xiiVec2I32 ConvertCellIndexToCoordinate(xiiUInt32 uiIndex) const { return xiiVec2I32(uiIndex % m_uiGridSizeX, uiIndex / m_uiGridSizeX); }

  /// Converts a cell coordinate into a cell index.
  xiiUInt32 ConvertCellCoordinateToIndex(const xiiVec2I32& vCoord) const { return vCoord.y * m_uiGridSizeX + vCoord.x; }

  /// Returns the lower left world space position of the cell with the given coordinates.
  xiiVec3 GetCellWorldSpaceOrigin(const xiiVec2I32& vCoord) const;
  xiiVec3 GetCellLocalSpaceOrigin(const xiiVec2I32& vCoord) const;

  /// Returns the center world space position of the cell with the given coordinates.
  xiiVec3 GetCellWorldSpaceCenter(const xiiVec2I32& vCoord) const;
  xiiVec3 GetCellLocalSpaceCenter(const xiiVec2I32& vCoord) const;

  /// Checks whether the given cell coordinate is inside valid ranges.
  bool IsValidCellCoordinate(const xiiVec2I32& vCoord) const;

  /// Casts a world space ray through the grid and determines which cell is hit (if any).
  /// \note The picked cell is determined from where the ray hits the 'ground plane', ie. the plane that goes through the world space
  /// origin.
  bool PickCell(const xiiVec3& vRayStartPos, const xiiVec3& vRayDirNorm, xiiVec2I32* out_pCellCoord, xiiVec3* out_pIntersection = nullptr) const;

  /// Returns the lower left corner position in world space of the grid
  const xiiVec3& GetWorldSpaceOrigin() const { return m_vWorldSpaceOrigin; }

  /// Returns the matrix used to rotate coordinates from grid space to world space
  const xiiMat3& GetRotationToWorldSpace() const { return m_mRotateToWorldspace; }

  /// Returns the matrix used to rotate coordinates from world space to grid space
  const xiiMat3& GetRotationToGridSpace() const { return m_mRotateToGridspace; }

  /// Tests where and at which cell the given world space ray intersects the grids bounding box
  bool GetRayIntersection(const xiiVec3& vRayStartWorldSpace, const xiiVec3& vRayDirNormalizedWorldSpace, float fMaxLength, float& out_fIntersection, xiiVec2I32& out_vCellCoord) const;

  /// Tests whether a ray would hit the grid bounding box, if it were expanded by a constant.
  bool GetRayIntersectionExpandedBBox(const xiiVec3& vRayStartWorldSpace, const xiiVec3& vRayDirNormalizedWorldSpace, float fMaxLength, float& out_fIntersection, const xiiVec3& vExpandBBoxByThis) const;

  xiiResult Serialize(xiiStreamWriter& ref_stream) const;
  xiiResult Deserialize(xiiStreamReader& ref_stream);

private:
  xiiUInt16 m_uiGridSizeX;
  xiiUInt16 m_uiGridSizeY;

  xiiMat3 m_mRotateToWorldspace;
  xiiMat3 m_mRotateToGridspace;

  xiiVec3 m_vWorldSpaceOrigin;
  xiiVec3 m_vLocalSpaceCellSize;
  xiiVec3 m_vInverseLocalSpaceCellSize;

  xiiDynamicArray<CellData> m_Cells;
};

#include <Utilities/DataStructures/Implementation/GameGrid_inl.h>
