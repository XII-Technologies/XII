#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EnginePluginJolt/SceneExport/JoltStaticMeshConversion.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <JoltCooking/JoltCooking.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneExportModifier_JoltStaticMeshConversion, 1, xiiRTTIDefaultAllocator<xiiSceneExportModifier_JoltStaticMeshConversion>)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiSceneExportModifier_JoltStaticMeshConversion::ModifyWorld(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport)
{
  if (sDocumentType == "Prefab")
  {
    // the auto generated static meshes are needed in the prefab document, so that physical interactions for previewing purposes work
    // however, the scene also exports the static colmesh, including all the prefabs (with overridden materials)
    // in the final scene this would create double colmeshes in the same place, but the materials may differ
    // therefore we don't want to export the colmesh other than for preview purposes, so we ignore this, if 'bForExport' is true

    if (bForExport)
    {
      return;
    }
  }

  XII_LOCK(ref_world.GetWriteMarker());

  xiiSmcDescription desc;
  desc.m_Surfaces.PushBack(); // add a dummy empty material

  xiiMsgBuildStaticMesh msg;
  msg.m_pStaticMeshDescription = &desc;

  for (auto it = ref_world.GetObjects(); it.IsValid(); ++it)
  {
    if (!it->IsStatic())
      continue;

    it->SendMessage(msg);
  }

  if (desc.m_SubMeshes.IsEmpty() || desc.m_Vertices.IsEmpty() || desc.m_Triangles.IsEmpty())
    return;

  const xiiUInt32 uiNumVertices  = desc.m_Vertices.GetCount();
  const xiiUInt32 uiNumTriangles = desc.m_Triangles.GetCount();
  const xiiUInt32 uiNumSubMeshes = desc.m_SubMeshes.GetCount();

  xiiJoltCookingMesh xMesh;
  xMesh.m_Vertices.SetCountUninitialized(uiNumVertices);

  for (xiiUInt32 i = 0; i < uiNumVertices; ++i)
  {
    xMesh.m_Vertices[i] = desc.m_Vertices[i];
  }

  xMesh.m_PolygonIndices.SetCountUninitialized(uiNumTriangles * 3);
  xMesh.m_VerticesInPolygon.SetCountUninitialized(uiNumTriangles);
  xMesh.m_PolygonSurfaceID.SetCount(uiNumTriangles);

  for (xiiUInt32 i = 0; i < uiNumTriangles; ++i)
  {
    xMesh.m_VerticesInPolygon[i] = 3;

    xMesh.m_PolygonIndices[i * 3 + 0] = desc.m_Triangles[i].m_uiVertexIndices[0];
    xMesh.m_PolygonIndices[i * 3 + 1] = desc.m_Triangles[i].m_uiVertexIndices[1];
    xMesh.m_PolygonIndices[i * 3 + 2] = desc.m_Triangles[i].m_uiVertexIndices[2];
  }

  xiiHybridArray<xiiString, 32> surfaces;

  // copy materials
  // we could collate identical materials here and merge meshes, but the mesh cooking will probably do the same already
  {
    for (xiiUInt32 i = 0; i < desc.m_Surfaces.GetCount(); ++i)
    {
      surfaces.PushBack(desc.m_Surfaces[i]);
    }

    for (xiiUInt32 i = 0; i < uiNumSubMeshes; ++i)
    {
      const xiiUInt32 uiLastTriangle = desc.m_SubMeshes[i].m_uiFirstTriangle + desc.m_SubMeshes[i].m_uiNumTriangles;
      const xiiUInt16 uiSurface      = desc.m_SubMeshes[i].m_uiSurfaceIndex;

      for (xiiUInt32 t = desc.m_SubMeshes[i].m_uiFirstTriangle; t < uiLastTriangle; ++t)
      {
        xMesh.m_PolygonSurfaceID[t] = uiSurface;
      }
    }
  }

  xiiStringBuilder sDocGuid, sOutputFile;
  xiiConversionUtils::ToString(documentGuid, sDocGuid);

  sOutputFile.Format(":project/AssetCache/Generated/{0}.xiiJoltMesh", sDocGuid);

  xiiDeferredFileWriter file;
  file.SetOutput(sOutputFile);

  xiiAssetFileHeader header;
  header.Write(file).IgnoreResult();

  xiiChunkStreamWriter chunk(file);
  chunk.BeginStream(1);

  xiiJoltCooking::WriteResourceToStream(chunk, xMesh, surfaces, xiiJoltCooking::MeshType::Triangle);

  chunk.EndStream();

  if (file.Close().Failed())
  {
    xiiLog::Error("Could not write to global collision mesh file");
    return;
  }

  {
    xiiGameObject*    pGo;
    xiiGameObjectDesc god;
    god.m_sName.Assign("Greybox Collision Mesh");
    ref_world.CreateObject(god, pGo);

    auto* pCompMan = ref_world.GetOrCreateComponentManager<xiiJoltStaticActorComponentManager>();

    xiiJoltStaticActorComponent* pComp;
    pCompMan->CreateComponent(pGo, pComp);

    pComp->SetMeshFile(sOutputFile);
  }
}
