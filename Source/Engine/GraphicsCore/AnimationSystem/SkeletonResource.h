/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

#ifndef XII_SKELETON_RESOURCE_HANDLE_DEFINED
#  define XII_SKELETON_RESOURCE_HANDLE_DEFINED
using xiiSkeletonResourceHandle = xiiTypedResourceHandle<class xiiSkeletonResource>;
#endif

namespace ozz
{
  namespace animation
  {
    class Skeleton;
  }
} // namespace ozz

struct XII_GRAPHICSCORE_DLL xiiSkeletonJoint
{
  xiiHashedString      m_sName;
  xiiUInt16            m_uiParentIndex   = xiiMath::MaxValue<xiiUInt16>();
  xiiTransform         m_LocalRestPose   = xiiTransform::MakeIdentity();
  xiiMat4              m_ModelRestPose   = xiiMat4::MakeIdentity();
  xiiMat4              m_InverseBindPose = xiiMat4::MakeIdentity();
  xiiBoundingBoxSphere m_LocalBounds     = xiiBoundingBoxSphere::MakeInvalid();

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

struct XII_GRAPHICSCORE_DLL xiiSkeletonResourceDescriptor
{
  void Clear();

  xiiUInt16 AddJoint(xiiStringView sName, xiiUInt16 uiParentIndex, const xiiTransform& localRestPose);
  xiiUInt16 FindJointByName(const xiiTempHashedString& sName) const;
  void      BuildModelSpaceRestPose();
  void      ComputeRuntimeHash();

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

  xiiDynamicArray<xiiSkeletonJoint> m_Joints;
  xiiBoundingBoxSphere              m_Bounds        = xiiBoundingBoxSphere::MakeInvalid();
  xiiUInt16                         m_uiRootJoint   = xiiMath::MaxValue<xiiUInt16>();
  xiiUInt32                         m_uiRuntimeHash = 0U;
  xiiDynamicArray<xiiUInt8>         m_OzzSkeletonData;
};

class XII_GRAPHICSCORE_DLL xiiSkeletonResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkeletonResource, xiiResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiSkeletonResource);

  XII_RESOURCE_DECLARE_CREATEABLE(xiiSkeletonResource, xiiSkeletonResourceDescriptor);

public:
  xiiSkeletonResource();
  ~xiiSkeletonResource();

  xiiUInt32                           GetJointCount() const;
  xiiUInt16                           FindJointByName(const xiiTempHashedString& sName) const;
  const xiiSkeletonJoint&             GetJoint(xiiUInt32 uiIndex) const;
  xiiArrayPtr<const xiiSkeletonJoint> GetJoints() const;
  const xiiBoundingBoxSphere&         GetBounds() const;
  xiiUInt32                           GetRuntimeHash() const;

#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
  const ozz::animation::Skeleton* GetOzzSkeleton() const;
#endif

private:
  virtual xiiResourceLoadDescription UnloadData(Unload whatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  virtual void                       UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void BuildJointLookup();
  void LoadOzzSkeleton(const xiiDynamicArray<xiiUInt8>& ozzData);

private:
  xiiSkeletonResourceDescriptor            m_Descriptor;
  xiiHashTable<xiiHashedString, xiiUInt16> m_JointLookup;

#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
  xiiUniquePtr<ozz::animation::Skeleton> m_pOzzSkeleton;
#endif
};
