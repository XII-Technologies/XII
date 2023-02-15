#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/Rasterizer/Thirdparty/VectorMath.h>

xiiMutex                                             xiiRasterizerObject::s_Mutex;
xiiMap<xiiString, xiiSharedPtr<xiiRasterizerObject>> xiiRasterizerObject::s_Objects;

xiiRasterizerObject::xiiRasterizerObject()  = default;
xiiRasterizerObject::~xiiRasterizerObject() = default;

#if XII_ENABLED(XII_RASTERIZER_SUPPORTED)

// needed for xiiHybridArray below
XII_DEFINE_AS_POD_TYPE(__m128);

void xiiRasterizerObject::CreateMesh(const xiiGeometry& geo)
{
  xiiHybridArray<__m128, 64, xiiAlignedAllocatorWrapper> vertices;
  vertices.Reserve(geo.GetPolygons().GetCount() * 4);

  Aabb bounds;

  auto addVtx = [&](xiiVec3 vtxPos) {
    xiiSimdVec4f v;
    v.Load<4>(vtxPos.GetAsPositionVec4().GetData());
    vertices.PushBack(v.m_v);
  };

  for (const auto& poly : geo.GetPolygons())
  {
    const xiiUInt32 uiNumVertices = poly.m_Vertices.GetCount();
    xiiUInt32       uiQuadVtx     = 0;

    // ignore complex polygons entirely
    if (uiNumVertices > 4)
      continue;

    for (xiiUInt32 i = 0; i < uiNumVertices; ++i)
    {
      if (uiQuadVtx == 4)
      {
        // TODO: restart next quad (also flip this one's front face)
        break;
      }

      const xiiUInt32 vtxIdx = poly.m_Vertices[i];

      addVtx(geo.GetVertices()[vtxIdx].m_vPosition);

      bounds.include(vertices.PeekBack());
      ++uiQuadVtx;
    }

    // if the polygon is a triangle, duplicate the last vertex to make it a degenerate quad
    if (uiQuadVtx == 3)
    {
      vertices.PushBack(vertices.PeekBack());
      ++uiQuadVtx;
    }

    if (uiQuadVtx == 4)
    {
      const xiiUInt32 n = vertices.GetCount();

      // swap two vertices in the quad to flip the front face (different convention between XII and the rasterizer)
      xiiMath::Swap(vertices[n - 1], vertices[n - 3]);
    }

    XII_ASSERT_DEV(uiQuadVtx == 4, "Degenerate polygon encountered");
  }

  // pad vertices to 32 for proper alignment during baking
  while (vertices.GetCount() % 32 != 0)
  {
    vertices.PushBack(vertices[0]);
  }

  m_Occluder.bake(vertices.GetData(), vertices.GetCount(), bounds.m_min, bounds.m_max);
}

xiiSharedPtr<const xiiRasterizerObject> xiiRasterizerObject::GetObject(xiiStringView sUniqueName)
{
  XII_LOCK(s_Mutex);

  auto it = s_Objects.Find(sUniqueName);

  if (it.IsValid())
    return it.Value();

  return nullptr;
}

xiiSharedPtr<const xiiRasterizerObject> xiiRasterizerObject::CreateBox(const xiiVec3& vFullExtents)
{
  XII_LOCK(s_Mutex);

  xiiStringBuilder sName;
  sName.Format("Box-{}-{}-{}", vFullExtents.x, vFullExtents.y, vFullExtents.z);

  xiiSharedPtr<xiiRasterizerObject>& pObj = s_Objects[sName];

  if (pObj == nullptr)
  {
    pObj = XII_NEW(xiiFoundation::GetAlignedAllocator(), xiiRasterizerObject);

    xiiGeometry geometry;
    geometry.AddBox(vFullExtents, false, {});

    pObj->CreateMesh(geometry);
  }

  return pObj;
}

xiiSharedPtr<const xiiRasterizerObject> xiiRasterizerObject::CreateMesh(xiiStringView sUniqueName, const xiiGeometry& geometry)
{
  XII_LOCK(s_Mutex);

  xiiSharedPtr<xiiRasterizerObject>& pObj = s_Objects[sUniqueName];

  if (pObj == nullptr)
  {
    pObj = XII_NEW(xiiFoundation::GetAlignedAllocator(), xiiRasterizerObject);

    pObj->CreateMesh(geometry);
  }

  return pObj;
}

#else

void xiiRasterizerObject::CreateMesh(const xiiGeometry& geo)
{
}

xiiSharedPtr<const xiiRasterizerObject> xiiRasterizerObject::GetObject(xiiStringView sUniqueName)
{
  return nullptr;
}

xiiSharedPtr<const xiiRasterizerObject> xiiRasterizerObject::CreateBox(const xiiVec3& vFullExtents)
{
  return nullptr;
}

xiiSharedPtr<const xiiRasterizerObject> xiiRasterizerObject::CreateMesh(xiiStringView sUniqueName, const xiiGeometry& geometry)
{
  return nullptr;
}

#endif


XII_STATICLINK_FILE(RendererCore, RendererCore_Rasterizer_Implementation_RasterizerObject);
