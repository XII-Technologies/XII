#pragma once

#include <Core/World/WorldModule.h>
#include <GraphicsCore/Declarations.h>

/// \brief Feedback entry produced by the GPU's virtual-texture page-table analysis pass.
struct XII_GRAPHICSCORE_DLL xiiVTFeedbackEntry
{
  xiiUInt32 m_uiPageX      = 0; ///< Logical page column in the virtual texture.
  xiiUInt32 m_uiPageY      = 0; ///< Logical page row in the virtual texture.
  xiiUInt8  m_uiMipLevel   = 0; ///< Requested mip level.
  xiiUInt8  m_uiAtlasSlot  = 0; ///< Physical atlas tile where the page should be placed.
};

/// \brief World module that manages virtual texture (sparse/megatexture) streaming.
///
/// Each frame the manager:
///   1. Reads the GPU feedback buffer to determine which pages were accessed.
///   2. Evicts the least-recently-used pages from the physical tile atlas.
///   3. Schedules streaming loads for the highest-priority missing pages.
///   4. Updates the GPU page table texture so shaders resolve correctly.
class XII_GRAPHICSCORE_DLL xiiVirtualTexturingManager : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiVirtualTexturingManager, xiiWorldModule);

public:
  xiiVirtualTexturingManager(xiiWorld* pWorld);
  ~xiiVirtualTexturingManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  /// \brief Maximum number of new pages that may be streamed in per frame.
  void      SetMaxUploadsPerFrame(xiiUInt32 n) { m_uiMaxUploadsPerFrame = n; }
  xiiUInt32  GetMaxUploadsPerFrame() const     { return m_uiMaxUploadsPerFrame; }

  /// \brief Physical atlas tile count along one axis (atlas is square).
  void      SetAtlasTileCount(xiiUInt32 n)     { m_uiAtlasTileCount = n; }
  xiiUInt32  GetAtlasTileCount() const         { return m_uiAtlasTileCount; }

  /// \brief Forces all cached pages to be evicted and re-streamed.
  void FlushAll();

  /// \brief Returns the number of pages currently resident in the atlas.
  xiiUInt32 GetResidentPageCount() const { return m_uiResidentPages; }

private:
  void Update(const xiiWorldModule::UpdateContext& ctx);
  void ProcessFeedback();
  void EvictLRUPages(xiiUInt32 uiSlotCount);

  xiiUInt32                           m_uiMaxUploadsPerFrame = 4;
  xiiUInt32                           m_uiAtlasTileCount     = 256;
  xiiUInt32                           m_uiResidentPages      = 0;
  xiiDynamicArray<xiiVTFeedbackEntry> m_PendingUploads;
  bool                                m_bFlushPending        = false;
};
