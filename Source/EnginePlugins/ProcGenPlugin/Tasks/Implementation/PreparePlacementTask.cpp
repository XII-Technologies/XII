#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>
#include <ProcGenPlugin/Tasks/PreparePlacementTask.h>
#include <ProcGenPlugin/Tasks/Utils.h>

using namespace xiiProcGenInternal;

PreparePlacementTask::PreparePlacementTask(PlacementData* pData, const char* szName) :
  m_pData(pData)
{
  ConfigureTask(szName, xiiTaskNesting::Maybe);
}

PreparePlacementTask::~PreparePlacementTask() = default;

void PreparePlacementTask::Execute()
{
  const xiiWorld&       world  = *m_pData->m_pWorld;
  const xiiBoundingBox& box    = m_pData->m_TileBoundingBox;
  const Output&         output = *m_pData->m_pOutput;

  xiiProcGenInternal::ExtractVolumeCollections(world, box, output, m_pData->m_VolumeCollections, m_pData->m_GlobalData);
}
