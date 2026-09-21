/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>

/// Computes convex hulls for 3D meshes.
///
/// By default it will also simplify the result to a reasonable degree,
/// to reduce complexity and vertex/triangle count.
///
/// Currently there is an upper limit of 16384 vertices to accept meshes.
/// Everything larger than that will not be processed.
class XII_CORE_DLL xiiConvexHullGenerator
{
public:
  struct Face
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt16 m_uiVertexIdx[3];
  };

  xiiConvexHullGenerator();
  ~xiiConvexHullGenerator();

  /// Used to remove degenerate and unnecessary triangles that have corners with very little angle change.
  /// Ie. specifying 10 degree, means that all triangle corners must have at least a 10 degree change (and inner angle of 170 degree).
  /// Default is 22 degree.
  void SetSimplificationMinTriangleAngle(xiiAngle angle) { m_MinTriangleAngle = angle; }

  /// Used to remove vertices that do not contribute much to the silhouette.
  /// Vertices whose adjacent triangle normals do not differ by more than angle, will be discarded.
  /// Default is 5 degree.
  void SetSimplificationFlatVertexNormalThreshold(xiiAngle angle) { m_FlatVertexNormalThreshold = angle; }

  /// The minimum triangle edge length. Every edge shorter than this will be discarded and replaced by a single vertex at the
  /// average position.
  /// \note The length is not in 'mesh space' coordinates, but instead in 'unit cube space'.
  /// That means, every mesh is scaled to fit into a cube of size [-1; +1] for each axis. Thus the exact scale of the mesh does not matter
  /// when setting this value. Default is 0.05.
  void SetSimplificationMinTriangleEdgeLength(double fLen) { m_fMinTriangleEdgeLength = fLen; }

  /// Generates the convex hull. Simplifies the mesh according to the previously specified parameters.
  xiiResult Build(const xiiArrayPtr<const xiiVec3> vertices);

  /// When Build() was successful this can be called to retrieve the resulting vertices and triangles.
  void Retrieve(xiiDynamicArray<xiiVec3>& out_vertices, xiiDynamicArray<Face>& out_faces);

  /// Same as Retrieve() but only returns the vertices.
  void RetrieveVertices(xiiDynamicArray<xiiVec3>& out_vertices);

private:
  xiiResult ComputeCenterAndScale(const xiiArrayPtr<const xiiVec3> vertices);
  xiiResult StoreNormalizedVertices(const xiiArrayPtr<const xiiVec3> vertices);
  void      StoreTriangle(xiiUInt16 i, xiiUInt16 j, xiiUInt16 k);
  xiiResult InitializeHull();
  xiiResult ComputeHull();
  bool      IsInside(xiiUInt32 vtxId) const;
  void      RemoveVisibleFaces(xiiUInt32 vtxId);
  void      PatchHole(xiiUInt32 vtxId);
  bool      PruneFlatVertices(double fNormalThreshold);
  bool      PruneDegenerateTriangles(double fMaxCosAngle);
  bool      PruneSmallTriangles(double fMaxEdgeLen);
  xiiResult ProcessVertices(const xiiArrayPtr<const xiiVec3> vertices);

  struct TwoSet
  {
    XII_ALWAYS_INLINE TwoSet()
    {
      a = 0xFFFF;
      b = 0xFFFF;
    }
    XII_ALWAYS_INLINE void Add(xiiUInt16 x) { (a == 0xFFFF ? a : b) = x; }
    XII_ALWAYS_INLINE bool Contains(xiiUInt16 x) { return a == x || b == x; }
    XII_ALWAYS_INLINE void Remove(xiiUInt16 x) { (a == x ? a : b) = 0xFFFF; }
    XII_ALWAYS_INLINE int  GetSize() { return (a != 0xFFFF) + (b != 0xFFFF); }

    xiiUInt16 a, b;
  };

  struct Triangle
  {
    xiiVec3d  m_vNormal;
    double    m_fPlaneDistance;
    xiiUInt16 m_uiVertexIdx[3];
    bool      m_bFlip;
    bool      m_bIsDegenerate;
  };

  // used for mesh simplification
  xiiAngle m_MinTriangleAngle          = xiiAngle::MakeFromDegree(22.0f);
  xiiAngle m_FlatVertexNormalThreshold = xiiAngle::MakeFromDegree(5);
  double   m_fMinTriangleEdgeLength    = 0.05;

  xiiVec3d m_vCenter;
  double   m_fScale;

  xiiVec3d m_vInside;

  // all the 'good' vertices (no duplicates)
  // normalized to be within a unit-cube
  xiiDynamicArray<xiiVec3d> m_Vertices;

  // Will be resized to Square(m_Vertices.GetCount())
  // Index [i * m_Vertices.GetCount() + j] indicates which (up to two) other points
  // combine with the edge i and j to make a triangle in the hull.  Only defined when i < j.
  xiiDynamicArray<TwoSet> m_Edges;

  xiiDeque<Triangle> m_Triangles;
};
