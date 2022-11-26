#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/RendererCoreDLL.h>

using xiiSurfaceResourceHandle = xiiTypedResourceHandle<class xiiSurfaceResource>;

struct xiiSkeletonResourceGeometry
{
  // scale is used to resize a unit sphere / box / capsule
  xiiTransform                          m_Transform;
  xiiUInt16                             m_uiAttachedToJoint = 0;
  xiiEnum<xiiSkeletonJointGeometryType> m_Type;
  xiiHashedString                       m_sName;
  xiiSurfaceResourceHandle              m_hSurface;
  xiiUInt8                              m_uiCollisionLayer = 0;
};

struct XII_RENDERERCORE_DLL xiiSkeletonResourceDescriptor
{
  xiiSkeletonResourceDescriptor();
  ~xiiSkeletonResourceDescriptor();
  xiiSkeletonResourceDescriptor(const xiiSkeletonResourceDescriptor& rhs) = delete;
  xiiSkeletonResourceDescriptor(xiiSkeletonResourceDescriptor&& rhs);
  void operator=(xiiSkeletonResourceDescriptor&& rhs);
  void operator=(const xiiSkeletonResourceDescriptor& rhs) = delete;

  xiiResult Serialize(xiiStreamWriter& stream) const;
  xiiResult Deserialize(xiiStreamReader& stream);

  xiiUInt64 GetHeapMemoryUsage() const;

  xiiTransform m_RootTransform = xiiTransform::IdentityTransform();
  xiiSkeleton  m_Skeleton;

  xiiDynamicArray<xiiSkeletonResourceGeometry> m_Geometry;
};

using xiiSkeletonResourceHandle = xiiTypedResourceHandle<class xiiSkeletonResource>;

class XII_RENDERERCORE_DLL xiiSkeletonResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkeletonResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiSkeletonResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiSkeletonResource, xiiSkeletonResourceDescriptor);

public:
  xiiSkeletonResource();
  ~xiiSkeletonResource();

  const xiiSkeletonResourceDescriptor& GetDescriptor() const { return *m_pDescriptor; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiUniquePtr<xiiSkeletonResourceDescriptor> m_pDescriptor;
};
