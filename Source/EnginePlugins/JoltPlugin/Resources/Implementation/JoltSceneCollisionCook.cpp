/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <JoltPlugin/Resources/Implementation/JoltSceneCollisionCook.h>

xiiJoltSceneCollisionCook::xiiJoltSceneCollisionCook()
{
  ConfigureTask("Jolt static scene collision", xiiTaskNesting::Never);
}

void xiiJoltSceneCollisionCook::Execute()
{
  if (HasBeenCanceled())
    return;
  if (m_pGeometry)
  {
    if (xiiJoltCollisionMeshResource::BuildStaticMesh(*m_pGeometry, m_pResult).Failed())
      xiiLog::Warning("Jolt could not cook a scene static geometry provider's output.");
    m_pGeometry.Clear();
  }
  else if (CookMesh().Failed() && !HasBeenCanceled())
  {
    xiiLog::Warning("Jolt could not extract static collision from mesh '{}' (section {}). Use a transformed triangle-list mesh or assign a cooked collision mesh.",
                    m_hMesh.GetResourceID(), m_uiSection);
  }
}

xiiResult xiiJoltSceneCollisionCook::CookMesh()
{
  // Load the transformed CPU descriptor through XII's VFS. Render resources may have
  // already discarded these streams or may be unavailable in a headless world.
  xiiFileReader reader;
  XII_SUCCEED_OR_RETURN(reader.Open(m_hMesh.GetResourceID(), 0, xiiFileShareMode::SharedReads, false));
  xiiAssetFileHeader        header;
  xiiMeshResourceDescriptor mesh;
  XII_SUCCEED_OR_RETURN(header.Read(reader));
  XII_SUCCEED_OR_RETURN(mesh.Load(reader));
  if (HasBeenCanceled())
    return XII_FAILURE;

  const auto& buffer      = mesh.GetMeshBufferDescriptor();
  const auto  vertices    = buffer.GetVertexData();
  const auto  indices     = buffer.GetIndexData();
  const auto  vertexCount = buffer.GetVertexCount();
  const auto  indexCount  = buffer.GetIndexCount();
  const auto  stride      = buffer.GetVertexStride();
  if (buffer.m_Topology != xiiGALPrimitiveTopology::TriangleList || vertexCount < 3 || vertexCount > 1000000 ||
      indexCount > 6000000 || xiiUInt64(vertexCount) * stride > vertices.GetCount())
    return XII_FAILURE;

  const xiiMeshVertexStream* pPosition = nullptr;
  for (const auto& stream : buffer.m_VertexStreams)
    if (stream.m_Semantic == xiiMeshVertexSemantic::Position)
      pPosition = &stream;
  if (!pPosition || pPosition->m_Format != xiiMeshVertexStreamFormat::Float3 ||
      xiiUInt64(pPosition->m_uiOffset) + sizeof(xiiVec3) > stride)
    return XII_FAILURE;

  const xiiUInt32 indexSize = buffer.m_IndexType == xiiGALValueType::UInt16 ? 2U : buffer.m_IndexType == xiiGALValueType::UInt32 ? 4U :
                                                                                                                                   0U;
  if (indexCount != 0 && (indexSize == 0 || xiiUInt64(indexCount) * indexSize > indices.GetCount()))
    return XII_FAILURE;

  auto sections = mesh.GetSubMeshes();
  if (!mesh.GetLODs().IsEmpty())
    sections = mesh.GetLODs()[0].m_Sections;
  if (m_uiSection != xiiInvalidIndex)
  {
    if (m_uiSection >= sections.GetCount())
      return XII_FAILURE;
    sections = sections.GetSubArray(m_uiSection, 1);
  }

  xiiDynamicArray<xiiVec3>                     positions;
  xiiDynamicArray<xiiUInt32>                   triangles;
  xiiDynamicArray<xiiUInt16>                   materials;
  xiiHybridArray<xiiSurfaceResourceHandle, 32> surfaceSlots;
  for (const auto& section : sections)
  {
    if (HasBeenCanceled())
      return XII_FAILURE;
    if (section.m_uiPrimitiveCount == 0)
      continue;
    const auto end = (xiiUInt64(section.m_uiFirstPrimitive) + section.m_uiPrimitiveCount) * 3;
    if (end > (indexCount != 0 ? indexCount : vertexCount))
      return XII_FAILURE;
    if (xiiUInt64(triangles.GetCount()) + xiiUInt64(section.m_uiPrimitiveCount) * 3 > 6000000)
      return XII_FAILURE;

    auto material = m_MaterialSlots.IndexOf(section.m_uiMaterialIndex);
    if (material == xiiInvalidIndex)
    {
      material = m_MaterialSlots.GetCount();
      if (material == 32)
        return XII_FAILURE;
      m_MaterialSlots.PushBack(section.m_uiMaterialIndex);
      surfaceSlots.ExpandAndGetRef();
    }

    for (auto i = xiiUInt64(section.m_uiFirstPrimitive) * 3; i < end; i += 3)
    {
      xiiUInt32 triangle[3];
      for (xiiUInt32 j = 0; j < 3; ++j)
      {
        auto index = static_cast<xiiUInt32>(i + j);
        if (indexCount != 0)
        {
          if (indexSize == 2)
          {
            xiiUInt16 value;
            xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&value), indices.GetPtr() + xiiUInt64(index) * indexSize, sizeof(value));
            index = value;
          }
          else
          {
            xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&index), indices.GetPtr() + xiiUInt64(index) * indexSize, sizeof(index));
          }
        }
        if (index >= vertexCount)
          return XII_FAILURE;
        triangle[j] = index;
      }
      triangles.PushBackRange(xiiMakeArrayPtr(triangle));
      materials.PushBack(static_cast<xiiUInt16>(material));
    }
  }
  positions.SetCountUninitialized(vertexCount);
  for (xiiUInt32 i = 0; i < vertexCount; ++i)
    xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&positions[i]), vertices.GetPtr() + xiiUInt64(i) * stride + pPosition->m_uiOffset, sizeof(xiiVec3));
  m_Materials = mesh.GetMaterials();
  return xiiJoltCollisionMeshResource::BuildShape(xiiJoltCollisionGeometryType::TriangleMesh, positions, triangles, materials, surfaceSlots, m_pResult);
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Resources_Implementation_JoltSceneCollisionCook);
