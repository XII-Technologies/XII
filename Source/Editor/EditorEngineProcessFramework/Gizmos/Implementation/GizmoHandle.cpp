#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <Utilities/FileFormats/OBJLoader.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGizmoHandle, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Visible", m_bVisible),
    XII_MEMBER_PROPERTY("Transformation", m_Transformation),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEngineGizmoHandle, 1, xiiRTTIDefaultAllocator<xiiEngineGizmoHandle>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("HandleType", m_iHandleType),
    XII_MEMBER_PROPERTY("HandleMesh", m_sGizmoHandleMesh),
    XII_MEMBER_PROPERTY("Color", m_Color),
    XII_MEMBER_PROPERTY("ConstantSize", m_bConstantSize),
    XII_MEMBER_PROPERTY("AlwaysOnTop", m_bAlwaysOnTop),
    XII_MEMBER_PROPERTY("Visualizer", m_bVisualizer),
    XII_MEMBER_PROPERTY("Ortho", m_bShowInOrtho),
    XII_MEMBER_PROPERTY("Pickable", m_bIsPickable),
    XII_MEMBER_PROPERTY("FaceCam", m_bFaceCamera),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGizmoHandle::xiiGizmoHandle()
{
  m_Transformation.SetIdentity();
  m_Transformation.m_vScale.SetZero(); // make sure it is different from anything valid
}

void xiiGizmoHandle::SetVisible(bool bVisible)
{
  if (bVisible != m_bVisible)
  {
    m_bVisible = bVisible;
    SetModified(true);
  }
}

void xiiGizmoHandle::SetTransformation(const xiiTransform& m)
{
  if (m_Transformation != m)
  {
    m_Transformation = m;
    if (m_bVisible)
      SetModified(true);
  }
}

void xiiGizmoHandle::SetTransformation(const xiiMat4& m)
{
  xiiTransform t = xiiTransform::MakeFromMat4(m);
  SetTransformation(t);
}

static xiiMeshBufferResourceHandle CreateMeshBufferResource(xiiGeometry& inout_geom, const char* szResourceName, const char* szDescription, xiiGALPrimitiveTopology::Enum topology)
{
  inout_geom.ComputeFaceNormals();
  inout_geom.ComputeSmoothVertexNormals();

  xiiMeshBufferResourceDescriptor desc;
  desc.AddStream(xiiGALInputLayoutSemantic::Position, xiiGALTextureFormat::RGB32Float);
  desc.AddStream(xiiGALInputLayoutSemantic::Color0, xiiGALTextureFormat::RGBA8UNormalized);
  desc.AddStream(xiiGALInputLayoutSemantic::Normal, xiiGALTextureFormat::RGB32Float);
  desc.AllocateStreamsFromGeometry(inout_geom, topology);
  desc.ComputeBounds();

  return xiiResourceManager::CreateResource<xiiMeshBufferResource>(szResourceName, std::move(desc), szDescription);
}

static xiiMeshBufferResourceHandle CreateMeshBufferArrow()
{
  const char* szResourceName = "{B9DC6776-38D8-4C1F-994F-225E69E71283}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fThickness = 0.02f;
  const float fLength    = 1.0f;

  xiiGeometry::GeoOptions opt;
  opt.m_Transform = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(90));

  xiiGeometry geom;
  geom.AddCylinderOnePiece(fThickness, fThickness, fLength * 0.5f, fLength * 0.5f, 16, opt);

  opt.m_Transform.SetTranslationVector(xiiVec3(fLength * 0.5f, 0, 0));
  geom.AddCone(fThickness * 3.0f, fThickness * 6.0f, true, 16, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Arrow", xiiGALPrimitiveTopology::TriangleList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferPiston()
{
  const char* szResourceName = "{E2B59B8F-8F61-48C0-AE37-CF31107BA2CE}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fThickness = 0.02f;
  const float fLength    = 1.0f;

  xiiGeometry::GeoOptions opt;
  opt.m_Transform = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(90));

  xiiGeometry geom;
  geom.AddCylinderOnePiece(fThickness, fThickness, fLength * 0.5f, fLength * 0.5f, 16, opt);

  opt.m_Transform.SetTranslationVector(xiiVec3(fLength * 0.5f, 0, 0));
  geom.AddBox(xiiVec3(fThickness * 5.0f), false, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Piston", xiiGALPrimitiveTopology::TriangleList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferHalfPiston()
{
  const char* szResourceName = "{BA17D025-B280-4940-8DFD-5486B0E4B41B}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fThickness = 0.04f;
  const float fLength    = 1.0f;

  xiiGeometry::GeoOptions opt;
  opt.m_Transform = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(90));
  opt.m_Transform.SetTranslationVector(xiiVec3(fLength * 0.5f, 0, 0));

  xiiGeometry geom;
  geom.AddCylinderOnePiece(fThickness, fThickness, fLength * 0.5f, fLength * 0.5f, 16, opt);

  opt.m_Transform.SetTranslationVector(xiiVec3(fLength, 0, 0));
  geom.AddBox(xiiVec3(fThickness * 5.0f), false, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_HalfPiston", xiiGALPrimitiveTopology::TriangleList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferRect()
{
  const char* szResourceName = "{75597E89-CDEE-4C90-A377-9441F64B9DB2}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  // weird size because of translate gizmo, should be fixed through scaling there instead
  const float fLength = 2.0f / 3.0f;

  xiiGeometry geom;
  geom.AddRect(xiiVec2(fLength));

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Rect", xiiGALPrimitiveTopology::TriangleList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferLineRect()
{
  const char* szResourceName = "{A1EA52B0-DA73-4176-B50D-3470DDB053F8}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  xiiMat4 m;
  m.SetIdentity();

  xiiGeometry geom;

  const xiiVec2 halfSize(1.0f);

  geom.AddVertex(m, xiiVec3(-halfSize.x, -halfSize.y, 0), xiiVec3(0, 0, 1), xiiVec2(0, 1));
  geom.AddVertex(m, xiiVec3(halfSize.x, -halfSize.y, 0), xiiVec3(0, 0, 1), xiiVec2(0, 0));
  geom.AddVertex(m, xiiVec3(halfSize.x, halfSize.y, 0), xiiVec3(0, 0, 1), xiiVec2(1, 0));
  geom.AddVertex(m, xiiVec3(-halfSize.x, halfSize.y, 0), xiiVec3(0, 0, 1), xiiVec2(1, 1));

  geom.AddLine(0, 1);
  geom.AddLine(1, 2);
  geom.AddLine(2, 3);
  geom.AddLine(3, 0);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_LineRect", xiiGALPrimitiveTopology::LineList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferRing()
{
  const char* szResourceName = "{EA8677E3-F623-4FD8-BFAB-349CE1BEB3CA}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fInnerRadius = 1.3f;
  const float fOuterRadius = fInnerRadius + 0.1f;

  xiiMat4 m;
  m.SetIdentity();

  xiiGeometry geom;
  geom.AddTorus(fInnerRadius, fOuterRadius, 32, 8, false);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Ring", xiiGALPrimitiveTopology::TriangleList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferBox()
{
  const char* szResourceName = "{F14D4CD3-8F21-442B-B07F-3567DBD58A3F}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  xiiGeometry geom;
  geom.AddBox(xiiVec3(1.0f), false);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Box", xiiGALPrimitiveTopology::TriangleList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferLineBox()
{
  const char* szResourceName = "{55DF000E-EE88-4BDC-8A7B-FA496941064E}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  xiiGeometry geom;
  geom.AddLineBox(xiiVec3(1.0f));

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_LineBox", xiiGALPrimitiveTopology::LineList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferSphere()
{
  const char* szResourceName = "{A88779B0-4728-4411-A9D7-532AFE6F4704}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  xiiGeometry geom;
  geom.AddGeodesicSphere(1.0f, 2);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Sphere", xiiGALPrimitiveTopology::TriangleList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferCylinderZ()
{
  const char* szResourceName = "{3BBE2251-0DE4-4B71-979E-A407D8F5CB59}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  xiiGeometry geom;
  geom.AddCylinderOnePiece(1.0f, 1.0f, 0.5f, 0.5f, 16);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_CylinderZ", xiiGALPrimitiveTopology::TriangleList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferHalfSphereZ()
{
  const char* szResourceName = "{05BDED8B-96C1-4F2E-8F1B-5C07B3C28D22}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  xiiGeometry geom;
  geom.AddHalfSphere(1.0f, 16, 8, false);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_HalfSphereZ", xiiGALPrimitiveTopology::TriangleList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferBoxFaces()
{
  const char* szResourceName = "{BD925A8E-480D-41A6-8F62-0AC5F72DA4F6}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  xiiGeometry             geom;
  xiiGeometry::GeoOptions opt;
  opt.m_Transform = xiiMat4::MakeTranslation(xiiVec3(0, 0, 0.5f));

  geom.AddRect(xiiVec2(0.5f), 1, 1, opt);

  opt.m_Transform = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(180.0));
  opt.m_Transform.SetTranslationVector(xiiVec3(0, 0, -0.5f));
  geom.AddRect(xiiVec2(0.5f), 1, 1, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_BoxFaces", xiiGALPrimitiveTopology::TriangleList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferBoxEdges()
{
  const char* szResourceName = "{FE700F28-514E-4193-A0F6-4351E0BAC222}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  xiiMat4 rot;

  xiiGeometry             geom;
  xiiGeometry::GeoOptions opt;

  for (xiiUInt32 i = 0; i < 4; ++i)
  {
    rot = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(90.0f * i));

    opt.m_Transform = xiiMat4::MakeTranslation(xiiVec3(0.5f - 0.125f, 0, 0.5f));
    opt.m_Transform = rot * opt.m_Transform;
    geom.AddRect(xiiVec2(0.25f, 0.5f), 1, 1, opt);

    opt.m_Transform = xiiMat4::MakeTranslation(xiiVec3(-0.5f + 0.125f, 0, 0.5f));
    geom.AddRect(xiiVec2(0.25f, 0.5f), 1, 1, opt);
  }

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_BoxEdges", xiiGALPrimitiveTopology::TriangleList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferBoxCorners()
{
  const char* szResourceName = "{FBDB6A82-D4B0-447F-815B-228D340451CB}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  xiiMat4 rot[6];
  rot[0].SetIdentity();
  rot[1] = xiiMat4::MakeRotationX(xiiAngle::MakeFromDegree(90));
  rot[2] = xiiMat4::MakeRotationX(xiiAngle::MakeFromDegree(180));
  rot[3] = xiiMat4::MakeRotationX(xiiAngle::MakeFromDegree(270));
  rot[4] = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(90));
  rot[5] = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(-90));

  xiiGeometry             geom;
  xiiGeometry::GeoOptions opt;

  for (xiiUInt32 i = 0; i < 6; ++i)
  {
    opt.m_Transform = xiiMat4::MakeTranslation(xiiVec3(0.5f - 0.125f, 0.5f - 0.125f, 0.5f));
    opt.m_Transform = rot[i] * opt.m_Transform;
    geom.AddRect(xiiVec2(0.25f, 0.25f), 1, 1, opt);

    opt.m_Transform = xiiMat4::MakeTranslation(xiiVec3(0.5f - 0.125f, -0.5f + 0.125f, 0.5f));
    opt.m_Transform = rot[i] * opt.m_Transform;
    geom.AddRect(xiiVec2(0.25f, 0.25f), 1, 1, opt);

    opt.m_Transform = xiiMat4::MakeTranslation(xiiVec3(-0.5f + 0.125f, 0.5f - 0.125f, 0.5f));
    opt.m_Transform = rot[i] * opt.m_Transform;
    geom.AddRect(xiiVec2(0.25f, 0.25f), 1, 1, opt);

    opt.m_Transform = xiiMat4::MakeTranslation(xiiVec3(-0.5f + 0.125f, -0.5f + 0.125f, 0.5f));
    opt.m_Transform = rot[i] * opt.m_Transform;
    geom.AddRect(xiiVec2(0.25f, 0.25f), 1, 1, opt);
  }

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_BoxCorners", xiiGALPrimitiveTopology::TriangleList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferCone()
{
  const char* szResourceName = "{BED97C9E-4E7A-486C-9372-1FB1A5FAE786}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  xiiGeometry::GeoOptions opt;
  opt.m_Transform = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(270.0f));
  opt.m_Transform.SetTranslationVector(xiiVec3(1.0f, 0, 0));

  xiiGeometry geom;
  geom.AddCone(1.0f, 1.0f, false, 16, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Cone", xiiGALPrimitiveTopology::TriangleList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferFrustum()
{
  const char* szResourceName = "{61A7BE38-797D-4BFC-AED6-33CE4F4C6FF6}";

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  xiiMat4 m;
  m.SetIdentity();

  xiiGeometry geom;

  geom.AddVertex(m, xiiVec3(0, 0, 0), xiiVec3(0, 0, 1));

  geom.AddVertex(m, xiiVec3(1.0f, -1.0f, 1.0f), xiiVec3(0, 0, 1));
  geom.AddVertex(m, xiiVec3(1.0f, 1.0f, 1.0f), xiiVec3(0, 0, 1));
  geom.AddVertex(m, xiiVec3(1.0f, -1.0f, -1.0f), xiiVec3(0, 0, 1));
  geom.AddVertex(m, xiiVec3(1.0f, 1.0f, -1.0f), xiiVec3(0, 0, 1));

  geom.AddLine(0, 1);
  geom.AddLine(0, 2);
  geom.AddLine(0, 3);
  geom.AddLine(0, 4);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Frustum", xiiGALPrimitiveTopology::LineList);
}

static xiiMeshBufferResourceHandle CreateMeshBufferFromFile(const char* szFile)
{
  const char* szResourceName = szFile;

  xiiMeshBufferResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  xiiOBJLoader obj;
  obj.LoadOBJ(szFile, true).AssertSuccess("Couldn't load gizmo model '{}'", szFile);

  xiiMat4 m;
  m.SetIdentity();

  xiiGeometry geom;
  for (xiiUInt32 v = 0; v < obj.m_Positions.GetCount(); ++v)
  {
    geom.AddVertex(obj.m_Positions[v], xiiVec3::MakeZero(), xiiVec2::MakeZero(), xiiColor::White);
  }

  xiiStaticArray<xiiUInt32, 3> triangle;
  triangle.SetCount(3);
  for (xiiUInt32 f = 0; f < obj.m_Faces.GetCount(); ++f)
  {
    triangle[0] = obj.m_Faces[f].m_Vertices[0].m_uiPositionID;
    triangle[1] = obj.m_Faces[f].m_Vertices[1].m_uiPositionID;
    triangle[2] = obj.m_Faces[f].m_Vertices[2].m_uiPositionID;

    geom.AddPolygon(triangle, false);
  }

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_FromFile", xiiGALPrimitiveTopology::TriangleList);
}

static xiiMeshResourceHandle CreateMeshResource(const char* szMeshResourceName, xiiMeshBufferResourceHandle hMeshBuffer, const char* szMaterial)
{
  const xiiStringBuilder sIdentifier(szMeshResourceName, "-with-", szMaterial);

  xiiMeshResourceHandle hMesh = xiiResourceManager::GetExistingResource<xiiMeshResource>(sIdentifier);

  if (hMesh.IsValid())
    return hMesh;

  xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(hMeshBuffer, xiiResourceAcquireMode::AllowLoadingFallback);

  xiiMeshResourceDescriptor md;
  md.UseExistingMeshBuffer(hMeshBuffer);
  md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
  md.SetMaterial(0, szMaterial);
  md.ComputeBounds();

  return xiiResourceManager::GetOrCreateResource<xiiMeshResource>(sIdentifier, std::move(md), pMeshBuffer->GetResourceDescription());
}

xiiEngineGizmoHandle::xiiEngineGizmoHandle() = default;

xiiEngineGizmoHandle::~xiiEngineGizmoHandle()
{
  if (m_hGameObject.IsInvalidated())
    return;

  m_pWorld->DeleteObjectDelayed(m_hGameObject);
}

void xiiEngineGizmoHandle::ConfigureHandle(xiiGizmo* pParentGizmo, xiiEngineGizmoHandleType type, const xiiColor& col, xiiBitflags<xiiGizmoFlags> flags, const char* szCustomMesh)
{
  SetParentGizmo(pParentGizmo);

  m_iHandleType      = (int)type;
  m_sGizmoHandleMesh = szCustomMesh;
  m_Color            = col;

  m_bConstantSize = flags.IsSet(xiiGizmoFlags::ConstantSize);
  m_bAlwaysOnTop  = flags.IsSet(xiiGizmoFlags::OnTop);
  m_bVisualizer   = flags.IsSet(xiiGizmoFlags::Visualizer);
  m_bShowInOrtho  = flags.IsSet(xiiGizmoFlags::ShowInOrtho);
  m_bIsPickable   = flags.IsSet(xiiGizmoFlags::Pickable);
  m_bFaceCamera   = flags.IsSet(xiiGizmoFlags::FaceCamera);
}

bool xiiEngineGizmoHandle::SetupForEngine(xiiWorld* pWorld, xiiUInt32 uiNextComponentPickingID)
{
  m_pWorld = pWorld;

  if (!m_hGameObject.IsInvalidated())
    return false;

  xiiMeshBufferResourceHandle hMeshBuffer;
  const char*                 szMeshGuid = "";

  switch (m_iHandleType)
  {
    case xiiEngineGizmoHandleType::Arrow:
    {
      hMeshBuffer = CreateMeshBufferArrow();
      szMeshGuid  = "{9D02CF27-7A15-44EA-A372-C417AF2A8E9B}";
    }
    break;
    case xiiEngineGizmoHandleType::Rect:
    {
      hMeshBuffer = CreateMeshBufferRect();
      szMeshGuid  = "{3DF4DDDA-F598-4A37-9691-D4C3677905A8}";
    }
    break;
    case xiiEngineGizmoHandleType::LineRect:
    {
      hMeshBuffer = CreateMeshBufferLineRect();
      szMeshGuid  = "{96129543-897C-4DEE-922D-931BC91C5725}";
    }
    break;
    case xiiEngineGizmoHandleType::Ring:
    {
      hMeshBuffer = CreateMeshBufferRing();
      szMeshGuid  = "{629AD0C6-C81B-4850-A5BC-41494DC0BF95}";
    }
    break;
    case xiiEngineGizmoHandleType::Box:
    {
      hMeshBuffer = CreateMeshBufferBox();
      szMeshGuid  = "{13A59253-4A98-4638-8B94-5AA370E929A7}";
    }
    break;
    case xiiEngineGizmoHandleType::Piston:
    {
      hMeshBuffer = CreateMeshBufferPiston();
      szMeshGuid  = "{44A4FE37-6AE3-44C1-897D-E8B95AE53EF6}";
    }
    break;
    case xiiEngineGizmoHandleType::HalfPiston:
    {
      hMeshBuffer = CreateMeshBufferHalfPiston();
      szMeshGuid  = "{64A45DD0-D7F9-4D1D-9F68-782FA3274200}";
    }
    break;
    case xiiEngineGizmoHandleType::Sphere:
    {
      hMeshBuffer = CreateMeshBufferSphere();
      szMeshGuid  = "{FC322E80-5EB0-452F-9D8E-9E65FCFDA652}";
    }
    break;
    case xiiEngineGizmoHandleType::CylinderZ:
    {
      hMeshBuffer = CreateMeshBufferCylinderZ();
      szMeshGuid  = "{893384EA-2F43-4265-AF75-662E2C81C167}";
    }
    break;
    case xiiEngineGizmoHandleType::HalfSphereZ:
    {
      hMeshBuffer = CreateMeshBufferHalfSphereZ();
      szMeshGuid  = "{0FC9B680-7B6B-40B6-97BD-CBFFA47F0EFF}";
    }
    break;
    case xiiEngineGizmoHandleType::BoxCorners:
    {
      hMeshBuffer = CreateMeshBufferBoxCorners();
      szMeshGuid  = "{89CCC389-11D5-43F4-9C18-C634EE3154B9}";
    }
    break;
    case xiiEngineGizmoHandleType::BoxEdges:
    {
      hMeshBuffer = CreateMeshBufferBoxEdges();
      szMeshGuid  = "{21508253-2E74-44CE-9399-523214BE7C3D}";
    }
    break;
    case xiiEngineGizmoHandleType::BoxFaces:
    {
      hMeshBuffer = CreateMeshBufferBoxFaces();
      szMeshGuid  = "{FD1A3C29-F8F0-42B0-BBB0-D0A2B28A65A0}";
    }
    break;
    case xiiEngineGizmoHandleType::LineBox:
    {
      hMeshBuffer = CreateMeshBufferLineBox();
      szMeshGuid  = "{4B136D72-BF43-4C4B-96D7-51C5028A7006}";
    }
    break;
    case xiiEngineGizmoHandleType::Cone:
    {
      hMeshBuffer = CreateMeshBufferCone();
      szMeshGuid  = "{9A48962D-127A-445C-899A-A054D6AD8A9A}";
    }
    break;
    case xiiEngineGizmoHandleType::Frustum:
    {
      szMeshGuid  = "{22EC5D48-E8BE-410B-8EAD-51B7775BA058}";
      hMeshBuffer = CreateMeshBufferFrustum();
    }
    break;
    case xiiEngineGizmoHandleType::FromFile:
    {
      szMeshGuid  = m_sGizmoHandleMesh;
      hMeshBuffer = CreateMeshBufferFromFile(m_sGizmoHandleMesh);
    }
    break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  xiiStringBuilder sName;
  sName.SetFormat("Gizmo{0}", m_iHandleType);

  xiiGameObjectDesc god;
  god.m_LocalPosition = m_Transformation.m_vPosition;
  god.m_LocalRotation = m_Transformation.m_qRotation;
  god.m_LocalScaling  = m_Transformation.m_vScale;
  god.m_sName.Assign(sName.GetData());
  god.m_bDynamic = true;

  xiiGameObject* pObject;
  m_hGameObject = pWorld->CreateObject(god, pObject);

  if (!m_bShowInOrtho)
  {
    const xiiTag& tagNoOrtho = xiiTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode");

    pObject->SetTag(tagNoOrtho);
  }

  {
    const xiiTag& tagEditor = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

    pObject->SetTag(tagEditor);
  }

  xiiGizmoComponent::CreateComponent(pObject, m_pGizmoComponent);


  xiiMeshResourceHandle hMesh;

  if (m_bVisualizer)
  {
    hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Editor/Materials/Visualizer.xiiMaterial");
  }
  else if (m_bConstantSize)
  {
    if (m_bFaceCamera)
    {
      hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Editor/Materials/GizmoHandleConstantSizeCamFacing.xiiMaterial");
    }
    else
    {
      hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Editor/Materials/GizmoHandleConstantSize.xiiMaterial");
    }
  }
  else
  {
    hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Editor/Materials/GizmoHandle.xiiMaterial");
  }

  m_pGizmoComponent->m_GizmoColor  = m_Color;
  m_pGizmoComponent->m_bIsPickable = m_bIsPickable;
  m_pGizmoComponent->SetMesh(hMesh);

  m_pGizmoComponent->SetUniqueID(uiNextComponentPickingID);

  return true;
}

void xiiEngineGizmoHandle::UpdateForEngine(xiiWorld* pWorld)
{
  if (m_hGameObject.IsInvalidated())
    return;

  xiiGameObject* pObject;
  if (!pWorld->TryGetObject(m_hGameObject, pObject))
    return;

  pObject->SetLocalPosition(m_Transformation.m_vPosition);
  pObject->SetLocalRotation(m_Transformation.m_qRotation);
  pObject->SetLocalScaling(m_Transformation.m_vScale);

  m_pGizmoComponent->m_GizmoColor = m_Color;
  m_pGizmoComponent->SetActiveFlag(m_bVisible);
}

void xiiEngineGizmoHandle::SetColor(const xiiColor& col)
{
  m_Color = col;
  SetModified();
}
