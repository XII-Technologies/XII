#include <GameEngine/GameEnginePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>
#include <GraphicsCore/Meshes/CpuMeshResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGreyBoxShape, 1)
  XII_ENUM_CONSTANTS(xiiGreyBoxShape::Box, xiiGreyBoxShape::RampX, xiiGreyBoxShape::RampY, xiiGreyBoxShape::Column)
  XII_ENUM_CONSTANTS(xiiGreyBoxShape::StairsX, xiiGreyBoxShape::StairsY, xiiGreyBoxShape::ArchX, xiiGreyBoxShape::ArchY, xiiGreyBoxShape::SpiralStairs)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_COMPONENT_TYPE(xiiGreyBoxComponent, 5, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_ACCESSOR_PROPERTY("Shape", xiiGreyBoxShape, GetShape, SetShape),
    XII_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::White), new xiiExposeColorAlphaAttribute()),
    XII_ACCESSOR_PROPERTY("SizeNegX", GetSizeNegX, SetSizeNegX)->AddAttributes(new xiiGroupAttribute("Size", "Size")),//->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("SizePosX", GetSizePosX, SetSizePosX),//->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("SizeNegY", GetSizeNegY, SetSizeNegY),//->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("SizePosY", GetSizePosY, SetSizePosY),//->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("SizeNegZ", GetSizeNegZ, SetSizeNegZ),//->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("SizePosZ", GetSizePosZ, SetSizePosZ),//->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Detail", GetDetail, SetDetail)->AddAttributes(new xiiGroupAttribute("Misc"), new xiiDefaultValueAttribute(16), new xiiClampValueAttribute(3, 32)),
    XII_ACCESSOR_PROPERTY("Curvature", GetCurvature, SetCurvature)->AddAttributes(new xiiClampValueAttribute(xiiAngle::MakeFromDegree(-360), xiiAngle::MakeFromDegree(360))),
    XII_ACCESSOR_PROPERTY("Thickness", GetThickness, SetThickness)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("SlopedTop", GetSlopedTop, SetSlopedTop),
    XII_ACCESSOR_PROPERTY("SlopedBottom", GetSlopedBottom, SetSlopedBottom),
    XII_ACCESSOR_PROPERTY("GenerateCollision", GetGenerateCollision, SetGenerateCollision)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("IncludeInNavmesh", GetIncludeInNavmesh, SetIncludeInNavmesh)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("UseAsOccluder", m_bUseAsOccluder)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Construction"),
    new xiiNonUniformBoxManipulatorAttribute("SizeNegX", "SizePosX", "SizeNegY", "SizePosY", "SizeNegZ", "SizePosZ"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
    XII_MESSAGE_HANDLER(xiiMsgBuildStaticMesh, OnBuildStaticMesh),
    XII_MESSAGE_HANDLER(xiiMsgExtractGeometry, OnMsgExtractGeometry),
    XII_MESSAGE_HANDLER(xiiMsgExtractOccluderData, OnMsgExtractOccluderData),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiGreyBoxComponent::xiiGreyBoxComponent()  = default;
xiiGreyBoxComponent::~xiiGreyBoxComponent() = default;

void xiiGreyBoxComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  xiiStreamWriter& s = stream.GetStream();

  s << m_Shape;
  s << m_hMaterial;
  s << m_fSizeNegX;
  s << m_fSizePosX;
  s << m_fSizeNegY;
  s << m_fSizePosY;
  s << m_fSizeNegZ;
  s << m_fSizePosZ;
  s << m_uiDetail;

  // Version 2
  s << m_Curvature;
  s << m_fThickness;
  s << m_bSlopedTop;
  s << m_bSlopedBottom;

  // Version 3
  s << m_Color;

  // Version 4
  s << m_bGenerateCollision;
  s << m_bIncludeInNavmesh;

  // Version 5
  s << m_bUseAsOccluder;
}

void xiiGreyBoxComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32  uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = stream.GetStream();

  s >> m_Shape;
  s >> m_hMaterial;
  s >> m_fSizeNegX;
  s >> m_fSizePosX;
  s >> m_fSizeNegY;
  s >> m_fSizePosY;
  s >> m_fSizeNegZ;
  s >> m_fSizePosZ;
  s >> m_uiDetail;

  if (uiVersion >= 2)
  {
    s >> m_Curvature;
    s >> m_fThickness;
    s >> m_bSlopedTop;
    s >> m_bSlopedBottom;
  }

  if (uiVersion >= 3)
  {
    s >> m_Color;
  }

  if (uiVersion >= 4)
  {
    s >> m_bGenerateCollision;
    s >> m_bIncludeInNavmesh;
  }

  if (uiVersion >= 5)
  {
    s >> m_bUseAsOccluder;
  }
}

void xiiGreyBoxComponent::OnActivated()
{
  if (!m_hMesh.IsValid())
  {
    m_hMesh = GenerateMesh<xiiMeshResource>();
  }

  // First generate the mesh and then call the base implementation which will update the bounds
  SUPER::OnActivated();
}

xiiResult xiiGreyBoxComponent::GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg)
{
  if (m_hMesh.IsValid())
  {
    xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
    bounds = pMesh->GetBounds();

    if (m_bUseAsOccluder)
    {
      msg.AddBounds(bounds, GetOwner()->IsStatic() ? xiiDefaultSpatialDataCategories::OcclusionStatic : xiiDefaultSpatialDataCategories::OcclusionDynamic);
    }

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiGreyBoxComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const xiiUInt32 uiFlipWinding  = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;
  const xiiUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  xiiResourceLock<xiiMeshResource>                      pMesh(m_hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
  xiiArrayPtr<const xiiMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (xiiUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const xiiUInt32           uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    xiiMaterialResourceHandle hMaterial       = m_hMaterial.IsValid() ? m_hMaterial : pMesh->GetMaterials()[uiMaterialIndex];

    xiiMeshRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiMeshRenderData>(GetOwner());
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds    = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh           = m_hMesh;
      pRenderData->m_hMaterial       = hMaterial;
      pRenderData->m_Color           = m_Color;

      pRenderData->m_uiSubMeshIndex = uiPartIndex;
      pRenderData->m_uiFlipWinding  = uiFlipWinding;
      pRenderData->m_uiUniformScale = uiUniformScale;

      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillBatchIdAndSortingKey();
    }

    bool bDontCacheYet = false;

    // Determine render data category.
    xiiRenderData::Category category = xiiDefaultRenderDataCategories::LitOpaque;

    if (hMaterial.IsValid())
    {
      xiiResourceLock<xiiMaterialResource> pMaterial(hMaterial, xiiResourceAcquireMode::AllowLoadingFallback);

      if (pMaterial.GetAcquireResult() == xiiResourceAcquireResult::LoadingFallback)
        bDontCacheYet = true;

      category = pMaterial->GetRenderDataCategory();
    }

    msg.AddRenderData(pRenderData, category, bDontCacheYet ? xiiRenderData::Caching::Never : xiiRenderData::Caching::IfStatic);
  }
}

void xiiGreyBoxComponent::SetShape(xiiEnum<xiiGreyBoxShape> shape)
{
  m_Shape = shape;
  InvalidateMesh();
}

void xiiGreyBoxComponent::SetMaterialFile(const char* szFile)
{
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    m_hMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>(szFile);
  }
  else
  {
    m_hMaterial.Invalidate();
  }
}

const char* xiiGreyBoxComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void xiiGreyBoxComponent::SetSizeNegX(float f)
{
  m_fSizeNegX = f;
  InvalidateMesh();
}

void xiiGreyBoxComponent::SetSizePosX(float f)
{
  m_fSizePosX = f;
  InvalidateMesh();
}

void xiiGreyBoxComponent::SetSizeNegY(float f)
{
  m_fSizeNegY = f;
  InvalidateMesh();
}

void xiiGreyBoxComponent::SetSizePosY(float f)
{
  m_fSizePosY = f;
  InvalidateMesh();
}

void xiiGreyBoxComponent::SetSizeNegZ(float f)
{
  m_fSizeNegZ = f;
  InvalidateMesh();
}

void xiiGreyBoxComponent::SetSizePosZ(float f)
{
  m_fSizePosZ = f;
  InvalidateMesh();
}

void xiiGreyBoxComponent::SetDetail(xiiUInt32 uiDetail)
{
  m_uiDetail = uiDetail;
  InvalidateMesh();
}

void xiiGreyBoxComponent::SetCurvature(xiiAngle curvature)
{
  m_Curvature = xiiAngle::MakeFromDegree(xiiMath::RoundToMultiple(curvature.GetDegree(), 5.0f));
  InvalidateMesh();
}

void xiiGreyBoxComponent::SetSlopedTop(bool b)
{
  m_bSlopedTop = b;
  InvalidateMesh();
}

void xiiGreyBoxComponent::SetSlopedBottom(bool b)
{
  m_bSlopedBottom = b;
  InvalidateMesh();
}

void xiiGreyBoxComponent::SetThickness(float f)
{
  m_fThickness = f;
  InvalidateMesh();
}

void xiiGreyBoxComponent::SetGenerateCollision(bool b)
{
  m_bGenerateCollision = b;
}

void xiiGreyBoxComponent::SetIncludeInNavmesh(bool b)
{
  m_bIncludeInNavmesh = b;
}

void xiiGreyBoxComponent::OnBuildStaticMesh(xiiMsgBuildStaticMesh& msg) const
{
  if (!m_bGenerateCollision)
    return;

  xiiGeometry geom;
  BuildGeometry(geom, m_Shape, false);
  geom.TriangulatePolygons();

  auto* pDesc               = msg.m_pStaticMeshDescription;
  auto& subMesh             = pDesc->m_SubMeshes.ExpandAndGetRef();
  subMesh.m_uiFirstTriangle = pDesc->m_Triangles.GetCount();

  const xiiTransform t = GetOwner()->GetGlobalTransform();

  const xiiUInt32 uiTriOffset = pDesc->m_Vertices.GetCount();

  for (const auto& verts : geom.GetVertices())
  {
    pDesc->m_Vertices.PushBack(t * verts.m_vPosition);
  }

  for (const auto& polys : geom.GetPolygons())
  {
    auto& tri                = pDesc->m_Triangles.ExpandAndGetRef();
    tri.m_uiVertexIndices[0] = uiTriOffset + polys.m_Vertices[0];
    tri.m_uiVertexIndices[1] = uiTriOffset + polys.m_Vertices[1];
    tri.m_uiVertexIndices[2] = uiTriOffset + polys.m_Vertices[2];
  }

  subMesh.m_uiNumTriangles = pDesc->m_Triangles.GetCount() - subMesh.m_uiFirstTriangle;

  xiiMaterialResourceHandle hMaterial = m_hMaterial;
  if (!hMaterial.IsValid())
  {
    // Data/Base/Materials/Common/Pattern.xiiMaterialAsset
    hMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>("{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");
  }

  if (hMaterial.IsValid())
  {
    xiiResourceLock<xiiMaterialResource> pMaterial(hMaterial, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pMaterial.GetAcquireResult() == xiiResourceAcquireResult::Final)
    {
      const xiiString surface = pMaterial->GetSurface().GetString();

      if (!surface.IsEmpty())
      {
        xiiUInt32 idx = pDesc->m_Surfaces.IndexOf(surface);
        if (idx == xiiInvalidIndex)
        {
          idx = pDesc->m_Surfaces.GetCount();
          pDesc->m_Surfaces.PushBack(surface);
        }

        subMesh.m_uiSurfaceIndex = static_cast<xiiUInt16>(idx);
      }
    }
  }
}

void xiiGreyBoxComponent::OnMsgExtractGeometry(xiiMsgExtractGeometry& msg) const
{
  if (msg.m_Mode == xiiWorldGeoExtractionUtil::ExtractionMode::CollisionMesh && (m_bGenerateCollision == false || GetOwner()->IsDynamic()))
    return;

  if (msg.m_Mode == xiiWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration && (m_bIncludeInNavmesh == false || GetOwner()->IsDynamic()))
    return;

  msg.AddMeshObject(GetOwner()->GetGlobalTransform(), GenerateMesh<xiiCpuMeshResource>());
}

void xiiGreyBoxComponent::OnMsgExtractOccluderData(xiiMsgExtractOccluderData& msg) const
{
  if (!IsActiveAndInitialized() || !m_bUseAsOccluder)
    return;

  if (m_pOccluderObject == nullptr)
  {
    xiiEnum<xiiGreyBoxShape> shape = m_Shape;
    if (shape == xiiGreyBoxShape::StairsX && m_Curvature == xiiAngle())
      shape = xiiGreyBoxShape::RampX;
    if (shape == xiiGreyBoxShape::StairsY && m_Curvature == xiiAngle())
      shape = xiiGreyBoxShape::RampY;

    xiiStringBuilder sResourceName;
    GenerateMeshName(sResourceName);

    m_pOccluderObject = xiiRasterizerObject::GetObject(sResourceName);

    if (m_pOccluderObject == nullptr)
    {
      xiiGeometry geom;
      BuildGeometry(geom, shape, true);

      m_pOccluderObject = xiiRasterizerObject::CreateMesh(sResourceName, geom);
    }
  }

  msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform());
}

void xiiGreyBoxComponent::InvalidateMesh()
{
  m_pOccluderObject = nullptr;

  if (m_hMesh.IsValid())
  {
    m_hMesh.Invalidate();

    m_hMesh = GenerateMesh<xiiMeshResource>();

    TriggerLocalBoundsUpdate();
  }
}

void xiiGreyBoxComponent::BuildGeometry(xiiGeometry& geom, xiiEnum<xiiGreyBoxShape> shape, bool bOnlyRoughDetails) const
{
  xiiGeometry::GeoOptions opt;
  opt.m_Color = m_Color;

  xiiVec3 size;
  size.x = m_fSizeNegX + m_fSizePosX;
  size.y = m_fSizeNegY + m_fSizePosY;
  size.z = m_fSizeNegZ + m_fSizePosZ;

  if (size.x == 0 || size.y == 0 || size.z == 0)
  {
    // create a tiny dummy box, so that we have valid geometry
    geom.AddBox(xiiVec3(0.01f), true, opt);
    return;
  }

  xiiVec3 offset(0);
  offset.x = (m_fSizePosX - m_fSizeNegX) * 0.5f;
  offset.y = (m_fSizePosY - m_fSizeNegY) * 0.5f;
  offset.z = (m_fSizePosZ - m_fSizeNegZ) * 0.5f;

  xiiMat4 t2, t3;

  opt.m_Transform = xiiMat4::MakeTranslation(offset);

  switch (shape)
  {
    case xiiGreyBoxShape::Box:
      geom.AddBox(size, true, opt);
      break;

    case xiiGreyBoxShape::RampX:
      geom.AddTexturedRamp(size, opt);
      break;

    case xiiGreyBoxShape::RampY:
      xiiMath::Swap(size.x, size.y);
      opt.m_Transform = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(-90.0f));
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddTexturedRamp(size, opt);
      break;

    case xiiGreyBoxShape::Column:
      opt.m_Transform.SetScalingFactors(size).IgnoreResult();
      geom.AddCylinder(0.5f, 0.5f, 0.5f, 0.5f, true, true, xiiMath::Min<xiiUInt16>(bOnlyRoughDetails ? 14 : 32, static_cast<xiiUInt16>(m_uiDetail)), opt);
      break;

    case xiiGreyBoxShape::StairsX:
      geom.AddStairs(size, m_uiDetail, m_Curvature, m_bSlopedTop, opt);
      break;

    case xiiGreyBoxShape::StairsY:
      xiiMath::Swap(size.x, size.y);
      opt.m_Transform = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(-90.0f));
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddStairs(size, m_uiDetail, m_Curvature, m_bSlopedTop, opt);
      break;

    case xiiGreyBoxShape::ArchX:
    {
      const float tmp = size.z;
      size.z          = size.x;
      size.x          = size.y;
      size.y          = tmp;
      opt.m_Transform = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(-90));
      t2              = xiiMat4::MakeRotationX(xiiAngle::MakeFromDegree(90));
      opt.m_Transform = t2 * opt.m_Transform;
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, false, false, false, !bOnlyRoughDetails, opt);
    }
    break;

    case xiiGreyBoxShape::ArchY:
    {
      opt.m_Transform = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(-90));
      t2              = xiiMat4::MakeRotationX(xiiAngle::MakeFromDegree(90));
      t3              = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(90));
      xiiMath::Swap(size.y, size.z);
      opt.m_Transform = t3 * t2 * opt.m_Transform;
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, false, false, false, !bOnlyRoughDetails, opt);
    }
    break;

    case xiiGreyBoxShape::SpiralStairs:
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, true, m_bSlopedBottom, m_bSlopedTop, true, opt);
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }
}

void xiiGreyBoxComponent::GenerateMeshName(xiiStringBuilder& out_sName) const
{
  switch (m_Shape)
  {
    case xiiGreyBoxShape::Box:
      out_sName.SetFormat("Grey-Box:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case xiiGreyBoxShape::RampX:
      out_sName.SetFormat("Grey-RampX:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case xiiGreyBoxShape::RampY:
      out_sName.SetFormat("Grey-RampY:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case xiiGreyBoxShape::Column:
      out_sName.SetFormat("Grey-Column:{0}-{1},{2}-{3},{4}-{5}-d{6}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail);
      break;

    case xiiGreyBoxShape::StairsX:
      out_sName.SetFormat("Grey-StairsX:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-st{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_bSlopedTop);
      break;

    case xiiGreyBoxShape::StairsY:
      out_sName.SetFormat("Grey-StairsY:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-st{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_bSlopedTop);
      break;

    case xiiGreyBoxShape::ArchX:
      out_sName.SetFormat("Grey-ArchX:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness);
      break;

    case xiiGreyBoxShape::ArchY:
      out_sName.SetFormat("Grey-ArchY:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness);
      break;

    case xiiGreyBoxShape::SpiralStairs:
      out_sName.SetFormat("Grey-Spiral:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}-st{9}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness, m_bSlopedTop);
      out_sName.AppendFormat("-sb{0}", m_bSlopedBottom);
      break;


    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }
}

void xiiGreyBoxComponent::GenerateMeshResourceDescriptor(xiiMeshResourceDescriptor& desc) const
{
  xiiGeometry geom;
  BuildGeometry(geom, m_Shape, false);

  bool bInvertedGeo = false;

  if (-m_fSizeNegX > m_fSizePosX)
    bInvertedGeo = !bInvertedGeo;
  if (-m_fSizeNegY > m_fSizePosY)
    bInvertedGeo = !bInvertedGeo;
  if (-m_fSizeNegZ > m_fSizePosZ)
    bInvertedGeo = !bInvertedGeo;

  if (bInvertedGeo)
  {
    for (auto vert : geom.GetVertices())
    {
      vert.m_vNormal = -vert.m_vNormal;
    }
  }

  geom.TriangulatePolygons();
  geom.ComputeTangents();

  // Data/Base/Materials/Common/Pattern.xiiMaterialAsset
  desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");

  desc.MeshBufferDesc().AddCommonStreams();
  desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::TriangleList);

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

  desc.ComputeBounds();
}

template <typename ResourceType>
xiiTypedResourceHandle<ResourceType> xiiGreyBoxComponent::GenerateMesh() const
{
  xiiStringBuilder sResourceName;
  GenerateMeshName(sResourceName);

  xiiTypedResourceHandle<ResourceType> hResource = xiiResourceManager::GetExistingResource<ResourceType>(sResourceName);
  if (hResource.IsValid())
    return hResource;

  xiiMeshResourceDescriptor desc;
  GenerateMeshResourceDescriptor(desc);

  return xiiResourceManager::GetOrCreateResource<ResourceType>(sResourceName, std::move(desc), sResourceName);
}


XII_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_GreyBoxComponent);
