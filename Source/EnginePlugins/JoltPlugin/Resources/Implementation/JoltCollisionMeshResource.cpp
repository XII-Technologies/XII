/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <JoltPlugin/Resources/Implementation/JoltCollisionMeshData.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/StreamIn.h>
#include <Jolt/Core/StreamOut.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

namespace
{
  constexpr xiiUInt32 CollisionMagic  = 0x39434A58; // XJC9
  constexpr xiiUInt32 MaxCookedBytes  = 256 * 1024 * 1024;
  constexpr xiiUInt32 MaxSurfaceSlots = 32;

  class CollisionWriter final : public JPH::StreamOut
  {
  public:
    explicit CollisionWriter(xiiStreamWriter& stream) : m_Stream(stream) {}
    void WriteBytes(const void* pData, size_t count) override { m_bFailed |= m_Stream.WriteBytes(pData, count).Failed(); }
    bool IsFailed() const override { return m_bFailed; }

  private:
    xiiStreamWriter& m_Stream;
    bool             m_bFailed = false;
  };
  class CollisionReader final : public JPH::StreamIn
  {
  public:
    explicit CollisionReader(xiiStreamReader& stream) : m_Stream(stream) {}
    void ReadBytes(void* pData, size_t count) override
    {
      const auto read = m_Stream.ReadBytes(pData, count);
      if (read < count)
      {
        xiiMemoryUtils::ZeroFill(static_cast<xiiUInt8*>(pData) + read, count - read);
        m_bFailed = true;
      }
    }
    bool IsEOF() const override { return m_bFailed; }
    bool IsFailed() const override { return m_bFailed; }

  private:
    xiiStreamReader& m_Stream;
    bool             m_bFailed = false;
  };
} // namespace

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiJoltCollisionGeometryType, 1)
  XII_ENUM_CONSTANTS(xiiJoltCollisionGeometryType::ConvexHull, xiiJoltCollisionGeometryType::TriangleMesh)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltCollisionMeshResource, 1, xiiRTTIDefaultAllocator<xiiJoltCollisionMeshResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiJoltCollisionMeshResource);

xiiUInt64 xiiJoltCollisionMeshResource::GetCookingVersion()
{
  using JPH::uint64;
  // Feature bits include Jolt precision and build options. Hashing the native words also distinguishes byte order.
  // Hull and mesh binary state omits debug geometry. Canonicalize the renderer feature
  // to its previous enabled value so Shipping can consume existing cooked assets.
  constexpr xiiUInt64 uiDebugRendererVersionBit = xiiUInt64{1} << 29;
  const xiiUInt64     version[]                 = {JPH_VERSION_ID | uiDebugRendererVersionBit, sizeof(void*), 1};
  return xiiHashingUtils::xxHash64(version, sizeof(version));
}

xiiResult xiiJoltCollisionMeshResourceDescriptor::Save(xiiStreamWriter& stream) const
{
  if (m_uiCookingVersion != xiiJoltCollisionMeshResource::GetCookingVersion() || m_CookedData.IsEmpty() ||
      m_CookedData.GetCount() > MaxCookedBytes || m_Surfaces.IsEmpty() || m_Surfaces.GetCount() > MaxSurfaceSlots ||
      m_Type > xiiJoltCollisionGeometryType::TriangleMesh || (m_Type == xiiJoltCollisionGeometryType::ConvexHull && m_Surfaces.GetCount() != 1))
    return XII_FAILURE;
  stream << CollisionMagic << xiiUInt8{1} << m_uiCookingVersion << m_Type;
  stream << m_Surfaces.GetCount();
  for (const auto& surface : m_Surfaces)
    stream << surface;
  stream << m_CookedData.GetCount() << xiiHashingUtils::xxHash64(m_CookedData.GetData(), m_CookedData.GetCount());
  return stream.WriteBytes(m_CookedData.GetData(), m_CookedData.GetCount());
}

xiiResult xiiJoltCollisionMeshResourceDescriptor::Load(xiiStreamReader& stream)
{
  xiiUInt32 magic = 0, slots = 0, bytes = 0;
  xiiUInt8  version = 0, type = 0;
  xiiUInt64 cooking = 0, hash = 0;
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&magic));
  if (stream.ReadBytes(&version, 1) != 1)
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(stream.ReadQWordValue(&cooking));
  if (stream.ReadBytes(&type, 1) != 1)
    return XII_FAILURE;
  if (magic != CollisionMagic || version != 1 || cooking != xiiJoltCollisionMeshResource::GetCookingVersion() ||
      type > xiiJoltCollisionGeometryType::TriangleMesh)
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&slots));
  if (slots == 0 || slots > MaxSurfaceSlots || (type == xiiJoltCollisionGeometryType::ConvexHull && slots != 1))
    return XII_FAILURE;
  xiiJoltCollisionMeshResourceDescriptor descriptor;
  descriptor.m_uiCookingVersion = cooking;
  descriptor.m_Type             = static_cast<xiiJoltCollisionGeometryType::Enum>(type);
  descriptor.m_Surfaces.SetCount(slots);
  for (auto& surface : descriptor.m_Surfaces)
    stream >> surface;
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&bytes));
  XII_SUCCEED_OR_RETURN(stream.ReadQWordValue(&hash));
  if (bytes == 0 || bytes > MaxCookedBytes)
    return XII_FAILURE;
  descriptor.m_CookedData.SetCountUninitialized(bytes);
  if (stream.ReadBytes(descriptor.m_CookedData.GetData(), bytes) != bytes ||
      xiiHashingUtils::xxHash64(descriptor.m_CookedData.GetData(), bytes) != hash)
    return XII_FAILURE;
  *this = std::move(descriptor);
  return XII_SUCCESS;
}

xiiResult xiiJoltCollisionMeshResource::Cook(xiiJoltCollisionGeometryType::Enum type, xiiArrayPtr<const xiiVec3> vertices,
                                             xiiArrayPtr<const xiiUInt32> indices, xiiArrayPtr<const xiiUInt16> triangleSurfaces,
                                             xiiArrayPtr<const xiiSurfaceResourceHandle> surfaces, xiiJoltCollisionMeshResourceDescriptor& out_descriptor)
{
  xiiSharedPtr<xiiJoltCollisionMeshData> data;
  XII_SUCCEED_OR_RETURN(BuildShape(type, vertices, indices, triangleSurfaces, surfaces, data));
  return SerializeShape(*data, out_descriptor);
}

xiiResult xiiJoltCollisionMeshResource::BuildShape(xiiJoltCollisionGeometryType::Enum type, xiiArrayPtr<const xiiVec3> vertices,
                                                   xiiArrayPtr<const xiiUInt32> indices, xiiArrayPtr<const xiiUInt16> triangleSurfaces,
                                                   xiiArrayPtr<const xiiSurfaceResourceHandle> surfaces, xiiSharedPtr<xiiJoltCollisionMeshData>& out_data)
{
  if (JPH::Factory::sInstance == nullptr || vertices.GetCount() < 3 || vertices.GetCount() > 1000000 ||
      surfaces.GetCount() > MaxSurfaceSlots || type > xiiJoltCollisionGeometryType::TriangleMesh ||
      (type == xiiJoltCollisionGeometryType::ConvexHull && (vertices.GetCount() < 4 || surfaces.GetCount() > 1)))
  {
    xiiLog::Error("Jolt collision cooking requires initialized Jolt, valid geometry, and supported surface slots.");
    return XII_FAILURE;
  }
  for (const auto& vertex : vertices)
  {
    if (!vertex.IsValid())
    {
      xiiLog::Error("Jolt collision geometry contains a non-finite vertex.");
      return XII_FAILURE;
    }
  }
  JPH::Shape::ShapeResult result;
  if (type == xiiJoltCollisionGeometryType::ConvexHull)
  {
    JPH::ConvexHullShapeSettings settings;
    settings.mPoints.reserve(vertices.GetCount());
    for (const auto& vertex : vertices)
      settings.mPoints.emplace_back(vertex.x, vertex.y, vertex.z);
    settings.SetDensity(1000.0f);
    result = settings.Create();
    if (!result.HasError() && (!xiiMath::IsFinite(result.Get()->GetVolume()) || result.Get()->GetVolume() <= 0.0f))
    {
      xiiLog::Error("Jolt convex hull requires a non-degenerate three-dimensional volume.");
      return XII_FAILURE;
    }
  }
  else
  {
    if (indices.IsEmpty() || indices.GetCount() % 3 != 0 || indices.GetCount() > 6000000 ||
        (!triangleSurfaces.IsEmpty() && triangleSurfaces.GetCount() != indices.GetCount() / 3))
    {
      xiiLog::Error("Jolt triangle cooking requires triangle-list indices and one optional surface slot per triangle.");
      return XII_FAILURE;
    }
    JPH::VertexList nativeVertices;
    nativeVertices.reserve(vertices.GetCount());
    for (const auto& vertex : vertices)
      nativeVertices.emplace_back(vertex.x, vertex.y, vertex.z);
    JPH::IndexedTriangleList triangles;
    triangles.reserve(indices.GetCount() / 3);
    for (xiiUInt32 i = 0; i < indices.GetCount(); i += 3)
    {
      const auto      a = indices[i], b = indices[i + 1], c = indices[i + 2];
      const xiiUInt16 material = triangleSurfaces.IsEmpty() ? 0 : triangleSurfaces[i / 3];
      if (a >= vertices.GetCount() || b >= vertices.GetCount() || c >= vertices.GetCount() ||
          material >= xiiMath::Max(1U, surfaces.GetCount()))
      {
        xiiLog::Error("Jolt triangle {} has an invalid vertex or surface index.", i / 3);
        return XII_FAILURE;
      }
      const auto normal = (vertices[b] - vertices[a]).CrossRH(vertices[c] - vertices[a]);
      if (!normal.IsValid() || normal.GetLengthSquared() <= 0)
      {
        xiiLog::Error("Jolt triangle {} is degenerate.", i / 3);
        return XII_FAILURE;
      }
      triangles.emplace_back(a, b, c, material);
    }
    JPH::PhysicsMaterialList materials;
    materials.resize(xiiMath::Max(1U, surfaces.GetCount()), JPH::PhysicsMaterial::sDefault);
    JPH::MeshShapeSettings settings(std::move(nativeVertices), std::move(triangles), std::move(materials));
    result = settings.Create();
  }
  if (result.HasError())
  {
    xiiLog::Error("Jolt collision cooking failed: {}", result.GetError().c_str());
    return XII_FAILURE;
  }
  auto data        = XII_DEFAULT_NEW(xiiJoltCollisionMeshData);
  data->m_Type     = type;
  data->m_pShape   = result.Get();
  data->m_Surfaces = surfaces;
  if (data->m_Surfaces.IsEmpty())
    data->m_Surfaces.SetCount(1);
  out_data = std::move(data);
  return XII_SUCCESS;
}

xiiResult xiiJoltCollisionMeshResource::SerializeShape(const xiiJoltCollisionMeshData& data, xiiJoltCollisionMeshResourceDescriptor& out_descriptor)
{
  xiiJoltCollisionMeshResourceDescriptor descriptor;
  descriptor.m_Type             = data.m_Type;
  descriptor.m_uiCookingVersion = GetCookingVersion();
  descriptor.m_Surfaces         = data.m_Surfaces;
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         stream(&storage);
  CollisionWriter               writer(stream);
  data.m_pShape->SaveBinaryState(writer);
  if (writer.IsFailed() || storage.GetStorageSize64() > MaxCookedBytes)
    return XII_FAILURE;
  descriptor.m_CookedData.SetCountUninitialized(storage.GetStorageSize32());
  xiiMemoryStreamReader reader(&storage);
  if (reader.ReadBytes(descriptor.m_CookedData.GetData(), descriptor.m_CookedData.GetCount()) != descriptor.m_CookedData.GetCount())
    return XII_FAILURE;
  out_descriptor = std::move(descriptor);
  return XII_SUCCESS;
}

xiiResult xiiJoltCollisionMeshResource::CookStaticMesh(const xiiSmcDescription& geometry, xiiJoltCollisionMeshResourceDescriptor& out_descriptor)
{
  xiiSharedPtr<xiiJoltCollisionMeshData> data;
  XII_SUCCEED_OR_RETURN(BuildStaticMesh(geometry, data));
  return SerializeShape(*data, out_descriptor);
}

xiiResult xiiJoltCollisionMeshResource::BuildStaticMesh(const xiiSmcDescription& geometry, xiiSharedPtr<xiiJoltCollisionMeshData>& out_data)
{
  xiiDynamicArray<xiiVec3>                  vertices;
  xiiDynamicArray<xiiUInt32>                indices;
  xiiDynamicArray<xiiUInt16>                materials;
  xiiDynamicArray<xiiSurfaceResourceHandle> surfaces;
  for (const auto& vertex : geometry.m_Vertices)
    vertices.PushBack(vertex);
  for (const auto& triangle : geometry.m_Triangles)
    for (auto index : triangle.m_uiVertexIndices)
      indices.PushBack(index);
  materials.SetCount(geometry.m_Triangles.GetCount(), 0);
  for (const auto& surface : geometry.m_Surfaces)
    surfaces.PushBack(surface.IsEmpty() ? xiiSurfaceResourceHandle{} : xiiResourceManager::LoadResource<xiiSurfaceResource>(surface));
  for (const auto& section : geometry.m_SubMeshes)
  {
    if (section.m_uiFirstTriangle > materials.GetCount() || section.m_uiNumTriangles > materials.GetCount() - section.m_uiFirstTriangle)
      return XII_FAILURE;
    for (xiiUInt32 i = 0; i < section.m_uiNumTriangles; ++i)
      materials[section.m_uiFirstTriangle + i] = section.m_uiSurfaceIndex;
  }
  return BuildShape(xiiJoltCollisionGeometryType::TriangleMesh, vertices, indices, materials, surfaces, out_data);
}

xiiJoltCollisionMeshResource::xiiJoltCollisionMeshResource() : xiiResource(DoUpdate::OnAnyThread, 1) {}
xiiJoltCollisionMeshResource::~xiiJoltCollisionMeshResource() = default;

xiiResourceLoadDescription xiiJoltCollisionMeshResource::UnloadData(Unload)
{
  XII_LOCK(m_DataMutex);
  m_pData.Clear();
  xiiResourceLoadDescription result;
  result.m_State                      = xiiResourceState::Unloaded;
  result.m_uiQualityLevelsDiscardable = result.m_uiQualityLevelsLoadable = 0;
  return result;
}

xiiResourceLoadDescription xiiJoltCollisionMeshResource::CreateResource(xiiJoltCollisionMeshResourceDescriptor&& descriptor)
{
  xiiResourceLoadDescription description;
  description.m_State                      = xiiResourceState::LoadedResourceMissing;
  description.m_uiQualityLevelsDiscardable = description.m_uiQualityLevelsLoadable = 0;
  if (JPH::Factory::sInstance == nullptr || descriptor.m_Type > xiiJoltCollisionGeometryType::TriangleMesh ||
      descriptor.m_uiCookingVersion != GetCookingVersion() || descriptor.m_CookedData.IsEmpty() ||
      descriptor.m_CookedData.GetCount() > MaxCookedBytes || descriptor.m_Surfaces.IsEmpty() || descriptor.m_Surfaces.GetCount() > MaxSurfaceSlots ||
      (descriptor.m_Type == xiiJoltCollisionGeometryType::ConvexHull && descriptor.m_Surfaces.GetCount() != 1))
    return description;
  xiiRawMemoryStreamReader bytes(descriptor.m_CookedData.GetData(), descriptor.m_CookedData.GetCount());
  CollisionReader          reader(bytes);
  auto                     shape = JPH::Shape::sRestoreFromBinaryState(reader);
  if (reader.IsFailed() || shape.HasError() ||
      shape.Get()->GetSubType() != (descriptor.m_Type == xiiJoltCollisionGeometryType::ConvexHull ? JPH::EShapeSubType::ConvexHull : JPH::EShapeSubType::Mesh))
  {
    xiiLog::Error("Could not restore Jolt collision resource '{}'.", GetResourceID());
    return description;
  }
  JPH::PhysicsMaterialList materials;
  materials.resize(descriptor.m_Surfaces.GetCount(), JPH::PhysicsMaterial::sDefault);
  shape.Get()->RestoreMaterialState(materials.data(), static_cast<JPH::uint>(materials.size()));
  auto data        = XII_DEFAULT_NEW(xiiJoltCollisionMeshData);
  data->m_pShape   = shape.Get();
  data->m_Type     = static_cast<xiiJoltCollisionGeometryType::Enum>(descriptor.m_Type.GetValue());
  data->m_Surfaces = std::move(descriptor.m_Surfaces);
  {
    XII_LOCK(m_DataMutex);
    m_pData = std::move(data);
  }
  description.m_State = xiiResourceState::Loaded;
  return description;
}

xiiResourceLoadDescription xiiJoltCollisionMeshResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiJoltCollisionMeshResourceDescriptor descriptor;
  bool                                   valid = false;
  if (pStream)
  {
    xiiString path;
    *pStream >> path;
    xiiAssetFileHeader header;
    valid = header.Read(*pStream).Succeeded() && descriptor.Load(*pStream).Succeeded();
  }
  if (valid)
  {
    auto description = CreateResource(std::move(descriptor));
    if (description.m_State != xiiResourceState::Loaded)
      UnloadData(Unload::AllQualityLevels);
    return description;
  }
  xiiLog::Error("Jolt collision resource '{}' is missing, invalid, or requires recooking for this Jolt build.", GetResourceID());
  UnloadData(Unload::AllQualityLevels);
  xiiResourceLoadDescription result;
  result.m_State                      = xiiResourceState::LoadedResourceMissing;
  result.m_uiQualityLevelsDiscardable = result.m_uiQualityLevelsLoadable = 0;
  return result;
}

xiiSharedPtr<xiiJoltCollisionMeshData> xiiJoltCollisionMeshResource::GetData() const
{
  XII_LOCK(m_DataMutex);
  return m_pData;
}

void xiiJoltCollisionMeshResource::UpdateMemoryUsage(MemoryUsage& out_usage)
{
  XII_LOCK(m_DataMutex);
  out_usage.m_uiMemoryCPU = sizeof(*this);
  if (m_pData)
    out_usage.m_uiMemoryCPU += sizeof(xiiJoltCollisionMeshData) + m_pData->m_pShape->GetStats().mSizeBytes + m_pData->m_Surfaces.GetHeapMemoryUsage();
  out_usage.m_uiMemoryGPU = 0;
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Resources_Implementation_JoltCollisionMeshResource);
