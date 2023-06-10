#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgSetMeshMaterial);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgSetMeshMaterial, 1, xiiRTTIDefaultAllocator<xiiMsgSetMeshMaterial>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_MEMBER_PROPERTY("MaterialSlot", m_uiMaterialSlot),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiAutoGenVisScriptMsgSender,
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiMsgSetMeshMaterial::SetMaterialFile(const char* szFile)
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

const char* xiiMsgSetMeshMaterial::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void xiiMsgSetMeshMaterial::Serialize(xiiStreamWriter& ref_stream) const
{
  // has to be stringyfied for transfer
  ref_stream << GetMaterialFile();
  ref_stream << m_uiMaterialSlot;
}

void xiiMsgSetMeshMaterial::Deserialize(xiiStreamReader& ref_stream, xiiUInt8 uiTypeVersion)
{
  xiiStringBuilder file;
  ref_stream >> file;
  SetMaterialFile(file);

  ref_stream >> m_uiMaterialSlot;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshRenderData, 1, xiiRTTIDefaultAllocator<xiiMeshRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiMeshRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(0);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiMeshComponentBase, 1)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
    XII_MESSAGE_HANDLER(xiiMsgSetMeshMaterial, OnMsgSetMeshMaterial),
    XII_MESSAGE_HANDLER(xiiMsgSetColor, OnMsgSetColor),
  } XII_END_MESSAGEHANDLERS;
}
XII_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

xiiMeshComponentBase::xiiMeshComponentBase()  = default;
xiiMeshComponentBase::~xiiMeshComponentBase() = default;

void xiiMeshComponentBase::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  xiiStreamWriter& s = ref_stream.GetStream();

  // ignore components that have created meshes (?)

  s << m_hMesh;

  xiiUInt32 uiCategory = m_RenderDataCategory.m_uiValue;
  s << uiCategory;

  s << m_Materials.GetCount();

  for (const auto& mat : m_Materials)
  {
    s << mat;
  }

  s << m_Color;
}

void xiiMeshComponentBase::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  const xiiUInt32 uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = ref_stream.GetStream();

  s >> m_hMesh;

  xiiUInt32 uiCategory = 0;
  s >> uiCategory;
  m_RenderDataCategory.m_uiValue = static_cast<xiiUInt16>(uiCategory);

  xiiUInt32 uiMaterials = 0;
  s >> uiMaterials;

  m_Materials.SetCount(uiMaterials);

  for (auto& mat : m_Materials)
  {
    s >> mat;
  }

  s >> m_Color;
}

xiiResult xiiMeshComponentBase::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  if (m_hMesh.IsValid())
  {
    xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
    ref_bounds = pMesh->GetBounds();
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiMeshComponentBase::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  xiiResourceLock<xiiMeshResource>                      pMesh(m_hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
  xiiArrayPtr<const xiiMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (xiiUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const xiiUInt32           uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    xiiMaterialResourceHandle hMaterial;

    // If we have a material override, use that otherwise use the default mesh material.
    if (GetMaterial(uiMaterialIndex).IsValid())
      hMaterial = m_Materials[uiMaterialIndex];
    else
      hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    xiiMeshRenderData* pRenderData = CreateRenderData();
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform() * pRenderData->m_GlobalTransform;
      pRenderData->m_GlobalBounds    = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh           = m_hMesh;
      pRenderData->m_hMaterial       = hMaterial;
      pRenderData->m_Color           = m_Color;
      pRenderData->m_uiSubMeshIndex  = uiPartIndex;
      pRenderData->m_uiUniqueID      = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillBatchIdAndSortingKey();
    }

    bool bDontCacheYet = false;

    // Determine render data category.
    xiiRenderData::Category category = m_RenderDataCategory;
    if (category == xiiInvalidRenderDataCategory)
    {
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
      else
      {
        category = xiiDefaultRenderDataCategories::LitOpaque;
      }
    }

    msg.AddRenderData(pRenderData, category, bDontCacheYet ? xiiRenderData::Caching::Never : xiiRenderData::Caching::IfStatic);
  }
}

void xiiMeshComponentBase::SetMesh(const xiiMeshResourceHandle& hMesh)
{
  if (m_hMesh != hMesh)
  {
    m_hMesh = hMesh;

    TriggerLocalBoundsUpdate();
    InvalidateCachedRenderData();
  }
}

void xiiMeshComponentBase::SetMaterial(xiiUInt32 uiIndex, const xiiMaterialResourceHandle& hMaterial)
{
  m_Materials.EnsureCount(uiIndex + 1);

  if (m_Materials[uiIndex] != hMaterial)
  {
    m_Materials[uiIndex] = hMaterial;

    InvalidateCachedRenderData();
  }
}

xiiMaterialResourceHandle xiiMeshComponentBase::GetMaterial(xiiUInt32 uiIndex) const
{
  if (uiIndex >= m_Materials.GetCount())
    return xiiMaterialResourceHandle();

  return m_Materials[uiIndex];
}

void xiiMeshComponentBase::SetMeshFile(const char* szFile)
{
  xiiMeshResourceHandle hMesh;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = xiiResourceManager::LoadResource<xiiMeshResource>(szFile);
  }

  SetMesh(hMesh);
}

const char* xiiMeshComponentBase::GetMeshFile() const
{
  if (!m_hMesh.IsValid())
    return "";

  return m_hMesh.GetResourceID();
}

void xiiMeshComponentBase::SetColor(const xiiColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const xiiColor& xiiMeshComponentBase::GetColor() const
{
  return m_Color;
}

void xiiMeshComponentBase::OnMsgSetMeshMaterial(xiiMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_uiMaterialSlot, ref_msg.m_hMaterial);
}

void xiiMeshComponentBase::OnMsgSetColor(xiiMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

xiiMeshRenderData* xiiMeshComponentBase::CreateRenderData() const
{
  return xiiCreateRenderDataForThisFrame<xiiMeshRenderData>(GetOwner());
}

xiiUInt32 xiiMeshComponentBase::Materials_GetCount() const
{
  return m_Materials.GetCount();
}

const char* xiiMeshComponentBase::Materials_GetValue(xiiUInt32 uiIndex) const
{
  auto hMat = GetMaterial(uiIndex);

  if (!hMat.IsValid())
    return "";

  return hMat.GetResourceID();
}


void xiiMeshComponentBase::Materials_SetValue(xiiUInt32 uiIndex, const char* value)
{
  if (xiiStringUtils::IsNullOrEmpty(value))
    SetMaterial(uiIndex, xiiMaterialResourceHandle());
  else
  {
    auto hMat = xiiResourceManager::LoadResource<xiiMaterialResource>(value);
    SetMaterial(uiIndex, hMat);
  }
}


void xiiMeshComponentBase::Materials_Insert(xiiUInt32 uiIndex, const char* value)
{
  xiiMaterialResourceHandle hMat;

  if (!xiiStringUtils::IsNullOrEmpty(value))
    hMat = xiiResourceManager::LoadResource<xiiMaterialResource>(value);

  m_Materials.Insert(hMat, uiIndex);

  InvalidateCachedRenderData();
}


void xiiMeshComponentBase::Materials_Remove(xiiUInt32 uiIndex)
{
  m_Materials.RemoveAtAndCopy(uiIndex);

  InvalidateCachedRenderData();
}


XII_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponentBase);
