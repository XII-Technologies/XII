/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/Interfaces/NavmeshGeoWorldModule.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Meshes/MeshSkinning.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/Device.h>
#include <SharedPluginScene/Scene/SceneGeometryExporter.h>

namespace
{
  xiiResult ReadBuffer(xiiGALBuffer* source, xiiDynamicArray<xiiUInt8>& bytes)
  {
    if (!source || source->GetDescription().m_uiSize > xiiMath::MaxValue<xiiUInt32>())
      return XII_FAILURE;
    auto  device = source->GetDevice();
    auto* queue  = device->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
    if (!queue)
      return XII_FAILURE;
    // Synchronize any previous frame's writes before a cold tooling readback.
    device->WaitIdle();
    xiiGALBufferCreationDescription description;
    description.m_uiSize                         = source->GetDescription().m_uiSize;
    description.m_Usage                          = xiiGALResourceUsage::Staging;
    description.m_CPUAccessFlags                 = xiiGALCPUAccessFlag::Read;
    auto                                 staging = device->CreateBuffer(description);
    xiiGALCommandListCreationDescription commands;
    commands.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer;
    auto copy             = device->CreateCommandList(commands);
    if (!staging || !copy)
      return XII_FAILURE;
    copy->Begin();
    copy->CopyBuffer(source, staging.Borrow());
    copy->End();
    const auto fence = queue->Submit(copy.Borrow());
    if (fence == 0)
      return XII_FAILURE;
    queue->WaitForFenceValue(fence);
    auto mapping = device->CreateCommandList(commands);
    if (!mapping)
      return XII_FAILURE;
    mapping->Begin();
    XII_SCOPE_EXIT(mapping->End());
    void* data = nullptr;
    XII_SUCCEED_OR_RETURN(mapping->MapBuffer(staging.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, data));
    if (!data)
    {
      mapping->UnmapBuffer(staging.Borrow(), xiiGALMapType::Read).IgnoreResult();
      return XII_FAILURE;
    }
    bytes.SetCountUninitialized(static_cast<xiiUInt32>(description.m_uiSize));
    xiiMemoryUtils::Copy(bytes.GetData(), static_cast<const xiiUInt8*>(data), bytes.GetCount());
    return mapping->UnmapBuffer(staging.Borrow(), xiiGALMapType::Read);
  }

  const xiiMeshVertexStream* FindStream(const xiiMeshBufferResourceDescriptor& data, xiiMeshVertexSemantic::Enum semantic)
  {
    for (const auto& stream : data.m_VertexStreams)
      if (stream.m_Semantic == semantic)
        return &stream;
    return nullptr;
  }

  bool ValidStream(const xiiMeshVertexStream* stream, xiiUInt32 stride, xiiUInt32 size)
  {
    return stream && stream->m_uiOffset <= stride && size <= stride - stream->m_uiOffset && stream->m_uiStride == stride;
  }

  struct ObjWriter
  {
    xiiStreamWriter& m_Stream;
    xiiUInt64        m_uiVertices = 0;

    xiiResult Vertex(const xiiVec3& position)
    {
      if (!position.IsValid())
        return XII_FAILURE;
      xiiStringBuilder line;
      // Nine significant digits preserve float coordinates on text round trips.
      line.SetFormat("v {} {} {}\n", xiiArgF(position.x, 8, true), xiiArgF(position.y, 8, true), xiiArgF(position.z, 8, true));
      XII_SUCCEED_OR_RETURN(m_Stream.WriteBytes(line.GetData(), line.GetElementCount()));
      ++m_uiVertices;
      return XII_SUCCESS;
    }

    xiiResult Face(xiiUInt64 a, xiiUInt64 b, xiiUInt64 c, bool mirrored)
    {
      xiiStringBuilder line;
      line.SetFormat("f {} {} {}\n", a, mirrored ? c : b, mirrored ? b : c);
      return m_Stream.WriteBytes(line.GetData(), line.GetElementCount());
    }
  };

  xiiStatus ExportMesh(ObjWriter& writer, const xiiMeshComponentBase& component, const xiiMat3& coordinates)
  {
    if (!component.GetMesh().IsValid())
      return XII_SUCCESS;
    xiiResourceLock<xiiMeshResource> mesh(component.GetMesh(), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (mesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
      return xiiStatus("Cannot acquire a native mesh for geometry export.");
    xiiResourceLock<xiiMeshBufferResource> buffer(mesh->GetMeshBuffer(), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (buffer.GetAcquireResult() != xiiResourceAcquireResult::Final)
      return xiiStatus("Cannot acquire a native mesh buffer for geometry export.");
    const auto& data     = buffer->GetDescriptor();
    const auto  stride   = data.GetVertexStride();
    const auto* position = FindStream(data, xiiMeshVertexSemantic::Position);
    if (data.m_Topology != xiiGALPrimitiveTopology::TriangleList || !ValidStream(position, stride, sizeof(xiiVec3)) || position->m_Format != xiiMeshVertexStreamFormat::Float3)
      return xiiStatus("Geometry export requires native float3 positions and triangle-list topology.");
    xiiDynamicArray<xiiUInt8> vertexReadback, indexReadback;
    auto                      vertices = data.GetVertexData();
    auto                      indices  = data.GetIndexData();
    if (vertices.IsEmpty() && data.GetVertexCount() != 0)
    {
      XII_SUCCEED_OR_RETURN(ReadBuffer(buffer->GetVertexBuffer().Borrow(), vertexReadback));
      vertices = vertexReadback;
    }
    if (indices.IsEmpty() && data.GetIndexCount() != 0)
    {
      XII_SUCCEED_OR_RETURN(ReadBuffer(buffer->GetIndexBuffer().Borrow(), indexReadback));
      indices = indexReadback;
    }
    const xiiUInt32 indexSize = data.m_IndexType == xiiGALValueType::UInt16 ? 2 : data.m_IndexType == xiiGALValueType::UInt32 ? 4 :
                                                                                                                                0;
    if (vertices.GetCount() < xiiUInt64(data.GetVertexCount()) * stride ||
        (data.GetIndexCount() != 0 && (indexSize == 0 || indices.GetCount() < xiiUInt64(data.GetIndexCount()) * indexSize)))
      return xiiStatus("Native mesh geometry has incomplete vertex or index data.");
    auto readIndex = [&](xiiUInt32 index) -> xiiUInt32 {
      if (data.GetIndexCount() == 0)
        return index;
      xiiUInt32 result = 0;
      xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&result), indices.GetPtr() + xiiUInt64(index) * indexSize, indexSize);
      return result;
    };
    const auto*                skinned = xiiDynamicCast<const xiiSkinnedMeshComponent*>(&component);
    xiiArrayPtr<const xiiMat4> skin    = skinned ? skinned->GetSkinningMatrices() : xiiArrayPtr<const xiiMat4>();
    if (skinned)
      for (float weight : skinned->GetMorphWeights())
        if (weight != 0.0f)
          return xiiStatus("The native mesh has no CPU morph-delta export interface; nonzero morph weights cannot be exported.");
    const auto* bones   = FindStream(data, xiiMeshVertexSemantic::BoneIndices0);
    const auto* weights = FindStream(data, xiiMeshVertexSemantic::BoneWeights0);
    if (!skin.IsEmpty() && (!ValidStream(bones, stride, 8) || bones->m_Format != xiiMeshVertexStreamFormat::UShort4 || !ValidStream(weights, stride, 4) || weights->m_Format != xiiMeshVertexStreamFormat::UByte4Normalized))
      return xiiStatus("Skinned geometry export requires native ushort4 indices and normalized byte4 weights.");
    xiiDynamicArray<xiiVec3> positions;
    positions.SetCountUninitialized(data.GetVertexCount());
    for (xiiUInt32 i = 0; i < positions.GetCount(); ++i)
    {
      const auto* vertex = vertices.GetPtr() + xiiUInt64(i) * stride;
      xiiVec3     p;
      xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&p), vertex + position->m_uiOffset, sizeof(p));
      if (!skin.IsEmpty())
      {
        xiiVec4U16       joints;
        xiiColorLinearUB influence;
        xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&joints), vertex + bones->m_uiOffset, sizeof(joints));
        xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&influence), vertex + weights->m_uiOffset, sizeof(influence));
        if (xiiMeshSkinning::TransformPosition(p, joints, influence, skin, p).Failed())
          return xiiStatus("Native mesh skinning references invalid influences or produces a non-finite position.");
      }
      if (!p.IsValid())
        return xiiStatus("Native mesh export encountered a non-finite vertex.");
      positions[i] = p;
    }
    const auto*     instanced     = xiiDynamicCast<const xiiInstancedMeshComponent*>(&component);
    const auto      instances     = instanced ? instanced->GetInstanceTransforms() : xiiArrayPtr<const xiiMat4>();
    const xiiUInt32 instanceCount = instanced ? instances.GetCount() : 1;
    const auto      global        = component.GetOwner()->GetGlobalTransform().GetAsMat4();
    const auto      sections      = mesh->GetSections();
    if (component.GetSectionIndex() != xiiInvalidIndex && component.GetSectionIndex() >= sections.GetCount())
      return xiiStatus("Mesh component selects a nonexistent section.");
    for (xiiUInt32 instance = 0; instance < instanceCount; ++instance)
    {
      const auto  transform   = instanced ? global * instances[instance] : global;
      const float determinant = coordinates.GetDeterminant() * transform.GetRotationalPart().GetDeterminant();
      if (!transform.IsValid() || !xiiMath::IsFinite(determinant) || determinant == 0.0f)
        return xiiStatus("Geometry export encountered a singular or non-finite instance transform.");
      const auto base = writer.m_uiVertices + 1;
      for (const auto& p : positions)
        XII_SUCCEED_OR_RETURN(writer.Vertex(coordinates * transform.TransformPosition(p)));
      auto range = [&](xiiUInt32 first, xiiUInt32 count) -> xiiResult {
        if (first > data.GetPrimitiveCount() || count > data.GetPrimitiveCount() - first)
          return XII_FAILURE;
        for (xiiUInt32 triangle = first; triangle < first + count; ++triangle)
        {
          const auto a = readIndex(triangle * 3), b = readIndex(triangle * 3 + 1), c = readIndex(triangle * 3 + 2);
          if (a >= positions.GetCount() || b >= positions.GetCount() || c >= positions.GetCount())
            return XII_FAILURE;
          XII_SUCCEED_OR_RETURN(writer.Face(base + a, base + b, base + c, determinant < 0));
        }
        return XII_SUCCESS;
      };
      if (component.GetSectionIndex() != xiiInvalidIndex)
      {
        const auto& section = sections[component.GetSectionIndex()];
        XII_SUCCEED_OR_RETURN(range(section.m_uiFirstPrimitive, section.m_uiPrimitiveCount));
      }
      else if (!mesh->GetLODs().IsEmpty())
      {
        for (const auto& section : mesh->GetLODs()[0].m_Sections)
          XII_SUCCEED_OR_RETURN(range(section.m_uiFirstPrimitive, section.m_uiPrimitiveCount));
      }
      else
        XII_SUCCEED_OR_RETURN(range(0, data.GetPrimitiveCount()));
    }
    return XII_SUCCESS;
  }
} // namespace

xiiStatus xiiSceneGeometryExporter::ExportOBJ(xiiWorld& world, xiiStringView file, xiiSceneGeometryExportMode mode,
                                              const xiiMat3& coordinates, bool selectionOnly, xiiArrayPtr<const xiiGameObjectHandle> selection)
{
  if (!coordinates.IsValid() || !xiiMath::IsFinite(coordinates.GetDeterminant()) || coordinates.GetDeterminant() == 0)
    return xiiStatus("Geometry export requires a finite invertible coordinate transform.");
  if (mode < xiiSceneGeometryExportMode::RenderMesh || mode > xiiSceneGeometryExportMode::NavmeshObstacles)
    return xiiStatus("Unknown scene geometry export mode.");
  xiiHashSet<xiiGameObjectHandle> selected;
  if (selectionOnly)
  {
    selected.Reserve(selection.GetCount());
    for (auto handle : selection)
      selected.Insert(handle);
  }
  xiiDeferredFileWriter output;
  output.SetOutput(file);
  bool published = false;
  XII_SCOPE_EXIT(if (!published) output.Discard());
  ObjWriter writer{output};
  if (mode == xiiSceneGeometryExportMode::NavmeshObstacles)
  {
    const auto* provider = world.GetModule<xiiNavmeshGeoWorldModuleInterface>();
    if (!provider || selectionOnly)
      return xiiStatus("Navmesh export requires a native geometry provider and a whole-scene query.");
    xiiBoundingBox bounds = xiiBoundingBox::MakeInvalid();
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      it->UpdateGlobalTransformAndBounds();
      if (it->IsActive() && !it->GetTags().IsSetByName(xiiStringView("Editor")) && it->GetGlobalBounds().IsValid())
        bounds.ExpandToInclude(it->GetGlobalBounds().GetBox());
    }
    if (bounds.IsValid())
    {
      xiiDynamicArray<xiiNavmeshTriangle> triangles;
      provider->RetrieveGeometryInArea(0, bounds, triangles);
      for (const auto& triangle : triangles)
      {
        const auto base = writer.m_uiVertices + 1;
        for (const auto& p : triangle.m_Vertices)
          XII_SUCCEED_OR_RETURN(writer.Vertex(coordinates * p));
        XII_SUCCEED_OR_RETURN(writer.Face(base, base + 1, base + 2, coordinates.GetDeterminant() < 0));
      }
    }
  }
  else
  {
    xiiSmcDescription collision;
    bool              handledCollision = false;
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      if (!it->IsActive() || it->GetTags().IsSetByName(xiiStringView("Editor")) || (selectionOnly && !selected.Contains(it->GetHandle())))
        continue;
      it->UpdateGlobalTransform();
      if (mode == xiiSceneGeometryExportMode::CollisionMesh)
      {
        xiiMsgBuildStaticMesh message;
        message.m_pStaticMeshDescription = &collision;
        handledCollision |= it->SendMessage(message);
      }
      else
        for (const auto* component : it->GetComponents())
          if (const auto* mesh = xiiDynamicCast<const xiiMeshComponentBase*>(component); mesh && mesh->IsActive())
            XII_SUCCEED_OR_RETURN(ExportMesh(writer, *mesh, coordinates));
    }
    if (mode == xiiSceneGeometryExportMode::CollisionMesh)
    {
      if (!handledCollision)
        return xiiStatus("No selected native component provides static collision geometry.");
      for (const auto& p : collision.m_Vertices)
        XII_SUCCEED_OR_RETURN(writer.Vertex(coordinates * p));
      for (const auto& triangle : collision.m_Triangles)
      {
        const auto* index = triangle.m_uiVertexIndices;
        if (index[0] >= collision.m_Vertices.GetCount() || index[1] >= collision.m_Vertices.GetCount() || index[2] >= collision.m_Vertices.GetCount())
          return xiiStatus("Collision geometry provider returned invalid triangle indices.");
        XII_SUCCEED_OR_RETURN(writer.Face(xiiUInt64(index[0]) + 1, xiiUInt64(index[1]) + 1, xiiUInt64(index[2]) + 1, coordinates.GetDeterminant() < 0));
      }
    }
  }
  XII_SUCCEED_OR_RETURN(output.Close());
  published = true;
  return XII_SUCCESS;
}
