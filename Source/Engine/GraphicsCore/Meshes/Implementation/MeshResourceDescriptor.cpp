/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GraphicsCore/Meshes/MeshResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiMeshResourceUsageFlags, 1)
  XII_BITFLAGS_CONSTANTS(xiiMeshResourceUsageFlags::StaticGeometry, xiiMeshResourceUsageFlags::DynamicGeometry, xiiMeshResourceUsageFlags::Skinned, xiiMeshResourceUsageFlags::MorphTargets)
  XII_BITFLAGS_CONSTANTS(xiiMeshResourceUsageFlags::Instancing, xiiMeshResourceUsageFlags::MeshShaderReady, xiiMeshResourceUsageFlags::RayTracingReady, xiiMeshResourceUsageFlags::Streaming, xiiMeshResourceUsageFlags::CpuReadable)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMeshLodSelectionMode, 1)
  XII_ENUM_CONSTANTS(xiiMeshLodSelectionMode::Distance, xiiMeshLodSelectionMode::ScreenSize, xiiMeshLodSelectionMode::Explicit)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

namespace
{
  static constexpr xiiUInt32 s_uiMeshResourceDescriptorVersion = 1U;

  static xiiBoundingBoxSphere MergeBounds(xiiArrayPtr<const xiiMeshSection> sections, const xiiBoundingBoxSphere& fallbackBounds)
  {
    xiiBoundingBoxSphere result = xiiBoundingBoxSphere::MakeInvalid();

    for (const xiiMeshSection& section : sections)
    {
      if (section.m_Bounds.IsValid())
      {
        if (result.IsValid())
          result.ExpandToInclude(section.m_Bounds);
        else
          result = section.m_Bounds;
      }
    }

    return result.IsValid() ? result : fallbackBounds;
  }
} // namespace

xiiResult xiiMeshSection::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_uiFirstPrimitive;
  inout_stream << m_uiPrimitiveCount;
  inout_stream << m_uiFirstMeshlet;
  inout_stream << m_uiMeshletCount;
  inout_stream << m_uiMaterialIndex;
  inout_stream << m_uiFlags;
  inout_stream << m_Bounds.m_vCenter;
  inout_stream << m_Bounds.m_fSphereRadius;
  inout_stream << m_Bounds.m_vBoxHalfExtents;

  return XII_SUCCESS;
}

xiiResult xiiMeshSection::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_uiFirstPrimitive;
  inout_stream >> m_uiPrimitiveCount;
  inout_stream >> m_uiFirstMeshlet;
  inout_stream >> m_uiMeshletCount;
  inout_stream >> m_uiMaterialIndex;
  inout_stream >> m_uiFlags;
  inout_stream >> m_Bounds.m_vCenter;
  inout_stream >> m_Bounds.m_fSphereRadius;
  inout_stream >> m_Bounds.m_vBoxHalfExtents;

  return XII_SUCCESS;
}

xiiResult xiiMeshLOD::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_fScreenSize;
  inout_stream << m_fMaxDistance;
  inout_stream << m_uiFirstMeshlet;
  inout_stream << m_uiMeshletCount;
  inout_stream << m_Bounds.m_vCenter;
  inout_stream << m_Bounds.m_fSphereRadius;
  inout_stream << m_Bounds.m_vBoxHalfExtents;

  return inout_stream.WriteArray(m_Sections);
}

xiiResult xiiMeshLOD::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_fScreenSize;
  inout_stream >> m_fMaxDistance;
  inout_stream >> m_uiFirstMeshlet;
  inout_stream >> m_uiMeshletCount;
  inout_stream >> m_Bounds.m_vCenter;
  inout_stream >> m_Bounds.m_fSphereRadius;
  inout_stream >> m_Bounds.m_vBoxHalfExtents;

  return inout_stream.ReadArray(m_Sections);
}

xiiResult xiiMeshMorphTarget::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_Name;
  inout_stream << m_uiVertexOffset;
  inout_stream << m_uiVertexCount;
  inout_stream << m_Bounds.m_vCenter;
  inout_stream << m_Bounds.m_fSphereRadius;
  inout_stream << m_Bounds.m_vBoxHalfExtents;

  return XII_SUCCESS;
}

xiiResult xiiMeshMorphTarget::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_Name;
  inout_stream >> m_uiVertexOffset;
  inout_stream >> m_uiVertexCount;
  inout_stream >> m_Bounds.m_vCenter;
  inout_stream >> m_Bounds.m_fSphereRadius;
  inout_stream >> m_Bounds.m_vBoxHalfExtents;

  return XII_SUCCESS;
}

xiiResult xiiMeshBoneData::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_GlobalInverseRestPoseMatrix;
  inout_stream << m_uiBoneIndex;

  return XII_SUCCESS;
}

xiiResult xiiMeshBoneData::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_GlobalInverseRestPoseMatrix;
  inout_stream >> m_uiBoneIndex;

  return XII_SUCCESS;
}

xiiMeshResourceDescriptor::xiiMeshResourceDescriptor()
{
  AddLOD(1.0f);
}

void xiiMeshResourceDescriptor::Clear()
{
  m_UsageFlags = xiiMeshResourceUsageFlags::Default;
  m_LodMode    = xiiMeshLodSelectionMode::ScreenSize;
  m_uiStreamingGroup = 0U;
  m_uiMaxResidentLod = 0U;
  m_uiRuntimeHash    = 0U;

  m_hDefaultSkeleton.Invalidate();
  m_Bones.Clear();
  m_MorphTargets.Clear();
  m_fMaxBoneVertexOffset = 0.0f;

  m_Materials.Clear();
  m_Sections.Clear();
  m_LODs.Clear();
  m_MeshBufferDescriptor.Clear();
  m_hMeshBuffer.Invalidate();
  m_Bounds = xiiBoundingBoxSphere::MakeInvalid();

  AddLOD(1.0f);
}

xiiMeshBufferResourceDescriptor& xiiMeshResourceDescriptor::MeshBufferDescriptor()
{
  m_hMeshBuffer.Invalidate();
  return m_MeshBufferDescriptor;
}

const xiiMeshBufferResourceDescriptor& xiiMeshResourceDescriptor::MeshBufferDescriptor() const
{
  return m_MeshBufferDescriptor;
}

void xiiMeshResourceDescriptor::UseExistingMeshBuffer(const xiiMeshBufferResourceHandle& hBuffer)
{
  m_hMeshBuffer = hBuffer;
}

const xiiMeshBufferResourceHandle& xiiMeshResourceDescriptor::GetExistingMeshBuffer() const
{
  return m_hMeshBuffer;
}

xiiUInt32 xiiMeshResourceDescriptor::AddMaterialSlot(xiiStringView sPathToMaterial)
{
  const xiiUInt32 uiIndex = m_Materials.GetCount();
  SetMaterial(uiIndex, sPathToMaterial);
  return uiIndex;
}

void xiiMeshResourceDescriptor::SetMaterial(xiiUInt32 uiMaterialIndex, xiiStringView sPathToMaterial)
{
  m_Materials.SetCount(xiiMath::Max(m_Materials.GetCount(), uiMaterialIndex + 1U));
  m_Materials[uiMaterialIndex] = sPathToMaterial;
}

xiiArrayPtr<const xiiString> xiiMeshResourceDescriptor::GetMaterials() const
{
  return m_Materials;
}

xiiMeshSection& xiiMeshResourceDescriptor::AddSection(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiMaterialIndex, xiiUInt32 uiLodIndex)
{
  while (m_LODs.GetCount() <= uiLodIndex)
  {
    AddLOD(m_LODs.IsEmpty() ? 1.0f : m_LODs.PeekBack().m_fScreenSize * 0.5f);
  }

  xiiMeshSection& section  = m_Sections.ExpandAndGetRef();
  section.m_uiFirstPrimitive = uiFirstPrimitive;
  section.m_uiPrimitiveCount = uiPrimitiveCount;
  section.m_uiMaterialIndex  = static_cast<xiiUInt16>(uiMaterialIndex);

  m_LODs[uiLodIndex].m_Sections.PushBack(section);
  return section;
}

void xiiMeshResourceDescriptor::AddSubMesh(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiMaterialIndex)
{
  AddSection(uiPrimitiveCount, uiFirstPrimitive, uiMaterialIndex, 0U);
}

xiiArrayPtr<const xiiMeshSection> xiiMeshResourceDescriptor::GetSubMeshes() const
{
  return m_Sections;
}

xiiMeshLOD& xiiMeshResourceDescriptor::AddLOD(float fScreenSize, float fMaxDistance)
{
  xiiMeshLOD& lod  = m_LODs.ExpandAndGetRef();
  lod.m_fScreenSize = xiiMath::Max(fScreenSize, 0.0f);
  lod.m_fMaxDistance = xiiMath::Max(fMaxDistance, 0.0f);
  return lod;
}

xiiArrayPtr<const xiiMeshLOD> xiiMeshResourceDescriptor::GetLODs() const
{
  return m_LODs;
}

void xiiMeshResourceDescriptor::CollapseSubMeshes()
{
  if (m_Sections.GetCount() <= 1U)
    return;

  xiiUInt32 uiFirstPrimitive = xiiMath::MaxValue<xiiUInt32>();
  xiiUInt32 uiLastPrimitive  = 0U;

  for (const xiiMeshSection& section : m_Sections)
  {
    uiFirstPrimitive = xiiMath::Min(uiFirstPrimitive, section.m_uiFirstPrimitive);
    uiLastPrimitive  = xiiMath::Max(uiLastPrimitive, section.m_uiFirstPrimitive + section.m_uiPrimitiveCount);
  }

  m_Sections.Clear();
  xiiMeshSection& section  = m_Sections.ExpandAndGetRef();
  section.m_uiFirstPrimitive = uiFirstPrimitive == xiiMath::MaxValue<xiiUInt32>() ? 0U : uiFirstPrimitive;
  section.m_uiPrimitiveCount = uiLastPrimitive - section.m_uiFirstPrimitive;
  section.m_uiMaterialIndex  = 0U;
  section.m_Bounds           = m_Bounds;

  for (xiiMeshLOD& lod : m_LODs)
  {
    lod.m_Sections.Clear();
    lod.m_Sections.PushBack(section);
  }
}

void xiiMeshResourceDescriptor::ComputeBounds()
{
  if (m_hMeshBuffer.IsValid())
  {
    xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(m_hMeshBuffer, xiiResourceAcquireMode::PointerOnly);
    m_Bounds = pMeshBuffer->GetBounds();
  }
  else
  {
    m_MeshBufferDescriptor.ComputeBounds();
    m_Bounds = m_MeshBufferDescriptor.GetBounds();
  }

  if (m_Bounds.IsValid())
  {
    for (xiiMeshSection& section : m_Sections)
    {
      if (!section.m_Bounds.IsValid())
        section.m_Bounds = m_Bounds;
    }

    for (xiiMeshLOD& lod : m_LODs)
    {
      for (xiiMeshSection& section : lod.m_Sections)
      {
        if (!section.m_Bounds.IsValid())
          section.m_Bounds = m_Bounds;
      }

      lod.m_Bounds = MergeBounds(lod.m_Sections, m_Bounds);
    }
  }

  xiiHashStreamWriter32 hashWriter;
  hashWriter << m_Bounds.m_vCenter;
  hashWriter << m_Bounds.m_fSphereRadius;
  hashWriter << m_Bounds.m_vBoxHalfExtents;
  hashWriter << m_MeshBufferDescriptor.GetVertexCount();
  hashWriter << m_MeshBufferDescriptor.GetIndexCount();
  hashWriter << m_MeshBufferDescriptor.m_Meshlets.GetCount();
  hashWriter << m_Sections.GetCount();
  hashWriter << m_Materials.GetCount();
  m_uiRuntimeHash = hashWriter.GetHashValue();
}

void xiiMeshResourceDescriptor::BuildMeshlets(xiiUInt32 uiMaxVertices, xiiUInt32 uiMaxPrimitives)
{
  if (!m_hMeshBuffer.IsValid())
  {
    m_MeshBufferDescriptor.BuildMeshlets(uiMaxVertices, uiMaxPrimitives);
  }

  xiiArrayPtr<const xiiMeshlet> meshlets = m_MeshBufferDescriptor.m_Meshlets;

  for (xiiMeshSection& section : m_Sections)
  {
    section.m_uiFirstMeshlet = xiiMath::MaxValue<xiiUInt32>();
    section.m_uiMeshletCount = 0U;

    const xiiUInt32 uiSectionEnd = section.m_uiFirstPrimitive + section.m_uiPrimitiveCount;
    for (xiiUInt32 i = 0; i < meshlets.GetCount(); ++i)
    {
      const xiiMeshlet& meshlet = meshlets[i];
      if (meshlet.m_uiFirstPrimitive >= section.m_uiFirstPrimitive && meshlet.m_uiFirstPrimitive < uiSectionEnd)
      {
        section.m_uiFirstMeshlet = xiiMath::Min(section.m_uiFirstMeshlet, i);
        ++section.m_uiMeshletCount;
      }
    }

    if (section.m_uiFirstMeshlet == xiiMath::MaxValue<xiiUInt32>())
    {
      section.m_uiFirstMeshlet = 0U;
    }
  }

  for (xiiMeshLOD& lod : m_LODs)
  {
    lod.m_uiFirstMeshlet = xiiMath::MaxValue<xiiUInt32>();
    lod.m_uiMeshletCount = 0U;

    for (xiiMeshSection& section : lod.m_Sections)
    {
      const xiiUInt32 uiSectionEnd = section.m_uiFirstPrimitive + section.m_uiPrimitiveCount;
      section.m_uiFirstMeshlet = xiiMath::MaxValue<xiiUInt32>();
      section.m_uiMeshletCount = 0U;

      for (xiiUInt32 i = 0; i < meshlets.GetCount(); ++i)
      {
        const xiiMeshlet& meshlet = meshlets[i];
        if (meshlet.m_uiFirstPrimitive >= section.m_uiFirstPrimitive && meshlet.m_uiFirstPrimitive < uiSectionEnd)
        {
          section.m_uiFirstMeshlet = xiiMath::Min(section.m_uiFirstMeshlet, i);
          ++section.m_uiMeshletCount;
        }
      }

      if (section.m_uiFirstMeshlet != xiiMath::MaxValue<xiiUInt32>())
      {
        lod.m_uiFirstMeshlet = xiiMath::Min(lod.m_uiFirstMeshlet, section.m_uiFirstMeshlet);
        lod.m_uiMeshletCount += section.m_uiMeshletCount;
      }
      else
      {
        section.m_uiFirstMeshlet = 0U;
      }
    }

    if (lod.m_uiFirstMeshlet == xiiMath::MaxValue<xiiUInt32>())
      lod.m_uiFirstMeshlet = 0U;
  }

  m_UsageFlags.Add(xiiMeshResourceUsageFlags::MeshShaderReady);
}

const xiiBoundingBoxSphere& xiiMeshResourceDescriptor::GetBounds() const
{
  return m_Bounds;
}

void xiiMeshResourceDescriptor::SetBounds(const xiiBoundingBoxSphere& bounds)
{
  m_Bounds = bounds;
}

void xiiMeshResourceDescriptor::Save(xiiStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiMeshResourceDescriptorVersion);
  inout_stream << m_UsageFlags;
  inout_stream << m_LodMode;
  inout_stream << m_uiStreamingGroup;
  inout_stream << m_uiMaxResidentLod;
  inout_stream << m_uiRuntimeHash;
  inout_stream << m_hDefaultSkeleton;
  inout_stream << m_fMaxBoneVertexOffset;
  inout_stream << m_Bounds.m_vCenter;
  inout_stream << m_Bounds.m_fSphereRadius;
  inout_stream << m_Bounds.m_vBoxHalfExtents;
  inout_stream << m_hMeshBuffer;

  inout_stream.WriteArray(m_Materials).IgnoreResult();
  inout_stream.WriteArray(m_Sections).IgnoreResult();
  inout_stream.WriteArray(m_LODs).IgnoreResult();
  inout_stream.WriteArray(m_MorphTargets).IgnoreResult();
  inout_stream.WriteHashTable(m_Bones).IgnoreResult();
  m_MeshBufferDescriptor.Serialize(inout_stream).IgnoreResult();
}

xiiResult xiiMeshResourceDescriptor::Save(const char* szFile) const
{
  xiiFileWriter file;
  XII_SUCCEED_OR_RETURN(file.Open(szFile));

  Save(file);
  return XII_SUCCESS;
}

xiiResult xiiMeshResourceDescriptor::Load(xiiStreamReader& inout_stream)
{
  Clear();

  inout_stream.ReadVersion(s_uiMeshResourceDescriptorVersion);
  inout_stream >> m_UsageFlags;
  inout_stream >> m_LodMode;
  inout_stream >> m_uiStreamingGroup;
  inout_stream >> m_uiMaxResidentLod;
  inout_stream >> m_uiRuntimeHash;
  inout_stream >> m_hDefaultSkeleton;
  inout_stream >> m_fMaxBoneVertexOffset;
  inout_stream >> m_Bounds.m_vCenter;
  inout_stream >> m_Bounds.m_fSphereRadius;
  inout_stream >> m_Bounds.m_vBoxHalfExtents;
  inout_stream >> m_hMeshBuffer;

  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Materials));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Sections));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_LODs));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_MorphTargets));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadHashTable(m_Bones));
  XII_SUCCEED_OR_RETURN(m_MeshBufferDescriptor.Deserialize(inout_stream));

  return XII_SUCCESS;
}

xiiResult xiiMeshResourceDescriptor::Load(const char* szFile)
{
  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(szFile));

  return Load(file);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_MeshResourceDescriptor);
