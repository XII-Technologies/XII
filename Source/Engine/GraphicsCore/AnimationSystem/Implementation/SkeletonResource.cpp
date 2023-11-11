#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <GraphicsCore/AnimationSystem/Implementation/OzzUtils.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/io/archive.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkeletonResource, 1, xiiRTTIDefaultAllocator<xiiSkeletonResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiSkeletonResource);
// clang-format on

xiiSkeletonResource::xiiSkeletonResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiSkeletonResource::~xiiSkeletonResource() = default;

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiSkeletonResource, xiiSkeletonResourceDescriptor)
{
  m_pDescriptor  = XII_DEFAULT_NEW(xiiSkeletonResourceDescriptor);
  *m_pDescriptor = std::move(descriptor);

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

xiiResourceLoadDesc xiiSkeletonResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDesc xiiSkeletonResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_LOG_BLOCK("xiiSkeletonResource::UpdateContent", GetResourceDescription().GetData());

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

  // skip the asset file header at the start of the file
  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_pDescriptor = XII_DEFAULT_NEW(xiiSkeletonResourceDescriptor);
  m_pDescriptor->Deserialize(*Stream).IgnoreResult();

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiSkeletonResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiSkeletonResource); // TODO
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiSkeletonResourceDescriptor::xiiSkeletonResourceDescriptor()  = default;
xiiSkeletonResourceDescriptor::~xiiSkeletonResourceDescriptor() = default;
xiiSkeletonResourceDescriptor::xiiSkeletonResourceDescriptor(xiiSkeletonResourceDescriptor&& rhs)
{
  *this = std::move(rhs);
}

void xiiSkeletonResourceDescriptor::operator=(xiiSkeletonResourceDescriptor&& rhs)
{
  m_Skeleton = std::move(rhs.m_Skeleton);
  m_Geometry = std::move(rhs.m_Geometry);
}

xiiUInt64 xiiSkeletonResourceDescriptor::GetHeapMemoryUsage() const
{
  return m_Geometry.GetHeapMemoryUsage() + m_Skeleton.GetHeapMemoryUsage();
}

xiiResult xiiSkeletonResourceDescriptor::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(7);

  m_Skeleton.Save(inout_stream);
  inout_stream << m_RootTransform;
  inout_stream << m_fMaxImpulse;

  const xiiUInt16 uiNumGeom = static_cast<xiiUInt16>(m_Geometry.GetCount());
  inout_stream << uiNumGeom;

  for (xiiUInt32 i = 0; i < uiNumGeom; ++i)
  {
    const auto& geo = m_Geometry[i];

    inout_stream << geo.m_uiAttachedToJoint;
    inout_stream << geo.m_Type;
    inout_stream << geo.m_Transform;

    XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(geo.m_VertexPositions));
    XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(geo.m_TriangleIndices));
  }

  return XII_SUCCESS;
}

xiiResult xiiSkeletonResourceDescriptor::Deserialize(xiiStreamReader& inout_stream)
{
  const xiiTypeVersion version = inout_stream.ReadVersion(7);

  if (version < 6)
    return XII_FAILURE;

  m_Skeleton.Load(inout_stream);

  inout_stream >> m_RootTransform;

  if (version >= 7)
  {
    inout_stream >> m_fMaxImpulse;
  }

  m_Geometry.Clear();

  xiiUInt16 uiNumGeom = 0;
  inout_stream >> uiNumGeom;
  m_Geometry.Reserve(uiNumGeom);

  for (xiiUInt32 i = 0; i < uiNumGeom; ++i)
  {
    auto& geo = m_Geometry.ExpandAndGetRef();

    inout_stream >> geo.m_uiAttachedToJoint;
    inout_stream >> geo.m_Type;
    inout_stream >> geo.m_Transform;

    if (version <= 6)
    {
      xiiStringBuilder         sName;
      xiiSurfaceResourceHandle hSurface;
      xiiUInt8                 uiCollisionLayer;

      inout_stream >> sName;
      inout_stream >> hSurface;
      inout_stream >> uiCollisionLayer;
    }

    if (version >= 7)
    {
      XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(geo.m_VertexPositions));
      XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(geo.m_TriangleIndices));
    }
  }

  // make sure the geometry is sorted by bones
  // this allows to make the algorithm for creating the bone geometry more efficient
  m_Geometry.Sort([](const xiiSkeletonResourceGeometry& lhs, const xiiSkeletonResourceGeometry& rhs) -> bool { return lhs.m_uiAttachedToJoint < rhs.m_uiAttachedToJoint; });

  return XII_SUCCESS;
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_Implementation_SkeletonResource);
