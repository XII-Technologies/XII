#include <JoltCooking/JoltCookingPCH.h>

#include <Core/Graphics/ConvexHull.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/Progress.h>
#include <JoltCooking/JoltCooking.h>

#include <Foundation/IO/MemoryStream.h>
#include <Jolt/Core/StreamOut.h>
#include <Jolt/Geometry/IndexedTriangle.h>
#include <Jolt/Math/Float3.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <Physics/Collision/Shape/ConvexHullShape.h>

#define ENABLE_VHACD_IMPLEMENTATION 1
#include <VHACD/VHACD.h>
using namespace VHACD;

class xiiJoltStreamOut : public JPH::StreamOut
{
public:
  xiiJoltStreamOut(xiiStreamWriter* pPassThrough)
  {
    m_pWriter = pPassThrough;
  }

  virtual void WriteBytes(const void* inData, size_t inNumBytes) override
  {
    if (m_pWriter->WriteBytes(inData, inNumBytes).Failed())
      m_bFailed = true;
  }

  virtual bool IsFailed() const override
  {
    return m_bFailed;
  }


private:
  xiiStreamWriter* m_pWriter = nullptr;
  bool             m_bFailed = false;
};

xiiResult xiiJoltCooking::CookTriangleMesh(const xiiJoltCookingMesh& mesh, xiiStreamWriter& OutputStream)
{
  JPH::RegisterDefaultAllocator();

  JPH::VertexList          vertexList;
  JPH::IndexedTriangleList triangleList;

  // copy vertices
  {
    vertexList.resize(mesh.m_Vertices.GetCount());
    for (xiiUInt32 i = 0; i < mesh.m_Vertices.GetCount(); ++i)
    {
      vertexList[i] = xiiJoltConversionUtils::ToFloat3(mesh.m_Vertices[i]);
    }
  }

  xiiUInt32 uiMaxMaterialIndex = 0;

  // compute number of triangles
  {
    xiiUInt32 uiTriangles = 0;

    for (xiiUInt32 i = 0; i < mesh.m_VerticesInPolygon.GetCount(); ++i)
    {
      if (mesh.m_PolygonSurfaceID[i] == 0xFFFF)
        continue;

      uiTriangles += mesh.m_VerticesInPolygon[i] - 2;
    }

    triangleList.resize(uiTriangles);
  }

  // triangulate
  {
    xiiUInt32 uiIdxOffset = 0;
    xiiUInt32 uiTriIdx    = 0;

    for (xiiUInt32 poly = 0; poly < mesh.m_VerticesInPolygon.GetCount(); ++poly)
    {
      const xiiUInt32 polyVerts = mesh.m_VerticesInPolygon[poly];

      if (mesh.m_PolygonSurfaceID[poly] != 0xFFFF)
      {
        for (xiiUInt32 tri = 0; tri < polyVerts - 2; ++tri)
        {
          const xiiUInt32 uiMaterialID = mesh.m_PolygonSurfaceID[poly];

          uiMaxMaterialIndex = xiiMath::Max(uiMaxMaterialIndex, uiMaterialID);

          const xiiUInt32 idx0 = mesh.m_PolygonIndices[uiIdxOffset + 0];
          const xiiUInt32 idx1 = mesh.m_PolygonIndices[uiIdxOffset + tri + 1];
          const xiiUInt32 idx2 = mesh.m_PolygonIndices[uiIdxOffset + tri + 2];

          triangleList[uiTriIdx].mMaterialIndex = uiMaterialID;
          triangleList[uiTriIdx].mIdx[0]        = idx0;
          triangleList[uiTriIdx].mIdx[1]        = idx1;
          triangleList[uiTriIdx].mIdx[2]        = idx2;

          ++uiTriIdx;
        }
      }

      uiIdxOffset += polyVerts;
    }
  }

  // cook mesh (create Jolt shape, then save to binary stream)
  {
    JPH::MeshShapeSettings meshSettings(vertexList, triangleList);
    meshSettings.mMaterials.resize(uiMaxMaterialIndex + 1);

    auto shapeRes = meshSettings.Create();

    if (shapeRes.HasError())
    {
      xiiLog::Error("Cooking Jolt triangle mesh failed: {}", shapeRes.GetError().c_str());
      return XII_FAILURE;
    }

    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         memWriter(&storage);

    xiiJoltStreamOut jOut(&memWriter);
    shapeRes.Get()->SaveBinaryState(jOut);

    OutputStream << storage.GetStorageSize32();
    storage.CopyToStream(OutputStream).AssertSuccess();

    const xiiUInt32 uiNumVertices = static_cast<xiiUInt32>(vertexList.size());
    OutputStream << uiNumVertices;

    const xiiUInt32 uiNumTriangles = shapeRes.Get()->GetStats().mNumTriangles;
    OutputStream << uiNumTriangles;
  }

  return XII_SUCCESS;
}

xiiResult xiiJoltCooking::CookConvexMesh(const xiiJoltCookingMesh& mesh0, xiiStreamWriter& OutputStream)
{
  xiiProgressRange range("Cooking Convex Mesh", 2, false);

  range.BeginNextStep("Computing Convex Hull");

  xiiJoltCookingMesh mesh;
  XII_SUCCEED_OR_RETURN(ComputeConvexHull(mesh0, mesh));

  range.BeginNextStep("Cooking Convex Hull");

  XII_SUCCEED_OR_RETURN(CookSingleConvexJoltMesh(mesh, OutputStream));

  return XII_SUCCESS;
}

XII_DEFINE_AS_POD_TYPE(JPH::Vec3);

xiiResult xiiJoltCooking::CookSingleConvexJoltMesh(const xiiJoltCookingMesh& mesh, xiiStreamWriter& OutputStream)
{
  JPH::RegisterDefaultAllocator();

  xiiHybridArray<JPH::Vec3, 256> verts;
  verts.SetCountUninitialized(mesh.m_Vertices.GetCount());

  for (xiiUInt32 i = 0; i < verts.GetCount(); ++i)
  {
    xiiVec3 v = mesh.m_Vertices[i];
    verts[i]  = JPH::Vec3(v.x, v.y, v.z);
  }

  JPH::ConvexHullShapeSettings shapeSettings(verts.GetData(), (int)verts.GetCount());

  auto shapeRes = shapeSettings.Create();

  if (shapeRes.HasError())
  {
    xiiLog::Error("Cooking convex Jolt mesh failed: {}", shapeRes.GetError().c_str());
    return XII_FAILURE;
  }

  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         memWriter(&storage);

  xiiJoltStreamOut jOut(&memWriter);
  shapeRes.Get()->SaveBinaryState(jOut);

  OutputStream << storage.GetStorageSize32();
  storage.CopyToStream(OutputStream).AssertSuccess();

  const xiiUInt32 uiNumVertices = verts.GetCount();
  OutputStream << uiNumVertices;

  const xiiUInt32 uiNumTriangles = shapeRes.Get()->GetStats().mNumTriangles;
  OutputStream << uiNumTriangles;

  return XII_SUCCESS;
}

xiiResult xiiJoltCooking::ComputeConvexHull(const xiiJoltCookingMesh& mesh, xiiJoltCookingMesh& outMesh)
{
  xiiStopwatch timer;

  outMesh.m_bFlipNormals = mesh.m_bFlipNormals;


  xiiConvexHullGenerator gen;
  if (gen.Build(mesh.m_Vertices).Failed())
  {
    xiiLog::Error("Computing the convex hull failed.");
    return XII_FAILURE;
  }

  xiiDynamicArray<xiiConvexHullGenerator::Face> faces;
  gen.Retrieve(outMesh.m_Vertices, faces);

  if (faces.GetCount() >= 255)
  {
    xiiConvexHullGenerator gen2;
    gen2.SetSimplificationMinTriangleAngle(xiiAngle::Degree(30));
    gen2.SetSimplificationFlatVertexNormalThreshold(xiiAngle::Degree(10));
    gen2.SetSimplificationMinTriangleEdgeLength(0.08f);

    if (gen2.Build(outMesh.m_Vertices).Failed())
    {
      xiiLog::Error("Computing the convex hull failed (second try).");
      return XII_FAILURE;
    }

    gen2.Retrieve(outMesh.m_Vertices, faces);
  }


  for (const auto& face : faces)
  {
    outMesh.m_VerticesInPolygon.ExpandAndGetRef() = 3;
    outMesh.m_PolygonSurfaceID.ExpandAndGetRef()  = 0;

    for (int vert = 0; vert < 3; ++vert)
      outMesh.m_PolygonIndices.ExpandAndGetRef() = face.m_uiVertexIdx[vert];
  }

  xiiLog::Dev("Computed the convex hull in {0} milliseconds", xiiArgF(timer.GetRunningTotal().GetMilliseconds(), 1));
  return XII_SUCCESS;
}

xiiStatus xiiJoltCooking::WriteResourceToStream(xiiChunkStreamWriter& stream, const xiiJoltCookingMesh& mesh, const xiiArrayPtr<xiiString>& surfaces, MeshType meshType, xiiUInt32 uiMaxConvexPieces)
{
  xiiResult resCooking = XII_FAILURE;

  {
    stream.BeginChunk("Surfaces", 1);

    stream << surfaces.GetCount();

    for (const auto& slot : surfaces)
    {
      stream << slot;
    }

    stream.EndChunk();
  }

  {
    stream.BeginChunk("Details", 1);

    xiiBoundingBoxSphere aabb;
    aabb.SetFromPoints(mesh.m_Vertices.GetData(), mesh.m_Vertices.GetCount());

    stream << aabb;

    stream.EndChunk();
  }

  if (meshType == MeshType::Triangle)
  {
    stream.BeginChunk("TriangleMesh", 1);

    xiiStopwatch timer;
    resCooking = xiiJoltCooking::CookTriangleMesh(mesh, stream);
    xiiLog::Dev("Triangle Mesh Cooking time: {0}s", xiiArgF(timer.GetRunningTotal().GetSeconds(), 2));

    stream.EndChunk();
  }
  else
  {
    if (meshType == MeshType::ConvexDecomposition)
    {
      stream.BeginChunk("ConvexDecompositionMesh", 1);

      xiiStopwatch timer;
      resCooking = xiiJoltCooking::CookDecomposedConvexMesh(mesh, stream, uiMaxConvexPieces);
      xiiLog::Dev("Decomposed Convex Mesh Cooking time: {0}s", xiiArgF(timer.GetRunningTotal().GetSeconds(), 2));

      stream.EndChunk();
    }

    if (meshType == MeshType::ConvexHull)
    {
      stream.BeginChunk("ConvexMesh", 1);

      xiiStopwatch timer;
      resCooking = xiiJoltCooking::CookConvexMesh(mesh, stream);
      xiiLog::Dev("Convex Mesh Cooking time: {0}s", xiiArgF(timer.GetRunningTotal().GetSeconds(), 2));

      stream.EndChunk();
    }
  }

  if (resCooking.Failed())
    return xiiStatus("Cooking the collision mesh failed.");


  return xiiStatus(XII_SUCCESS);
}

xiiResult xiiJoltCooking::CookDecomposedConvexMesh(const xiiJoltCookingMesh& mesh, xiiStreamWriter& OutputStream, xiiUInt32 uiMaxConvexPieces)
{
  XII_LOG_BLOCK("Decomposing Mesh");

  IVHACD*            pConDec = CreateVHACD();
  IVHACD::Parameters params;
  params.m_maxConvexHulls = xiiMath::Max(1u, uiMaxConvexPieces);

  if (uiMaxConvexPieces <= 2)
  {
    params.m_resolution = 10 * 10 * 10;
  }
  else if (uiMaxConvexPieces <= 5)
  {
    params.m_resolution = 20 * 20 * 20;
  }
  else if (uiMaxConvexPieces <= 10)
  {
    params.m_resolution = 40 * 40 * 40;
  }
  else if (uiMaxConvexPieces <= 25)
  {
    params.m_resolution = 60 * 60 * 60;
  }
  else if (uiMaxConvexPieces <= 50)
  {
    params.m_resolution = 80 * 80 * 80;
  }
  else
  {
    params.m_resolution = 100 * 100 * 100;
  }

  if (!pConDec->Compute(mesh.m_Vertices.GetData()->GetData(), mesh.m_Vertices.GetCount(), mesh.m_PolygonIndices.GetData(), mesh.m_VerticesInPolygon.GetCount(), params))
  {
    xiiLog::Error("Failed to compute convex decomposition");
    return XII_FAILURE;
  }

  xiiUInt16 uiNumParts = 0;

  for (xiiUInt32 i = 0; i < pConDec->GetNConvexHulls(); ++i)
  {
    IVHACD::ConvexHull ch;
    pConDec->GetConvexHull(i, ch);

    if (ch.m_triangles.empty())
      continue;

    ++uiNumParts;
  }

  xiiLog::Dev("Convex mesh parts: {}", uiNumParts);

  OutputStream << uiNumParts;

  for (xiiUInt32 i = 0; i < pConDec->GetNConvexHulls(); ++i)
  {
    IVHACD::ConvexHull ch;
    pConDec->GetConvexHull(i, ch);

    if (ch.m_triangles.empty())
      continue;

    xiiJoltCookingMesh chm;

    chm.m_Vertices.SetCount((xiiUInt32)ch.m_points.size());

    for (xiiUInt32 v = 0; v < (xiiUInt32)ch.m_points.size(); ++v)
    {
      chm.m_Vertices[v].Set((float)ch.m_points[v].mX, (float)ch.m_points[v].mY, (float)ch.m_points[v].mZ);
    }

    chm.m_VerticesInPolygon.SetCount((xiiUInt32)ch.m_triangles.size());
    chm.m_PolygonSurfaceID.SetCount((xiiUInt32)ch.m_triangles.size());
    chm.m_PolygonIndices.SetCount((xiiUInt32)ch.m_triangles.size() * 3);

    for (xiiUInt32 t = 0; t < (xiiUInt32)ch.m_triangles.size(); ++t)
    {
      chm.m_VerticesInPolygon[t] = 3;
      chm.m_PolygonSurfaceID[t]  = 0;

      chm.m_PolygonIndices[t * 3 + 0] = ch.m_triangles[t].mI0;
      chm.m_PolygonIndices[t * 3 + 1] = ch.m_triangles[t].mI1;
      chm.m_PolygonIndices[t * 3 + 2] = ch.m_triangles[t].mI2;
    }

    XII_SUCCEED_OR_RETURN(CookSingleConvexJoltMesh(chm, OutputStream));
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(JoltCooking, JoltCooking_JoltCooking);
