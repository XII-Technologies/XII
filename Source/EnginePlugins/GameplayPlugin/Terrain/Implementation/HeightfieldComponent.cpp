#include <GameplayPlugin/GameplayPluginPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <GameplayPlugin/Terrain/HeightfieldComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageUtils.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiHeightfieldComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("HeightfieldImage", GetHeightfieldFile, SetHeightfieldFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Data_2D")),
    XII_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_ACCESSOR_PROPERTY("HalfExtents", GetHalfExtents, SetHalfExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(50))),
    XII_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new xiiDefaultValueAttribute(50)),
    XII_ACCESSOR_PROPERTY("Tesselation", GetTesselation, SetTesselation)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2U32(128))),
    XII_ACCESSOR_PROPERTY("TexCoordOffset", GetTexCoordOffset, SetTexCoordOffset)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(0))),
    XII_ACCESSOR_PROPERTY("TexCoordScale", GetTexCoordScale, SetTexCoordScale)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(1))),
    XII_ACCESSOR_PROPERTY("GenerateCollision", GetGenerateCollision, SetGenerateCollision)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("ColMeshTesselation", GetColMeshTesselation, SetColMeshTesselation)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2U32(64))),    
    XII_ACCESSOR_PROPERTY("IncludeInNavmesh", GetIncludeInNavmesh, SetIncludeInNavmesh)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Terrain"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
    XII_MESSAGE_HANDLER(xiiMsgBuildStaticMesh, OnBuildStaticMesh),
    XII_MESSAGE_HANDLER(xiiMsgExtractGeometry, OnMsgExtractGeometry),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiHeightfieldComponent::xiiHeightfieldComponent()  = default;
xiiHeightfieldComponent::~xiiHeightfieldComponent() = default;

void xiiHeightfieldComponent::SetHalfExtents(xiiVec2 value)
{
  m_vHalfExtents = value;
  InvalidateMesh();
}

void xiiHeightfieldComponent::SetHeight(float value)
{
  m_fHeight = value;
  InvalidateMesh();
}

void xiiHeightfieldComponent::SetTexCoordOffset(xiiVec2 value)
{
  m_vTexCoordOffset = value;
  InvalidateMesh();
}

void xiiHeightfieldComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  xiiStreamWriter& s = stream.GetStream();

  s << m_hHeightfield;
  s << m_hMaterial;
  s << m_vHalfExtents;
  s << m_fHeight;
  s << m_vTexCoordOffset;
  s << m_vTexCoordScale;
  s << m_vTesselation;
  s << m_vColMeshTesselation;

  // Version 2
  s << m_bGenerateCollision;
  s << m_bIncludeInNavmesh;
}

void xiiHeightfieldComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32  uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = stream.GetStream();

  s >> m_hHeightfield;
  s >> m_hMaterial;
  s >> m_vHalfExtents;
  s >> m_fHeight;
  s >> m_vTexCoordOffset;
  s >> m_vTexCoordScale;
  s >> m_vTesselation;
  s >> m_vColMeshTesselation;

  if (uiVersion >= 2)
  {
    s >> m_bGenerateCollision;
    s >> m_bIncludeInNavmesh;
  }
}

void xiiHeightfieldComponent::OnActivated()
{
  if (!m_hMesh.IsValid())
  {
    m_hMesh = GenerateMesh<xiiMeshResource>();
  }

  // First generate the mesh and then call the base implementation which will update the bounds
  SUPER::OnActivated();
}

xiiResult xiiHeightfieldComponent::GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg)
{
  if (m_hMesh.IsValid())
  {
    xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
    bounds = pMesh->GetBounds();
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiHeightfieldComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
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
      pRenderData->m_Color           = xiiColor::White;

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

      xiiTempHashedString blendModeValue = pMaterial->GetPermutationValue("BLEND_MODE");
      if (blendModeValue == "BLEND_MODE_OPAQUE" || blendModeValue == "")
      {
        category = xiiDefaultRenderDataCategories::LitOpaque;
      }
      else if (blendModeValue == "BLEND_MODE_MASKED")
      {
        category = xiiDefaultRenderDataCategories::LitMasked;
      }
      else
      {
        category = xiiDefaultRenderDataCategories::LitTransparent;
      }
    }

    msg.AddRenderData(pRenderData, category, bDontCacheYet ? xiiRenderData::Caching::Never : xiiRenderData::Caching::IfStatic);
  }
}

void xiiHeightfieldComponent::SetTexCoordScale(xiiVec2 value) // [ property ]
{
  m_vTexCoordScale = value;
  InvalidateMesh();
}

void xiiHeightfieldComponent::SetMaterialFile(const char* szFile)
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

const char* xiiHeightfieldComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}


void xiiHeightfieldComponent::SetHeightfieldFile(const char* szFile)
{
  xiiImageDataResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiImageDataResource>(szFile);
  }

  SetHeightfield(hResource);
}

const char* xiiHeightfieldComponent::GetHeightfieldFile() const
{
  if (!m_hHeightfield.IsValid())
    return "";

  return m_hHeightfield.GetResourceID();
}

void xiiHeightfieldComponent::SetHeightfield(const xiiImageDataResourceHandle& hResource)
{
  m_hHeightfield = hResource;
  InvalidateMesh();
}

void xiiHeightfieldComponent::SetTesselation(xiiVec2U32 value)
{
  m_vTesselation = value;
  InvalidateMesh();
}

void xiiHeightfieldComponent::SetGenerateCollision(bool b)
{
  m_bGenerateCollision = b;
}

void xiiHeightfieldComponent::SetColMeshTesselation(xiiVec2U32 value)
{
  m_vColMeshTesselation = value;
  // don't invalidate the render mesh
}

void xiiHeightfieldComponent::SetIncludeInNavmesh(bool b)
{
  m_bIncludeInNavmesh = b;
}

void xiiHeightfieldComponent::OnBuildStaticMesh(xiiMsgBuildStaticMesh& msg) const
{
  if (!m_bGenerateCollision)
    return;

  xiiGeometry geom;
  BuildGeometry(geom);

  auto* pDesc               = msg.m_pStaticMeshDescription;
  auto& subMesh             = pDesc->m_SubMeshes.ExpandAndGetRef();
  subMesh.m_uiFirstTriangle = pDesc->m_Triangles.GetCount();

  const xiiTransform trans = GetOwner()->GetGlobalTransform();

  const xiiUInt32 uiTriOffset = pDesc->m_Vertices.GetCount();

  for (const auto& verts : geom.GetVertices())
  {
    pDesc->m_Vertices.PushBack(trans * verts.m_vPosition);
  }

  for (const auto& polys : geom.GetPolygons())
  {
    for (xiiUInt32 t = 0; t < polys.m_Vertices.GetCount() - 2; ++t)
    {
      auto& tri                = pDesc->m_Triangles.ExpandAndGetRef();
      tri.m_uiVertexIndices[0] = uiTriOffset + polys.m_Vertices[0];
      tri.m_uiVertexIndices[1] = uiTriOffset + polys.m_Vertices[t + 1];
      tri.m_uiVertexIndices[2] = uiTriOffset + polys.m_Vertices[t + 2];
    }
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

void xiiHeightfieldComponent::OnMsgExtractGeometry(xiiMsgExtractGeometry& msg) const
{
  if (msg.m_Mode == xiiWorldGeoExtractionUtil::ExtractionMode::CollisionMesh && (m_bGenerateCollision == false || GetOwner()->IsDynamic()))
    return;

  if (msg.m_Mode == xiiWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration && (m_bIncludeInNavmesh == false || GetOwner()->IsDynamic()))
    return;

  msg.AddMeshObject(GetOwner()->GetGlobalTransform(), GenerateMesh<xiiCpuMeshResource>());
}

void xiiHeightfieldComponent::InvalidateMesh()
{
  if (m_hMesh.IsValid())
  {
    m_hMesh.Invalidate();

    m_hMesh = GenerateMesh<xiiMeshResource>();

    TriggerLocalBoundsUpdate();
  }
}

void xiiHeightfieldComponent::BuildGeometry(xiiGeometry& geom) const
{
  if (!m_hHeightfield.IsValid())
    return;

  XII_PROFILE_SCOPE("Heightfield: BuildGeometry");

  xiiResourceLock<xiiImageDataResource> pImageData(m_hHeightfield, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pImageData.GetAcquireResult() != xiiResourceAcquireResult::Final)
  {
    xiiLog::Error("Failed to load heightmap image data '{}'", m_hHeightfield.GetResourceID());
    return;
  }

  const xiiImage& heightmap = pImageData->GetDescriptor().m_Image;

  const xiiUInt32 uiNumVerticesX = xiiMath::Clamp(m_vColMeshTesselation.x + 1u, 5u, 512u);
  const xiiUInt32 uiNumVerticesY = xiiMath::Clamp(m_vColMeshTesselation.y + 1u, 5u, 512u);

  const xiiVec3 vSize(m_vHalfExtents.x * 2, m_vHalfExtents.y * 2, m_fHeight);
  const xiiVec2 vToNDC = xiiVec2(1.0f / (uiNumVerticesX - 1), 1.0f / (uiNumVerticesY - 1));
  const xiiVec3 vPosOffset(-m_vHalfExtents.x, -m_vHalfExtents.y, 0);

  const xiiColor* pImgData  = heightmap.GetPixelPointer<xiiColor>();
  const xiiUInt32 imgWidth  = heightmap.GetWidth();
  const xiiUInt32 imgHeight = heightmap.GetHeight();

  for (xiiUInt32 y = 0; y < uiNumVerticesY; ++y)
  {
    for (xiiUInt32 x = 0; x < uiNumVerticesX; ++x)
    {
      const xiiVec2 ndc      = xiiVec2((float)x, (float)y).CompMul(vToNDC);
      const xiiVec2 tc       = m_vTexCoordOffset + ndc.CompMul(m_vTexCoordScale);
      const xiiVec2 heightTC = ndc;

      const float fHeightScale = 1.0f - xiiImageUtils::BilinearSample(pImgData, imgWidth, imgHeight, xiiImageAddressMode::Clamp, heightTC).r;

      const xiiVec3 vNewPos = vPosOffset + xiiVec3(ndc.x, ndc.y, -fHeightScale).CompMul(vSize);

      geom.AddVertex(vNewPos, xiiVec3(0, 0, 1), tc, xiiColor::White);
    }
  }

  xiiUInt32 uiVertexIdx = 0;

  for (xiiUInt32 y = 0; y < uiNumVerticesY - 1; ++y)
  {
    for (xiiUInt32 x = 0; x < uiNumVerticesX - 1; ++x)
    {
      xiiUInt32 indices[4];
      indices[0] = uiVertexIdx;
      indices[1] = uiVertexIdx + 1;
      indices[2] = uiVertexIdx + uiNumVerticesX + 1;
      indices[3] = uiVertexIdx + uiNumVerticesX;

      geom.AddPolygon(indices, false);

      ++uiVertexIdx;
    }

    ++uiVertexIdx;
  }
}

xiiResult xiiHeightfieldComponent::BuildMeshDescriptor(xiiMeshResourceDescriptor& desc) const
{
  XII_PROFILE_SCOPE("Heightfield: GenerateRenderMesh");

  xiiResourceLock<xiiImageDataResource> pImageData(m_hHeightfield, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pImageData.GetAcquireResult() != xiiResourceAcquireResult::Final)
  {
    xiiLog::Error("Failed to load heightmap image data '{}'", m_hHeightfield.GetResourceID());
    return XII_FAILURE;
  }

  const xiiImage& heightmap = pImageData->GetDescriptor().m_Image;

  // Data/Base/Materials/Common/Pattern.xiiMaterialAsset
  desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");

  desc.MeshBufferDesc().AddCommonStreams();
  // 0 = position
  // 1 = texcoord
  // 2 = normal
  // 3 = tangent

  {
    auto& mb = desc.MeshBufferDesc();

    const xiiUInt32 uiNumVerticesX = xiiMath::Clamp(m_vTesselation.x + 1u, 5u, 1024u);
    const xiiUInt32 uiNumVerticesY = xiiMath::Clamp(m_vTesselation.y + 1u, 5u, 1024u);
    const xiiUInt32 uiNumTriangles = (uiNumVerticesX - 1) * (uiNumVerticesY - 1) * 2;

    mb.AllocateStreams(uiNumVerticesX * uiNumVerticesY, xiiGALPrimitiveTopology::Triangles, uiNumTriangles);

    const xiiVec3 vSize(m_vHalfExtents.x * 2, m_vHalfExtents.y * 2, m_fHeight);
    const xiiVec2 vToNDC = xiiVec2(1.0f / (uiNumVerticesX - 1), 1.0f / (uiNumVerticesY - 1));
    const xiiVec3 vPosOffset(-m_vHalfExtents.x, -m_vHalfExtents.y, -m_fHeight);

    const auto texCoordFormat = xiiMeshTexCoordPrecision::ToResourceFormat(xiiMeshTexCoordPrecision::Default);
    const auto normalFormat   = xiiMeshNormalPrecision::ToResourceFormatNormal(xiiMeshNormalPrecision::Default);
    const auto tangentFormat  = xiiMeshNormalPrecision::ToResourceFormatTangent(xiiMeshNormalPrecision::Default);

    // access the vertex data directly
    // this is way more complicated than going through SetVertexData, but it is ~20% faster

    auto positionData = mb.GetVertexData(0, 0);
    auto texcoordData = mb.GetVertexData(1, 0);
    auto normalData   = mb.GetVertexData(2, 0);
    auto tangentData  = mb.GetVertexData(3, 0);

    const xiiUInt32 uiVertexDataSize = mb.GetVertexDataSize();

    xiiUInt32 uiVertexIdx = 0;

    const xiiColor* pImgData  = heightmap.GetPixelPointer<xiiColor>();
    const xiiUInt32 imgWidth  = heightmap.GetWidth();
    const xiiUInt32 imgHeight = heightmap.GetHeight();

    for (xiiUInt32 y = 0; y < uiNumVerticesY; ++y)
    {
      for (xiiUInt32 x = 0; x < uiNumVerticesX; ++x)
      {
        const xiiVec2 ndc      = xiiVec2((float)x, (float)y).CompMul(vToNDC);
        const xiiVec2 tc       = m_vTexCoordOffset + ndc.CompMul(m_vTexCoordScale);
        const xiiVec2 heightTC = ndc;

        const size_t uiByteOffset = (size_t)uiVertexIdx * (size_t)uiVertexDataSize;

        const float fHeightScale = xiiImageUtils::BilinearSample(pImgData, imgWidth, imgHeight, xiiImageAddressMode::Clamp, heightTC).r;

        // complicated but faster
        *reinterpret_cast<xiiVec3*>(positionData.GetPtr() + uiByteOffset) = vPosOffset + xiiVec3(ndc.x, ndc.y, fHeightScale).CompMul(vSize);
        xiiMeshBufferUtils::EncodeTexCoord(tc, xiiByteArrayPtr(texcoordData.GetPtr() + uiByteOffset, 32), texCoordFormat).IgnoreResult();

        // easier to understand, but slower
        // mb.SetVertexData(0, uiVertexIdx, vPosOffset + xiiVec3(ndc.x, ndc.y, -fHeightScale).CompMul(vSize));
        // xiiMeshBufferUtils::EncodeTexCoord(tc, mb.GetVertexData(1, uiVertexIdx), texCoordFormat).IgnoreResult();

        ++uiVertexIdx;
      }
    }

    uiVertexIdx = 0;

    for (xiiUInt32 y = 0; y < uiNumVerticesY; ++y)
    {
      for (xiiUInt32 x = 0; x < uiNumVerticesX; ++x)
      {
        const size_t uiByteOffset = (size_t)uiVertexIdx * (size_t)uiVertexDataSize;

        const xiiInt32 centerIDx = uiVertexIdx;
        xiiInt32       leftIDx   = uiVertexIdx - 1;
        xiiInt32       rightIDx  = uiVertexIdx + 1;
        xiiInt32       bottomIDx = uiVertexIdx - uiNumVerticesX;
        xiiInt32       topIDx    = uiVertexIdx + uiNumVerticesX;

        // clamp the indices
        if (x == 0)
          leftIDx = centerIDx;
        if (x + 1 == uiNumVerticesX)
          rightIDx = centerIDx;
        if (y == 0)
          bottomIDx = centerIDx;
        if (y + 1 == uiNumVerticesY)
          topIDx = centerIDx;

        const xiiVec3 vPosCenter = *reinterpret_cast<xiiVec3*>(positionData.GetPtr() + (size_t)centerIDx * (size_t)uiVertexDataSize);
        const xiiVec3 vPosLeft   = *reinterpret_cast<xiiVec3*>(positionData.GetPtr() + (size_t)leftIDx * (size_t)uiVertexDataSize);
        const xiiVec3 vPosRight  = *reinterpret_cast<xiiVec3*>(positionData.GetPtr() + (size_t)rightIDx * (size_t)uiVertexDataSize);
        const xiiVec3 vPosBottom = *reinterpret_cast<xiiVec3*>(positionData.GetPtr() + (size_t)bottomIDx * (size_t)uiVertexDataSize);
        const xiiVec3 vPosTop    = *reinterpret_cast<xiiVec3*>(positionData.GetPtr() + (size_t)topIDx * (size_t)uiVertexDataSize);

        xiiVec3 edgeL = vPosLeft - vPosCenter;
        xiiVec3 edgeR = vPosRight - vPosCenter;
        xiiVec3 edgeB = vPosBottom - vPosCenter;
        xiiVec3 edgeT = vPosTop - vPosCenter;

        // rotate edges by 90 degrees, so that they become normals
        xiiMath::Swap(edgeL.x, edgeL.z);
        xiiMath::Swap(edgeR.x, edgeR.z);
        xiiMath::Swap(edgeB.y, edgeB.z);
        xiiMath::Swap(edgeT.y, edgeT.z);

        edgeL.z = -edgeL.z;
        edgeR.x = -edgeR.x;
        edgeB.z = -edgeB.z;
        edgeT.y = -edgeT.y;

        // don't normalize the edges first, if they are longer, they shall have more influence
        xiiVec3 vNormal(0);
        vNormal += edgeL;
        vNormal += edgeR;
        vNormal += edgeB;
        vNormal += edgeT;
        vNormal.Normalize();

        xiiVec3 vTangent = xiiVec3(1, 0, 0).CrossRH(vNormal).GetNormalized();

        // complicated but faster
        xiiMeshBufferUtils::EncodeNormal(vNormal, xiiByteArrayPtr(normalData.GetPtr() + uiByteOffset, 32), normalFormat).IgnoreResult();
        xiiMeshBufferUtils::EncodeTangent(vTangent, 1.0f, xiiByteArrayPtr(tangentData.GetPtr() + uiByteOffset, 32), tangentFormat).IgnoreResult();

        // easier to understand, but slower
        // xiiMeshBufferUtils::EncodeNormal(xiiVec3(0, 0, 1), mb.GetVertexData(2, uiVertexIdx), normalFormat).IgnoreResult();
        // xiiMeshBufferUtils::EncodeTangent(xiiVec3(1, 0, 0), 1.0f, mb.GetVertexData(3, uiVertexIdx), tangentFormat).IgnoreResult();

        ++uiVertexIdx;
      }
    }

    desc.SetBounds(xiiBoundingBox(vPosOffset, vPosOffset + vSize));

    xiiUInt32 uiTriangleIdx = 0;
    uiVertexIdx             = 0;

    for (xiiUInt32 y = 0; y < uiNumVerticesY - 1; ++y)
    {
      for (xiiUInt32 x = 0; x < uiNumVerticesX - 1; ++x)
      {
        mb.SetTriangleIndices(uiTriangleIdx + 0, uiVertexIdx, uiVertexIdx + 1, uiVertexIdx + uiNumVerticesX);
        mb.SetTriangleIndices(uiTriangleIdx + 1, uiVertexIdx + 1, uiVertexIdx + uiNumVerticesX + 1, uiVertexIdx + uiNumVerticesX);
        uiTriangleIdx += 2;

        ++uiVertexIdx;
      }

      ++uiVertexIdx;
    }
  }

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);
  return XII_SUCCESS;
}

template <typename ResourceType>
xiiTypedResourceHandle<ResourceType> xiiHeightfieldComponent::GenerateMesh() const
{
  if (!m_hHeightfield.IsValid())
    return xiiTypedResourceHandle<ResourceType>();

  xiiStringBuilder sResourceName;

  {
    xiiUInt64 uiSettingsHash = m_hHeightfield.GetResourceIDHash() + m_uiHeightfieldChangeCounter;
    uiSettingsHash           = xiiHashingUtils::xxHash64(&m_vHalfExtents, sizeof(m_vHalfExtents), uiSettingsHash);
    uiSettingsHash           = xiiHashingUtils::xxHash64(&m_fHeight, sizeof(m_fHeight), uiSettingsHash);
    uiSettingsHash           = xiiHashingUtils::xxHash64(&m_vTexCoordOffset, sizeof(m_vTexCoordOffset), uiSettingsHash);
    uiSettingsHash           = xiiHashingUtils::xxHash64(&m_vTexCoordScale, sizeof(m_vTexCoordScale), uiSettingsHash);
    uiSettingsHash           = xiiHashingUtils::xxHash64(&m_vTesselation, sizeof(m_vTesselation), uiSettingsHash);

    sResourceName.Format("Heightfield:{}", uiSettingsHash);

    xiiTypedResourceHandle<ResourceType> hResource = xiiResourceManager::GetExistingResource<ResourceType>(sResourceName);
    if (hResource.IsValid())
      return hResource;
  }

  xiiMeshResourceDescriptor desc;
  if (BuildMeshDescriptor(desc).Succeeded())
  {
    return xiiResourceManager::CreateResource<ResourceType>(sResourceName, std::move(desc), sResourceName);
  }

  return xiiTypedResourceHandle<ResourceType>();
}

//////////////////////////////////////////////////////////////////////////

xiiHeightfieldComponentManager::xiiHeightfieldComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<ComponentType, xiiBlockStorageType::Compact>(pWorld)
{
  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiHeightfieldComponentManager::ResourceEventHandler, this));
}

xiiHeightfieldComponentManager::~xiiHeightfieldComponentManager()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiHeightfieldComponentManager::ResourceEventHandler, this));
}

void xiiHeightfieldComponentManager::Initialize()
{
  auto desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiHeightfieldComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void xiiHeightfieldComponentManager::ResourceEventHandler(const xiiResourceEvent& e)
{
  if (e.m_Type == xiiResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<xiiImageDataResource>())
  {
    xiiImageDataResource*      pResource       = (xiiImageDataResource*)(e.m_pResource);
    const xiiUInt32            uiChangeCounter = pResource->GetCurrentResourceChangeCounter();
    xiiImageDataResourceHandle hResource(pResource);

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hHeightfield == hResource)
      {
        it->m_uiHeightfieldChangeCounter = uiChangeCounter;
        AddToUpdateList(it);
      }
    }
  }
}

void xiiHeightfieldComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    xiiHeightfieldComponent* pComponent;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    if (!pComponent->IsActive())
      continue;

    pComponent->InvalidateMesh();
  }

  m_ComponentsToUpdate.Clear();
}

void xiiHeightfieldComponentManager::AddToUpdateList(xiiHeightfieldComponent* pComponent)
{
  xiiComponentHandle hComponent = pComponent->GetHandle();

  if (m_ComponentsToUpdate.IndexOf(hComponent) == xiiInvalidIndex)
  {
    m_ComponentsToUpdate.PushBack(hComponent);
  }
}
