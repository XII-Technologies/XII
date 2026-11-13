/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Meshes/MeshResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltCollisionMeshAssetProperties, 1, xiiRTTIDefaultAllocator<xiiJoltCollisionMeshAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Mesh", m_sMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Static", xiiDependencyFlags::Transform)),
    XII_ENUM_MEMBER_PROPERTY("GeometryType", xiiJoltCollisionGeometryType, m_Type),
    XII_ARRAY_MEMBER_PROPERTY("Surfaces", m_Surfaces)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface")),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltCollisionMeshAssetDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltCollisionMeshAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiJoltCollisionMeshAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltCollisionMeshAssetDocument::xiiJoltCollisionMeshAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiJoltCollisionMeshAssetProperties>(sDocumentPath, xiiAssetDocEngineConnection::None)
{
}

xiiTransformStatus xiiJoltCollisionMeshAssetDocument::InternalTransformAsset(xiiStreamWriter&          stream, xiiStringView,
                                                                             const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader&, xiiBitflags<xiiTransformFlags>)
{
  const auto* pProperties = GetProperties();
  auto        asset       = xiiAssetCurator::GetSingleton()->FindSubAsset(pProperties->m_sMesh);
  if (!asset.isValid())
    return xiiStatus("Collision mesh source asset could not be resolved.");

  auto*         pInfo = asset->m_pAssetInfo;
  const auto    path  = pInfo->GetManager()->GetAbsoluteOutputFileName(pInfo->m_pDocumentTypeDescriptor, pInfo->m_Path, {}, pAssetProfile);
  xiiFileReader reader;
  if (reader.Open(path).Failed())
    return xiiStatus(xiiFmt("Transformed source mesh '{}' is unavailable. Transform the source mesh first.", path));
  xiiAssetFileHeader        header;
  xiiMeshResourceDescriptor mesh;
  if (header.Read(reader).Failed() || mesh.Load(reader).Failed())
    return xiiStatus(xiiFmt("Could not read transformed source mesh '{}'.", path));

  const auto& buffer = mesh.GetMeshBufferDescriptor();
  if (buffer.m_Topology != xiiGALPrimitiveTopology::TriangleList || buffer.GetVertexCount() == 0 ||
      buffer.GetVertexCount() > 1000000 || buffer.GetIndexCount() > 6000000)
    return xiiStatus("Collision cooking requires a non-empty triangle-list mesh within the geometry size limits.");
  const xiiMeshVertexStream* pPosition = nullptr;
  for (const auto& vertexStream : buffer.m_VertexStreams)
    if (vertexStream.m_Semantic == xiiMeshVertexSemantic::Position)
      pPosition = &vertexStream;
  if (pPosition == nullptr || pPosition->m_Format != xiiMeshVertexStreamFormat::Float3)
    return xiiStatus("Collision cooking requires CPU-accessible Float3 vertex positions.");
  const auto      vertices = buffer.GetVertexData();
  const auto      indices  = buffer.GetIndexData();
  const xiiUInt32 stride   = pPosition->m_uiStride;
  if (stride < sizeof(xiiVec3) || xiiUInt64(buffer.GetVertexCount() - 1) * stride + pPosition->m_uiOffset + sizeof(xiiVec3) > vertices.GetCount())
    return xiiStatus("Collision source mesh has an invalid position stream layout.");
  xiiUInt32 indexSize = 0;
  if (buffer.m_IndexType == xiiGALValueType::UInt16)
    indexSize = 2;
  if (buffer.m_IndexType == xiiGALValueType::UInt32)
    indexSize = 4;
  if (indexSize == 0 || xiiUInt64(buffer.GetIndexCount()) * indexSize > indices.GetCount())
    return xiiStatus("Collision source mesh requires valid UInt16 or UInt32 indices.");

  xiiArrayPtr<const xiiMeshSection> sections = mesh.GetSubMeshes();
  if (!mesh.GetLODs().IsEmpty())
    sections = mesh.GetLODs()[0].m_Sections;
  if (sections.IsEmpty())
    return xiiStatus("Collision source mesh contains no sections.");
  if (pProperties->m_Surfaces.GetCount() > 32 ||
      (pProperties->m_Type == xiiJoltCollisionGeometryType::ConvexHull && pProperties->m_Surfaces.GetCount() > 1))
    return xiiStatus("Collision meshes support up to 32 surface slots; convex hulls support one surface.");

  xiiDynamicArray<xiiVec3>   positions;
  xiiDynamicArray<xiiUInt32> triangles;
  xiiDynamicArray<xiiUInt16> triangleSurfaces;
  xiiDynamicArray<xiiUInt32> remap;
  remap.SetCount(buffer.GetVertexCount(), xiiInvalidIndex);
  for (const auto& section : sections)
  {
    const xiiUInt64 end = (xiiUInt64(section.m_uiFirstPrimitive) + section.m_uiPrimitiveCount) * 3;
    if (end > buffer.GetIndexCount())
      return xiiStatus("Collision source section exceeds its index buffer.");
    for (xiiUInt64 i = xiiUInt64(section.m_uiFirstPrimitive) * 3; i < end; ++i)
    {
      xiiUInt32 index = 0;
      if (indexSize == 2)
      {
        xiiUInt16 shortIndex;
        xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&shortIndex), indices.GetPtr() + i * indexSize, sizeof(shortIndex));
        index = shortIndex;
      }
      else
        xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&index), indices.GetPtr() + i * indexSize, sizeof(index));
      if (index >= remap.GetCount())
        return xiiStatus("Collision source mesh contains an out-of-range vertex index.");
      if (remap[index] == xiiInvalidIndex)
      {
        xiiVec3 position;
        xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&position), vertices.GetPtr() + xiiUInt64(index) * stride + pPosition->m_uiOffset, sizeof(position));
        remap[index] = positions.GetCount();
        positions.PushBack(position);
      }
      triangles.PushBack(remap[index]);
    }
    const xiiUInt32 slot = pProperties->m_Type == xiiJoltCollisionGeometryType::ConvexHull ? 0 : section.m_uiMaterialIndex;
    if (pProperties->m_Type == xiiJoltCollisionGeometryType::TriangleMesh && !pProperties->m_Surfaces.IsEmpty() && slot >= pProperties->m_Surfaces.GetCount())
      return xiiStatus("Assign a surface slot for each source mesh material, or leave all slots empty to inherit the actor surface.");
    for (xiiUInt32 i = 0; i < section.m_uiPrimitiveCount; ++i)
      triangleSurfaces.PushBack(pProperties->m_Surfaces.IsEmpty() ? 0 : static_cast<xiiUInt16>(slot));
  }
  xiiDynamicArray<xiiSurfaceResourceHandle> surfaces;
  for (const auto& surface : pProperties->m_Surfaces)
    surfaces.PushBack(surface.IsEmpty() ? xiiSurfaceResourceHandle{} : xiiResourceManager::LoadResource<xiiSurfaceResource>(surface));

  xiiJoltCollisionMeshResourceDescriptor collision;
  if (xiiJoltCollisionMeshResource::Cook(static_cast<xiiJoltCollisionGeometryType::Enum>(pProperties->m_Type.GetValue()), positions,
                                         triangles, triangleSurfaces, surfaces, collision)
        .Failed())
    return xiiStatus("Jolt collision cooking failed. See the transform log for geometry diagnostics.");
  if (collision.Save(stream).Failed())
    return xiiStatus("Could not write cooked Jolt collision geometry.");
  return XII_SUCCESS;
}

xiiJoltCollisionMeshAssetDocumentManager::xiiJoltCollisionMeshAssetDocumentManager()
{
  m_TypeDescriptor.m_sDocumentTypeName = "JoltCollisionMesh";
  m_TypeDescriptor.m_sFileExtension    = "xiiJoltCollisionMeshAsset";
  m_TypeDescriptor.m_sAssetCategory    = "Physics";
  m_TypeDescriptor.m_pDocumentType     = xiiGetStaticRTTI<xiiJoltCollisionMeshAssetDocument>();
  m_TypeDescriptor.m_pManager          = this;
  m_TypeDescriptor.m_CompatibleTypes.PushBack("CompatibleAsset_JoltCollisionMesh");
  m_TypeDescriptor.m_sResourceFileExtension = "xiiBinJoltCollisionMesh";
  m_TypeDescriptor.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave;
}

xiiStatus xiiJoltCollisionMeshAssetDocumentManager::InternalCreateDocument(xiiStringView, xiiStringView sPath, bool,
                                                                           xiiDocument*& out_pDocument, const xiiDocumentObject*)
{
  out_pDocument = new xiiJoltCollisionMeshAssetDocument(sPath);
  return XII_SUCCESS;
}

void xiiJoltCollisionMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_types) const
{
  inout_types.PushBack(&m_TypeDescriptor);
}

xiiUInt64 xiiJoltCollisionMeshAssetDocumentManager::ComputeAssetProfileHashImpl(const xiiPlatformProfile*) const
{
  return xiiJoltCollisionMeshResource::GetCookingVersion();
}

XII_STATICLINK_FILE(EditorPluginJolt, EditorPluginJolt_CollisionMeshAsset_Implementation_JoltCollisionMeshAsset);
