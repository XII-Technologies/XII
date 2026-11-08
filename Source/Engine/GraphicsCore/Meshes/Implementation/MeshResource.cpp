/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <GraphicsCore/Meshes/MeshResource.h>

namespace
{
  xiiResult ValidateCompiledMeshlets(const xiiMeshResourceDescriptor& descriptor)
  {
    XII_PROFILE_SCOPE("Compiled meshlet validation");
    const auto& buffer         = descriptor.GetMeshBufferDescriptor();
    const auto  primitiveCount = buffer.GetPrimitiveCount();
    const auto  indexSize      = buffer.m_IndexType == xiiGALValueType::UInt16 ? 2U : 4U;
    if (buffer.m_Topology != xiiGALPrimitiveTopology::TriangleList || buffer.GetIndexCount() % 3 != 0 ||
        (buffer.m_IndexType != xiiGALValueType::UInt16 && buffer.m_IndexType != xiiGALValueType::UInt32) ||
        xiiUInt64(buffer.GetVertexCount()) * buffer.GetVertexStride() > buffer.m_VertexData.GetCount() ||
        xiiUInt64(buffer.GetIndexCount()) * indexSize > buffer.m_IndexData.GetCount())
      return XII_FAILURE;
    const xiiMeshVertexStream* position = nullptr;
    for (const auto& stream : buffer.m_VertexStreams)
      if (stream.m_Semantic == xiiMeshVertexSemantic::Position)
        position = &stream;
    if (!position || position->m_Format != xiiMeshVertexStreamFormat::Float3 || position->m_uiStride < sizeof(xiiVec3) ||
        position->m_uiOffset > position->m_uiStride - sizeof(xiiVec3) || xiiUInt64(buffer.GetVertexCount()) * position->m_uiStride > buffer.m_VertexData.GetCount())
      return XII_FAILURE;
    for (xiiUInt32 i = 0; i < buffer.GetVertexCount(); ++i)
    {
      xiiVec3 value;
      xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&value), buffer.m_VertexData.GetData() + i * position->m_uiStride + position->m_uiOffset, sizeof(value));
      if (!value.IsValid())
        return XII_FAILURE;
    }
    for (xiiUInt32 i = 0; i < buffer.GetIndexCount(); ++i)
    {
      const xiiUInt32 index = indexSize == 2U ? reinterpret_cast<const xiiUInt16*>(buffer.m_IndexData.GetData())[i] : reinterpret_cast<const xiiUInt32*>(buffer.m_IndexData.GetData())[i];
      if (index >= buffer.GetVertexCount())
        return XII_FAILURE;
    }
    for (const auto& meshlet : buffer.m_Meshlets)
    {
      if (meshlet.m_uiVertexCount == 0 || meshlet.m_uiVertexCount > xiiMeshlet::s_uiMaxVertices || meshlet.m_uiPrimitiveCount == 0 || meshlet.m_uiPrimitiveCount > xiiMeshlet::s_uiMaxPrimitives ||
          meshlet.m_uiFirstPrimitive > primitiveCount || meshlet.m_uiPrimitiveCount > primitiveCount - meshlet.m_uiFirstPrimitive ||
          meshlet.m_uiVertexRemapOffset > buffer.m_MeshletVertexRemap.GetCount() || meshlet.m_uiVertexCount > buffer.m_MeshletVertexRemap.GetCount() - meshlet.m_uiVertexRemapOffset ||
          meshlet.m_uiPrimitiveIndexOffset > buffer.m_MeshletPrimitiveIndices.GetCount() || meshlet.m_uiPrimitiveCount * 3U > buffer.m_MeshletPrimitiveIndices.GetCount() - meshlet.m_uiPrimitiveIndexOffset ||
          !meshlet.m_Bounds.IsValid() || !meshlet.m_vConeAxis.IsValid() || !xiiMath::IsFinite(meshlet.m_fConeCutoff))
        return XII_FAILURE;
      for (xiiUInt32 i = 0; i < meshlet.m_uiVertexCount; ++i)
        if (buffer.m_MeshletVertexRemap[meshlet.m_uiVertexRemapOffset + i] >= buffer.GetVertexCount())
          return XII_FAILURE;
      for (xiiUInt32 i = 0; i < meshlet.m_uiPrimitiveCount * 3U; ++i)
        if (buffer.m_MeshletPrimitiveIndices[meshlet.m_uiPrimitiveIndexOffset + i] >= meshlet.m_uiVertexCount)
          return XII_FAILURE;
    }
    for (const auto& section : descriptor.GetSubMeshes())
      if (section.m_uiFirstPrimitive > primitiveCount || section.m_uiPrimitiveCount > primitiveCount - section.m_uiFirstPrimitive ||
          section.m_uiFirstMeshlet > buffer.m_Meshlets.GetCount() || section.m_uiMeshletCount > buffer.m_Meshlets.GetCount() - section.m_uiFirstMeshlet)
        return XII_FAILURE;
    for (const auto& lod : descriptor.GetLODs())
    {
      if (lod.m_uiFirstMeshlet > buffer.m_Meshlets.GetCount() || lod.m_uiMeshletCount > buffer.m_Meshlets.GetCount() - lod.m_uiFirstMeshlet)
        return XII_FAILURE;
      for (const auto& section : lod.m_Sections)
        if (section.m_uiFirstPrimitive > primitiveCount || section.m_uiPrimitiveCount > primitiveCount - section.m_uiFirstPrimitive ||
            section.m_uiFirstMeshlet < lod.m_uiFirstMeshlet || section.m_uiFirstMeshlet > lod.m_uiFirstMeshlet + lod.m_uiMeshletCount ||
            section.m_uiMeshletCount > lod.m_uiFirstMeshlet + lod.m_uiMeshletCount - section.m_uiFirstMeshlet)
          return XII_FAILURE;
    }
    return XII_SUCCESS;
  }
} // namespace

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
  if (descriptor.Load(*pStream, GetCurrentlyUpdatingLoadData()).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(descriptor));
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
  // Reloads publish new immutable GPU data. Retained buffers from previous content remain
  // valid, including when the parent resource is later destroyed and recreated with its ID.
  static xiiAtomicInteger64 s_uiBufferGeneration;
  sBufferResourceId.SetFormat("{}#MeshBuffer-{}", GetResourceID(), s_uiBufferGeneration.Increment());

  xiiMeshBufferResourceDescriptor bufferDescriptor = std::move(inout_descriptor.GetMeshBufferDescriptor());
  bufferDescriptor.m_bStreamableGeometry           = !inout_descriptor.m_UsageFlags.IsSet(xiiMeshResourceUsageFlags::Skinned) && inout_descriptor.m_MorphTargets.IsEmpty();
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
  XII_PROFILE_SCOPE("Mesh resource preparation");
  if (!descriptor.GetBounds().IsValid())
    descriptor.ComputeBounds();
  const auto& compiled    = static_cast<const xiiMeshResourceDescriptor&>(descriptor).GetMeshBufferDescriptor();
  xiiResult   preparation = XII_FAILURE;
  if (!descriptor.GetExistingMeshBuffer().IsValid() && !compiled.m_Meshlets.IsEmpty())
  {
    // Buffer-level clustering can precede section/LOD authoring for procedural
    // meshes. Validate the payload before rebuilding its missing draw ranges;
    // complete compiled assets retain their clusters without repeating the build.
    preparation = ValidateCompiledMeshlets(descriptor);
    if (preparation.Succeeded())
    {
      bool bMissingRanges = false;
      for (const auto& section : descriptor.GetSubMeshes())
        bMissingRanges |= section.m_uiPrimitiveCount != 0 && section.m_uiMeshletCount == 0;
      for (const auto& lod : static_cast<const xiiMeshResourceDescriptor&>(descriptor).GetLODs())
        for (const auto& section : lod.m_Sections)
          bMissingRanges |= section.m_uiPrimitiveCount != 0 && (section.m_uiMeshletCount == 0 || lod.m_uiMeshletCount == 0);
      bMissingRanges |= descriptor.GetSubMeshes().IsEmpty();
      if (bMissingRanges)
        preparation = descriptor.BuildMeshlets();
    }
  }
  else
    preparation = descriptor.BuildMeshlets();
  if (preparation.Failed())
    return {xiiResourceState::LoadedResourceMissing, 0, 0};

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
