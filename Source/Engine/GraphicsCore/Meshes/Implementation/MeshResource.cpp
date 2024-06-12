#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshResource, 1, xiiRTTIDefaultAllocator<xiiMeshResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiMeshResource);
// clang-format on

xiiUInt32 xiiMeshResource::s_uiMeshBufferNameSuffix = 0;

xiiMeshResource::xiiMeshResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
  m_Bounds = xiiBoundingBoxSphere::MakeInvalid();
}

xiiResourceLoadDesc xiiMeshResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDesc res;
  res.m_State                      = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable    = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire mesh
  // if (WhatToUnload == Unload::AllQualityLevels)
  {
    m_SubMeshes.Clear();
    m_SubMeshes.Compact();
    m_Materials.Clear();
    m_Materials.Compact();
    m_Bones.Clear();
    m_Bones.Compact();

    m_hMeshBuffer.Invalidate();
    m_hDefaultSkeleton.Invalidate();

    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable    = 0;
    res.m_State                      = xiiResourceState::Unloaded;
  }

  return res;
}

xiiResourceLoadDesc xiiMeshResource::UpdateContent(xiiStreamReader* Stream)
{
  xiiMeshResourceDescriptor desc;
  xiiResourceLoadDesc       res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(desc));
}

void xiiMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiMeshResource) + (xiiUInt32)m_SubMeshes.GetHeapMemoryUsage() + (xiiUInt32)m_Materials.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiMeshResource, xiiMeshResourceDescriptor)
{
  // if there is an existing mesh buffer to use, take that
  m_hMeshBuffer = descriptor.GetExistingMeshBuffer();

  m_hDefaultSkeleton     = descriptor.m_hDefaultSkeleton;
  m_Bones                = descriptor.m_Bones;
  m_fMaxBoneVertexOffset = descriptor.m_fMaxBoneVertexOffset;

  // otherwise create a new mesh buffer from the descriptor
  if (!m_hMeshBuffer.IsValid())
  {
    s_uiMeshBufferNameSuffix++;
    xiiStringBuilder sMbName;
    sMbName.SetFormat("{0}  [MeshBuffer {1}]", GetResourceID(), xiiArgU(s_uiMeshBufferNameSuffix, 4, true, 16, true));

    // note: this gets move'd, might be invalid afterwards
    xiiMeshBufferResourceDescriptor& mb = descriptor.MeshBufferDesc();

    m_hMeshBuffer = xiiResourceManager::CreateResource<xiiMeshBufferResource>(sMbName, std::move(mb), GetResourceDescription());
  }

  m_SubMeshes = descriptor.GetSubMeshes();

  m_Materials.Clear();
  m_Materials.Reserve(descriptor.GetMaterials().GetCount());

  // copy all the material assignments and load the materials
  for (const auto& mat : descriptor.GetMaterials())
  {
    xiiMaterialResourceHandle hMat;

    if (!mat.m_sPath.IsEmpty())
      hMat = xiiResourceManager::LoadResource<xiiMaterialResource>(mat.m_sPath);

    m_Materials.PushBack(hMat); // may be an invalid handle
  }

  m_Bounds = descriptor.GetBounds();
  XII_ASSERT_DEV(m_Bounds.IsValid(), "The mesh bounds are invalid. Make sure to call xiiMeshResourceDescriptor::ComputeBounds()");

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_MeshResource);
