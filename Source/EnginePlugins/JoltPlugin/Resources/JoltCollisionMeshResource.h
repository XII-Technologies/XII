/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/SharedPtr.h>

struct xiiJoltCollisionGeometryType
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    ConvexHull,
    TriangleMesh,
    Default = ConvexHull
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_JOLTPLUGIN_DLL, xiiJoltCollisionGeometryType);

/// Versioned cooked geometry and surface slots. Cook outside world updates, then share the immutable resource.
struct XII_JOLTPLUGIN_DLL xiiJoltCollisionMeshResourceDescriptor
{
  xiiEnum<xiiJoltCollisionGeometryType>     m_Type;
  xiiDynamicArray<xiiUInt8>                 m_CookedData;
  xiiDynamicArray<xiiSurfaceResourceHandle> m_Surfaces;
  xiiUInt64                                 m_uiCookingVersion = 0;
  [[nodiscard]] xiiResult                   Save(xiiStreamWriter& stream) const;
  [[nodiscard]] xiiResult                   Load(xiiStreamReader& stream);
};

using xiiJoltCollisionMeshResourceHandle = xiiTypedResourceHandle<class xiiJoltCollisionMeshResource>;
class xiiJoltCollisionMeshData;
class xiiJoltSceneCollisionCook;

class XII_JOLTPLUGIN_DLL xiiJoltCollisionMeshResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltCollisionMeshResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiJoltCollisionMeshResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiJoltCollisionMeshResource, xiiJoltCollisionMeshResourceDescriptor);

public:
  xiiJoltCollisionMeshResource();
  ~xiiJoltCollisionMeshResource() override;

  static xiiUInt64 GetCookingVersion();
  /// Vertices use XII's right-handed Z-up coordinates. Triangle winding is preserved.
  /// Triangle surfaces are optional, or one slot per triangle; hulls use surface slot zero.
  [[nodiscard]] static xiiResult Cook(xiiJoltCollisionGeometryType::Enum type, xiiArrayPtr<const xiiVec3> vertices,
                                      xiiArrayPtr<const xiiUInt32> indices, xiiArrayPtr<const xiiUInt16> triangleSurfaces,
                                      xiiArrayPtr<const xiiSurfaceResourceHandle> surfaces, xiiJoltCollisionMeshResourceDescriptor& out_descriptor);
  /// Consumes XII's existing scene collision extraction representation without importing mesh files.
  [[nodiscard]] static xiiResult CookStaticMesh(const xiiSmcDescription& geometry, xiiJoltCollisionMeshResourceDescriptor& out_descriptor);

private:
  friend class xiiJoltPhysicsWorldModule;
  friend class xiiJoltSceneCollisionCook;
  static xiiResult                       BuildShape(xiiJoltCollisionGeometryType::Enum type, xiiArrayPtr<const xiiVec3> vertices,
                                                    xiiArrayPtr<const xiiUInt32> indices, xiiArrayPtr<const xiiUInt16> triangleSurfaces,
                                                    xiiArrayPtr<const xiiSurfaceResourceHandle> surfaces, xiiSharedPtr<xiiJoltCollisionMeshData>& out_data);
  static xiiResult                       BuildStaticMesh(const xiiSmcDescription& geometry, xiiSharedPtr<xiiJoltCollisionMeshData>& out_data);
  static xiiResult                       SerializeShape(const xiiJoltCollisionMeshData& data, xiiJoltCollisionMeshResourceDescriptor& out_descriptor);
  xiiSharedPtr<xiiJoltCollisionMeshData> GetData() const;
  xiiResourceLoadDescription             UnloadData(Unload what) override;
  xiiResourceLoadDescription             UpdateContent(xiiStreamReader* pStream) override;
  void                                   UpdateMemoryUsage(MemoryUsage& out_usage) override;
  mutable xiiMutex                       m_DataMutex;
  xiiSharedPtr<xiiJoltCollisionMeshData> m_pData;
};
