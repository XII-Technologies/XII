#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Managers/VirtualTexturingManager.h>

// clang-format off
XII_IMPLEMENT_WORLD_MODULE(xiiVirtualTexturingManager);

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVirtualTexturingManager, 1, xiiRTTIDefaultAllocator<xiiVirtualTexturingManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVirtualTexturingManager::xiiVirtualTexturingManager(xiiWorld* pWorld)
  : xiiWorldModule(pWorld)
{
}

xiiVirtualTexturingManager::~xiiVirtualTexturingManager() = default;

void xiiVirtualTexturingManager::Initialize()
{
  auto desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiVirtualTexturingManager::Update, this);
  desc.m_Phase                     = xiiWorldModule::UpdateFunctionDesc::Phase::PostTransform;
  desc.m_bOnlyUpdateWhenSimulating = false;
  RegisterUpdateFunction(desc);
}

void xiiVirtualTexturingManager::Deinitialize()
{
  m_PendingUploads.Clear();
  m_uiResidentPages = 0;
}

void xiiVirtualTexturingManager::FlushAll()
{
  m_bFlushPending  = true;
  m_uiResidentPages = 0;
  m_PendingUploads.Clear();
}

void xiiVirtualTexturingManager::Update(const xiiWorldModule::UpdateContext& ctx)
{
  XII_IGNORE_UNUSED(ctx);

  if (m_bFlushPending)
  {
    m_bFlushPending = false;
    // In a full implementation: clear the GPU page table and evict all atlas tiles.
  }

  // 1. Read GPU feedback — in a real system this maps a readback buffer.
  ProcessFeedback();

  // 2. Sort pending uploads by priority (here trivially FIFO).
  const xiiUInt32 uiCount = xiiMath::Min<xiiUInt32>(m_uiMaxUploadsPerFrame, m_PendingUploads.GetCount());

  // 3. Upload pages and update the GPU page table.
  for (xiiUInt32 i = 0; i < uiCount; ++i)
  {
    const xiiVTFeedbackEntry& entry = m_PendingUploads[i];
    XII_IGNORE_UNUSED(entry);
    // Stub: in a full implementation, stream the page from disk and upload to the atlas.
    ++m_uiResidentPages;
  }

  // Remove processed entries
  if (uiCount > 0)
    m_PendingUploads.RemoveAtAndCopy(0, uiCount);
}

void xiiVirtualTexturingManager::ProcessFeedback()
{
  // Stub: in a full implementation this reads the GPU feedback buffer via a mapped
  // readback resource and fills m_PendingUploads with entries for missing pages.
}

void xiiVirtualTexturingManager::EvictLRUPages(xiiUInt32 uiSlotCount)
{
  // Stub: in a full implementation this removes the least-recently-used pages
  // from the physical atlas to make room for new ones.
  const xiiUInt32 uiEvict = xiiMath::Min(uiSlotCount, m_uiResidentPages);
  m_uiResidentPages        = (m_uiResidentPages > uiEvict) ? (m_uiResidentPages - uiEvict) : 0;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Managers_VirtualTexturingManager);
