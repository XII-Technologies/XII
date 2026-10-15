/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoGeometry.h>
#include <Foundation/Math/Intersection.h>
#include <Utilities/FileFormats/OBJLoader.h>

namespace
{
  xiiResult LoadMesh(xiiStringView sFile, xiiGeometry& out_geometry)
  {
    if (sFile.IsEmpty())
      return XII_FAILURE;
    xiiStringBuilder path(sFile);
    xiiOBJLoader     loader;
    XII_SUCCEED_OR_RETURN(loader.LoadOBJ(path.GetData(), true));
    for (const auto& position : loader.m_Positions)
      out_geometry.AddVertex(position, xiiVec3::MakeAxisZ());
    for (const auto& face : loader.m_Faces)
    {
      xiiHybridArray<xiiUInt32, 4> vertices;
      for (const auto& vertex : face.m_Vertices)
      {
        if (vertex.m_uiPositionID >= loader.m_Positions.GetCount())
          return XII_FAILURE;
        vertices.PushBack(vertex.m_uiPositionID);
      }
      out_geometry.AddPolygon(vertices, false);
    }
    return XII_SUCCESS;
  }

  // Minimize squared distance between a ray (t >= 0) and segment (0 <= u <= 1).
  bool PickLine(const xiiVec3& start, const xiiVec3& direction, const xiiVec3& a, const xiiVec3& b, float fRadius, float& out_fTime)
  {
    const xiiVec3d d(direction.x, direction.y, direction.z);
    const xiiVec3d v(b.x - a.x, b.y - a.y, b.z - a.z);
    const xiiVec3d w(start.x - a.x, start.y - a.y, start.z - a.z);
    const double   dd          = d.Dot(d);
    const double   vv          = v.Dot(v);
    const double   dv          = d.Dot(v);
    const double   dw          = d.Dot(w);
    const double   vw          = v.Dot(w);
    const double   determinant = dd * vv - dv * dv;
    double         u           = determinant > dd * vv * 1.0e-12 ? xiiMath::Clamp((dd * vw - dv * dw) / determinant, 0.0, 1.0) :
                                                                   (vv > 0.0 ? xiiMath::Clamp(vw / vv, 0.0, 1.0) : 0.0);
    double         t           = (dv * u - dw) / dd;
    if (t < 0.0)
    {
      t = 0.0;
      u = vv > 0.0 ? xiiMath::Clamp(vw / vv, 0.0, 1.0) : 0.0;
    }
    if ((w + d * t - v * u).GetLengthSquared() > static_cast<double>(fRadius) * fRadius)
      return false;
    out_fTime = static_cast<float>(t);
    return xiiMath::IsFinite(out_fTime);
  }
} // namespace

xiiResult xiiGizmoGeometry::Build(xiiEngineGizmoHandleType type, xiiStringView sMeshFile)
{
  m_Geometry.Clear();
  m_Bounds = xiiBoundingBox::MakeInvalid();
  xiiGeometry::GeoOptions options;
  switch (type)
  {
    case xiiEngineGizmoHandleType::Arrow:
      options.m_MainAxis = xiiBasisAxis::PositiveX;
      m_Geometry.AddCylinderOnePiece(0.02f, 0.02f, 0.5f, 0.5f, 16, options);
      options.m_Transform = xiiMat4::MakeTranslation(xiiVec3(0.5f, 0, 0));
      m_Geometry.AddCone(0.06f, 0.12f, true, 16, options);
      break;
    case xiiEngineGizmoHandleType::Box:
      m_Geometry.AddBox(xiiVec3(1), false);
      break;
    case xiiEngineGizmoHandleType::Sphere:
      m_Geometry.AddStackedSphere(1, 24, 12);
      break;
    case xiiEngineGizmoHandleType::CylinderZ:
      m_Geometry.AddCylinderOnePiece(1, 1, 0.5f, 0.5f, 24);
      break;
    case xiiEngineGizmoHandleType::HalfSphereZ:
      m_Geometry.AddHalfSphere(1, 24, 6, true);
      break;
    case xiiEngineGizmoHandleType::BoxCorners:
      for (xiiUInt32 axis = 0; axis < 6; ++axis)
      {
        const xiiMat4 rotation = axis < 4 ? xiiMat4::MakeRotationX(xiiAngle::MakeFromDegree(90.0f * axis)) :
                                            xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(axis == 4 ? 90.0f : -90.0f));
        for (float x : {-0.375f, 0.375f})
          for (float y : {-0.375f, 0.375f})
          {
            options.m_Transform = rotation * xiiMat4::MakeTranslation(xiiVec3(x, y, 0.5f));
            m_Geometry.AddRect(xiiVec2(0.25f), 1, 1, options);
          }
      }
      break;
    case xiiEngineGizmoHandleType::BoxEdges:
      for (xiiUInt32 i = 0; i < 4; ++i)
      {
        const xiiMat4 rotation = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(90.0f * i));
        for (float x : {-0.375f, 0.375f})
        {
          options.m_Transform = rotation * xiiMat4::MakeTranslation(xiiVec3(x, 0, 0.5f));
          m_Geometry.AddRect(xiiVec2(0.25f, 0.5f), 1, 1, options);
        }
      }
      break;
    case xiiEngineGizmoHandleType::BoxFaces:
      options.m_Transform = xiiMat4::MakeTranslation(xiiVec3(0, 0, 0.5f));
      m_Geometry.AddRect(xiiVec2(0.5f), 1, 1, options);
      options.m_Transform = xiiMat4::MakeTranslation(xiiVec3(0, 0, -0.5f));
      m_Geometry.AddRect(xiiVec2(0.5f), 1, 1, options);
      break;
    case xiiEngineGizmoHandleType::LineBox:
      m_Geometry.AddLineBox(xiiVec3(1));
      break;
    case xiiEngineGizmoHandleType::LineRect:
      for (const auto& p : {xiiVec3(-1, -1, 0), xiiVec3(1, -1, 0), xiiVec3(1, 1, 0), xiiVec3(-1, 1, 0)})
        m_Geometry.AddVertex(p, xiiVec3::MakeAxisZ());
      for (xiiUInt32 i = 0; i < 4; ++i)
        m_Geometry.AddLine(i, (i + 1) % 4);
      break;
    case xiiEngineGizmoHandleType::Cone:
      options.m_MainAxis  = xiiBasisAxis::NegativeX;
      options.m_Transform = xiiMat4::MakeTranslation(xiiVec3(1, 0, 0));
      m_Geometry.AddCone(1, 1, false, 24, options);
      break;
    case xiiEngineGizmoHandleType::Frustum:
      m_Geometry.AddVertex(xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
      for (float y : {-1.0f, 1.0f})
        for (float z : {-1.0f, 1.0f})
          m_Geometry.AddLine(0, m_Geometry.AddVertex(xiiVec3(1, y, z), xiiVec3::MakeAxisZ()));
      break;
    case xiiEngineGizmoHandleType::FromFile:
      if (LoadMesh(sMeshFile, m_Geometry).Failed())
      {
        m_Geometry.Clear();
        return XII_FAILURE;
      }
      break;
    default:
      return XII_FAILURE;
  }

  m_Geometry.TriangulatePolygons();
  if (m_Geometry.GetLines().IsEmpty() && m_Geometry.GetPolygons().IsEmpty())
  {
    m_Geometry.Clear();
    return XII_FAILURE;
  }
  for (const auto& vertex : m_Geometry.GetVertices())
  {
    if (!vertex.m_vPosition.IsValid())
    {
      m_Geometry.Clear();
      m_Bounds = xiiBoundingBox::MakeInvalid();
      return XII_FAILURE;
    }
    m_Bounds.ExpandToInclude(vertex.m_vPosition);
  }
  return XII_SUCCESS;
}

xiiResult xiiGizmoGeometry::PickRay(const xiiMat4& transform, const xiiVec3& vRayStart, const xiiVec3& vRayDirection, float fLineRadius, float& out_fRayTime) const
{
  if (!m_Bounds.IsValid() || !transform.IsValid() || !vRayStart.IsValid() || !vRayDirection.IsValid() || !xiiMath::IsFinite(fLineRadius) || fLineRadius < 0 || vRayDirection.IsZero(0.0f))
    return XII_FAILURE;
  xiiMat4 inverse = transform;
  // Screen-sized nobs can have very small, perfectly invertible scales. Reject singularity rather than
  // comparing the dimensional determinant against a world-scale epsilon; validate the inverse instead.
  if (inverse.Invert(0.0f).Failed() || !inverse.IsValid())
    return XII_FAILURE;
  const xiiVec3 localStart     = inverse.TransformPosition(vRayStart);
  const xiiVec3 localDirection = inverse.TransformDirection(vRayDirection);
  if (!localStart.IsValid() || !localDirection.IsValid())
    return XII_FAILURE;
  if (m_Geometry.GetLines().IsEmpty() && !m_Bounds.Contains(localStart) && !m_Bounds.GetRayIntersection(localStart, localDirection))
    return XII_FAILURE;
  float       fClosest = xiiMath::Infinity<float>();
  const auto& vertices = m_Geometry.GetVertices();
  for (const auto& polygon : m_Geometry.GetPolygons())
  {
    float fTime;
    if (xiiIntersectionUtils::RayTriangleIntersection(localStart, localDirection, vertices[polygon.m_Vertices[0]].m_vPosition, vertices[polygon.m_Vertices[1]].m_vPosition, vertices[polygon.m_Vertices[2]].m_vPosition, &fTime) && fTime >= 0.0f)
      fClosest = xiiMath::Min(fClosest, fTime);
  }
  for (const auto& line : m_Geometry.GetLines())
  {
    float fTime;
    if (PickLine(vRayStart, vRayDirection, transform.TransformPosition(vertices[line.m_uiStartVertex].m_vPosition), transform.TransformPosition(vertices[line.m_uiEndVertex].m_vPosition), fLineRadius, fTime))
      fClosest = xiiMath::Min(fClosest, fTime);
  }
  if (!xiiMath::IsFinite(fClosest))
    return XII_FAILURE;
  out_fRayTime = fClosest;
  return XII_SUCCESS;
}
