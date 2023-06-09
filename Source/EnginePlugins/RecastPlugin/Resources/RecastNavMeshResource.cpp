#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <Recast/DetourNavMesh.h>
#include <Recast/Recast.h>
#include <Recast/RecastAlloc.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRecastNavMeshResource, 1, xiiRTTIDefaultAllocator<xiiRecastNavMeshResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiRecastNavMeshResource);
// clang-format on

//////////////////////////////////////////////////////////////////////////

xiiRecastNavMeshResourceDescriptor::xiiRecastNavMeshResourceDescriptor() = default;
xiiRecastNavMeshResourceDescriptor::xiiRecastNavMeshResourceDescriptor(xiiRecastNavMeshResourceDescriptor&& rhs)
{
  *this = std::move(rhs);
}

xiiRecastNavMeshResourceDescriptor::~xiiRecastNavMeshResourceDescriptor()
{
  Clear();
}

void xiiRecastNavMeshResourceDescriptor::operator=(xiiRecastNavMeshResourceDescriptor&& rhs)
{
  m_DetourNavmeshData = std::move(rhs.m_DetourNavmeshData);

  m_pNavMeshPolygons     = rhs.m_pNavMeshPolygons;
  rhs.m_pNavMeshPolygons = nullptr;
}

void xiiRecastNavMeshResourceDescriptor::Clear()
{
  m_DetourNavmeshData.Clear();
  XII_DEFAULT_DELETE(m_pNavMeshPolygons);
}

//////////////////////////////////////////////////////////////////////////

xiiResult xiiRecastNavMeshResourceDescriptor::Serialize(xiiStreamWriter& ref_stream) const
{
  ref_stream.WriteVersion(1);
  XII_SUCCEED_OR_RETURN(ref_stream.WriteArray(m_DetourNavmeshData));

  const bool hasPolygons = m_pNavMeshPolygons != nullptr;
  ref_stream << hasPolygons;

  if (hasPolygons)
  {
    XII_CHECK_AT_COMPILETIME_MSG(sizeof(rcPolyMesh) == sizeof(void*) * 5 + sizeof(int) * 14, "rcPolyMesh data structure has changed");

    const auto& mesh = *m_pNavMeshPolygons;

    ref_stream << (int)mesh.nverts;
    ref_stream << (int)mesh.npolys;
    ref_stream << (int)mesh.npolys; // do not use mesh.maxpolys
    ref_stream << (int)mesh.nvp;
    ref_stream << (float)mesh.bmin[0];
    ref_stream << (float)mesh.bmin[1];
    ref_stream << (float)mesh.bmin[2];
    ref_stream << (float)mesh.bmax[0];
    ref_stream << (float)mesh.bmax[1];
    ref_stream << (float)mesh.bmax[2];
    ref_stream << (float)mesh.cs;
    ref_stream << (float)mesh.ch;
    ref_stream << (int)mesh.borderSize;
    ref_stream << (float)mesh.maxEdgeError;

    XII_ASSERT_DEBUG(mesh.maxpolys >= mesh.npolys, "Invalid navmesh polygon count");

    XII_SUCCEED_OR_RETURN(ref_stream.WriteBytes(mesh.verts, sizeof(xiiUInt16) * mesh.nverts * 3));
    XII_SUCCEED_OR_RETURN(ref_stream.WriteBytes(mesh.polys, sizeof(xiiUInt16) * mesh.npolys * mesh.nvp * 2));
    XII_SUCCEED_OR_RETURN(ref_stream.WriteBytes(mesh.regs, sizeof(xiiUInt16) * mesh.npolys));
    XII_SUCCEED_OR_RETURN(ref_stream.WriteBytes(mesh.flags, sizeof(xiiUInt16) * mesh.npolys));
    XII_SUCCEED_OR_RETURN(ref_stream.WriteBytes(mesh.areas, sizeof(xiiUInt8) * mesh.npolys));
  }

  return XII_SUCCESS;
}

xiiResult xiiRecastNavMeshResourceDescriptor::Deserialize(xiiStreamReader& ref_stream)
{
  Clear();

  const xiiTypeVersion version = ref_stream.ReadVersion(1);
  XII_SUCCEED_OR_RETURN(ref_stream.ReadArray(m_DetourNavmeshData));

  bool hasPolygons = false;
  ref_stream >> hasPolygons;

  if (hasPolygons)
  {
    XII_CHECK_AT_COMPILETIME_MSG(sizeof(rcPolyMesh) == sizeof(void*) * 5 + sizeof(int) * 14, "rcPolyMesh data structure has changed");

    m_pNavMeshPolygons = XII_DEFAULT_NEW(rcPolyMesh);

    auto& mesh = *m_pNavMeshPolygons;

    ref_stream >> mesh.nverts;
    ref_stream >> mesh.npolys;
    ref_stream >> mesh.maxpolys;
    ref_stream >> mesh.nvp;
    ref_stream >> mesh.bmin[0];
    ref_stream >> mesh.bmin[1];
    ref_stream >> mesh.bmin[2];
    ref_stream >> mesh.bmax[0];
    ref_stream >> mesh.bmax[1];
    ref_stream >> mesh.bmax[2];
    ref_stream >> mesh.cs;
    ref_stream >> mesh.ch;
    ref_stream >> mesh.borderSize;
    ref_stream >> mesh.maxEdgeError;

    XII_ASSERT_DEBUG(mesh.maxpolys >= mesh.npolys, "Invalid navmesh polygon count");

    mesh.verts = (xiiUInt16*)rcAlloc(sizeof(xiiUInt16) * mesh.nverts * 3, RC_ALLOC_PERM);
    mesh.polys = (xiiUInt16*)rcAlloc(sizeof(xiiUInt16) * mesh.maxpolys * mesh.nvp * 2, RC_ALLOC_PERM);
    mesh.regs  = (xiiUInt16*)rcAlloc(sizeof(xiiUInt16) * mesh.maxpolys, RC_ALLOC_PERM);
    mesh.areas = (xiiUInt8*)rcAlloc(sizeof(xiiUInt8) * mesh.maxpolys, RC_ALLOC_PERM);

    ref_stream.ReadBytes(mesh.verts, sizeof(xiiUInt16) * mesh.nverts * 3);
    ref_stream.ReadBytes(mesh.polys, sizeof(xiiUInt16) * mesh.maxpolys * mesh.nvp * 2);
    ref_stream.ReadBytes(mesh.regs, sizeof(xiiUInt16) * mesh.maxpolys);
    ref_stream.ReadBytes(mesh.flags, sizeof(xiiUInt16) * mesh.maxpolys);
    ref_stream.ReadBytes(mesh.areas, sizeof(xiiUInt8) * mesh.maxpolys);
  }

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

xiiRecastNavMeshResource::xiiRecastNavMeshResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(xiiRecastNavMeshResource);
}

xiiRecastNavMeshResource::~xiiRecastNavMeshResource()
{
  XII_DEFAULT_DELETE(m_pNavMeshPolygons);
  XII_DEFAULT_DELETE(m_pNavMesh);
}

xiiResourceLoadDesc xiiRecastNavMeshResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  m_DetourNavmeshData.Clear();
  XII_DEFAULT_DELETE(m_pNavMesh);
  XII_DEFAULT_DELETE(m_pNavMeshPolygons);

  return res;
}

xiiResourceLoadDesc xiiRecastNavMeshResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_LOG_BLOCK("xiiRecastNavMeshResource::UpdateContent", GetResourceDescription().GetData());

  xiiResourceLoadDesc res;
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

  xiiRecastNavMeshResourceDescriptor descriptor;
  descriptor.Deserialize(*Stream).IgnoreResult();

  return CreateResource(std::move(descriptor));
}

void xiiRecastNavMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiRecastNavMeshResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_DetourNavmeshData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_pNavMesh != nullptr ? sizeof(dtNavMesh) : 0;
  out_NewMemoryUsage.m_uiMemoryCPU += m_pNavMeshPolygons != nullptr ? sizeof(rcPolyMesh) : 0;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

xiiResourceLoadDesc xiiRecastNavMeshResource::CreateResource(xiiRecastNavMeshResourceDescriptor&& descriptor)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  m_pNavMeshPolygons            = descriptor.m_pNavMeshPolygons;
  descriptor.m_pNavMeshPolygons = nullptr;

  m_DetourNavmeshData = std::move(descriptor.m_DetourNavmeshData);

  if (!m_DetourNavmeshData.IsEmpty())
  {
    m_pNavMesh = XII_DEFAULT_NEW(dtNavMesh);

    // the dtNavMesh does not need to free the data, the resource owns it
    const int dtMeshFlags = 0;
    m_pNavMesh->init(m_DetourNavmeshData.GetData(), m_DetourNavmeshData.GetCount(), dtMeshFlags);
  }

  return res;
}
