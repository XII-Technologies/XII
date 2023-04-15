#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Mesh/ParticleTypeMesh.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTypeMeshFactory, 1, xiiRTTIDefaultAllocator<xiiParticleTypeMeshFactory>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Mesh", m_sMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    XII_MEMBER_PROPERTY("Material", m_sMaterial)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTypeMesh, 1, xiiRTTIDefaultAllocator<xiiParticleTypeMesh>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const xiiRTTI* xiiParticleTypeMeshFactory::GetTypeType() const
{
  return xiiGetStaticRTTI<xiiParticleTypeMesh>();
}


void xiiParticleTypeMeshFactory::CopyTypeProperties(xiiParticleType* pObject, bool bFirstTime) const
{
  xiiParticleTypeMesh* pType = static_cast<xiiParticleTypeMesh*>(pObject);

  pType->m_hMesh.Invalidate();
  pType->m_hMaterial.Invalidate();
  pType->m_sTintColorParameter = xiiTempHashedString(m_sTintColorParameter.GetData());

  if (!m_sMesh.IsEmpty())
    pType->m_hMesh = xiiResourceManager::LoadResource<xiiMeshResource>(m_sMesh);

  if (!m_sMaterial.IsEmpty())
    pType->m_hMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>(m_sMaterial);
}

enum class TypeMeshVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added material

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleTypeMeshFactory::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = (int)TypeMeshVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_sMesh;
  inout_stream << m_sTintColorParameter;

  // Version 2
  inout_stream << m_sMaterial;
}

void xiiParticleTypeMeshFactory::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)TypeMeshVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_sMesh;
  inout_stream >> m_sTintColorParameter;

  if (uiVersion >= 2)
  {
    inout_stream >> m_sMaterial;
  }
}

xiiParticleTypeMesh::xiiParticleTypeMesh()  = default;
xiiParticleTypeMesh::~xiiParticleTypeMesh() = default;

void xiiParticleTypeMesh::CreateRequiredStreams()
{
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", xiiProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", xiiProcessingStream::DataType::Half4, &m_pStreamColor, false);
  CreateStream("RotationSpeed", xiiProcessingStream::DataType::Half, &m_pStreamRotationSpeed, false);
  CreateStream("RotationOffset", xiiProcessingStream::DataType::Half, &m_pStreamRotationOffset, false);
  CreateStream("Axis", xiiProcessingStream::DataType::Float3, &m_pStreamAxis, true);
}

void xiiParticleTypeMesh::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  xiiVec3*   pAxis = m_pStreamAxis->GetWritableData<xiiVec3>();
  xiiRandom& rng   = GetRNG();

  for (xiiUInt32 i = 0; i < uiNumElements; ++i)
  {
    const xiiUInt64 uiElementIdx = uiStartIndex + i;

    pAxis[uiElementIdx] = xiiVec3::CreateRandomDirection(rng);
  }
}

bool xiiParticleTypeMesh::QueryMeshAndMaterialInfo() const
{
  if (!m_hMesh.IsValid())
  {
    m_bRenderDataCached = true;
    m_hMaterial.Invalidate();
    return true;
  }

  xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
  if (pMesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return false;

  if (!m_hMaterial.IsValid())
  {
    m_hMaterial = pMesh->GetMaterials()[0];

    if (!m_hMaterial.IsValid())
    {
      m_bRenderDataCached = true;
      return true;
    }
  }

  xiiResourceLock<xiiMaterialResource> pMaterial(m_hMaterial, xiiResourceAcquireMode::AllowLoadingFallback);
  if (pMaterial.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return false;

  m_Bounds = pMesh->GetBounds();

  {
    m_RenderCategory = xiiDefaultRenderDataCategories::LitOpaque;

    xiiTempHashedString blendModeValue = pMaterial->GetPermutationValue("BLEND_MODE");
    if (blendModeValue == "BLEND_MODE_OPAQUE" || blendModeValue == "")
    {
      m_RenderCategory = xiiDefaultRenderDataCategories::LitOpaque;
    }
    else if (blendModeValue == "BLEND_MODE_MASKED")
    {
      m_RenderCategory = xiiDefaultRenderDataCategories::LitMasked;
    }
    else
    {
      m_RenderCategory = xiiDefaultRenderDataCategories::LitTransparent;
    }
  }

  m_bRenderDataCached = true;
  return true;
}

void xiiParticleTypeMesh::ExtractTypeRenderData(xiiMsgExtractRenderData& ref_msg, const xiiTransform& instanceTransform) const
{
  if (!m_bRenderDataCached)
  {
    // check if we now know how to render this thing
    if (!QueryMeshAndMaterialInfo())
      return;
  }

  if (!m_hMaterial.IsValid())
    return;

  if (m_RenderCategory.m_uiValue == 0xFFFF)
  {
    m_bRenderDataCached = false;
    return;
  }

  const xiiUInt32 numParticles = (xiiUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  XII_PROFILE_SCOPE("PFX: Mesh");

  const xiiTime  tCur      = GetOwnerEffect()->GetTotalEffectLifeTime();
  const xiiColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, xiiColor::White);

  const xiiVec4*           pPosition       = m_pStreamPosition->GetData<xiiVec4>();
  const xiiFloat16*        pSize           = m_pStreamSize->GetData<xiiFloat16>();
  const xiiColorLinear16f* pColor          = m_pStreamColor->GetData<xiiColorLinear16f>();
  const xiiFloat16*        pRotationSpeed  = m_pStreamRotationSpeed->GetData<xiiFloat16>();
  const xiiFloat16*        pRotationOffset = m_pStreamRotationOffset->GetData<xiiFloat16>();
  const xiiVec3*           pAxis           = m_pStreamAxis->GetData<xiiVec3>();

  {
    const xiiUInt32 uiFlipWinding = 0;

    for (xiiUInt32 p = 0; p < numParticles; ++p)
    {
      const xiiUInt32 idx = p;

      xiiTransform trans;
      trans.m_qRotation.SetFromAxisAndAngle(pAxis[p], xiiAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[idx]) + pRotationOffset[idx]));
      trans.m_vPosition = pPosition[idx].GetAsVec3();
      trans.m_vScale.Set(pSize[idx]);

      xiiMeshRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiMeshRenderData>(nullptr);
      {
        pRenderData->m_GlobalTransform = trans;
        pRenderData->m_GlobalBounds    = m_Bounds;
        pRenderData->m_hMesh           = m_hMesh;
        pRenderData->m_hMaterial       = m_hMaterial;
        pRenderData->m_Color           = pColor[idx].ToLinearFloat() * tintColor;

        pRenderData->m_uiSubMeshIndex = 0;
        pRenderData->m_uiUniqueID     = 0xFFFFFFFF;

        pRenderData->FillBatchIdAndSortingKey();
      }

      ref_msg.AddRenderData(pRenderData, m_RenderCategory, xiiRenderData::Caching::Never);
    }
  }
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Mesh_ParticleTypeMesh);
