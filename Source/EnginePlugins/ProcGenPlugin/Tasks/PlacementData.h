#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionFunctions.h>
#include <ProcGenPlugin/Declarations.h>

class xiiPhysicsWorldModuleInterface;
class xiiVolumeCollection;

namespace xiiProcGenInternal
{
  struct PlacementData
  {
    PlacementData();
    ~PlacementData();

    void Clear();

    const xiiPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;
    const xiiWorld*                       m_pWorld         = nullptr;

    xiiSharedPtr<const PlacementOutput> m_pOutput;
    xiiInt32                            m_iTileSeed = 0;
    xiiBoundingBox                      m_TileBoundingBox;

    xiiDynamicArray<xiiSimdMat4f, xiiAlignedAllocatorWrapper> m_GlobalToLocalBoxTransforms;

    xiiDeque<xiiVolumeCollection> m_VolumeCollections;
    xiiExpression::GlobalData     m_GlobalData;
  };
} // namespace xiiProcGenInternal
