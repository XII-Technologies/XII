/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Meshes/MeshResource.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshResource, 1, xiiRTTIDefaultAllocator<xiiMeshResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiMeshResource);

xiiMeshResource::xiiMeshResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiMeshResource::~xiiMeshResource() = default;

const xiiMeshBufferResourceHandle& xiiMeshResource::GetMeshBuffer() const
{
  return m_hMeshBuffer;
}

const xiiMaterialResourceHandle& xiiMeshResource::GetMaterial(xiiUInt32 uiMaterialIndex) const
{
  static xiiMaterialResourceHandle s_InvalidMaterial;

  if (uiMaterialIndex >= m_hMaterials.GetCount())
    return s_InvalidMaterial;

  return m_hMaterials[uiMaterialIndex];
}

xiiArrayPtr<const xiiMaterialResourceHandle> xiiMeshResource::GetMaterials() const
{
  return m_hMaterials;
}

xiiArrayPtr<const xiiMeshLOD> xiiMeshResource::GetLODs() const
{
  return m_Descriptor.GetLODs();
}

xiiArrayPtr<const xiiMeshSection> xiiMeshResource::GetSections() const
{
  return m_Descriptor.GetSubMeshes();
}

const xiiBoundingBoxSphere& xiiMeshResource::GetBounds() const
{
  return m_Descriptor.GetBounds();
}

xiiUInt32 xiiMeshResource::GetLODCount() const
{
  return m_Descriptor.GetLODs().GetCount();
}

xiiUInt32 xiiMeshResource::GetMeshletCount() const
{
  if (!m_hMeshBuffer.IsValid())
    return 0U;

  xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(m_hMeshBuffer, xiiResourceAcquireMode::PointerOnly);

  return pMeshBuffer->GetMeshletCount();
}

xiiBitflags<xiiMeshResourceUsageFlags> xiiMeshResource::GetUsageFlags() const
{
  return m_Descriptor.m_UsageFlags;
}

xiiEnum<xiiMeshLodSelectionMode> xiiMeshResource::GetLodMode() const
{
  return m_Descriptor.m_LodMode;
}

const xiiMeshResourceDescriptor& xiiMeshResource::GetDescriptor() const
{
  return m_Descriptor;
}

xiiResourceLoadDescription xiiMeshResource::UnloadData(Unload whatToUnload)
{
  XII_IGNORE_UNUSED(whatToUnload);

  m_Descriptor.Clear();
  m_hMeshBuffer.Invalidate();
  m_hMaterials.Clear();

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0U;
  res.m_uiQualityLevelsLoadable    = 0U;
  res.m_State                      = xiiResourceState::Unloaded;
  return res;
}

xiiResourceLoadDescription xiiMeshResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0U;
  res.m_uiQualityLevelsLoadable    = 0U;
  res.m_State                      = xiiResourceState::Loaded;

  if (pStream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  xiiMeshResourceDescriptor descriptor;
  if (descriptor.Load(*pStream).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  CreateResource(std::move(descriptor));
  return res;
}

void xiiMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiMeshResource) + static_cast<xiiUInt32>(m_hMaterials.GetHeapMemoryUsage() + m_Descriptor.GetMaterials().GetCount() * sizeof(xiiString) + m_Descriptor.GetSubMeshes().GetCount() * sizeof(xiiMeshSection) + m_Descriptor.GetLODs().GetCount() * sizeof(xiiMeshLOD));
  out_NewMemoryUsage.m_uiMemoryGPU = 0U;
}

void xiiMeshResource::CreateMeshBufferFromDescriptor(xiiMeshResourceDescriptor& inout_descriptor)
{
  if (inout_descriptor.GetExistingMeshBuffer().IsValid())
  {
    m_hMeshBuffer = inout_descriptor.GetExistingMeshBuffer();
    return;
  }

  xiiStringBuilder sBufferResourceId;
  sBufferResourceId.Set(GetResourceID(), "#MeshBuffer");

  xiiMeshBufferResourceDescriptor bufferDescriptor = std::move(inout_descriptor.GetMeshBufferDescriptor());
  bufferDescriptor.m_bKeepCpuMeshData              = inout_descriptor.m_UsageFlags.IsSet(xiiMeshResourceUsageFlags::CpuReadable);

  m_hMeshBuffer = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>(sBufferResourceId, std::move(bufferDescriptor), GetResourceIdOrDescription());
  inout_descriptor.UseExistingMeshBuffer(m_hMeshBuffer);
}

void xiiMeshResource::LoadMaterialSlots(const xiiMeshResourceDescriptor& descriptor)
{
  m_hMaterials.Clear();
  m_hMaterials.SetCount(descriptor.GetMaterials().GetCount());

  for (xiiUInt32 i = 0; i < descriptor.GetMaterials().GetCount(); ++i)
  {
    xiiStringView sMaterial = descriptor.GetMaterials()[i];

    if (!sMaterial.IsEmpty())
    {
      m_hMaterials[i] = xiiResourceManager::LoadResource<xiiMaterialResource>(sMaterial);
    }
    else
    {
      m_hMaterials[i].Invalidate();
    }
  }
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiMeshResource, xiiMeshResourceDescriptor)
{
  descriptor.ComputeBounds();
  descriptor.BuildMeshlets();

  CreateMeshBufferFromDescriptor(descriptor);
  LoadMaterialSlots(descriptor);

  m_Descriptor = std::move(descriptor);

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0U;
  res.m_uiQualityLevelsLoadable    = m_Descriptor.m_UsageFlags.IsSet(xiiMeshResourceUsageFlags::Streaming) ? 1U : 0U;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_MeshResource);
