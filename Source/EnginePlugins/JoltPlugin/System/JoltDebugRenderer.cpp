#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/System/JoltDebugRenderer.h>

#ifdef JPH_DEBUG_RENDERER

#  include <Jolt/Renderer/DebugRenderer.h>

void xiiJoltDebugRenderer::TriangleBatch::AddRef()
{
  ++m_iRefCount;
}

void xiiJoltDebugRenderer::TriangleBatch::Release()
{
  --m_iRefCount;

  if (m_iRefCount == 0)
  {
    auto* pThis = this;
    XII_DEFAULT_DELETE(pThis);
  }
}

xiiJoltDebugRenderer::xiiJoltDebugRenderer()
{
  Initialize();
}

void xiiJoltDebugRenderer::DrawLine(const JPH::Float3& inFrom, const JPH::Float3& inTo, JPH::ColorArg inColor)
{
  auto& l        = m_Lines.ExpandAndGetRef();
  l.m_start      = xiiJoltConversionUtils::ToVec3(inFrom);
  l.m_end        = xiiJoltConversionUtils::ToVec3(inTo);
  l.m_startColor = xiiJoltConversionUtils::ToColor(inColor);
  l.m_endColor   = l.m_startColor;
}


void xiiJoltDebugRenderer::DrawTriangle(JPH::Vec3Arg inV1, JPH::Vec3Arg inV2, JPH::Vec3Arg inV3, JPH::ColorArg inColor)
{
  auto& t         = m_Triangles.ExpandAndGetRef();
  t.m_position[0] = xiiJoltConversionUtils::ToVec3(inV1);
  t.m_position[1] = xiiJoltConversionUtils::ToVec3(inV2);
  t.m_position[2] = xiiJoltConversionUtils::ToVec3(inV3);
  t.m_color       = xiiJoltConversionUtils::ToColor(inColor);
}


JPH::DebugRenderer::Batch xiiJoltDebugRenderer::CreateTriangleBatch(const JPH::DebugRenderer::Triangle* inTriangles, int inTriangleCount)
{
  TriangleBatch* pBatch = XII_DEFAULT_NEW(TriangleBatch);
  pBatch->m_Triangles.Reserve(inTriangleCount);

  for (int i = 0; i < inTriangleCount; ++i)
  {
    auto& t         = pBatch->m_Triangles.ExpandAndGetRef();
    t.m_position[0] = xiiJoltConversionUtils::ToVec3(inTriangles[i].mV[0].mPosition);
    t.m_position[1] = xiiJoltConversionUtils::ToVec3(inTriangles[i].mV[1].mPosition);
    t.m_position[2] = xiiJoltConversionUtils::ToVec3(inTriangles[i].mV[2].mPosition);
    t.m_color       = xiiJoltConversionUtils::ToColor(inTriangles[i].mV[0].mColor);
  }

  return pBatch;
}


JPH::DebugRenderer::Batch xiiJoltDebugRenderer::CreateTriangleBatch(const JPH::DebugRenderer::Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount)
{
  const xiiUInt32 numTris = inIndexCount / 3;

  TriangleBatch* pBatch = XII_DEFAULT_NEW(TriangleBatch);
  pBatch->m_Triangles.Reserve(numTris);

  xiiUInt32 index = 0;

  for (xiiUInt32 i = 0; i < numTris; ++i)
  {
    auto& t         = pBatch->m_Triangles.ExpandAndGetRef();
    t.m_position[0] = xiiJoltConversionUtils::ToVec3(inVertices[inIndices[index + 0]].mPosition);
    t.m_position[1] = xiiJoltConversionUtils::ToVec3(inVertices[inIndices[index + 1]].mPosition);
    t.m_position[2] = xiiJoltConversionUtils::ToVec3(inVertices[inIndices[index + 2]].mPosition);
    t.m_color       = xiiJoltConversionUtils::ToColor(inVertices[inIndices[index + 0]].mColor);

    index += 3;
  }

  return pBatch;
}


void xiiJoltDebugRenderer::DrawGeometry(JPH::Mat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode /*= ECullMode::CullBackFace*/, ECastShadow inCastShadow /*= ECastShadow::On*/, EDrawMode inDrawMode /*= EDrawMode::Solid*/)
{
  if (inGeometry == nullptr)
    return;

  xiiUInt32 uiLod = 0;
  if (inGeometry->mLODs.size() > 1)
    uiLod = 1;
  if (inGeometry->mLODs.size() > 2)
    uiLod = 2;

  const TriangleBatch* pBatch = static_cast<const TriangleBatch*>(inGeometry->mLODs[uiLod].mTriangleBatch.GetPtr());

  const xiiMat4  trans = reinterpret_cast<const xiiMat4&>(inModelMatrix);
  const xiiColor color = xiiJoltConversionUtils::ToColor(inModelColor);

  if (inDrawMode == JPH::DebugRenderer::EDrawMode::Solid)
  {
    m_Triangles.Reserve(m_Triangles.GetCount() + pBatch->m_Triangles.GetCount() * ((inCullMode == JPH::DebugRenderer::ECullMode::Off) ? 2 : 1));

    if (inCullMode == JPH::DebugRenderer::ECullMode::CullBackFace || inCullMode == JPH::DebugRenderer::ECullMode::Off)
    {
      for (xiiUInt32 t = 0; t < pBatch->m_Triangles.GetCount(); ++t)
      {
        auto& tri         = m_Triangles.ExpandAndGetRef();
        tri.m_color       = pBatch->m_Triangles[t].m_color * color;
        tri.m_position[0] = trans * pBatch->m_Triangles[t].m_position[0];
        tri.m_position[1] = trans * pBatch->m_Triangles[t].m_position[1];
        tri.m_position[2] = trans * pBatch->m_Triangles[t].m_position[2];
      }
    }

    if (inCullMode == JPH::DebugRenderer::ECullMode::CullFrontFace || inCullMode == JPH::DebugRenderer::ECullMode::Off)
    {
      for (xiiUInt32 t = 0; t < pBatch->m_Triangles.GetCount(); ++t)
      {
        auto& tri         = m_Triangles.ExpandAndGetRef();
        tri.m_color       = pBatch->m_Triangles[t].m_color * color;
        tri.m_position[0] = trans * pBatch->m_Triangles[t].m_position[0];
        tri.m_position[1] = trans * pBatch->m_Triangles[t].m_position[2];
        tri.m_position[2] = trans * pBatch->m_Triangles[t].m_position[1];
      }
    }
  }
  else
  {
    m_Lines.Reserve(m_Lines.GetCount() + pBatch->m_Triangles.GetCount() * 3);

    for (xiiUInt32 t = 0; t < pBatch->m_Triangles.GetCount(); ++t)
    {
      const auto&    inTri = pBatch->m_Triangles[t];
      const xiiColor col   = pBatch->m_Triangles[t].m_color * color;

      const xiiVec3 v0 = trans * inTri.m_position[0];
      const xiiVec3 v1 = trans * inTri.m_position[1];
      const xiiVec3 v2 = trans * inTri.m_position[2];

      m_Lines.PushBack({v0, v1, col});
      m_Lines.PushBack({v1, v2, col});
      m_Lines.PushBack({v2, v0, col});
    }
  }
}

#endif
