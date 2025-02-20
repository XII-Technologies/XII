#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Meshes/MeshComponentBase.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Device/Device.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgSetMeshMaterial);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgSetMeshMaterial, 1, xiiRTTIDefaultAllocator<xiiMsgSetMeshMaterial>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_MEMBER_PROPERTY("MaterialSlot", m_uiMaterialSlot),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiMsgSetMeshMaterial::Serialize(xiiStreamWriter& inout_stream) const
{
  // has to be stringyfied for transfer
  inout_stream << GetMaterialFile();
  inout_stream << m_uiMaterialSlot;
}

void xiiMsgSetMeshMaterial::Deserialize(xiiStreamReader& inout_stream, xiiUInt8 uiTypeVersion)
{
  xiiStringBuilder file;
  inout_stream >> file;
  SetMaterialFile(file);

  inout_stream >> m_uiMaterialSlot;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshRenderData, 1, xiiRTTIDefaultAllocator<xiiMeshRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiMeshRenderData::FillSortingKey()
{
  m_uiFlipWinding  = m_GlobalTransform.ContainsNegativeScale() ? 1 : 0;
  m_uiUniformScale = m_GlobalTransform.ContainsUniformScale() ? 1 : 0;

  const xiiUInt32 uiMeshIDHash     = xiiHashingUtils::StringHashTo32(m_hMesh.GetResourceIDHash());
  const xiiUInt32 uiMaterialIDHash = m_hMaterial.IsValid() ? xiiHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash()) : 0;

  // Sort by material and then by mesh
  m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash + m_uiSubMeshIndex) & 0xFFFE) | m_uiFlipWinding;
}

bool xiiMeshRenderData::CanBatch(const xiiRenderData& other0) const
{
  const auto& other = xiiStaticCast<const xiiMeshRenderData&>(other0);

  return m_hMesh == other.m_hMesh && m_uiSubMeshIndex == other.m_uiSubMeshIndex && m_hMaterial == other.m_hMaterial && m_uiFlipWinding == other.m_uiFlipWinding;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiMeshComponentBase, 3)
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

void xiiMeshComponentBase::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  // ignore components that have created meshes (?)

  s << m_hMesh;

  s << m_Materials.GetCount();
  for (const auto& mat : m_Materials)
  {
    s << mat;
  }

  s << m_Color;
  s << m_fSortingDepthOffset;
}

void xiiMeshComponentBase::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_hMesh;

  if (uiVersion < 2)
  {
    xiiUInt32 uiCategory = 0;
    s >> uiCategory;
  }

  xiiUInt32 uiMaterials = 0;
  s >> uiMaterials;

  m_Materials.SetCount(uiMaterials);

  for (auto& mat : m_Materials)
  {
    s >> mat;
  }

  s >> m_Color;

  if (uiVersion >= 3)
  {
    s >> m_fSortingDepthOffset;
  }
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
      pRenderData->m_GlobalTransform     = GetOwner()->GetGlobalTransform() * pRenderData->m_GlobalTransform;
      pRenderData->m_GlobalBounds        = GetOwner()->GetGlobalBounds();
      pRenderData->m_fSortingDepthOffset = m_fSortingDepthOffset;
      pRenderData->m_hMesh               = m_hMesh;
      pRenderData->m_hMaterial           = hMaterial;
      pRenderData->m_Color               = m_Color;
      pRenderData->m_uiSubMeshIndex      = uiPartIndex;
      pRenderData->m_uiUniqueID          = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillSortingKey();
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

void xiiMeshComponentBase::SetColor(const xiiColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const xiiColor& xiiMeshComponentBase::GetColor() const
{
  return m_Color;
}

void xiiMeshComponentBase::SetSortingDepthOffset(float fOffset)
{
  m_fSortingDepthOffset = fOffset;

  InvalidateCachedRenderData();
}

float xiiMeshComponentBase::GetSortingDepthOffset() const
{
  return m_fSortingDepthOffset;
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

xiiStringView xiiMeshComponentBase::Materials_GetValue(xiiUInt32 uiIndex) const
{
  return GetMaterial(uiIndex).GetResourceID();
}

void xiiMeshComponentBase::Materials_SetValue(xiiUInt32 uiIndex, xiiStringView sValue)
{
  if (sValue.IsEmpty())
  {
    SetMaterial(uiIndex, xiiMaterialResourceHandle());
  }
  else
  {
    auto hMat = xiiResourceManager::LoadResource<xiiMaterialResource>(sValue);
    SetMaterial(uiIndex, hMat);
  }
}

void xiiMeshComponentBase::Materials_Insert(xiiUInt32 uiIndex, xiiStringView sValue)
{
  xiiMaterialResourceHandle hMat;

  if (!sValue.IsEmpty())
    hMat = xiiResourceManager::LoadResource<xiiMaterialResource>(sValue);

  m_Materials.InsertAt(uiIndex, hMat);

  InvalidateCachedRenderData();
}

void xiiMeshComponentBase::Materials_Remove(xiiUInt32 uiIndex)
{
  m_Materials.RemoveAtAndCopy(uiIndex);

  InvalidateCachedRenderData();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_MeshComponentBase);
