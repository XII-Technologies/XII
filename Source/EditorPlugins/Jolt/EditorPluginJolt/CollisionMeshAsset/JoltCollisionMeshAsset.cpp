#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAsset.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <JoltCooking/JoltCooking.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltCollisionMeshAssetDocument, 8, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static xiiMat3 CalculateTransformationMatrix(const xiiJoltCollisionMeshAssetProperties* pProp)
{
  const float us = xiiMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  const xiiBasisAxis::Enum forwardDir = xiiBasisAxis::GetOrthogonalAxis(pProp->m_RightDir, pProp->m_UpDir, !pProp->m_bFlipForwardDir);

  return xiiBasisAxis::CalculateTransformationMatrix(forwardDir, pProp->m_RightDir, pProp->m_UpDir, us);
}

xiiJoltCollisionMeshAssetDocument::xiiJoltCollisionMeshAssetDocument(const char* szDocumentPath, bool bConvexMesh) :
  xiiSimpleAssetDocument<xiiJoltCollisionMeshAssetProperties>(szDocumentPath, xiiAssetDocEngineConnection::Simple)
{
  m_bIsConvexMesh = bConvexMesh;
}

void xiiJoltCollisionMeshAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  // this logic is for backwards compatibility, to sync the convex state with existing data
  if (m_bIsConvexMesh)
  {
    GetPropertyObject()->GetTypeAccessor().SetValue("IsConvexMesh", m_bIsConvexMesh);
  }
  else
  {
    m_bIsConvexMesh = GetPropertyObject()->GetTypeAccessor().GetValue("IsConvexMesh").ConvertTo<bool>();
  }

  // the GetProperties object seems distinct from the GetPropertyObject, so keep them in sync
  GetProperties()->m_bIsConvexMesh = m_bIsConvexMesh;
}


//////////////////////////////////////////////////////////////////////////


xiiTransformStatus xiiJoltCollisionMeshAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  xiiProgressRange range("Transforming Asset", 2, false);

  xiiJoltCollisionMeshAssetProperties* pProp = GetProperties();

  const xiiUInt8 uiVersion = 2;
  stream << uiVersion;

  xiiUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  xiiCompressedStreamWriterZstd compressor(&stream, xiiCompressedStreamWriterZstd::Compression::Average);
  xiiChunkStreamWriter          chunk(compressor);
#else
  xiiChunkStreamWriter chunk(stream);
#endif

  stream << uiCompressionMode;

  chunk.BeginStream(1);

  {
    range.BeginNextStep("Preparing Mesh");

    xiiJoltCookingMesh xMesh;

    if (!m_bIsConvexMesh || pProp->m_ConvexMeshType == xiiJoltConvexCollisionMeshType::ConvexHull || pProp->m_ConvexMeshType == xiiJoltConvexCollisionMeshType::ConvexDecomposition)
    {
      XII_SUCCEED_OR_RETURN(CreateMeshFromFile(xMesh));
    }
    else
    {
      const xiiMat3 mTransformation = CalculateTransformationMatrix(pProp);

      xMesh.m_bFlipNormals = xiiGraphicsUtils::IsTriangleFlipRequired(mTransformation);

      xiiGeometry             geom;
      xiiGeometry::GeoOptions opt;
      opt.m_Transform = xiiMat4(mTransformation, xiiVec3::ZeroVector());

      if (pProp->m_ConvexMeshType == xiiJoltConvexCollisionMeshType::Cylinder)
      {
        geom.AddCylinderOnePiece(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight * 0.5f, pProp->m_fHeight * 0.5f, xiiMath::Clamp<xiiUInt16>(pProp->m_uiDetail, 3, 32), opt);
      }

      XII_SUCCEED_OR_RETURN(CreateMeshFromGeom(geom, xMesh));
    }

    range.BeginNextStep("Writing Result");
    XII_SUCCEED_OR_RETURN(WriteToStream(chunk, xMesh, GetProperties()));
  }

  chunk.EndStream();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  XII_SUCCEED_OR_RETURN(compressor.FinishCompressedStream());

  xiiLog::Dev("Compressed collision mesh data from {0} KB to {1} KB ({2}%%)", xiiArgF((float)compressor.GetUncompressedSize() / 1024.0f, 1), xiiArgF((float)compressor.GetCompressedSize() / 1024.0f, 1), xiiArgF(100.0f * compressor.GetCompressedSize() / compressor.GetUncompressedSize(), 1));

#endif

  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiJoltCollisionMeshAssetDocument::CreateMeshFromFile(xiiJoltCookingMesh& outMesh)
{
  xiiJoltCollisionMeshAssetProperties* pProp = GetProperties();

  xiiStringBuilder sAbsFilename = pProp->m_sMeshFile;
  if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return xiiStatus(xiiFmt("Couldn't make path absolute: '{0};", sAbsFilename));
  }

  xiiUniquePtr<xiiModelImporter2::Importer> pImporter = xiiModelImporter2::RequestImporterForFileType(sAbsFilename);
  if (pImporter == nullptr)
    return xiiStatus("No known importer for this file type.");

  xiiMeshResourceDescriptor meshDesc;

  xiiModelImporter2::ImportOptions opt;
  opt.m_sSourceFile   = sAbsFilename;
  opt.m_pMeshOutput   = &meshDesc;
  opt.m_RootTransform = CalculateTransformationMatrix(pProp);

  if (pImporter->Import(opt).Failed())
    return xiiStatus("Model importer was unable to read this asset.");

  const auto& meshBuffer = meshDesc.MeshBufferDesc();

  const xiiUInt32 uiNumTriangles = meshBuffer.GetPrimitiveCount();
  const xiiUInt32 uiNumVertices  = meshBuffer.GetVertexCount();

  outMesh.m_PolygonSurfaceID.SetCountUninitialized(uiNumTriangles);
  outMesh.m_VerticesInPolygon.SetCountUninitialized(uiNumTriangles);

  for (xiiUInt32 uiTriangle = 0; uiTriangle < uiNumTriangles; ++uiTriangle)
  {
    outMesh.m_PolygonSurfaceID[uiTriangle]  = 0; // default value, will be updated below when extracting materials.
    outMesh.m_VerticesInPolygon[uiTriangle] = 3; // Triangles!
  }

  // Extract vertices
  {
    const xiiUInt8* pVertexData  = meshDesc.MeshBufferDesc().GetVertexData(0, 0).GetPtr();
    const xiiUInt32 uiVertexSize = meshBuffer.GetVertexDataSize();

    outMesh.m_Vertices.SetCountUninitialized(uiNumVertices);
    for (xiiUInt32 v = 0; v < uiNumVertices; ++v)
    {
      outMesh.m_Vertices[v] = *reinterpret_cast<const xiiVec3*>(pVertexData + v * uiVertexSize);
    }
  }

  // Extract indices
  {
    outMesh.m_PolygonIndices.SetCountUninitialized(uiNumTriangles * 3);

    if (meshBuffer.Uses32BitIndices())
    {
      const xiiUInt32* pIndices = reinterpret_cast<const xiiUInt32*>(meshBuffer.GetIndexBufferData().GetPtr());

      for (xiiUInt32 tri = 0; tri < uiNumTriangles * 3; ++tri)
      {
        outMesh.m_PolygonIndices[tri] = pIndices[tri];
      }
    }
    else
    {
      const xiiUInt16* pIndices = reinterpret_cast<const xiiUInt16*>(meshBuffer.GetIndexBufferData().GetPtr());

      for (xiiUInt32 tri = 0; tri < uiNumTriangles * 3; ++tri)
      {
        outMesh.m_PolygonIndices[tri] = pIndices[tri];
      }
    }
  }

  // Extract Material Information
  if (m_bIsConvexMesh)
  {
    meshDesc.CollapseSubMeshes();
    pProp->m_Slots.SetCount(1);
    pProp->m_Slots[0].m_sLabel    = "Convex";
    pProp->m_Slots[0].m_sResource = pProp->m_sConvexMeshSurface;

    const auto subMeshInfo = meshDesc.GetSubMeshes()[0];

    for (xiiUInt32 tri = 0; tri < subMeshInfo.m_uiPrimitiveCount; ++tri)
    {
      outMesh.m_PolygonSurfaceID[subMeshInfo.m_uiFirstPrimitive + tri] = 0;
    }
  }
  else
  {
    pProp->m_Slots.SetCount(meshDesc.GetSubMeshes().GetCount());

    for (xiiUInt32 matIdx = 0; matIdx < pImporter->m_OutputMaterials.GetCount(); ++matIdx)
    {
      const xiiInt32 subMeshIdx = pImporter->m_OutputMaterials[matIdx].m_iReferencedByMesh;
      if (subMeshIdx < 0)
        continue;

      pProp->m_Slots[subMeshIdx].m_sLabel = pImporter->m_OutputMaterials[matIdx].m_sName;

      const auto subMeshInfo = meshDesc.GetSubMeshes()[subMeshIdx];

      if (pProp->m_Slots[subMeshIdx].m_bExclude)
      {
        // update the triangle material information
        for (xiiUInt32 tri = 0; tri < subMeshInfo.m_uiPrimitiveCount; ++tri)
        {
          outMesh.m_PolygonSurfaceID[subMeshInfo.m_uiFirstPrimitive + tri] = 0xFFFF;
        }
      }
      else
      {
        // update the triangle material information
        for (xiiUInt32 tri = 0; tri < subMeshInfo.m_uiPrimitiveCount; ++tri)
        {
          outMesh.m_PolygonSurfaceID[subMeshInfo.m_uiFirstPrimitive + tri] = subMeshIdx;
        }
      }
    }

    ApplyNativePropertyChangesToObjectManager();
    pProp = GetProperties();
  }

  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiJoltCollisionMeshAssetDocument::CreateMeshFromGeom(xiiGeometry& geom, xiiJoltCookingMesh& outMesh)
{
  xiiJoltCollisionMeshAssetProperties* pProp = GetProperties();

  // Material setup.
  {
    // Ensure there is just one slot.
    if (pProp->m_Slots.GetCount() != 1)
    {
      GetObjectAccessor()->StartTransaction("Update Mesh Material Info");

      pProp->m_Slots.SetCount(1);
      pProp->m_Slots[0].m_sLabel = "Default";

      ApplyNativePropertyChangesToObjectManager();
      GetObjectAccessor()->FinishTransaction();

      // Need to reacquire pProp pointer since it might be reallocated.
      pProp = GetProperties();
    }
  }

  // copy vertex positions
  {
    outMesh.m_Vertices.SetCountUninitialized(geom.GetVertices().GetCount());
    for (xiiUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
    {
      outMesh.m_Vertices[v] = geom.GetVertices()[v].m_vPosition;
    }
  }

  // Copy Polygon Data
  {
    outMesh.m_PolygonSurfaceID.SetCountUninitialized(geom.GetPolygons().GetCount());
    outMesh.m_VerticesInPolygon.SetCountUninitialized(geom.GetPolygons().GetCount());
    outMesh.m_PolygonIndices.Reserve(geom.GetPolygons().GetCount() * 4);

    for (xiiUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    {
      const auto& poly               = geom.GetPolygons()[p];
      outMesh.m_VerticesInPolygon[p] = poly.m_Vertices.GetCount();
      outMesh.m_PolygonSurfaceID[p]  = 0;

      for (xiiUInt32 posIdx : poly.m_Vertices)
      {
        outMesh.m_PolygonIndices.PushBack(posIdx);
      }
    }
  }

  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiJoltCollisionMeshAssetDocument::WriteToStream(xiiChunkStreamWriter& stream, const xiiJoltCookingMesh& mesh, const xiiJoltCollisionMeshAssetProperties* pProp)
{
  xiiHybridArray<xiiString, 32> surfaces;

  for (const auto& slot : pProp->m_Slots)
  {
    surfaces.PushBack(slot.m_sResource);
  }

  xiiJoltCooking::MeshType meshType = xiiJoltCooking::MeshType::Triangle;

  if (pProp->m_bIsConvexMesh)
  {
    if (pProp->m_ConvexMeshType == xiiJoltConvexCollisionMeshType::ConvexDecomposition)
    {
      meshType = xiiJoltCooking::MeshType::ConvexDecomposition;
    }
    else
    {
      meshType = xiiJoltCooking::MeshType::ConvexHull;
    }
  }

  return xiiJoltCooking::WriteResourceToStream(stream, mesh, surfaces, meshType, pProp->m_uiMaxConvexPieces);
}

xiiTransformStatus xiiJoltCollisionMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  xiiStatus status = xiiAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

void xiiJoltCollisionMeshAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (GetProperties()->m_ConvexMeshType != xiiJoltConvexCollisionMeshType::ConvexHull)
  {
    // remove the mesh file dependency, if it is not actually used
    const auto& sMeshFile = GetProperties()->m_sMeshFile;
    pInfo->m_AssetTransformDependencies.Remove(sMeshFile);
  }
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltCollisionMeshAssetDocumentGenerator, 1, xiiRTTIDefaultAllocator<xiiJoltCollisionMeshAssetDocumentGenerator>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiJoltCollisionMeshAssetDocumentGenerator::xiiJoltCollisionMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

xiiJoltCollisionMeshAssetDocumentGenerator::~xiiJoltCollisionMeshAssetDocumentGenerator() = default;

void xiiJoltCollisionMeshAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  xiiStringBuilder baseOutputFile = szParentDirRelativePath;
  baseOutputFile.ChangeFileExtension("xiiJoltCollisionMeshAsset");

  {
    xiiAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority                       = xiiAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName                          = "Jolt_Colmesh_Triangle";
    info.m_sOutputFileParentRelative      = baseOutputFile;
    info.m_sIcon                          = ":/AssetIcons/Jolt_Collision_Mesh.png";
  }
}

xiiStatus xiiJoltCollisionMeshAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument)
{
  auto pApp = xiiQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, xiiDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return xiiStatus("Could not create target document");

  xiiJoltCollisionMeshAssetDocument* pAssetDoc = xiiDynamicCast<xiiJoltCollisionMeshAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return xiiStatus("Target document is not a valid xiiJoltCollisionMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", szDataDirRelativePath);

  return xiiStatus(XII_SUCCESS);
}


//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltConvexCollisionMeshAssetDocumentGenerator, 1, xiiRTTIDefaultAllocator<xiiJoltConvexCollisionMeshAssetDocumentGenerator>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiJoltConvexCollisionMeshAssetDocumentGenerator::xiiJoltConvexCollisionMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

xiiJoltConvexCollisionMeshAssetDocumentGenerator::~xiiJoltConvexCollisionMeshAssetDocumentGenerator() = default;

void xiiJoltConvexCollisionMeshAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  xiiStringBuilder baseOutputFile = szParentDirRelativePath;
  baseOutputFile.ChangeFileExtension("xiiJoltConvexCollisionMeshAsset");

  {
    xiiAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority                       = xiiAssetDocGeneratorPriority::LowPriority;
    info.m_sName                          = "Jolt_Colmesh_Convex";
    info.m_sOutputFileParentRelative      = baseOutputFile;
    info.m_sIcon                          = ":/AssetIcons/Jolt_Collision_Mesh_Convex.png";
  }
}

xiiStatus xiiJoltConvexCollisionMeshAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument)
{
  auto pApp = xiiQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, xiiDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return xiiStatus("Could not create target document");

  xiiJoltCollisionMeshAssetDocument* pAssetDoc = xiiDynamicCast<xiiJoltCollisionMeshAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return xiiStatus("Target document is not a valid xiiJoltCollisionMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", szDataDirRelativePath);

  return xiiStatus(XII_SUCCESS);
}
