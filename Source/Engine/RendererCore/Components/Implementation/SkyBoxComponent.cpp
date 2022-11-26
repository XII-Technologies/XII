#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/TextureCubeResource.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSkyBoxComponent, 4, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("CubeMap", GetCubeMapFile, SetCubeMapFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
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
    geom.AddRectXY(xiiVec2(2.0f));

    xiiMeshBufferResourceDescriptor desc;
    desc.AddStream(xiiGALVertexAttributeSemantic::Position, xiiGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::Triangles);

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

xiiResult xiiSkyBoxComponent::GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg)
{
  bAlwaysVisible = true;
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

    pRenderData->FillBatchIdAndSortingKey();
  }

  msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::Sky, xiiRenderData::Caching::Never);
}

void xiiSkyBoxComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  xiiStreamWriter& s = stream.GetStream();

  s << m_fExposureBias;
  s << m_bInverseTonemap;
  s << m_bUseFog;
  s << m_fVirtualDistance;
  s << m_hCubeMap;
}

void xiiSkyBoxComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = stream.GetStream();

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

void xiiSkyBoxComponent::SetCubeMapFile(const char* szFile)
{
  xiiTextureCubeResourceHandle hCubeMap;
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hCubeMap = xiiResourceManager::LoadResource<xiiTextureCubeResource>(szFile);
  }

  SetCubeMap(hCubeMap);
}

const char* xiiSkyBoxComponent::GetCubeMapFile() const
{
  return m_hCubeMap.IsValid() ? m_hCubeMap.GetResourceID().GetData() : "";
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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class xiiSkyBoxComponentPatch_1_2 : public xiiGraphPatch
{
public:
  xiiSkyBoxComponentPatch_1_2() :
    xiiGraphPatch("xiiSkyBoxComponent", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Exposure Bias", "ExposureBias");
    pNode->RenameProperty("Inverse Tonemap", "InverseTonemap");
    pNode->RenameProperty("Left Texture", "LeftTexture");
    pNode->RenameProperty("Front Texture", "FrontTexture");
    pNode->RenameProperty("Right Texture", "RightTexture");
    pNode->RenameProperty("Back Texture", "BackTexture");
    pNode->RenameProperty("Up Texture", "UpTexture");
    pNode->RenameProperty("Down Texture", "DownTexture");
  }
};

xiiSkyBoxComponentPatch_1_2 g_xiiSkyBoxComponentPatch_1_2;



XII_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SkyBoxComponent);
