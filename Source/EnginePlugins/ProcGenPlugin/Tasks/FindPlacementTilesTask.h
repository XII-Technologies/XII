#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>

enum
{
  EmptyTileIndex = xiiInvalidIndex,
  NewTileIndex   = EmptyTileIndex - 1
};

class xiiProcPlacementComponent;

namespace xiiProcGenInternal
{
  class FindPlacementTilesTask final : public xiiTask
  {
  public:
    FindPlacementTilesTask(xiiProcPlacementComponent* pComponent, xiiUInt32 uiOutputIndex);
    ~FindPlacementTilesTask();

    void AddCameraPosition(const xiiVec3& vCameraPosition) { m_CameraPositions.PushBack(vCameraPosition); }

    xiiArrayPtr<const PlacementTileDesc> GetNewTiles() const { return m_NewTiles; }
    xiiArrayPtr<const xiiUInt64>         GetOldTiles() const { return m_OldTileKeys; }

  private:
    virtual void Execute() override;

    xiiProcPlacementComponent* m_pComponent    = nullptr;
    xiiUInt32                  m_uiOutputIndex = 0;

    xiiHybridArray<xiiVec3, 2> m_CameraPositions;

    xiiDynamicArray<PlacementTileDesc, xiiAlignedAllocatorWrapper> m_NewTiles;
    xiiDynamicArray<xiiUInt64>                                     m_OldTileKeys;

    struct TileByAge
    {
      XII_DECLARE_POD_TYPE();

      xiiUInt64 m_uiTileKey;
      xiiUInt64 m_uiLastSeenFrame;
    };

    xiiDynamicArray<TileByAge> m_TilesByAge;
  };

  XII_ALWAYS_INLINE xiiUInt64 GetTileKey(xiiInt32 x, xiiInt32 y)
  {
    xiiUInt64 sx = (xiiUInt32)x;
    xiiUInt64 sy = (xiiUInt32)y;

    return (sx << 32) | sy;
  }
} // namespace xiiProcGenInternal
