#include <Core/CorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Quat.h>
#include <mikktspace/mikktspace.h>

bool xiiGeometry::GeoOptions::IsFlipWindingNecessary() const
{
  return m_Transform.GetRotationalPart().GetDeterminant() < 0;
}

bool xiiGeometry::Vertex::operator<(const xiiGeometry::Vertex& rhs) const
{
  if (m_vPosition != rhs.m_vPosition)
    return m_vPosition < rhs.m_vPosition;

  if (m_vNormal != rhs.m_vNormal)
    return m_vNormal < rhs.m_vNormal;

  if (m_vTangent != rhs.m_vTangent)
    return m_vTangent < rhs.m_vTangent;

  if (m_fBiTangentSign != rhs.m_fBiTangentSign)
    return m_fBiTangentSign < rhs.m_fBiTangentSign;

  if (m_vTexCoord != rhs.m_vTexCoord)
    return m_vTexCoord < rhs.m_vTexCoord;

  if (m_Color != rhs.m_Color)
    return m_Color < rhs.m_Color;

  if (m_BoneIndices != rhs.m_BoneIndices)
    return m_BoneIndices < rhs.m_BoneIndices;

  return m_BoneWeights < rhs.m_BoneWeights;
}

bool xiiGeometry::Vertex::operator==(const xiiGeometry::Vertex& rhs) const
{
  return m_vPosition == rhs.m_vPosition &&
    m_vNormal == rhs.m_vNormal &&
    m_vTangent == rhs.m_vTangent &&
    m_fBiTangentSign == rhs.m_fBiTangentSign &&
    m_vTexCoord == rhs.m_vTexCoord &&
    m_Color == rhs.m_Color &&
    m_BoneIndices == rhs.m_BoneIndices &&
    m_BoneWeights == rhs.m_BoneWeights;
}

void xiiGeometry::Polygon::FlipWinding()
{
  const xiiUInt32 uiCount     = m_Vertices.GetCount();
  const xiiUInt32 uiHalfCount = uiCount / 2;
  for (xiiUInt32 i = 0; i < uiHalfCount; i++)
  {
    xiiMath::Swap(m_Vertices[i], m_Vertices[uiCount - i - 1]);
  }
}

void xiiGeometry::Clear()
{
  m_Vertices.Clear();
  m_Polygons.Clear();
  m_Lines.Clear();
}

xiiUInt32 xiiGeometry::AddVertex(const xiiVec3& vPos, const xiiVec3& vNormal, const xiiVec2& vTexCoord, const xiiColor& color, const xiiVec4U16& vBoneIndices /*= xiiVec4U16::MakeZero()*/, const xiiColorLinearUB& boneWeights /*= xiiColorLinearUB(255, 0, 0, 0)*/)
{
  Vertex& v       = m_Vertices.ExpandAndGetRef();
  v.m_vPosition   = vPos;
  v.m_vNormal     = vNormal;
  v.m_vTexCoord   = vTexCoord;
  v.m_Color       = color;
  v.m_BoneIndices = vBoneIndices;
  v.m_BoneWeights = boneWeights;

  return m_Vertices.GetCount() - 1;
}

void xiiGeometry::AddPolygon(const xiiArrayPtr<xiiUInt32>& vertices, bool bFlipWinding)
{
  XII_ASSERT_DEV(vertices.GetCount() >= 3, "Polygon must have at least 3 vertices, not {0}", vertices.GetCount());

  for (xiiUInt32 v = 0; v < vertices.GetCount(); ++v)
  {
    XII_ASSERT_DEV(vertices[v] < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", vertices[v], m_Vertices.GetCount());
  }

  m_Polygons.ExpandAndGetRef().m_Vertices = vertices;

  if (bFlipWinding)
  {
    m_Polygons.PeekBack().FlipWinding();
  }
}

void xiiGeometry::AddLine(xiiUInt32 uiStartVertex, xiiUInt32 uiEndVertex)
{
  XII_ASSERT_DEV(uiStartVertex < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", uiStartVertex, m_Vertices.GetCount());
  XII_ASSERT_DEV(uiEndVertex < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", uiEndVertex, m_Vertices.GetCount());

  Line l;
  l.m_uiStartVertex = uiStartVertex;
  l.m_uiEndVertex   = uiEndVertex;

  m_Lines.PushBack(l);
}


void xiiGeometry::TriangulatePolygons(xiiUInt32 uiMaxVerticesInPolygon /*= 3*/)
{
  XII_ASSERT_DEV(uiMaxVerticesInPolygon >= 3, "Can't triangulate polygons that are already triangles.");
  uiMaxVerticesInPolygon = xiiMath::Max<xiiUInt32>(uiMaxVerticesInPolygon, 3);

  const xiiUInt32 uiNumPolys = m_Polygons.GetCount();

  for (xiiUInt32 p = 0; p < uiNumPolys; ++p)
  {
    const auto& poly = m_Polygons[p];

    const xiiUInt32 uiNumVerts = poly.m_Vertices.GetCount();
    if (uiNumVerts > uiMaxVerticesInPolygon)
    {
      for (xiiUInt32 v = 2; v < uiNumVerts; ++v)
      {
        auto& tri     = m_Polygons.ExpandAndGetRef();
        tri.m_vNormal = poly.m_vNormal;
        tri.m_Vertices.SetCountUninitialized(3);
        tri.m_Vertices[0] = poly.m_Vertices[0];
        tri.m_Vertices[1] = poly.m_Vertices[v - 1];
        tri.m_Vertices[2] = poly.m_Vertices[v];
      }

      m_Polygons.RemoveAtAndSwap(p);
    }
  }
}

void xiiGeometry::ComputeFaceNormals()
{
  for (xiiUInt32 p = 0; p < m_Polygons.GetCount(); ++p)
  {
    Polygon& poly = m_Polygons[p];

    const xiiVec3& v1 = m_Vertices[poly.m_Vertices[0]].m_vPosition;
    const xiiVec3& v2 = m_Vertices[poly.m_Vertices[1]].m_vPosition;
    const xiiVec3& v3 = m_Vertices[poly.m_Vertices[2]].m_vPosition;

    poly.m_vNormal.CalculateNormal(v1, v2, v3).IgnoreResult();
  }
}

void xiiGeometry::ComputeSmoothVertexNormals()
{
  // reset all vertex normals
  for (xiiUInt32 v = 0; v < m_Vertices.GetCount(); ++v)
  {
    m_Vertices[v].m_vNormal.SetZero();
  }

  // add face normal of all adjacent faces to each vertex
  for (xiiUInt32 p = 0; p < m_Polygons.GetCount(); ++p)
  {
    Polygon& poly = m_Polygons[p];

    for (xiiUInt32 v = 0; v < poly.m_Vertices.GetCount(); ++v)
    {
      m_Vertices[poly.m_Vertices[v]].m_vNormal += poly.m_vNormal;
    }
  }

  // normalize all vertex normals
  for (xiiUInt32 v = 0; v < m_Vertices.GetCount(); ++v)
  {
    m_Vertices[v].m_vNormal.NormalizeIfNotZero(xiiVec3(0, 1, 0)).IgnoreResult();
  }
}

struct TangentContext
{
  TangentContext(xiiGeometry* pGeom) :
    m_pGeom(pGeom)
  {
    m_Polygons = m_pGeom->GetPolygons();
  }

  static int getNumFaces(const SMikkTSpaceContext* pContext)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    return context.m_pGeom->GetPolygons().GetCount();
  }
  static int getNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    return context.m_pGeom->GetPolygons()[iFace].m_Vertices.GetCount();
  }
  static void getPosition(const SMikkTSpaceContext* pContext, float pPosOut[], const int iFace, const int iVert)
  {
    TangentContext& context      = *static_cast<TangentContext*>(pContext->m_pUserData);
    xiiUInt32       iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    const xiiVec3&  pos          = context.m_pGeom->GetVertices()[iVertexIndex].m_vPosition;
    pPosOut[0]                   = pos.x;
    pPosOut[1]                   = pos.y;
    pPosOut[2]                   = pos.z;
  }
  static void getNormal(const SMikkTSpaceContext* pContext, float pNormOut[], const int iFace, const int iVert)
  {
    TangentContext& context      = *static_cast<TangentContext*>(pContext->m_pUserData);
    xiiUInt32       iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    const xiiVec3&  normal       = context.m_pGeom->GetVertices()[iVertexIndex].m_vNormal;
    pNormOut[0]                  = normal.x;
    pNormOut[1]                  = normal.y;
    pNormOut[2]                  = normal.z;
  }
  static void getTexCoord(const SMikkTSpaceContext* pContext, float pTexcOut[], const int iFace, const int iVert)
  {
    TangentContext& context      = *static_cast<TangentContext*>(pContext->m_pUserData);
    xiiUInt32       iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    const xiiVec2&  tex          = context.m_pGeom->GetVertices()[iVertexIndex].m_vTexCoord;
    pTexcOut[0]                  = tex.x;
    pTexcOut[1]                  = tex.y;
  }
  static void setTSpaceBasic(const SMikkTSpaceContext* pContext, const float pTangent[], const float fSign, const int iFace, const int iVert)
  {
    TangentContext&     context      = *static_cast<TangentContext*>(pContext->m_pUserData);
    xiiUInt32           iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    xiiGeometry::Vertex v            = context.m_pGeom->GetVertices()[iVertexIndex];
    v.m_vTangent.x                   = pTangent[0];
    v.m_vTangent.y                   = pTangent[1];
    v.m_vTangent.z                   = pTangent[2];
    v.m_fBiTangentSign               = fSign;

    bool existed = false;
    auto it      = context.m_VertMap.FindOrAdd(v, &existed);
    if (!existed)
    {
      it.Value() = context.m_Vertices.GetCount();
      context.m_Vertices.PushBack(v);
    }
    xiiUInt32 iNewVertexIndex                   = it.Value();
    context.m_Polygons[iFace].m_Vertices[iVert] = iNewVertexIndex;
  }

  static void setTSpace(const SMikkTSpaceContext* pContext, const float pTangent[], const float pBiTangent[], const float fMagS, const float fMagT, const tbool isOrientationPreserving, const int iFace, const int iVert)
  {
    int i = 0;
    (void)i;
  }

  xiiGeometry*                           m_pGeom;
  xiiMap<xiiGeometry::Vertex, xiiUInt32> m_VertMap;
  xiiDeque<xiiGeometry::Vertex>          m_Vertices;
  xiiDeque<xiiGeometry::Polygon>         m_Polygons;
};

void xiiGeometry::ComputeTangents()
{
  for (xiiUInt32 i = 0; i < m_Polygons.GetCount(); ++i)
  {
    if (m_Polygons[i].m_Vertices.GetCount() > 4)
    {
      xiiLog::Error("Tangent generation does not support polygons with more than 4 vertices");
      break;
    }
  }

  SMikkTSpaceInterface sMikkTInterface;
  sMikkTInterface.m_getNumFaces          = &TangentContext::getNumFaces;
  sMikkTInterface.m_getNumVerticesOfFace = &TangentContext::getNumVerticesOfFace;
  sMikkTInterface.m_getPosition          = &TangentContext::getPosition;
  sMikkTInterface.m_getNormal            = &TangentContext::getNormal;
  sMikkTInterface.m_getTexCoord          = &TangentContext::getTexCoord;
  sMikkTInterface.m_setTSpaceBasic       = &TangentContext::setTSpaceBasic;
  sMikkTInterface.m_setTSpace            = &TangentContext::setTSpace;
  TangentContext context(this);

  SMikkTSpaceContext sMikkTContext;
  sMikkTContext.m_pInterface = &sMikkTInterface;
  sMikkTContext.m_pUserData  = &context;

  genTangSpaceDefault(&sMikkTContext);
  m_Polygons = std::move(context.m_Polygons);
  m_Vertices = std::move(context.m_Vertices);
}

void xiiGeometry::ValidateTangents(float fEpsilon)
{
  for (auto& vertex : m_Vertices)
  {
    // checking for orthogonality to the normal and for squared unit length (standard case) or 3 (magic number for binormal inversion)
    if (!xiiMath::IsEqual(vertex.m_vNormal.GetLengthSquared(), 1.f, fEpsilon) || !xiiMath::IsEqual(vertex.m_vNormal.Dot(vertex.m_vTangent), 0.f, fEpsilon) || !(xiiMath::IsEqual(vertex.m_vTangent.GetLengthSquared(), 1.f, fEpsilon) || xiiMath::IsEqual(vertex.m_vTangent.GetLengthSquared(), 3.f, fEpsilon)))
    {
      vertex.m_vTangent.SetZero();
    }
  }
}

xiiUInt32 xiiGeometry::CalculateTriangleCount() const
{
  const xiiUInt32 numPolys = m_Polygons.GetCount();
  xiiUInt32       numTris  = 0;

  for (xiiUInt32 p = 0; p < numPolys; ++p)
  {
    numTris += m_Polygons[p].m_Vertices.GetCount() - 2;
  }

  return numTris;
}

void xiiGeometry::SetAllVertexBoneIndices(const xiiVec4U16& vBoneIndices, xiiUInt32 uiFirstVertex)
{
  for (xiiUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_BoneIndices = vBoneIndices;
}

void xiiGeometry::SetAllVertexColor(const xiiColor& color, xiiUInt32 uiFirstVertex)
{
  for (xiiUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_Color = color;
}


void xiiGeometry::SetAllVertexTexCoord(const xiiVec2& vTexCoord, xiiUInt32 uiFirstVertex /*= 0*/)
{
  for (xiiUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_vTexCoord = vTexCoord;
}

void xiiGeometry::TransformVertices(const xiiMat4& mTransform, xiiUInt32 uiFirstVertex)
{
  if (mTransform.IsIdentity(xiiMath::SmallEpsilon<float>()))
    return;

  for (xiiUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
  {
    m_Vertices[v].m_vPosition = mTransform.TransformPosition(m_Vertices[v].m_vPosition);
    m_Vertices[v].m_vNormal   = mTransform.TransformDirection(m_Vertices[v].m_vNormal);
  }
}

void xiiGeometry::Transform(const xiiMat4& mTransform, bool bTransformPolyNormals)
{
  TransformVertices(mTransform, 0);

  if (bTransformPolyNormals)
  {
    for (xiiUInt32 p = 0; p < m_Polygons.GetCount(); ++p)
    {
      m_Polygons[p].m_vNormal = mTransform.TransformDirection(m_Polygons[p].m_vNormal);
    }
  }
}

void xiiGeometry::Merge(const xiiGeometry& other)
{
  const xiiUInt32 uiVertexOffset = m_Vertices.GetCount();

  for (xiiUInt32 v = 0; v < other.m_Vertices.GetCount(); ++v)
  {
    m_Vertices.PushBack(other.m_Vertices[v]);
  }

  for (xiiUInt32 p = 0; p < other.m_Polygons.GetCount(); ++p)
  {
    m_Polygons.PushBack(other.m_Polygons[p]);
    Polygon& poly = m_Polygons.PeekBack();

    for (xiiUInt32 pv = 0; pv < poly.m_Vertices.GetCount(); ++pv)
    {
      poly.m_Vertices[pv] += uiVertexOffset;
    }
  }

  for (xiiUInt32 l = 0; l < other.m_Lines.GetCount(); ++l)
  {
    Line line;
    line.m_uiStartVertex = other.m_Lines[l].m_uiStartVertex + uiVertexOffset;
    line.m_uiEndVertex   = other.m_Lines[l].m_uiEndVertex + uiVertexOffset;

    m_Lines.PushBack(line);
  }
}

void xiiGeometry::AddRectXY(const xiiVec2& vSize, xiiUInt32 uiTesselationX, xiiUInt32 uiTesselationY, const GeoOptions& options)
{
  if (uiTesselationX == 0)
    uiTesselationX = 1;
  if (uiTesselationY == 0)
    uiTesselationY = 1;

  const xiiVec2 halfSize     = vSize * 0.5f;
  const bool    bFlipWinding = options.IsFlipWindingNecessary();

  const xiiVec2 sizeFraction = vSize.CompDiv(xiiVec2(static_cast<float>(uiTesselationX), static_cast<float>(uiTesselationY)));

  for (xiiUInt32 vy = 0; vy < uiTesselationY + 1; ++vy)
  {
    for (xiiUInt32 vx = 0; vx < uiTesselationX + 1; ++vx)
    {
      const xiiVec2 tc((float)vx / (float)uiTesselationX, (float)vy / (float)uiTesselationY);

      AddVertex(xiiVec3(-halfSize.x + vx * sizeFraction.x, -halfSize.y + vy * sizeFraction.y, 0), xiiVec3(0, 0, 1), tc, options);
    }
  }

  xiiUInt32 idx[4];

  xiiUInt32 uiFirstIndex = 0;

  for (xiiUInt32 vy = 0; vy < uiTesselationY; ++vy)
  {
    for (xiiUInt32 vx = 0; vx < uiTesselationX; ++vx)
    {

      idx[0] = uiFirstIndex;
      idx[1] = uiFirstIndex + 1;
      idx[2] = uiFirstIndex + uiTesselationX + 2;
      idx[3] = uiFirstIndex + uiTesselationX + 1;

      AddPolygon(idx, bFlipWinding);

      ++uiFirstIndex;
    }

    ++uiFirstIndex;
  }
}

void xiiGeometry::AddBox(const xiiVec3& vFullExtents, bool bExtraVerticesForTexturing, const GeoOptions& options)
{
  const xiiVec3 halfSize     = vFullExtents * 0.5f;
  const bool    bFlipWinding = options.IsFlipWindingNecessary();

  if (bExtraVerticesForTexturing)
  {
    xiiUInt32 idx[4];

    {
      idx[0] = AddVertex(xiiVec3(-halfSize.x, -halfSize.y, +halfSize.z), xiiVec3(0, 0, 1), xiiVec2(0, 1), options);
      idx[1] = AddVertex(xiiVec3(+halfSize.x, -halfSize.y, +halfSize.z), xiiVec3(0, 0, 1), xiiVec2(0, 0), options);
      idx[2] = AddVertex(xiiVec3(+halfSize.x, +halfSize.y, +halfSize.z), xiiVec3(0, 0, 1), xiiVec2(1, 0), options);
      idx[3] = AddVertex(xiiVec3(-halfSize.x, +halfSize.y, +halfSize.z), xiiVec3(0, 0, 1), xiiVec2(1, 1), options);
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(xiiVec3(-halfSize.x, +halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(1, 0), options);
      idx[1] = AddVertex(xiiVec3(+halfSize.x, +halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(1, 1), options);
      idx[2] = AddVertex(xiiVec3(+halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0, 1), options);
      idx[3] = AddVertex(xiiVec3(-halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0, 0), options);
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(xiiVec3(-halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(-1, 0, 0), xiiVec2(0, 1), options);
      idx[1] = AddVertex(xiiVec3(-halfSize.x, -halfSize.y, +halfSize.z), xiiVec3(-1, 0, 0), xiiVec2(0, 0), options);
      idx[2] = AddVertex(xiiVec3(-halfSize.x, +halfSize.y, +halfSize.z), xiiVec3(-1, 0, 0), xiiVec2(1, 0), options);
      idx[3] = AddVertex(xiiVec3(-halfSize.x, +halfSize.y, -halfSize.z), xiiVec3(-1, 0, 0), xiiVec2(1, 1), options);
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(xiiVec3(+halfSize.x, +halfSize.y, -halfSize.z), xiiVec3(1, 0, 0), xiiVec2(0, 1), options);
      idx[1] = AddVertex(xiiVec3(+halfSize.x, +halfSize.y, +halfSize.z), xiiVec3(1, 0, 0), xiiVec2(0, 0), options);
      idx[2] = AddVertex(xiiVec3(+halfSize.x, -halfSize.y, +halfSize.z), xiiVec3(1, 0, 0), xiiVec2(1, 0), options);
      idx[3] = AddVertex(xiiVec3(+halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(1, 0, 0), xiiVec2(1, 1), options);
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(xiiVec3(+halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(0, -1, 0), xiiVec2(0, 1), options);
      idx[1] = AddVertex(xiiVec3(+halfSize.x, -halfSize.y, +halfSize.z), xiiVec3(0, -1, 0), xiiVec2(0, 0), options);
      idx[2] = AddVertex(xiiVec3(-halfSize.x, -halfSize.y, +halfSize.z), xiiVec3(0, -1, 0), xiiVec2(1, 0), options);
      idx[3] = AddVertex(xiiVec3(-halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(0, -1, 0), xiiVec2(1, 1), options);
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(xiiVec3(-halfSize.x, +halfSize.y, -halfSize.z), xiiVec3(0, +1, 0), xiiVec2(0, 1), options);
      idx[1] = AddVertex(xiiVec3(-halfSize.x, +halfSize.y, +halfSize.z), xiiVec3(0, +1, 0), xiiVec2(0, 0), options);
      idx[2] = AddVertex(xiiVec3(+halfSize.x, +halfSize.y, +halfSize.z), xiiVec3(0, +1, 0), xiiVec2(1, 0), options);
      idx[3] = AddVertex(xiiVec3(+halfSize.x, +halfSize.y, -halfSize.z), xiiVec3(0, +1, 0), xiiVec2(1, 1), options);
      AddPolygon(idx, bFlipWinding);
    }
  }
  else
  {
    xiiUInt32 idx[8];

    idx[0] = AddVertex(xiiVec3(-halfSize.x, -halfSize.y, halfSize.z), xiiVec3(0, 0, 1), xiiVec2(0), options);
    idx[1] = AddVertex(xiiVec3(halfSize.x, -halfSize.y, halfSize.z), xiiVec3(0, 0, 1), xiiVec2(0), options);
    idx[2] = AddVertex(xiiVec3(halfSize.x, halfSize.y, halfSize.z), xiiVec3(0, 0, 1), xiiVec2(0), options);
    idx[3] = AddVertex(xiiVec3(-halfSize.x, halfSize.y, halfSize.z), xiiVec3(0, 0, 1), xiiVec2(0), options);

    idx[4] = AddVertex(xiiVec3(-halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0), options);
    idx[5] = AddVertex(xiiVec3(halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0), options);
    idx[6] = AddVertex(xiiVec3(halfSize.x, halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0), options);
    idx[7] = AddVertex(xiiVec3(-halfSize.x, halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0), options);

    xiiUInt32 poly[4];

    poly[0] = idx[0];
    poly[1] = idx[1];
    poly[2] = idx[2];
    poly[3] = idx[3];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[1];
    poly[1] = idx[5];
    poly[2] = idx[6];
    poly[3] = idx[2];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[5];
    poly[1] = idx[4];
    poly[2] = idx[7];
    poly[3] = idx[6];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[4];
    poly[1] = idx[0];
    poly[2] = idx[3];
    poly[3] = idx[7];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[4];
    poly[1] = idx[5];
    poly[2] = idx[1];
    poly[3] = idx[0];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[3];
    poly[1] = idx[2];
    poly[2] = idx[6];
    poly[3] = idx[7];
    AddPolygon(poly, bFlipWinding);
  }
}

void xiiGeometry::AddLineBox(const xiiVec3& vSize, const GeoOptions& options)
{
  const xiiVec3 halfSize = vSize * 0.5f;

  AddVertex(xiiVec3(-halfSize.x, -halfSize.y, halfSize.z), xiiVec3(0, 0, 1), xiiVec2(0), options);
  AddVertex(xiiVec3(halfSize.x, -halfSize.y, halfSize.z), xiiVec3(0, 0, 1), xiiVec2(0), options);
  AddVertex(xiiVec3(halfSize.x, halfSize.y, halfSize.z), xiiVec3(0, 0, 1), xiiVec2(0), options);
  AddVertex(xiiVec3(-halfSize.x, halfSize.y, halfSize.z), xiiVec3(0, 0, 1), xiiVec2(0), options);

  AddVertex(xiiVec3(-halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0), options);
  AddVertex(xiiVec3(halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0), options);
  AddVertex(xiiVec3(halfSize.x, halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0), options);
  AddVertex(xiiVec3(-halfSize.x, halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0), options);

  AddLine(0, 1);
  AddLine(1, 2);
  AddLine(2, 3);
  AddLine(3, 0);

  AddLine(4, 5);
  AddLine(5, 6);
  AddLine(6, 7);
  AddLine(7, 4);

  AddLine(0, 4);
  AddLine(1, 5);
  AddLine(2, 6);
  AddLine(3, 7);
}

void xiiGeometry::AddLineBoxCorners(const xiiVec3& vSize, float fCornerFraction, const GeoOptions& options)
{
  XII_ASSERT_DEV(fCornerFraction >= 0.0f && fCornerFraction <= 1.0f, "A fraction value of {0} is invalid", xiiArgF(fCornerFraction, 2));

  fCornerFraction *= 0.5f;
  const xiiVec3 halfSize = vSize * 0.5f;

  AddVertex(xiiVec3(-halfSize.x, -halfSize.y, halfSize.z), xiiVec3(0, 0, 1), xiiVec2(0), options);
  AddVertex(xiiVec3(halfSize.x, -halfSize.y, halfSize.z), xiiVec3(0, 0, 1), xiiVec2(0), options);
  AddVertex(xiiVec3(halfSize.x, halfSize.y, halfSize.z), xiiVec3(0, 0, 1), xiiVec2(0), options);
  AddVertex(xiiVec3(-halfSize.x, halfSize.y, halfSize.z), xiiVec3(0, 0, 1), xiiVec2(0), options);

  AddVertex(xiiVec3(-halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0), options);
  AddVertex(xiiVec3(halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0), options);
  AddVertex(xiiVec3(halfSize.x, halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0), options);
  AddVertex(xiiVec3(-halfSize.x, halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0), options);

  for (xiiUInt32 c = 0; c < 8; ++c)
  {
    const xiiVec3& op = m_Vertices[c].m_vPosition;

    const xiiVec3 op1 = xiiVec3(op.x, op.y, -xiiMath::Sign(op.z) * xiiMath::Abs(op.z));
    const xiiVec3 op2 = xiiVec3(op.x, -xiiMath::Sign(op.y) * xiiMath::Abs(op.y), op.z);
    const xiiVec3 op3 = xiiVec3(-xiiMath::Sign(op.x) * xiiMath::Abs(op.x), op.y, op.z);

    const xiiUInt32 ix1 = AddVertex(xiiMath::Lerp(op, op1, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord, options);
    const xiiUInt32 ix2 = AddVertex(xiiMath::Lerp(op, op2, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord, options);
    const xiiUInt32 ix3 = AddVertex(xiiMath::Lerp(op, op3, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord, options);

    AddLine(c, ix1);
    AddLine(c, ix2);
    AddLine(c, ix3);
  }
}

void xiiGeometry::AddPyramid(const xiiVec3& vSize, bool bCap, const GeoOptions& options)
{
  const xiiVec3 halfSize     = vSize * 0.5f;
  const bool    bFlipWinding = options.IsFlipWindingNecessary();
  xiiUInt32     quad[4];

  quad[0] = AddVertex(xiiVec3(-halfSize.x, halfSize.y, 0), xiiVec3(-1, 1, 0).GetNormalized(), xiiVec2(0), options);
  quad[1] = AddVertex(xiiVec3(halfSize.x, halfSize.y, 0), xiiVec3(1, 1, 0).GetNormalized(), xiiVec2(0), options);
  quad[2] = AddVertex(xiiVec3(halfSize.x, -halfSize.y, 0), xiiVec3(1, -1, 0).GetNormalized(), xiiVec2(0), options);
  quad[3] = AddVertex(xiiVec3(-halfSize.x, -halfSize.y, 0), xiiVec3(-1, -1, 0).GetNormalized(), xiiVec2(0), options);

  const xiiUInt32 tip = AddVertex(xiiVec3(0, 0, vSize.z), xiiVec3(0, 0, 1), xiiVec2(0), options);

  if (bCap)
  {
    AddPolygon(quad, bFlipWinding);
  }

  xiiUInt32 tri[3];

  tri[0] = quad[1];
  tri[1] = quad[0];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);

  tri[0] = quad[2];
  tri[1] = quad[1];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);

  tri[0] = quad[3];
  tri[1] = quad[2];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);

  tri[0] = quad[0];
  tri[1] = quad[3];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);
}

void xiiGeometry::AddGeodesicSphere(float fRadius, xiiUInt8 uiSubDivisions, const GeoOptions& options)
{
  const bool bFlipWinding = options.IsFlipWindingNecessary();
  struct Triangle
  {
    Triangle(xiiUInt32 ui1, xiiUInt32 ui2, xiiUInt32 ui3)
    {
      m_uiIndex[0] = ui1;
      m_uiIndex[1] = ui2;
      m_uiIndex[2] = ui3;
    }

    xiiUInt32 m_uiIndex[3];
  };

  struct Edge
  {
    Edge() = default;

    Edge(xiiUInt32 uiId1, xiiUInt32 uiId2)
    {
      m_uiVertex[0] = xiiMath::Min(uiId1, uiId2);
      m_uiVertex[1] = xiiMath::Max(uiId1, uiId2);
    }

    bool operator<(const Edge& rhs) const
    {
      if (m_uiVertex[0] < rhs.m_uiVertex[0])
        return true;
      if (m_uiVertex[0] > rhs.m_uiVertex[0])
        return false;
      return m_uiVertex[1] < rhs.m_uiVertex[1];
    }

    bool operator==(const Edge& rhs) const { return m_uiVertex[0] == rhs.m_uiVertex[0] && m_uiVertex[1] == rhs.m_uiVertex[1]; }

    xiiUInt32 m_uiVertex[2];
  };

  const xiiUInt32 uiFirstVertex = m_Vertices.GetCount();

  xiiInt32           iCurrentList = 0;
  xiiDeque<Triangle> Tris[2];
  xiiVec4U16         boneIndices(options.m_uiBoneIndex, 0, 0, 0);

  // create icosahedron
  {
    xiiMat3 mRotX, mRotZ, mRotZh;
    mRotX  = xiiMat3::MakeRotationX(xiiAngle::MakeFromDegree(360.0f / 6.0f));
    mRotZ  = xiiMat3::MakeRotationZ(xiiAngle::MakeFromDegree(-360.0f / 5.0f));
    mRotZh = xiiMat3::MakeRotationZ(xiiAngle::MakeFromDegree(-360.0f / 10.0f));

    xiiUInt32 vert[12];
    xiiVec3   vDir(0, 0, 1);

    vDir.Normalize();
    vert[0] = AddVertex(vDir * fRadius, vDir, xiiVec2::MakeZero(), options.m_Color, boneIndices);

    vDir = mRotX * vDir;

    for (xiiInt32 i = 0; i < 5; ++i)
    {
      vDir.Normalize();
      vert[1 + i] = AddVertex(vDir * fRadius, vDir, xiiVec2::MakeZero(), options.m_Color, boneIndices);
      vDir        = mRotZ * vDir;
    }

    vDir = mRotX * vDir;
    vDir = mRotZh * vDir;

    for (xiiInt32 i = 0; i < 5; ++i)
    {
      vDir.Normalize();
      vert[6 + i] = AddVertex(vDir * fRadius, vDir, xiiVec2::MakeZero(), options.m_Color, boneIndices);
      vDir        = mRotZ * vDir;
    }

    vDir.Set(0, 0, -1);
    vDir.Normalize();
    vert[11] = AddVertex(vDir * fRadius, vDir, xiiVec2::MakeZero(), options.m_Color, boneIndices);


    Tris[0].PushBack(Triangle(vert[0], vert[2], vert[1]));
    Tris[0].PushBack(Triangle(vert[0], vert[3], vert[2]));
    Tris[0].PushBack(Triangle(vert[0], vert[4], vert[3]));
    Tris[0].PushBack(Triangle(vert[0], vert[5], vert[4]));
    Tris[0].PushBack(Triangle(vert[0], vert[1], vert[5]));

    Tris[0].PushBack(Triangle(vert[1], vert[2], vert[6]));
    Tris[0].PushBack(Triangle(vert[2], vert[3], vert[7]));
    Tris[0].PushBack(Triangle(vert[3], vert[4], vert[8]));
    Tris[0].PushBack(Triangle(vert[4], vert[5], vert[9]));
    Tris[0].PushBack(Triangle(vert[5], vert[1], vert[10]));

    Tris[0].PushBack(Triangle(vert[2], vert[7], vert[6]));
    Tris[0].PushBack(Triangle(vert[3], vert[8], vert[7]));
    Tris[0].PushBack(Triangle(vert[4], vert[9], vert[8]));
    Tris[0].PushBack(Triangle(vert[5], vert[10], vert[9]));
    Tris[0].PushBack(Triangle(vert[6], vert[10], vert[1]));

    Tris[0].PushBack(Triangle(vert[7], vert[11], vert[6]));
    Tris[0].PushBack(Triangle(vert[8], vert[11], vert[7]));
    Tris[0].PushBack(Triangle(vert[9], vert[11], vert[8]));
    Tris[0].PushBack(Triangle(vert[10], vert[11], vert[9]));
    Tris[0].PushBack(Triangle(vert[6], vert[11], vert[10]));
  }

  xiiMap<Edge, xiiUInt32> NewVertices;

  // subdivide the icosahedron n times (splitting every triangle into 4 new triangles)
  for (xiiUInt32 div = 0; div < uiSubDivisions; ++div)
  {
    // switch the last result and the new result
    const xiiInt32 iPrevList = iCurrentList;
    iCurrentList             = (iCurrentList + 1) % 2;

    Tris[iCurrentList].Clear();
    NewVertices.Clear();

    for (xiiUInt32 tri = 0; tri < Tris[iPrevList].GetCount(); ++tri)
    {
      xiiUInt32 uiVert[3] = {Tris[iPrevList][tri].m_uiIndex[0], Tris[iPrevList][tri].m_uiIndex[1], Tris[iPrevList][tri].m_uiIndex[2]};

      Edge Edges[3] = {Edge(uiVert[0], uiVert[1]), Edge(uiVert[1], uiVert[2]), Edge(uiVert[2], uiVert[0])};

      xiiUInt32 uiNewVert[3];

      // split each edge of the triangle in half
      for (xiiUInt32 i = 0; i < 3; ++i)
      {
        // do not split an edge that was split before, we want shared vertices everywhere
        if (NewVertices.Find(Edges[i]).IsValid())
          uiNewVert[i] = NewVertices[Edges[i]];
        else
        {
          const xiiVec3 vCenter = (m_Vertices[Edges[i].m_uiVertex[0]].m_vPosition + m_Vertices[Edges[i].m_uiVertex[1]].m_vPosition).GetNormalized();
          uiNewVert[i]          = AddVertex(vCenter * fRadius, vCenter, xiiVec2::MakeZero(), options.m_Color, boneIndices);

          NewVertices[Edges[i]] = uiNewVert[i];
        }
      }

      // now we turn one triangle into 4 smaller ones
      Tris[iCurrentList].PushBack(Triangle(uiVert[0], uiNewVert[0], uiNewVert[2]));
      Tris[iCurrentList].PushBack(Triangle(uiNewVert[0], uiVert[1], uiNewVert[1]));
      Tris[iCurrentList].PushBack(Triangle(uiNewVert[1], uiVert[2], uiNewVert[2]));

      Tris[iCurrentList].PushBack(Triangle(uiNewVert[0], uiNewVert[1], uiNewVert[2]));
    }
  }

  // add the final list of triangles to the output
  for (xiiUInt32 tri = 0; tri < Tris[iCurrentList].GetCount(); ++tri)
  {
    AddPolygon(Tris[iCurrentList][tri].m_uiIndex, bFlipWinding);
  }

  // finally apply the user transformation on the new vertices
  TransformVertices(options.m_Transform, uiFirstVertex);
}

void xiiGeometry::AddCylinder(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, bool bCapTop, bool bCapBottom, xiiUInt16 uiSegments, const GeoOptions& options, xiiAngle fraction /*= xiiAngle::MakeFromDegree(360.0f)*/)
{
  XII_ASSERT_DEV(uiSegments >= 3, "Cannot create a cylinder with only {0} segments", uiSegments);
  XII_ASSERT_DEV(fraction.GetDegree() >= -0.01f, "A cylinder cannot be built with more less than 0 degree");
  XII_ASSERT_DEV(fraction.GetDegree() <= 360.01f, "A cylinder cannot be built with more than 360 degree");

  fraction = xiiMath::Clamp(fraction, xiiAngle(), xiiAngle::MakeFromDegree(360.0f));

  const bool     bFlipWinding = options.IsFlipWindingNecessary();
  const bool     bIsFraction  = fraction.GetDegree() < 360.0f;
  const xiiAngle fDegStep     = xiiAngle::MakeFromDegree(fraction.GetDegree() / uiSegments);

  const xiiVec3 vTopCenter(0, 0, fPositiveLength);
  const xiiVec3 vBottomCenter(0, 0, -fNegativeLength);

  // cylinder wall
  {
    xiiHybridArray<xiiUInt32, 512> VertsTop;
    xiiHybridArray<xiiUInt32, 512> VertsBottom;

    for (xiiInt32 i = 0; i <= uiSegments; ++i)
    {
      const xiiAngle deg = (float)i * fDegStep;

      float fU = 4.0f - deg.GetDegree() / 90.0f;

      const float fX = xiiMath::Cos(deg);
      const float fY = xiiMath::Sin(deg);

      const xiiVec3 vDir(fX, fY, 0);

      VertsTop.PushBack(AddVertex(vTopCenter + vDir * fRadiusTop, vDir, xiiVec2(fU, 0), options));
      VertsBottom.PushBack(AddVertex(vBottomCenter + vDir * fRadiusBottom, vDir, xiiVec2(fU, 1), options));
    }

    for (xiiUInt32 i = 1; i <= uiSegments; ++i)
    {
      xiiUInt32 quad[4];
      quad[0] = VertsBottom[i - 1];
      quad[1] = VertsBottom[i];
      quad[2] = VertsTop[i];
      quad[3] = VertsTop[i - 1];


      AddPolygon(quad, bFlipWinding);
    }
  }

  // walls for fractional cylinders
  if (bIsFraction)
  {
    const xiiVec3 vDir0(1, 0, 0);
    const xiiVec3 vDir1(xiiMath::Cos(fraction), xiiMath::Sin(fraction), 0);

    xiiUInt32 quad[4];

    const xiiVec3 vNrm0 = -xiiVec3(0, 0, 1).CrossRH(vDir0).GetNormalized();
    quad[0]             = AddVertex(vTopCenter + vDir0 * fRadiusTop, vNrm0, xiiVec2(0, 0), options);
    quad[1]             = AddVertex(vTopCenter, vNrm0, xiiVec2(1, 0), options);
    quad[2]             = AddVertex(vBottomCenter, vNrm0, xiiVec2(1, 1), options);
    quad[3]             = AddVertex(vBottomCenter + vDir0 * fRadiusBottom, vNrm0, xiiVec2(0, 1), options);


    AddPolygon(quad, bFlipWinding);

    const xiiVec3 vNrm1 = xiiVec3(0, 0, 1).CrossRH(vDir1).GetNormalized();
    quad[0]             = AddVertex(vTopCenter, vNrm1, xiiVec2(0, 0), options);
    quad[1]             = AddVertex(vTopCenter + vDir1 * fRadiusTop, vNrm1, xiiVec2(1, 0), options);
    quad[2]             = AddVertex(vBottomCenter + vDir1 * fRadiusBottom, vNrm1, xiiVec2(1, 1), options);
    quad[3]             = AddVertex(vBottomCenter, vNrm1, xiiVec2(0, 1), options);

    AddPolygon(quad, bFlipWinding);
  }

  if (bCapBottom)
  {
    xiiHybridArray<xiiUInt32, 512> VertsBottom;

    if (bIsFraction)
    {
      const xiiUInt32 uiCenterVtx = AddVertex(vBottomCenter, xiiVec3(0, 0, -1), xiiVec2(0), options);

      for (xiiInt32 i = uiSegments; i >= 0; --i)
      {
        const xiiAngle deg = (float)i * fDegStep;

        const float fX = xiiMath::Cos(deg);
        const float fY = xiiMath::Sin(deg);

        const xiiVec3 vDir(fX, fY, 0);

        AddVertex(vBottomCenter + vDir * fRadiusBottom, xiiVec3(0, 0, -1), xiiVec2(fY, fX), options);
      }

      VertsBottom.SetCountUninitialized(3);
      VertsBottom[0] = uiCenterVtx;

      for (xiiUInt32 i = 0; i < uiSegments; ++i)
      {
        VertsBottom[1] = uiCenterVtx + i + 1;
        VertsBottom[2] = uiCenterVtx + i + 2;

        AddPolygon(VertsBottom, bFlipWinding);
      }
    }
    else
    {
      for (xiiInt32 i = uiSegments - 1; i >= 0; --i)
      {
        const xiiAngle deg = (float)i * fDegStep;

        const float fX = xiiMath::Cos(deg);
        const float fY = xiiMath::Sin(deg);

        const xiiVec3 vDir(fX, fY, 0);

        VertsBottom.PushBack(AddVertex(vBottomCenter + vDir * fRadiusBottom, xiiVec3(0, 0, -1), xiiVec2(fY, fX), options));
      }

      AddPolygon(VertsBottom, bFlipWinding);
    }
  }

  if (bCapTop)
  {
    xiiHybridArray<xiiUInt32, 512> VertsTop;

    if (bIsFraction)
    {
      const xiiUInt32 uiCenterVtx = AddVertex(vTopCenter, xiiVec3(0, 0, 1), xiiVec2(0), options);

      for (xiiInt32 i = 0; i <= uiSegments; ++i)
      {
        const xiiAngle deg = (float)i * fDegStep;

        const float fX = xiiMath::Cos(deg);
        const float fY = xiiMath::Sin(deg);

        const xiiVec3 vDir(fX, fY, 0);

        AddVertex(vTopCenter + vDir * fRadiusTop, xiiVec3(0, 0, 1), xiiVec2(fY, -fX), options);
      }

      VertsTop.SetCountUninitialized(3);
      VertsTop[0] = uiCenterVtx;

      for (xiiUInt32 i = 0; i < uiSegments; ++i)
      {
        VertsTop[1] = uiCenterVtx + i + 1;
        VertsTop[2] = uiCenterVtx + i + 2;

        AddPolygon(VertsTop, bFlipWinding);
      }
    }
    else
    {
      for (xiiInt32 i = 0; i < uiSegments; ++i)
      {
        const xiiAngle deg = (float)i * fDegStep;

        const float fX = xiiMath::Cos(deg);
        const float fY = xiiMath::Sin(deg);

        const xiiVec3 vDir(fX, fY, 0);

        VertsTop.PushBack(AddVertex(vTopCenter + vDir * fRadiusTop, xiiVec3(0, 0, 1), xiiVec2(fY, -fX), options));
      }

      AddPolygon(VertsTop, bFlipWinding);
    }
  }
}

void xiiGeometry::AddCylinderOnePiece(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, xiiUInt16 uiSegments, const GeoOptions& options)
{
  XII_ASSERT_DEV(uiSegments >= 3, "Cannot create a cylinder with only {0} segments", uiSegments);

  const bool     bFlipWinding = options.IsFlipWindingNecessary();
  const xiiAngle fDegStep     = xiiAngle::MakeFromDegree(360.0f / uiSegments);

  const xiiVec3 vTopCenter(0, 0, fPositiveLength);
  const xiiVec3 vBottomCenter(0, 0, -fNegativeLength);

  // cylinder wall
  {
    xiiHybridArray<xiiUInt32, 512> VertsTop;
    xiiHybridArray<xiiUInt32, 512> VertsBottom;

    for (xiiInt32 i = 0; i < uiSegments; ++i)
    {
      const xiiAngle deg = (float)i * fDegStep;

      float fU = 4.0f - deg.GetDegree() / 90.0f;

      const float fX = xiiMath::Cos(deg);
      const float fY = xiiMath::Sin(deg);

      const xiiVec3 vDir(fX, fY, 0);

      VertsTop.PushBack(AddVertex(vTopCenter + vDir * fRadiusTop, vDir, xiiVec2(fU, 0), options));
      VertsBottom.PushBack(AddVertex(vBottomCenter + vDir * fRadiusBottom, vDir, xiiVec2(fU, 1), options));
    }

    for (xiiUInt32 i = 1; i <= uiSegments; ++i)
    {
      xiiUInt32 quad[4];
      quad[0] = VertsBottom[i - 1];
      quad[1] = VertsBottom[i % uiSegments];
      quad[2] = VertsTop[i % uiSegments];
      quad[3] = VertsTop[i - 1];

      AddPolygon(quad, bFlipWinding);
    }

    AddPolygon(VertsTop, bFlipWinding);
    AddPolygon(VertsBottom, !bFlipWinding);
  }
}

void xiiGeometry::AddCone(float fRadius, float fHeight, bool bCap, xiiUInt16 uiSegments, const GeoOptions& options)
{
  XII_ASSERT_DEV(uiSegments >= 3, "Cannot create a cone with only {0} segments", uiSegments);

  const bool bFlipWinding = options.IsFlipWindingNecessary();

  xiiHybridArray<xiiUInt32, 512> VertsBottom;

  const xiiAngle fDegStep = xiiAngle::MakeFromDegree(360.0f / uiSegments);

  const xiiUInt32 uiTip = AddVertex(xiiVec3(0, 0, fHeight), xiiVec3(0, 0, 1), xiiVec2(0), options);

  for (xiiInt32 i = uiSegments - 1; i >= 0; --i)
  {
    const xiiAngle deg = (float)i * fDegStep;

    xiiVec3 vDir(xiiMath::Cos(deg), xiiMath::Sin(deg), 0);

    VertsBottom.PushBack(AddVertex(vDir * fRadius, vDir, xiiVec2(0), options));
  }

  xiiUInt32 uiPrevSeg = uiSegments - 1;

  for (xiiUInt32 i = 0; i < uiSegments; ++i)
  {
    xiiUInt32 tri[3];
    tri[0] = VertsBottom[uiPrevSeg];
    tri[1] = uiTip;
    tri[2] = VertsBottom[i];

    uiPrevSeg = i;

    AddPolygon(tri, bFlipWinding);
  }

  if (bCap)
  {
    AddPolygon(VertsBottom, bFlipWinding);
  }
}

void xiiGeometry::AddSphere(float fRadius, xiiUInt16 uiSegments, xiiUInt16 uiStacks, const GeoOptions& options)
{
  XII_ASSERT_DEV(uiSegments >= 3, "Sphere must have at least 3 segments");
  XII_ASSERT_DEV(uiStacks >= 2, "Sphere must have at least 2 stacks");

  const bool     bFlipWinding        = options.IsFlipWindingNecessary();
  const xiiAngle fDegreeDiffSegments = xiiAngle::MakeFromDegree(360.0f / (float)(uiSegments));
  const xiiAngle fDegreeDiffStacks   = xiiAngle::MakeFromDegree(180.0f / (float)(uiStacks));

  const xiiUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  for (xiiUInt32 st = 1; st < uiStacks; ++st)
  {
    const xiiAngle fDegreeStack = xiiAngle::MakeFromDegree(-90.0f + (st * fDegreeDiffStacks.GetDegree()));
    const float    fCosDS       = xiiMath::Cos(fDegreeStack);
    const float    fSinDS       = xiiMath::Sin(fDegreeStack);
    const float    fY           = -fSinDS * fRadius;

    const float fV = (float)st / (float)uiStacks;

    for (xiiUInt32 sp = 0; sp < uiSegments + 1u; ++sp)
    {
      float fU = ((float)sp / (float)(uiSegments)) * 2.0f;

      const xiiAngle fDegree = (float)sp * fDegreeDiffSegments;

      xiiVec3 vPos;
      vPos.x = xiiMath::Cos(fDegree) * fRadius * fCosDS;
      vPos.y = -xiiMath::Sin(fDegree) * fRadius * fCosDS;
      vPos.z = fY;

      xiiVec3 vNormal = vPos;
      vNormal.NormalizeIfNotZero(xiiVec3(0, 0, 1)).IgnoreResult();
      AddVertex(vPos, vNormal, xiiVec2(fU, fV), options);
    }
  }

  xiiUInt32 tri[3];
  xiiUInt32 quad[4];

  // now create the top cone
  for (xiiUInt32 p = 0; p < uiSegments; ++p)
  {
    float fU = ((p + 0.5f) / (float)(uiSegments)) * 2.0f;

    tri[0] = AddVertex(xiiVec3(0, 0, fRadius), xiiVec3(0, 0, 1), xiiVec2(fU, 0), options);
    tri[1] = uiFirstVertex + p + 1;
    tri[2] = uiFirstVertex + p;

    AddPolygon(tri, bFlipWinding);
  }

  // now create the stacks in the middle
  for (xiiUInt16 st = 0; st < uiStacks - 2; ++st)
  {
    const xiiUInt32 uiRowBottom = (uiSegments + 1) * st;
    const xiiUInt32 uiRowTop    = (uiSegments + 1) * (st + 1);

    for (xiiInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + i + 1);
      quad[1] = uiFirstVertex + (uiRowTop + i);
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[3] = uiFirstVertex + (uiRowBottom + i + 1);

      AddPolygon(quad, bFlipWinding);
    }
  }

  const xiiInt32 iTopStack = (uiSegments + 1) * (uiStacks - 2);

  // now create the bottom cone
  for (xiiUInt32 p = 0; p < uiSegments; ++p)
  {
    float fU = ((p + 0.5f) / (float)(uiSegments)) * 2.0f;

    tri[0] = AddVertex(xiiVec3(0, 0, -fRadius), xiiVec3(0, 0, -1), xiiVec2(fU, 1), options);
    tri[1] = uiFirstVertex + (iTopStack + p);
    tri[2] = uiFirstVertex + (iTopStack + p + 1);

    AddPolygon(tri, bFlipWinding);
  }
}

void xiiGeometry::AddHalfSphere(float fRadius, xiiUInt16 uiSegments, xiiUInt16 uiStacks, bool bCap, const GeoOptions& options)
{
  XII_ASSERT_DEV(uiSegments >= 3, "Sphere must have at least 3 segments");
  XII_ASSERT_DEV(uiStacks >= 1, "Sphere must have at least 1 stacks");

  const bool     bFlipWinding        = options.IsFlipWindingNecessary();
  const xiiAngle fDegreeDiffSegments = xiiAngle::MakeFromDegree(360.0f / (float)(uiSegments));
  const xiiAngle fDegreeDiffStacks   = xiiAngle::MakeFromDegree(90.0f / (float)(uiStacks));

  const xiiUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  for (xiiUInt32 st = 0; st < uiStacks; ++st)
  {
    const xiiAngle fDegreeStack = xiiAngle::MakeFromDegree(-90.0f + ((st + 1) * fDegreeDiffStacks.GetDegree()));
    const float    fCosDS       = xiiMath::Cos(fDegreeStack);
    const float    fSinDS       = xiiMath::Sin(fDegreeStack);
    const float    fY           = -fSinDS * fRadius;

    const float fV = (float)(st + 1) / (float)uiStacks;

    for (xiiUInt32 sp = 0; sp <= uiSegments; ++sp)
    {
      float fU = ((float)sp / (float)(uiSegments)) * 2.0f;

      if (fU > 1.0f)
        fU = 2.0f - fU;

      // the vertices for the bottom disk
      const xiiAngle fDegree = (float)sp * fDegreeDiffSegments;

      xiiVec3 vPos;
      vPos.x = xiiMath::Cos(fDegree) * fRadius * fCosDS;
      vPos.y = xiiMath::Sin(fDegree) * fRadius * fCosDS;
      vPos.z = fY;

      AddVertex(vPos, vPos.GetNormalized(), xiiVec2(fU, fV), options);
    }
  }

  xiiUInt32 uiTopVertex = AddVertex(xiiVec3(0, 0, fRadius), xiiVec3(0, 0, 1), xiiVec2(0.0f), options);

  xiiUInt32 tri[3];
  xiiUInt32 quad[4];

  // now create the top cone
  for (xiiUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiTopVertex;
    tri[1] = uiFirstVertex + p;
    tri[2] = uiFirstVertex + ((p + 1) % (uiSegments + 1));

    AddPolygon(tri, bFlipWinding);
  }

  // now create the stacks in the middle

  for (xiiUInt16 st = 0; st < uiStacks - 1; ++st)
  {
    const xiiUInt32 uiRowBottom = (uiSegments + 1) * st;
    const xiiUInt32 uiRowTop    = (uiSegments + 1) * (st + 1);

    for (xiiInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + ((i + 1) % (uiSegments + 1)));
      quad[1] = uiFirstVertex + (uiRowBottom + ((i + 1) % (uiSegments + 1)));
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[3] = uiFirstVertex + (uiRowTop + i);

      AddPolygon(quad, bFlipWinding);
    }
  }

  if (bCap)
  {
    xiiHybridArray<xiiUInt32, 256> uiCap;

    for (xiiUInt32 i = uiTopVertex - 1; i >= uiTopVertex - uiSegments; --i)
      uiCap.PushBack(i);

    AddPolygon(uiCap, bFlipWinding);
  }
}

void xiiGeometry::AddCapsule(float fRadius, float fHeight, xiiUInt16 uiSegments, xiiUInt16 uiStacks, const GeoOptions& options)
{
  XII_ASSERT_DEV(uiSegments >= 3, "Capsule must have at least 3 segments");
  XII_ASSERT_DEV(uiStacks >= 1, "Capsule must have at least 1 stacks");
  XII_ASSERT_DEV(fHeight >= 0.0f, "Height must be positive");

  const bool     bFlipWinding      = options.IsFlipWindingNecessary();
  const xiiAngle fDegreeDiffStacks = xiiAngle::MakeFromDegree(90.0f / (float)(uiStacks));

  const xiiUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  const float fDegreeStepSlices = 360.0f / (float)(uiSegments);

  float fOffset = fHeight * 0.5f;

  // for (xiiUInt32 h = 0; h < 2; ++h)
  {
    for (xiiUInt32 st = 0; st < uiStacks; ++st)
    {
      const xiiAngle fDegreeStack = xiiAngle::MakeFromDegree(-90.0f + ((st + 1) * fDegreeDiffStacks.GetDegree()));
      const float    fCosDS       = xiiMath::Cos(fDegreeStack);
      const float    fSinDS       = xiiMath::Sin(fDegreeStack);
      const float    fY           = -fSinDS * fRadius;

      for (xiiUInt32 sp = 0; sp < uiSegments; ++sp)
      {
        const xiiAngle fDegree = xiiAngle::MakeFromDegree(sp * fDegreeStepSlices);

        xiiVec3 vPos;
        vPos.x = xiiMath::Cos(fDegree) * fRadius * fCosDS;
        vPos.z = fY + fOffset;
        vPos.y = xiiMath::Sin(fDegree) * fRadius * fCosDS;

        AddVertex(vPos, vPos.GetNormalized(), xiiVec2(0), options);
      }
    }

    fOffset -= fHeight;

    for (xiiUInt32 st = 0; st < uiStacks; ++st)
    {
      const xiiAngle fDegreeStack = xiiAngle::MakeFromDegree(0.0f - (st * fDegreeDiffStacks.GetDegree()));
      const float    fCosDS       = xiiMath::Cos(fDegreeStack);
      const float    fSinDS       = xiiMath::Sin(fDegreeStack);
      const float    fY           = fSinDS * fRadius;

      for (xiiUInt32 sp = 0; sp < uiSegments; ++sp)
      {
        const xiiAngle fDegree = xiiAngle::MakeFromDegree(sp * fDegreeStepSlices);

        xiiVec3 vPos;
        vPos.x = xiiMath::Cos(fDegree) * fRadius * fCosDS;
        vPos.z = fY + fOffset;
        vPos.y = xiiMath::Sin(fDegree) * fRadius * fCosDS;

        AddVertex(vPos, vPos.GetNormalized(), xiiVec2(0), options);
      }
    }
  }

  xiiUInt32 uiTopVertex    = AddVertex(xiiVec3(0, 0, fRadius + fHeight * 0.5f), xiiVec3(0, 0, 1), xiiVec2(0), options);
  xiiUInt32 uiBottomVertex = AddVertex(xiiVec3(0, 0, -fRadius - fHeight * 0.5f), xiiVec3(0, 0, -1), xiiVec2(0), options);

  xiiUInt32 tri[3];
  xiiUInt32 quad[4];

  // now create the top cone
  for (xiiUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiTopVertex;
    tri[2] = uiFirstVertex + ((p + 1) % uiSegments);
    tri[1] = uiFirstVertex + p;

    AddPolygon(tri, bFlipWinding);
  }

  // now create the stacks in the middle
  xiiUInt16 uiMaxStacks = static_cast<xiiUInt16>(uiStacks * 2 - 1);
  for (xiiUInt16 st = 0; st < uiMaxStacks; ++st)
  {
    const xiiUInt32 uiRowBottom = uiSegments * st;
    const xiiUInt32 uiRowTop    = uiSegments * (st + 1);

    for (xiiInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + ((i + 1) % uiSegments));
      quad[3] = uiFirstVertex + (uiRowTop + i);
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[1] = uiFirstVertex + (uiRowBottom + ((i + 1) % uiSegments));

      AddPolygon(quad, bFlipWinding);
    }
  }

  const xiiInt32 iBottomStack = uiSegments * (uiStacks * 2 - 1);

  // now create the bottom cone
  for (xiiUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiBottomVertex;
    tri[2] = uiFirstVertex + (iBottomStack + p);
    tri[1] = uiFirstVertex + (iBottomStack + ((p + 1) % uiSegments));

    AddPolygon(tri, bFlipWinding);
  }
}

void xiiGeometry::AddTorus(float fInnerRadius, float fOuterRadius, xiiUInt16 uiSegments, xiiUInt16 uiSegmentDetail, bool bExtraVerticesForTexturing, const GeoOptions& options)
{
  XII_ASSERT_DEV(fInnerRadius < fOuterRadius, "Inner radius must be smaller than outer radius. Doh!");
  XII_ASSERT_DEV(uiSegments >= 3, "Invalid number of segments.");
  XII_ASSERT_DEV(uiSegmentDetail >= 3, "Invalid segment detail value.");

  const bool  bFlipWinding    = options.IsFlipWindingNecessary();
  const float fCylinderRadius = (fOuterRadius - fInnerRadius) * 0.5f;
  const float fLoopRadius     = fInnerRadius + fCylinderRadius;

  const xiiAngle fAngleStepSegment  = xiiAngle::MakeFromDegree(360.0f / uiSegments);
  const xiiAngle fAngleStepCylinder = xiiAngle::MakeFromDegree(360.0f / uiSegmentDetail);

  const xiiUInt16 uiFirstVertex = static_cast<xiiUInt16>(m_Vertices.GetCount());

  const xiiUInt16 uiNumSegments      = bExtraVerticesForTexturing ? uiSegments + 1 : uiSegments;
  const xiiUInt16 uiNumSegmentDetail = bExtraVerticesForTexturing ? uiSegmentDetail + 1 : uiSegmentDetail;

  // this is the loop for the torus ring
  for (xiiUInt16 seg = 0; seg < uiNumSegments; ++seg)
  {
    float fU = ((float)seg / (float)uiSegments) * 2.0f;

    const xiiAngle fAngle = (float)seg * fAngleStepSegment;

    const float fSinAngle = xiiMath::Sin(fAngle);
    const float fCosAngle = xiiMath::Cos(fAngle);

    const xiiVec3 vLoopPos = xiiVec3(fSinAngle, fCosAngle, 0) * fLoopRadius;

    // this is the loop to go round the cylinder
    for (xiiUInt16 p = 0; p < uiNumSegmentDetail; ++p)
    {
      float fV = (float)p / (float)uiSegmentDetail;

      const xiiAngle fCylinderAngle = (float)p * fAngleStepCylinder;

      const xiiVec3 vDir(xiiMath::Cos(fCylinderAngle) * fSinAngle, xiiMath::Cos(fCylinderAngle) * fCosAngle, xiiMath::Sin(fCylinderAngle));

      const xiiVec3 vPos = vLoopPos + fCylinderRadius * vDir;

      AddVertex(vPos, vDir, xiiVec2(fU, fV), options.m_Color, options.m_uiBoneIndex, options.m_Transform);
    }
  }

  if (bExtraVerticesForTexturing)
  {
    for (xiiUInt16 seg = 0; seg < uiSegments; ++seg)
    {
      const xiiUInt16 rs0 = uiFirstVertex + seg * (uiSegmentDetail + 1);
      const xiiUInt16 rs1 = uiFirstVertex + (seg + 1) * (uiSegmentDetail + 1);

      for (xiiUInt16 p = 0; p < uiSegmentDetail; ++p)
      {
        xiiUInt32 quad[4];
        quad[0] = rs1 + p;
        quad[3] = rs1 + p + 1;
        quad[2] = rs0 + p + 1;
        quad[1] = rs0 + p;

        AddPolygon(quad, bFlipWinding);
      }
    }
  }
  else
  {
    xiiUInt16 prevRing = (uiSegments - 1);

    for (xiiUInt16 seg = 0; seg < uiSegments; ++seg)
    {
      const xiiUInt16 thisRing = seg;

      const xiiUInt16 prevRingFirstVtx = uiFirstVertex + (prevRing * uiSegmentDetail);
      xiiUInt16       prevRingPrevVtx  = prevRingFirstVtx + (uiSegmentDetail - 1);

      const xiiUInt16 thisRingFirstVtx = uiFirstVertex + (thisRing * uiSegmentDetail);
      xiiUInt16       thisRingPrevVtx  = thisRingFirstVtx + (uiSegmentDetail - 1);

      for (xiiUInt16 p = 0; p < uiSegmentDetail; ++p)
      {
        const xiiUInt16 prevRingThisVtx = prevRingFirstVtx + p;
        const xiiUInt16 thisRingThisVtx = thisRingFirstVtx + p;

        xiiUInt32 quad[4];

        quad[0] = prevRingPrevVtx;
        quad[1] = prevRingThisVtx;
        quad[2] = thisRingThisVtx;
        quad[3] = thisRingPrevVtx;

        AddPolygon(quad, bFlipWinding);

        prevRingPrevVtx = prevRingThisVtx;
        thisRingPrevVtx = thisRingThisVtx;
      }

      prevRing = thisRing;
    }
  }
}

void xiiGeometry::AddTexturedRamp(const xiiVec3& vSize, const GeoOptions& options)
{
  const xiiVec3 halfSize     = vSize * 0.5f;
  const bool    bFlipWinding = options.IsFlipWindingNecessary();
  xiiUInt32     idx[4];
  xiiUInt32     idx3[3];

  {
    xiiVec3 vNormal = xiiVec3(-halfSize.z, 0, halfSize.x).GetNormalized();
    idx[0]          = AddVertex(xiiVec3(-halfSize.x, -halfSize.y, -halfSize.z), vNormal, xiiVec2(0, 1), options);
    idx[1]          = AddVertex(xiiVec3(+halfSize.x, -halfSize.y, +halfSize.z), vNormal, xiiVec2(0, 0), options);
    idx[2]          = AddVertex(xiiVec3(+halfSize.x, +halfSize.y, +halfSize.z), vNormal, xiiVec2(1, 0), options);
    idx[3]          = AddVertex(xiiVec3(-halfSize.x, +halfSize.y, -halfSize.z), vNormal, xiiVec2(1, 1), options);
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(xiiVec3(-halfSize.x, +halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(1, 0), options);
    idx[1] = AddVertex(xiiVec3(+halfSize.x, +halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(1, 1), options);
    idx[2] = AddVertex(xiiVec3(+halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0, 1), options);
    idx[3] = AddVertex(xiiVec3(-halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(0, 0, -1), xiiVec2(0, 0), options);
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(xiiVec3(+halfSize.x, +halfSize.y, -halfSize.z), xiiVec3(1, 0, 0), xiiVec2(0, 1), options);
    idx[1] = AddVertex(xiiVec3(+halfSize.x, +halfSize.y, +halfSize.z), xiiVec3(1, 0, 0), xiiVec2(0, 0), options);
    idx[2] = AddVertex(xiiVec3(+halfSize.x, -halfSize.y, +halfSize.z), xiiVec3(1, 0, 0), xiiVec2(1, 0), options);
    idx[3] = AddVertex(xiiVec3(+halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(1, 0, 0), xiiVec2(1, 1), options);
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx3[0] = AddVertex(xiiVec3(+halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(0, -1, 0), xiiVec2(0, 1), options);
    idx3[1] = AddVertex(xiiVec3(+halfSize.x, -halfSize.y, +halfSize.z), xiiVec3(0, -1, 0), xiiVec2(0, 0), options);
    idx3[2] = AddVertex(xiiVec3(-halfSize.x, -halfSize.y, -halfSize.z), xiiVec3(0, -1, 0), xiiVec2(1, 1), options);
    AddPolygon(idx3, bFlipWinding);
  }

  {
    idx3[0] = AddVertex(xiiVec3(-halfSize.x, +halfSize.y, -halfSize.z), xiiVec3(0, +1, 0), xiiVec2(0, 1), options);
    idx3[1] = AddVertex(xiiVec3(+halfSize.x, +halfSize.y, +halfSize.z), xiiVec3(0, +1, 0), xiiVec2(1, 0), options);
    idx3[2] = AddVertex(xiiVec3(+halfSize.x, +halfSize.y, -halfSize.z), xiiVec3(0, +1, 0), xiiVec2(1, 1), options);
    AddPolygon(idx3, bFlipWinding);
  }
}

void xiiGeometry::AddStairs(const xiiVec3& vSize, xiiUInt32 uiNumSteps, xiiAngle curvature, bool bSmoothSloped, const GeoOptions& options)
{
  const bool bFlipWinding = options.IsFlipWindingNecessary();

  curvature                = xiiMath::Clamp(curvature, -xiiAngle::MakeFromDegree(360), xiiAngle::MakeFromDegree(360));
  const xiiAngle curveStep = curvature / (float)uiNumSteps;

  const float fStepDiv    = 1.0f / uiNumSteps;
  const float fStepDepth  = vSize.x / uiNumSteps;
  const float fStepHeight = vSize.z / uiNumSteps;

  xiiVec3       vMoveFwd(fStepDepth, 0, 0);
  const xiiVec3 vMoveUp(0, 0, fStepHeight);
  xiiVec3       vMoveUpFwd(fStepDepth, 0, fStepHeight);

  xiiVec3 vBaseL0(-vSize.x * 0.5f, -vSize.y * 0.5f, -vSize.z * 0.5f);
  xiiVec3 vBaseL1(-vSize.x * 0.5f, +vSize.y * 0.5f, -vSize.z * 0.5f);
  xiiVec3 vBaseR0 = vBaseL0 + vMoveFwd;
  xiiVec3 vBaseR1 = vBaseL1 + vMoveFwd;

  xiiVec3 vTopL0 = vBaseL0 + vMoveUp;
  xiiVec3 vTopL1 = vBaseL1 + vMoveUp;
  xiiVec3 vTopR0 = vBaseR0 + vMoveUp;
  xiiVec3 vTopR1 = vBaseR1 + vMoveUp;

  xiiVec3 vPrevTopR0 = vBaseL0;
  xiiVec3 vPrevTopR1 = vBaseL1;

  float fTexU0 = 0;
  float fTexU1 = fStepDiv;

  xiiVec3 vSideNormal0(0, 1, 0);
  xiiVec3 vSideNormal1(0, 1, 0);
  xiiVec3 vStepFrontNormal(-1, 0, 0);

  xiiQuat qRot = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), curveStep);

  for (xiiUInt32 step = 0; step < uiNumSteps; ++step)
  {
    {
      const xiiVec3 vAvg = (vTopL0 + vTopL1 + vTopR0 + vTopR1) / 4.0f;

      vTopR0  = vAvg + qRot * (vTopR0 - vAvg);
      vTopR1  = vAvg + qRot * (vTopR1 - vAvg);
      vBaseR0 = vAvg + qRot * (vBaseR0 - vAvg);
      vBaseR1 = vAvg + qRot * (vBaseR1 - vAvg);

      vMoveFwd     = qRot * vMoveFwd;
      vMoveUpFwd   = vMoveFwd;
      vMoveUpFwd.z = fStepHeight;

      vSideNormal1 = qRot * vSideNormal1;
    }

    if (bSmoothSloped)
    {
      // don't care about exact normals for the top surfaces
      vTopL0 = vPrevTopR0;
      vTopL1 = vPrevTopR1;
    }

    xiiUInt32 poly[4];

    // top
    poly[0] = AddVertex(vTopL0, xiiVec3(0, 0, 1), xiiVec2(fTexU0, 0), options);
    poly[3] = AddVertex(vTopL1, xiiVec3(0, 0, 1), xiiVec2(fTexU0, 1), options);
    poly[1] = AddVertex(vTopR0, xiiVec3(0, 0, 1), xiiVec2(fTexU1, 0), options);
    poly[2] = AddVertex(vTopR1, xiiVec3(0, 0, 1), xiiVec2(fTexU1, 1), options);
    AddPolygon(poly, bFlipWinding);

    // bottom
    poly[0] = AddVertex(vBaseL0, xiiVec3(0, 0, -1), xiiVec2(fTexU0, 0), options);
    poly[1] = AddVertex(vBaseL1, xiiVec3(0, 0, -1), xiiVec2(fTexU0, 1), options);
    poly[3] = AddVertex(vBaseR0, xiiVec3(0, 0, -1), xiiVec2(fTexU1, 0), options);
    poly[2] = AddVertex(vBaseR1, xiiVec3(0, 0, -1), xiiVec2(fTexU1, 1), options);
    AddPolygon(poly, bFlipWinding);

    // step front
    if (!bSmoothSloped)
    {
      poly[0] = AddVertex(vPrevTopR0, xiiVec3(-1, 0, 0), xiiVec2(0, fTexU0), options);
      poly[3] = AddVertex(vPrevTopR1, xiiVec3(-1, 0, 0), xiiVec2(1, fTexU0), options);
      poly[1] = AddVertex(vTopL0, xiiVec3(-1, 0, 0), xiiVec2(0, fTexU1), options);
      poly[2] = AddVertex(vTopL1, xiiVec3(-1, 0, 0), xiiVec2(1, fTexU1), options);
      AddPolygon(poly, bFlipWinding);
    }

    // side 1
    poly[0] = AddVertex(vBaseL0, -vSideNormal0, xiiVec2(fTexU0, 0), options);
    poly[1] = AddVertex(vBaseR0, -vSideNormal1, xiiVec2(fTexU1, 0), options);
    poly[3] = AddVertex(vTopL0, -vSideNormal0, xiiVec2(fTexU0, fTexU1), options);
    poly[2] = AddVertex(vTopR0, -vSideNormal1, xiiVec2(fTexU1, fTexU1), options);
    AddPolygon(poly, bFlipWinding);

    // side 2
    poly[0] = AddVertex(vBaseL1, vSideNormal0, xiiVec2(fTexU0, 0), options);
    poly[3] = AddVertex(vBaseR1, vSideNormal1, xiiVec2(fTexU1, 0), options);
    poly[1] = AddVertex(vTopL1, vSideNormal0, xiiVec2(fTexU0, fTexU1), options);
    poly[2] = AddVertex(vTopR1, vSideNormal1, xiiVec2(fTexU1, fTexU1), options);
    AddPolygon(poly, bFlipWinding);

    vPrevTopR0 = vTopR0;
    vPrevTopR1 = vTopR1;

    vBaseL0 = vBaseR0;
    vBaseL1 = vBaseR1;
    vBaseR0 += vMoveFwd;
    vBaseR1 += vMoveFwd;

    vTopL0 = vTopR0 + vMoveUp;
    vTopL1 = vTopR1 + vMoveUp;
    vTopR0 += vMoveUpFwd;
    vTopR1 += vMoveUpFwd;

    fTexU0 = fTexU1;
    fTexU1 += fStepDiv;

    vSideNormal0     = vSideNormal1;
    vStepFrontNormal = qRot * vStepFrontNormal;
  }

  // back
  {
    xiiUInt32 poly[4];
    poly[0] = AddVertex(vBaseL0, -vStepFrontNormal, xiiVec2(0, 0), options);
    poly[1] = AddVertex(vBaseL1, -vStepFrontNormal, xiiVec2(1, 0), options);
    poly[3] = AddVertex(vPrevTopR0, -vStepFrontNormal, xiiVec2(0, 1), options);
    poly[2] = AddVertex(vPrevTopR1, -vStepFrontNormal, xiiVec2(1, 1), options);
    AddPolygon(poly, bFlipWinding);
  }
}


void xiiGeometry::AddArch(const xiiVec3& vSize, xiiUInt32 uiNumSegments, float fThickness, xiiAngle angle, bool bMakeSteps, bool bSmoothBottom, bool bSmoothTop, bool bCapTopAndBottom, const GeoOptions& options)
{
  // sanitize input values
  {
    if (angle.GetRadian() == 0.0f)
      angle = xiiAngle::MakeFromDegree(360);

    angle = xiiMath::Clamp(angle, xiiAngle::MakeFromDegree(-360.0f), xiiAngle::MakeFromDegree(360.0f));

    fThickness = xiiMath::Clamp(fThickness, 0.01f, xiiMath::Min(vSize.x, vSize.y) * 0.45f);

    bSmoothBottom = bMakeSteps && bSmoothBottom;
    bSmoothTop    = bMakeSteps && bSmoothTop;
  }

  bool bFlipWinding = options.IsFlipWindingNecessary();

  if (angle.GetRadian() < 0)
    bFlipWinding = !bFlipWinding;

  const xiiAngle angleStep   = angle / (float)uiNumSegments;
  const float    fScaleX     = vSize.x * 0.5f;
  const float    fScaleY     = vSize.y * 0.5f;
  const float    fHalfHeight = vSize.z * 0.5f;
  const float    fStepHeight = vSize.z / (float)uiNumSegments;

  float fBottomZ = -fHalfHeight;
  float fTopZ    = +fHalfHeight;

  if (bMakeSteps)
  {
    fTopZ = fBottomZ + fStepHeight;
  }

  // mutable variables
  xiiAngle nextAngle;
  xiiVec3  vCurDirOutwards, vNextDirOutwards;
  xiiVec3  vCurBottomOuter, vCurBottomInner, vCurTopOuter, vCurTopInner;
  xiiVec3  vNextBottomOuter, vNextBottomInner, vNextTopOuter, vNextTopInner;

  // Setup first round
  {
    vNextDirOutwards.Set(xiiMath::Cos(nextAngle), xiiMath::Sin(nextAngle), 0);
    vNextBottomOuter.Set(xiiMath::Cos(nextAngle) * fScaleX, xiiMath::Sin(nextAngle) * fScaleY, fBottomZ);
    vNextTopOuter.Set(vNextBottomOuter.x, vNextBottomOuter.y, fTopZ);

    const xiiVec3 vNextThickness = vNextDirOutwards * fThickness;
    vNextBottomInner             = vNextBottomOuter - vNextThickness;
    vNextTopInner                = vNextTopOuter - vNextThickness;

    if (bSmoothBottom)
    {
      vNextBottomInner.z += fStepHeight * 0.5f;
      vNextBottomOuter.z += fStepHeight * 0.5f;
    }

    if (bSmoothTop)
    {
      vNextTopInner.z += fStepHeight * 0.5f;
      vNextTopOuter.z += fStepHeight * 0.5f;
    }
  }

  const bool isFullCircle = xiiMath::Abs(angle.GetRadian()) >= xiiAngle::MakeFromDegree(360).GetRadian();

  const float fOuterUstep = 3.0f / uiNumSegments;
  for (xiiUInt32 segment = 0; segment < uiNumSegments; ++segment)
  {
    // step values
    {
      nextAngle = angleStep * (segment + 1.0f);

      vCurDirOutwards = vNextDirOutwards;

      vCurBottomOuter = vNextBottomOuter;
      vCurBottomInner = vNextBottomInner;
      vCurTopOuter    = vNextTopOuter;
      vCurTopInner    = vNextTopInner;

      vNextDirOutwards.Set(xiiMath::Cos(nextAngle), xiiMath::Sin(nextAngle), 0);

      vNextBottomOuter.Set(vNextDirOutwards.x * fScaleX, vNextDirOutwards.y * fScaleY, fBottomZ);
      vNextTopOuter.Set(vNextBottomOuter.x, vNextBottomOuter.y, fTopZ);

      const xiiVec3 vNextThickness = vNextDirOutwards * fThickness;
      vNextBottomInner             = vNextBottomOuter - vNextThickness;
      vNextTopInner                = vNextTopOuter - vNextThickness;

      if (bSmoothBottom)
      {
        vCurBottomInner.z -= fStepHeight;
        vCurBottomOuter.z -= fStepHeight;

        vNextBottomInner.z += fStepHeight * 0.5f;
        vNextBottomOuter.z += fStepHeight * 0.5f;
      }

      if (bSmoothTop)
      {
        vCurTopInner.z -= fStepHeight;
        vCurTopOuter.z -= fStepHeight;

        vNextTopInner.z += fStepHeight * 0.5f;
        vNextTopOuter.z += fStepHeight * 0.5f;
      }
    }

    const float fCurOuterU  = segment * fOuterUstep;
    const float fNextOuterU = (1 + segment) * fOuterUstep;

    xiiUInt32 poly[4];

    // Outside
    {
      poly[0] = AddVertex(vCurBottomOuter, vCurDirOutwards, xiiVec2(fCurOuterU, 0), options);
      poly[1] = AddVertex(vNextBottomOuter, vNextDirOutwards, xiiVec2(fNextOuterU, 0), options);
      poly[3] = AddVertex(vCurTopOuter, vCurDirOutwards, xiiVec2(fCurOuterU, 1), options);
      poly[2] = AddVertex(vNextTopOuter, vNextDirOutwards, xiiVec2(fNextOuterU, 1), options);
      AddPolygon(poly, bFlipWinding);
    }

    // Inside
    {
      poly[0] = AddVertex(vCurBottomInner, -vCurDirOutwards, xiiVec2(fCurOuterU, 0), options);
      poly[3] = AddVertex(vNextBottomInner, -vNextDirOutwards, xiiVec2(fNextOuterU, 0), options);
      poly[1] = AddVertex(vCurTopInner, -vCurDirOutwards, xiiVec2(fCurOuterU, 1), options);
      poly[2] = AddVertex(vNextTopInner, -vNextDirOutwards, xiiVec2(fNextOuterU, 1), options);
      AddPolygon(poly, bFlipWinding);
    }

    // Bottom
    if (bCapTopAndBottom)
    {
      poly[0] = AddVertex(vCurBottomInner, xiiVec3(0, 0, -1), vCurBottomInner.GetAsVec2(), options);
      poly[1] = AddVertex(vNextBottomInner, xiiVec3(0, 0, -1), vNextBottomInner.GetAsVec2(), options);
      poly[3] = AddVertex(vCurBottomOuter, xiiVec3(0, 0, -1), vCurBottomOuter.GetAsVec2(), options);
      poly[2] = AddVertex(vNextBottomOuter, xiiVec3(0, 0, -1), vNextBottomOuter.GetAsVec2(), options);
      AddPolygon(poly, bFlipWinding);
    }

    // Top
    if (bCapTopAndBottom)
    {
      poly[0] = AddVertex(vCurTopInner, xiiVec3(0, 0, 1), vCurTopInner.GetAsVec2(), options);
      poly[3] = AddVertex(vNextTopInner, xiiVec3(0, 0, 1), vNextTopInner.GetAsVec2(), options);
      poly[1] = AddVertex(vCurTopOuter, xiiVec3(0, 0, 1), vCurTopOuter.GetAsVec2(), options);
      poly[2] = AddVertex(vNextTopOuter, xiiVec3(0, 0, 1), vNextTopOuter.GetAsVec2(), options);
      AddPolygon(poly, bFlipWinding);
    }

    // Front
    if (bMakeSteps || (!isFullCircle && segment == 0))
    {
      const xiiVec3 vNormal = (bFlipWinding ? -1.0f : 1.0f) * vCurDirOutwards.CrossRH(xiiVec3(0, 0, 1));
      poly[0]               = AddVertex(vCurBottomInner, vNormal, xiiVec2(0, 0), options);
      poly[1]               = AddVertex(vCurBottomOuter, vNormal, xiiVec2(1, 0), options);
      poly[3]               = AddVertex(vCurTopInner, vNormal, xiiVec2(0, 1), options);
      poly[2]               = AddVertex(vCurTopOuter, vNormal, xiiVec2(1, 1), options);
      AddPolygon(poly, bFlipWinding);
    }

    // Back
    if (bMakeSteps || (!isFullCircle && segment == uiNumSegments - 1))
    {
      const xiiVec3 vNormal = (bFlipWinding ? -1.0f : 1.0f) * -vNextDirOutwards.CrossRH(xiiVec3(0, 0, 1));
      poly[0]               = AddVertex(vNextBottomInner, vNormal, xiiVec2(0, 0), options);
      poly[3]               = AddVertex(vNextBottomOuter, vNormal, xiiVec2(1, 0), options);
      poly[1]               = AddVertex(vNextTopInner, vNormal, xiiVec2(0, 1), options);
      poly[2]               = AddVertex(vNextTopOuter, vNormal, xiiVec2(1, 1), options);
      AddPolygon(poly, bFlipWinding);
    }

    if (bMakeSteps)
    {
      vNextTopOuter.z += fStepHeight;
      vNextTopInner.z += fStepHeight;
      vNextBottomOuter.z += fStepHeight;
      vNextBottomInner.z += fStepHeight;

      fBottomZ = fTopZ;
      fTopZ += fStepHeight;
    }
  }
}

XII_STATICLINK_FILE(Core, Core_Graphics_Implementation_Geometry);
