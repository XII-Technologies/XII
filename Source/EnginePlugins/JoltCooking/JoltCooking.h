#pragma once

#include <Foundation/Types/Status.h>
#include <JoltCooking/JoltCookingDLL.h>

class xiiStreamWriter;
class xiiChunkStreamWriter;

struct XII_JOLTCOOKING_DLL xiiJoltCookingMesh
{
  bool                       m_bFlipNormals = false;
  xiiDynamicArray<xiiVec3>   m_Vertices;
  xiiDynamicArray<xiiUInt8>  m_VerticesInPolygon;
  xiiDynamicArray<xiiUInt32> m_PolygonIndices;
  xiiDynamicArray<xiiUInt16> m_PolygonSurfaceID;
};

class XII_JOLTCOOKING_DLL xiiJoltCooking
{
public:
  enum class MeshType
  {
    Triangle,
    ConvexHull,
    ConvexDecomposition
  };

  static xiiResult CookTriangleMesh(const xiiJoltCookingMesh& mesh, xiiStreamWriter& OutputStream);
  static xiiResult CookConvexMesh(const xiiJoltCookingMesh& mesh, xiiStreamWriter& OutputStream);
  static xiiResult ComputeConvexHull(const xiiJoltCookingMesh& mesh, xiiJoltCookingMesh& outMesh);
  static xiiStatus WriteResourceToStream(xiiChunkStreamWriter& stream, const xiiJoltCookingMesh& mesh, const xiiArrayPtr<xiiString>& surfaces, MeshType meshType, xiiUInt32 uiMaxConvexPieces = 1);
  static xiiResult CookDecomposedConvexMesh(const xiiJoltCookingMesh& mesh, xiiStreamWriter& OutputStream, xiiUInt32 uiMaxConvexPieces);

private:
  static xiiResult CookSingleConvexJoltMesh(const xiiJoltCookingMesh& mesh, xiiStreamWriter& OutputStream);
};
