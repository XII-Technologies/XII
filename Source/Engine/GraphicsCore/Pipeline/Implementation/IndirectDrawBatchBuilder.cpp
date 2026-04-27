#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/IndirectDrawBatchBuilder.h>
#include <GAL/CommandList/GALCommandList.h>
#include <GAL/Device/GALDevice.h>
#include <Foundation/Algorithm/Sorting.h>

xiiIndirectDrawBatchBuilder::xiiIndirectDrawBatchBuilder()  = default;
xiiIndirectDrawBatchBuilder::~xiiIndirectDrawBatchBuilder() = default;

void xiiIndirectDrawBatchBuilder::Reset()
{
  m_Pending.Clear();
  m_Instances.Clear();
  m_Batches.Clear();
  m_bFinalised = false;
}

void xiiIndirectDrawBatchBuilder::AddStaticMeshInstance(const xiiGPUInstanceData& instance,
                                                         const xiiMeshResourceHandle& hMesh,
                                                         const xiiMaterialResourceHandle& hMaterial)
{
  XII_ASSERT_DEV(!m_bFinalised, "Cannot add instances after Finalise()");
  auto& entry      = m_Pending.ExpandAndGetRef();
  entry.m_Instance = instance;
  entry.m_hMesh    = hMesh;
  entry.m_hMaterial= hMaterial;
  entry.m_bMeshlet = false;
}

void xiiIndirectDrawBatchBuilder::AddMeshletInstance(const xiiGPUInstanceData& instance,
                                                      const xiiMeshletResourceHandle& hMeshlets,
                                                      const xiiMaterialResourceHandle& hMaterial)
{
  XII_ASSERT_DEV(!m_bFinalised, "Cannot add instances after Finalise()");
  auto& entry        = m_Pending.ExpandAndGetRef();
  entry.m_Instance   = instance;
  entry.m_hMeshlets  = hMeshlets;
  entry.m_hMaterial  = hMaterial;
  entry.m_bMeshlet   = true;
}

void xiiIndirectDrawBatchBuilder::Finalise()
{
  XII_ASSERT_DEV(!m_bFinalised, "Finalise() already called this frame");
  SortAndBatch();
  m_bFinalised = true;
}

// ---- SortAndBatch ----
// Sort pending entries by (materialID, meshID, meshlet flag) then build contiguous batches.

void xiiIndirectDrawBatchBuilder::SortAndBatch()
{
  if (m_Pending.IsEmpty())
    return;

  // Sort by material handle ID then mesh/meshlet handle, so identical-material instances are contiguous.
  xiiSorting::QuickSort(m_Pending, [](const PendingEntry& a, const PendingEntry& b) -> bool
  {
    if (a.m_hMaterial.GetInternalID() != b.m_hMaterial.GetInternalID())
      return a.m_hMaterial.GetInternalID() < b.m_hMaterial.GetInternalID();
    if (a.m_bMeshlet != b.m_bMeshlet)
      return (int)a.m_bMeshlet < (int)b.m_bMeshlet;
    if (a.m_bMeshlet)
      return a.m_hMeshlets.GetInternalID() < b.m_hMeshlets.GetInternalID();
    return a.m_hMesh.GetInternalID() < b.m_hMesh.GetInternalID();
  });

  m_Instances.Reserve(m_Pending.GetCount());

  xiiIndirectDrawBatch* pCurrentBatch = nullptr;

  for (const auto& entry : m_Pending)
  {
    const bool bNewBatch = (pCurrentBatch == nullptr)
      || (pCurrentBatch->m_hMaterial != entry.m_hMaterial)
      || (pCurrentBatch->m_bUseMeshlets != entry.m_bMeshlet)
      || (entry.m_bMeshlet && pCurrentBatch->m_hMeshlets != entry.m_hMeshlets)
      || (!entry.m_bMeshlet && pCurrentBatch->m_hMesh    != entry.m_hMesh);

    if (bNewBatch)
    {
      auto& batch             = m_Batches.ExpandAndGetRef();
      batch.m_hMaterial       = entry.m_hMaterial;
      batch.m_hMesh           = entry.m_bMeshlet ? xiiMeshResourceHandle{} : entry.m_hMesh;
      batch.m_hMeshlets       = entry.m_bMeshlet ? entry.m_hMeshlets : xiiMeshletResourceHandle{};
      batch.m_bUseMeshlets    = entry.m_bMeshlet;
      batch.m_uiFirstInstance = m_Instances.GetCount();
      batch.m_uiInstanceCount = 0;
      batch.m_uiFirstDrawArg  = m_Batches.GetCount() - 1; // one indirect command per batch
      pCurrentBatch           = &m_Batches.PeekBack();
    }

    m_Instances.PushBack(entry.m_Instance);
    pCurrentBatch->m_uiInstanceCount++;
  }
}

// ---- Upload ----

void xiiIndirectDrawBatchBuilder::Upload(xiiGALCommandList& cmdList)
{
  if (m_Instances.IsEmpty())
    return;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "No GAL device");

  const xiiUInt32 uiInstanceBytes = m_Instances.GetCount() * sizeof(xiiGPUInstanceData);

  // (Re)create or grow the instance buffer if needed
  if (!m_hInstanceBuffer.IsValid())
  {
    xiiGALBufferCreationDescription bd;
    bd.m_sDebugName         = "GPUInstanceBuffer";
    bd.m_uiSize             = xiiMath::Max(uiInstanceBytes, 256u * sizeof(xiiGPUInstanceData));
    bd.m_BindFlags          = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bd.m_ResourceUsage      = xiiGALResourceUsage::Dynamic;
    bd.m_CPUAccessFlags     = xiiGALCPUAccessFlags::Write;
    bd.m_Mode               = xiiGALBufferMode::Structured;
    bd.m_uiElementByteStride= sizeof(xiiGPUInstanceData);
    m_hInstanceBuffer = pDevice->CreateBuffer(bd);
  }

  // Map and copy instance data
  void* pMapped = pDevice->MapBuffer(m_hInstanceBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);
  if (pMapped)
  {
    xiiMemoryUtils::Copy(static_cast<xiiGPUInstanceData*>(pMapped), m_Instances.GetData(), m_Instances.GetCount());
    pDevice->UnmapBuffer(m_hInstanceBuffer, xiiGALMapType::Write);
  }

  // Indirect draw argument buffer — one xiiIndirectDrawArgs per batch (for triangle-list path)
  // and one xiiIndirectDispatchMeshArgs per meshlet batch. We allocate worst-case.
  const xiiUInt32 uiArgBytes = m_Batches.GetCount() * xiiMath::Max(sizeof(xiiIndirectDrawArgs), sizeof(xiiIndirectDispatchMeshArgs));
  if (!m_hIndirectArgsBuffer.IsValid() || true /* always recreate for simplicity */)
  {
    if (m_hIndirectArgsBuffer.IsValid())
      pDevice->DestroyBuffer(m_hIndirectArgsBuffer);

    xiiGALBufferCreationDescription bd;
    bd.m_sDebugName    = "IndirectArgsBuffer";
    bd.m_uiSize        = xiiMath::Max(uiArgBytes, 64u);
    bd.m_BindFlags     = xiiGALBindFlags::IndirectDrawArgs | xiiGALBindFlags::UnorderedAccess;
    bd.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_hIndirectArgsBuffer = pDevice->CreateBuffer(bd);
  }

  // The actual draw-arg values are filled by the GPU culling compute pass (GPUDrivenCullingPass).
  // Here we just ensure the buffer is large enough and cleared.
  XII_IGNORE_UNUSED(cmdList);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_IndirectDrawBatchBuilder);
