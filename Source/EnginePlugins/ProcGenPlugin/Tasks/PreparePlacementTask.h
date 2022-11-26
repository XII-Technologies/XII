#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>

namespace xiiProcGenInternal
{
  class PreparePlacementTask final : public xiiTask
  {
  public:
    PreparePlacementTask(PlacementData* pData, const char* szName);
    ~PreparePlacementTask();

    void Clear() {}

  private:
    friend class PlacementTile;

    PlacementData* m_pData = nullptr;

    virtual void Execute() override;
  };
} // namespace xiiProcGenInternal
