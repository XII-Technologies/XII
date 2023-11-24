#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GraphicsCore/Meshes/CpuMeshResource.h>
#include <GraphicsCore/Meshes/MeshBufferUtils.h>
#include <GraphicsCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgExtractGeometry);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgExtractGeometry, 1, xiiRTTIDefaultAllocator<xiiMsgExtractGeometry>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiWorldGeoExtractionUtil::ExtractWorldGeometry(MeshObjectList& ref_objects, const xiiWorld& world, ExtractionMode mode, xiiTagSet* pExcludeTags /*= nullptr*/)
{
  XII_PROFILE_SCOPE("ExtractWorldGeometry");
  XII_LOG_BLOCK("ExtractWorldGeometry", world.GetName());

  xiiMsgExtractGeometry msg;
  msg.m_Mode         = mode;
  msg.m_pMeshObjects = &ref_objects;

  XII_LOCK(world.GetReadMarker());

  for (auto it = world.GetObjects(); it.IsValid(); ++it)
  {
    if (pExcludeTags != nullptr && it->GetTags().IsAnySet(*pExcludeTags))
      continue;

    it->SendMessage(msg);
  }
}

void xiiWorldGeoExtractionUtil::ExtractWorldGeometry(MeshObjectList& ref_objects, const xiiWorld& world, ExtractionMode mode, const xiiDeque<xiiGameObjectHandle>& selection)
{
  XII_PROFILE_SCOPE("ExtractWorldGeometry");
  XII_LOG_BLOCK("ExtractWorldGeometry", world.GetName());

  xiiMsgExtractGeometry msg;
  msg.m_Mode         = mode;
  msg.m_pMeshObjects = &ref_objects;

  XII_LOCK(world.GetReadMarker());

  for (xiiGameObjectHandle hObject : selection)
  {
    const xiiGameObject* pObject;
    if (!world.TryGetObject(hObject, pObject))
      continue;

    pObject->SendMessage(msg);
  }
}

void xiiWorldGeoExtractionUtil::WriteWorldGeometryToOBJ(const char* szFile, const MeshObjectList& objects, const xiiMat3& mTransform)
{
  XII_LOG_BLOCK("Write World Geometry to OBJ", szFile);

  xiiFileWriter file;
  if (file.Open(szFile).Failed())
  {
    xiiLog::Error("Failed to open file for writing: '{0}'", szFile);
    return;
  }

  xiiMat4 transform = xiiMat4::IdentityMatrix();
  transform.SetRotationalPart(mTransform);

  xiiStringBuilder line;

  line = "\n\n# vertices\n\n";
  file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

  xiiUInt32           uiVertexOffset = 0;
  xiiDeque<xiiUInt32> indices;

  for (const MeshObject& object : objects)
  {
    xiiResourceLock<xiiCpuMeshResource> pCpuMesh(object.m_hMeshResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pCpuMesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    {
      continue;
    }

    const auto& meshBufferDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();

    const xiiVec3* pPositions      = nullptr;
    xiiUInt32      uiElementStride = 0;
    if (xiiMeshBufferUtils::GetPositionStream(meshBufferDesc, pPositions, uiElementStride).Failed())
    {
      continue;
    }

    xiiMat4 finalTransform = transform * object.m_GlobalTransform.GetAsMat4();

    // write out all vertices
    for (xiiUInt32 i = 0; i < meshBufferDesc.GetVertexCount(); ++i)
    {
      const xiiVec3 pos = finalTransform.TransformPosition(*pPositions);

      line.Format("v {0} {1} {2}\n", xiiArgF(pos.x, 8), xiiArgF(pos.y, 8), xiiArgF(pos.z, 8));
      file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

      pPositions = xiiMemoryUtils::AddByteOffset(pPositions, uiElementStride);
    }

    // collect all indices
    bool flip = xiiGraphicsUtils::IsTriangleFlipRequired(finalTransform.GetRotationalPart());

    if (meshBufferDesc.HasIndexBuffer())
    {
      if (meshBufferDesc.Uses32BitIndices())
      {
        const xiiUInt32* pTypedIndices = reinterpret_cast<const xiiUInt32*>(meshBufferDesc.GetIndexBufferData().GetPtr());

        for (xiiUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
        {
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + 1] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset);
        }
      }
      else
      {
        const xiiUInt16* pTypedIndices = reinterpret_cast<const xiiUInt16*>(meshBufferDesc.GetIndexBufferData().GetPtr());

        for (xiiUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
        {
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + 1] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset);
        }
      }
    }
    else
    {
      for (xiiUInt32 v = 0; v < meshBufferDesc.GetVertexCount(); ++v)
      {
        indices.PushBack(uiVertexOffset + v);
      }
    }

    uiVertexOffset += meshBufferDesc.GetVertexCount();
  }

  line = "\n\n# triangles\n\n";
  file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

  for (xiiUInt32 i = 0; i < indices.GetCount(); i += 3)
  {
    // indices are 1 based in obj
    line.Format("f {0} {1} {2}\n", indices[i + 0] + 1, indices[i + 1] + 1, indices[i + 2] + 1);
    file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();
  }

  xiiLog::Success("Wrote world geometry to '{0}'", file.GetFilePathAbsolute().GetView());
}

//////////////////////////////////////////////////////////////////////////

void xiiMsgExtractGeometry::AddMeshObject(const xiiTransform& transform, xiiCpuMeshResourceHandle hMeshResource)
{
  m_pMeshObjects->PushBack({transform, hMeshResource});
}

void xiiMsgExtractGeometry::AddBox(const xiiTransform& transform, xiiVec3 vExtents)
{
  const char*              szResourceName = "CpuMesh-UnitBox";
  xiiCpuMeshResourceHandle hBoxMesh       = xiiResourceManager::GetExistingResource<xiiCpuMeshResource>(szResourceName);
  if (hBoxMesh.IsValid() == false)
  {
    xiiGeometry geom;
    geom.AddBox(xiiVec3(1), false);
    geom.TriangulatePolygons();
    geom.ComputeTangents();

    xiiMeshResourceDescriptor desc;
    desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Data/Base/Materials/Common/Pattern.xiiMaterialAsset

    desc.MeshBufferDesc().AddCommonStreams();
    desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::TriangleList);

    desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

    desc.ComputeBounds();

    hBoxMesh = xiiResourceManager::GetOrCreateResource<xiiCpuMeshResource>(szResourceName, std::move(desc), szResourceName);
  }

  auto& meshObject             = m_pMeshObjects->ExpandAndGetRef();
  meshObject.m_GlobalTransform = transform;
  meshObject.m_GlobalTransform.m_vScale *= vExtents;
  meshObject.m_hMeshResource = hBoxMesh;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Utils_Implementation_WorldGeoExtractionUtil);
