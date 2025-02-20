#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/SkyBoxComponent.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSkyBoxComponent, 4, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("CubeMap", GetCubeMap, SetCubeMap)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    XII_ACCESSOR_PROPERTY("ExposureBias", GetExposureBias, SetExposureBias)->AddAttributes(new xiiClampValueAttribute(-32.0f, 32.0f)),
    XII_ACCESSOR_PROPERTY("InverseTonemap", GetInverseTonemap, SetInverseTonemap),
    XII_ACCESSOR_PROPERTY("UseFog", GetUseFog, SetUseFog)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("VirtualDistance", GetVirtualDistance, SetVirtualDistance)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1000.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiSkyBoxComponent::xiiSkyBoxComponent()  = default;
xiiSkyBoxComponent::~xiiSkyBoxComponent() = default;

void xiiSkyBoxComponent::Initialize()
{
  SUPER::Initialize();

  const char*                 szBufferResourceName = "SkyBoxBuffer";
  xiiMeshBufferResourceHandle hMeshBuffer          = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szBufferResourceName);
  if (!hMeshBuffer.IsValid())
  {
    xiiGeometry geom;
    geom.AddRect(xiiVec2(2.0f));

    xiiMeshBufferResourceDescriptor desc;
    desc.AddStream(xiiGALInputLayoutSemantic::Position, xiiGALResourceFormat::RGB32Float);
    desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::TriangleList);

    hMeshBuffer = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
  }

  const char* szMeshResourceName = "SkyBoxMesh";
  m_hMesh                        = xiiResourceManager::GetExistingResource<xiiMeshResource>(szMeshResourceName);
  if (!m_hMesh.IsValid())
  {
    xiiMeshResourceDescriptor desc;
    desc.UseExistingMeshBuffer(hMeshBuffer);
    desc.AddSubMesh(2, 0, 0);
    desc.ComputeBounds();

    m_hMesh = xiiResourceManager::GetOrCreateResource<xiiMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
  }

  xiiStringBuilder cubeMapMaterialName = "SkyBoxMaterial_CubeMap";
  cubeMapMaterialName.AppendFormat("_{0}", xiiArgP(GetWorld())); // make the resource unique for each world

  m_hCubeMapMaterial = xiiResourceManager::GetExistingResource<xiiMaterialResource>(cubeMapMaterialName);
  if (!m_hCubeMapMaterial.IsValid())
  {
    xiiMaterialResourceDescriptor desc;
    desc.m_hBaseMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>("{ b4b75b1c-c2c8-4a0e-8076-780bdd46d18b }"); // Sky.xiiMaterialAsset

    m_hCubeMapMaterial = xiiResourceManager::CreateResource<xiiMaterialResource>(cubeMapMaterialName, std::move(desc), cubeMapMaterialName);
  }

  UpdateMaterials();
}

xiiResult xiiSkyBoxComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return XII_SUCCESS;
}

void xiiSkyBoxComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  // Don't extract sky render data for selection or in orthographic views.
  if (msg.m_OverrideCategory != xiiInvalidRenderDataCategory || msg.m_pView->GetCamera()->IsOrthographic())
    return;

  xiiMeshRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalTransform.m_vPosition.SetZero(); // skybox should always be at the origin
    pRenderData->m_GlobalBounds   = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh          = m_hMesh;
    pRenderData->m_hMaterial      = m_hCubeMapMaterial;
    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiUniqueID     = GetUniqueIdForRendering();

    pRenderData->FillSortingKey();
  }

  msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::Sky, xiiRenderData::Caching::Never);
}

void xiiSkyBoxComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_fExposureBias;
  s << m_bInverseTonemap;
  s << m_bUseFog;
  s << m_fVirtualDistance;
  s << m_hCubeMap;
}

void xiiSkyBoxComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_fExposureBias;
  s >> m_bInverseTonemap;

  if (uiVersion >= 4)
  {
    s >> m_bUseFog;
    s >> m_fVirtualDistance;
  }

  if (uiVersion >= 3)
  {
    s >> m_hCubeMap;
  }
  else
  {
    xiiTexture2DResourceHandle dummyHandle;
    for (int i = 0; i < 6; i++)
    {
      s >> dummyHandle;
    }
  }
}

void xiiSkyBoxComponent::SetExposureBias(float fExposureBias)
{
  m_fExposureBias = fExposureBias;

  UpdateMaterials();
}

void xiiSkyBoxComponent::SetInverseTonemap(bool bInverseTonemap)
{
  m_bInverseTonemap = bInverseTonemap;

  UpdateMaterials();
}

void xiiSkyBoxComponent::SetUseFog(bool bUseFog)
{
  m_bUseFog = bUseFog;

  UpdateMaterials();
}

void xiiSkyBoxComponent::SetVirtualDistance(float fVirtualDistance)
{
  m_fVirtualDistance = fVirtualDistance;

  UpdateMaterials();
}

void xiiSkyBoxComponent::SetCubeMap(const xiiTextureCubeResourceHandle& hCubeMap)
{
  m_hCubeMap = hCubeMap;
  UpdateMaterials();
}

const xiiTextureCubeResourceHandle& xiiSkyBoxComponent::GetCubeMap() const
{
  return m_hCubeMap;
}

void xiiSkyBoxComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateMaterials();
}

void xiiSkyBoxComponent::UpdateMaterials()
{
  if (m_hCubeMapMaterial.IsValid())
  {
    xiiResourceLock<xiiMaterialResource> pMaterial(m_hCubeMapMaterial, xiiResourceAcquireMode::AllowLoadingFallback);

    pMaterial->SetParameter("ExposureBias", m_fExposureBias);
    pMaterial->SetParameter("InverseTonemap", m_bInverseTonemap);
    pMaterial->SetParameter("UseFog", m_bUseFog);
    pMaterial->SetParameter("VirtualDistance", m_fVirtualDistance);
    pMaterial->SetTextureCubeBinding("CubeMap", m_hCubeMap);

    pMaterial->PreserveCurrentDesc();
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_SkyBoxComponent);
