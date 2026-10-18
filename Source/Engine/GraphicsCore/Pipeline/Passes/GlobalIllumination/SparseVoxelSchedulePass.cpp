/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelSchedulePass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/View.h>
class xiiSparseVoxelSchedulerState : public xiiRefCounted
{
public:
  xiiSparseVoxelRadianceContext m_Context;
  xiiUInt64                     m_Frame = 0;
};
xiiSparseVoxelFrameData::xiiSparseVoxelFrameData(xiiUInt64 setup, xiiUInt64 frame, xiiSharedPtr<xiiSparseVoxelSchedulerState> scheduler) : xiiRenderPipelineFrameData(setup), m_Frame(frame), m_Scheduler(std::move(scheduler)) {}
xiiSparseVoxelFrameData::~xiiSparseVoxelFrameData() = default;
void xiiSparseVoxelFrameData::CommitUpdates() const
{
  if (m_Scheduler->m_Frame != m_Frame)
    return;
  for (const auto& update : m_Updates)
    m_Scheduler->m_Context.CommitBrickUpdate(update.m_vPhysicalLevelAndKey.x, xiiUInt64(update.m_vPhysicalLevelAndKey.y) | (xiiUInt64(update.m_vPhysicalLevelAndKey.z) << 32U), true);
}
xiiSparseVoxelSchedulePass::xiiSparseVoxelSchedulePass()  = default;
xiiSparseVoxelSchedulePass::~xiiSparseVoxelSchedulePass() = default;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSparseVoxelFrameData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSparseVoxelSchedulePass,1,xiiRTTIDefaultAllocator<xiiSparseVoxelSchedulePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PreviousPool",m_PreviousPool)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Schedule",m_Schedule)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ClipmapLevels",m_Levels)->AddAttributes(new xiiDefaultValueAttribute(5U),new xiiClampValueAttribute(1U,8U)),
    XII_MEMBER_PROPERTY("BrickGridResolution",m_Grid)->AddAttributes(new xiiDefaultValueAttribute(8U),new xiiClampValueAttribute(2U,64U)),
    XII_MEMBER_PROPERTY("BrickVoxelResolution",m_Voxels)->AddAttributes(new xiiDefaultValueAttribute(8U),new xiiClampValueAttribute(2U,32U)),
    XII_MEMBER_PROPERTY("ResidentBricks",m_Resident)->AddAttributes(new xiiDefaultValueAttribute(4096U),new xiiClampValueAttribute(1U,1048576U)),
    XII_MEMBER_PROPERTY("UpdateBudget",m_Budget)->AddAttributes(new xiiDefaultValueAttribute(64U),new xiiClampValueAttribute(1U,1048576U)),
    XII_MEMBER_PROPERTY("RefreshInterval",m_Refresh)->AddAttributes(new xiiDefaultValueAttribute(120U),new xiiClampValueAttribute(1U,1000000U)),
    XII_MEMBER_PROPERTY("MaximumPoolBytes",m_MaximumPoolBytes)->AddAttributes(new xiiDefaultValueAttribute(536870912U)),
    XII_MEMBER_PROPERTY("BaseVoxelSize",m_fVoxelSize)->AddAttributes(new xiiDefaultValueAttribute(0.5f),new xiiClampValueAttribute(0.001f,xiiVariant())),
    XII_MEMBER_PROPERTY("Hysteresis",m_fHysteresis)->AddAttributes(new xiiDefaultValueAttribute(0.95f),new xiiClampValueAttribute(0.0f,0.9999f)),
    XII_MEMBER_PROPERTY("UseViewCamera",m_bUseViewCamera)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("CameraPosition",m_CameraPosition),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiSparseVoxelSchedulePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  const xiiUInt64 required = xiiUInt64(m_Grid) * m_Grid * m_Grid * m_Levels;
  const xiiUInt64 count    = xiiUInt64(m_Voxels) * m_Voxels * m_Voxels * m_Resident;
  if (!m_Levels || m_Levels > 8 || m_Grid < 2 || m_Grid > 64 || m_Voxels < 2 || m_Voxels > 32 || required > m_Resident || m_Resident > 1048576 || !m_Budget || m_Budget > 1048576 || !m_Refresh || count * sizeof(xiiVec4) > m_MaximumPoolBytes || !xiiMath::IsFinite(m_fVoxelSize) || m_fVoxelSize <= 0 || !xiiMath::IsFinite(m_fVoxelSize * 128 * m_Voxels) || !xiiMath::IsFinite(m_fHysteresis) || m_fHysteresis < 0 || m_fHysteresis >= 1 || !m_CameraPosition.IsValid())
    return XII_FAILURE;
  m_State.Clear();
  return XII_SUCCESS;
}
void xiiSparseVoxelSchedulePass::InvalidateTemporalHistory()
{
  if (m_State)
    m_State->m_Context.InvalidateHistory();
}
xiiResult xiiSparseVoxelSchedulePass::Build(xiiRenderPipelinePassContext& context)
{
  auto camera = m_CameraPosition;
  if (m_bUseViewCamera)
  {
    if (!context.m_pView || !context.m_pView->GetCamera())
      return context.ReportFailure("Sparse voxel scheduling requires a camera or explicit position.");
    camera = context.m_pView->GetCamera()->GetPosition();
  }
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "SparseVoxelCameraPosition", camera));
  if (!m_State)
  {
    m_State = XII_DEFAULT_NEW(xiiSparseVoxelSchedulerState);
    xiiSparseVoxelRadianceSettings settings;
    settings.m_uiClipmapLevels          = m_Levels;
    settings.m_uiClipmapBrickResolution = m_Grid;
    settings.m_uiBrickVoxelResolution   = m_Voxels;
    settings.m_uiMaxResidentBricks      = m_Resident;
    settings.m_uiBrickUpdateBudget      = m_Budget;
    settings.m_uiRefreshIntervalFrames  = m_Refresh;
    settings.m_fBaseVoxelSize           = m_fVoxelSize;
    settings.m_fTemporalHysteresis      = m_fHysteresis;
    XII_SUCCEED_OR_RETURN(m_State->m_Context.Initialize(settings));
  }
  const auto  count    = m_Voxels * m_Voxels * m_Voxels * m_Resident;
  const auto* previous = context.m_Graph.GetBufferDescription(context.GetInputBuffer(m_PreviousPool));
  const bool  history  = previous && previous->m_Mode == xiiGALBufferMode::Structured && previous->m_uiElementByteStride == sizeof(xiiVec4) && previous->m_uiSize == xiiUInt64(count) * sizeof(xiiVec4);
  if (!history)
    m_State->m_Context.InvalidateHistory();
  if (m_State->m_Context.BeginFrame(camera, context.m_Graph.GetFrameIndex()).Failed())
    return context.ReportFailure("Sparse voxel camera/frame exceeds its clipmap coordinate domain or repeats a frame.");
  m_State->m_Frame                               = context.m_Graph.GetFrameIndex();
  xiiSharedPtr<xiiSparseVoxelFrameData> snapshot = XII_NEW(xiiFoundation::GetAlignedAllocator(), xiiSparseVoxelFrameData, context.m_Graph.GetSetupSerial(), m_State->m_Frame, m_State);
  snapshot->m_PoolCount                          = count;
  snapshot->m_bHistoryValid                      = history;
  snapshot->m_Constants.ClipmapAndUpdateCounts   = xiiVec4U32(m_Levels, m_Grid, m_Voxels, m_State->m_Context.GetScheduledUpdates().GetCount());
  snapshot->m_Constants.PoolLayout               = xiiVec4U32(m_Voxels * m_Voxels * m_Voxels, m_Resident, xiiUInt32(m_State->m_Frame), 0);
  snapshot->m_Constants.RadianceSettings         = xiiVec4(m_fHysteresis, 0, 0, 0);
  const auto pagesPerLevel                       = m_Grid * m_Grid * m_Grid;
  snapshot->m_Pages.SetCount(pagesPerLevel * m_Levels);
  for (auto& page : snapshot->m_Pages)
    page = xiiInvalidIndex;
  for (xiiUInt32 level = 0; level < m_Levels; ++level)
  {
    const auto minimum   = m_State->m_Context.GetClipmapMinimumCells()[level];
    const auto voxelSize = m_fVoxelSize * float(XII_BIT(level)), brickSize = voxelSize * m_Voxels;
    auto&      l                  = snapshot->m_Levels.ExpandAndGetRef();
    l.m_vMinimumCellAndPageOffset = xiiVec4I32(minimum.x, minimum.y, minimum.z, xiiInt32(level * pagesPerLevel));
    l.m_vVoxelAndBrickSize        = xiiVec4(voxelSize, brickSize, 1 / voxelSize, 1 / brickSize);
    for (xiiUInt32 z = 0; z < m_Grid; ++z)
      for (xiiUInt32 y = 0; y < m_Grid; ++y)
        for (xiiUInt32 x = 0; x < m_Grid; ++x)
        {
          const auto* brick = m_State->m_Context.FindBrick(xiiSparseVoxelRadianceContext::PackBrickKey(level, minimum + xiiVec3I32(xiiInt32(x), xiiInt32(y), xiiInt32(z))));
          if (brick && brick->m_Flags.IsSet(xiiSparseVoxelBrickFlags::Valid))
            snapshot->m_Pages[level * pagesPerLevel + (z * m_Grid + y) * m_Grid + x] = brick->m_uiPhysicalBrick;
        }
  }
  for (const auto& update : m_State->m_Context.GetScheduledUpdates())
  {
    auto& record                       = snapshot->m_Updates.ExpandAndGetRef();
    record.m_vWorldMinimumAndVoxelSize = xiiVec4(update.m_vWorldMinimum, update.m_fVoxelSize);
    xiiUInt32 weight;
    xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&weight), reinterpret_cast<const xiiUInt8*>(&update.m_fHistoryWeight), sizeof(weight));
    record.m_vPhysicalLevelAndKey = xiiVec4U32(update.m_uiPhysicalBrick, xiiUInt32(update.m_uiPackedKey), xiiUInt32(update.m_uiPackedKey >> 32U), weight);
  }
  return context.SetOutput(m_Schedule, snapshot);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_SparseVoxelSchedulePass);
