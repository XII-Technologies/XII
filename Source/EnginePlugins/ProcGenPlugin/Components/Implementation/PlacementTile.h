#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Types/UniquePtr.h>
#include <ProcGenPlugin/Declarations.h>

class xiiPhysicsWorldModuleInterface;

namespace xiiProcGenInternal
{
  class PlacementTile
  {
  public:
    PlacementTile();
    PlacementTile(PlacementTile&& other);
    ~PlacementTile();

    void Initialize(const PlacementTileDesc& desc, xiiSharedPtr<const PlacementOutput>& ref_pOutput);
    void Deinitialize(xiiWorld& ref_world);

    bool IsValid() const;

    const PlacementTileDesc&               GetDesc() const;
    const PlacementOutput*                 GetOutput() const;
    xiiArrayPtr<const xiiGameObjectHandle> GetPlacedObjects() const;
    xiiBoundingBox                         GetBoundingBox() const;
    xiiColor                               GetDebugColor() const;

    void PreparePlacementData(const xiiWorld* pWorld, const xiiPhysicsWorldModuleInterface* pPhysicsModule, PlacementData& ref_placementData);

    xiiUInt32 PlaceObjects(xiiWorld& ref_world, xiiArrayPtr<const PlacementTransform> objectTransforms);

  private:
    PlacementTileDesc                   m_Desc;
    xiiSharedPtr<const PlacementOutput> m_pOutput;

    struct State
    {
      enum Enum
      {
        Invalid,
        Initialized,
        Scheduled,
        Finished
      };
    };

    State::Enum                          m_State = State::Invalid;
    xiiDynamicArray<xiiGameObjectHandle> m_PlacedObjects;
  };
} // namespace xiiProcGenInternal
