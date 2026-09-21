/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Vec3.h>

namespace xiiIntersectionUtils
{
  /// Checks whether a ray intersects with a triangle.
  ///
  /// The vertex winding order does not matter, triangles will be hit from both sides.
  ///
  /// \param vRayStartPos
  ///   The start position of the ray.
  /// \param vRayDir
  ///   The direction of the ray. This does not need to be normalized. Depending on its length, out_fIntersectionTime will be scaled differently.
  /// \param vVertex0, vVertex1, vVertex2
  ///   The three vertices forming the triangle.
  /// \param out_fIntersectionTime
  ///   The 'time' at which the ray intersects the triangle. If \a vRayDir is normalized, this is the exact distance.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param out_fIntersectionPoint
  ///   The point where the ray intersects the triangle.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  ///
  /// \return
  ///   True, if the ray intersects the triangle, false otherwise.
  XII_FOUNDATION_DLL bool RayTriangleIntersection(const xiiVec3& vRayStartPos, const xiiVec3& vRayDir, const xiiVec3& vVertex0, const xiiVec3& vVertex1, const xiiVec3& vVertex2, float* out_pIntersectionTime = nullptr, xiiVec3* out_pIntersectionPoint = nullptr); // [tested]

  /// Checks whether a ray intersects with a triangle.
  ///
  /// The vertex winding order does not matter, triangles will be hit from both sides.
  ///
  /// \param vRayStartPos
  ///   The start position of the ray.
  /// \param vRayDir
  ///   The direction of the ray. This does not need to be normalized. Depending on its length, out_fIntersectionTime will be scaled differently.
  /// \param vVertex0, vVertex1, vVertex2
  ///   The three vertices forming the triangle.
  /// \param out_fIntersectionTime
  ///   The 'time' at which the ray intersects the triangle. If \a vRayDir is normalized, this is the exact distance.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param out_fIntersectionPoint
  ///   The point where the ray intersects the triangle.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  ///
  /// \return
  ///   True, if the ray intersects the triangle, false otherwise.
  XII_FOUNDATION_DLL bool RayTriangleIntersection(const xiiVec3d& vRayStartPos, const xiiVec3d& vRayDir, const xiiVec3d& vVertex0, const xiiVec3d& vVertex1, const xiiVec3d& vVertex2, double* out_pIntersectionTime = nullptr, xiiVec3d* out_pIntersectionPoint = nullptr); // [tested]

  /// Checks whether a ray intersects with a polygon.
  ///
  /// The vertex winding order does not matter, polygons will be hit from both sides.
  ///
  /// \param vRayStartPos
  ///   The start position of the ray.
  /// \param vRayDir
  ///   The direction of the ray. This does not need to be normalized. Depending on its length, out_fIntersectionTime will be scaled differently.
  /// \param pPolygonVertices
  ///   Pointer to the first vertex of the polygon.
  /// \param uiNumVertices
  ///   The number of vertices in the polygon.
  /// \param out_fIntersectionTime
  ///   The 'time' at which the ray intersects the polygon. If \a vRayDir is normalized, this is the exact distance.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param out_fIntersectionPoint
  ///   The point where the ray intersects the polygon.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param uiVertexStride
  ///   The stride in bytes between each vertex in the pPolygonVertices array. If the array is tightly packed, this will equal sizeof(xiiVec3), but it
  ///   can be larger, if the vertices are interleaved with other data.
  /// \return
  ///   True, if the ray intersects the polygon, false otherwise.
  XII_FOUNDATION_DLL bool RayPolygonIntersection(const xiiVec3& vRayStartPos, const xiiVec3& vRayDir, const xiiVec3* pPolygonVertices, xiiUInt32 uiNumVertices, float* out_pIntersectionTime = nullptr, xiiVec3* out_pIntersectionPoint = nullptr, xiiUInt32 uiVertexStride = sizeof(xiiVec3)); // [tested]

  /// Checks whether a ray intersects with a polygon.
  ///
  /// The vertex winding order does not matter, polygons will be hit from both sides.
  ///
  /// \param vRayStartPos
  ///   The start position of the ray.
  /// \param vRayDir
  ///   The direction of the ray. This does not need to be normalized. Depending on its length, out_fIntersectionTime will be scaled differently.
  /// \param pPolygonVertices
  ///   Pointer to the first vertex of the polygon.
  /// \param uiNumVertices
  ///   The number of vertices in the polygon.
  /// \param out_fIntersectionTime
  ///   The 'time' at which the ray intersects the polygon. If \a vRayDir is normalized, this is the exact distance.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param out_fIntersectionPoint
  ///   The point where the ray intersects the polygon.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param uiVertexStride
  ///   The stride in bytes between each vertex in the pPolygonVertices array. If the array is tightly packed, this will equal sizeof(xiiVec3d), but it
  ///   can be larger, if the vertices are interleaved with other data.
  /// \return
  ///   True, if the ray intersects the polygon, false otherwise.
  XII_FOUNDATION_DLL bool RayPolygonIntersection(const xiiVec3d& vRayStartPos, const xiiVec3d& vRayDir, const xiiVec3d* pPolygonVertices, xiiUInt32 uiNumVertices, double* out_pIntersectionTime = nullptr, xiiVec3d* out_pIntersectionPoint = nullptr, xiiUInt32 uiVertexStride = sizeof(xiiVec3d)); // [untested]


  /// Returns point on the line segment that is closest to \a vStartPoint. Optionally also returns the fraction along the segment, where that
  /// point is located.
  XII_FOUNDATION_DLL xiiVec3 ClosestPoint_PointLineSegment(const xiiVec3& vStartPoint, const xiiVec3& vLineSegmentPos0, const xiiVec3& vLineSegmentPos1, float* out_pFractionAlongSegment = nullptr); // [tested]

  /// Returns point on the line segment that is closest to \a vStartPoint. Optionally also returns the fraction along the segment, where that
  /// point is located.
  XII_FOUNDATION_DLL xiiVec3d ClosestPoint_PointLineSegment(const xiiVec3d& vStartPoint, const xiiVec3d& vLineSegmentPos0, const xiiVec3d& vLineSegmentPos1, double* out_pFractionAlongSegment = nullptr); // [untested]

  /// Computes the intersection point and time of the 2D ray with the 2D line segment. Returns true, if there is an intersection.
  XII_FOUNDATION_DLL bool Ray2DLine2D(const xiiVec2& vRayStartPos, const xiiVec2& vRayDir, const xiiVec2& vLineSegmentPos0, const xiiVec2& vLineSegmentPos1, float* out_pIntersectionTime = nullptr, xiiVec2* out_pIntersectionPoint = nullptr); // [tested]

  /// Computes the intersection point and time of the 2D ray with the 2D line segment. Returns true, if there is an intersection.
  XII_FOUNDATION_DLL bool Ray2DLine2D(const xiiVec2d& vRayStartPos, const xiiVec2d& vRayDir, const xiiVec2d& vLineSegmentPos0, const xiiVec2d& vLineSegmentPos1, double* out_pIntersectionTime = nullptr, xiiVec2d* out_pIntersectionPoint = nullptr); // [tested]

  /// Tests whether a point is located on a line
  XII_FOUNDATION_DLL bool IsPointOnLine(const xiiVec3& vLineStart, const xiiVec3& vLineEnd, const xiiVec3& vPoint, float fMaxDist = 0.01f);

  /// Tests whether a point is located on a line
  XII_FOUNDATION_DLL bool IsPointOnLine(const xiiVec3d& vLineStart, const xiiVec3d& vLineEnd, const xiiVec3d& vPoint, double fMaxDist = 0.001);

} // namespace xiiIntersectionUtils
