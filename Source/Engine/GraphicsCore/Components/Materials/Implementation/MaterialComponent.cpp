#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Materials/MaterialComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMaterialRenderData, 1, xiiRTTIDefaultAllocator<xiiMaterialRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiMaterialComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Material0", GetMaterial0File, SetMaterial0File)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Materials"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMaterialVariantRenderData, 1, xiiRTTIDefaultAllocator<xiiMaterialVariantRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiMaterialVariantComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("ActiveVariant", GetActiveVariant, SetActiveVariant),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Materials"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

// ==================== xiiMaterialComponent ====================

xiiMaterialComponent::xiiMaterialComponent()  = default;
xiiMaterialComponent::~xiiMaterialComponent() = default;

void xiiMaterialComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << static_cast<xiiUInt32>(m_Materials.GetCount());
  for (const auto& h : m_Materials) s << h;
}

void xiiMaterialComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto&     s = inout_stream.GetStream();
  xiiUInt32 n = 0;
  s >> n;
  m_Materials.SetCount(n);
  for (auto& h : m_Materials) s >> h;
}

xiiResult xiiMaterialComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bounds);
  XII_IGNORE_UNUSED(ref_bAlwaysVisible);
  XII_IGNORE_UNUSED(ref_msg);
  return XII_FAILURE; // Bounds from co-located mesh component
}

xiiUInt32 xiiMaterialComponent::GetMaterialCount() const { return m_Materials.GetCount(); }

void xiiMaterialComponent::SetMaterial(xiiUInt32 uiSlot, const xiiMaterialResourceHandle& hMat)
{
  m_Materials.EnsureCount(uiSlot + 1);
  m_Materials[uiSlot] = hMat;
  InvalidateCachedRenderData();
}

xiiMaterialResourceHandle xiiMaterialComponent::GetMaterial(xiiUInt32 uiSlot) const
{
  return (uiSlot < m_Materials.GetCount()) ? m_Materials[uiSlot] : xiiMaterialResourceHandle{};
}

void xiiMaterialComponent::SetMaterialFile(xiiUInt32 uiSlot, xiiStringView sFile)
{
  SetMaterial(uiSlot, sFile.IsEmpty() ? xiiMaterialResourceHandle{} : xiiResourceManager::LoadResource<xiiMaterialResource>(sFile));
}

xiiStringView xiiMaterialComponent::GetMaterialFile(xiiUInt32 uiSlot) const
{
  const auto& h = GetMaterial(uiSlot);
  return h.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(h) : xiiStringView{};
}

void          xiiMaterialComponent::SetMaterial0File(xiiStringView s) { SetMaterialFile(0, s); }
xiiStringView xiiMaterialComponent::GetMaterial0File() const { return GetMaterialFile(0); }

void xiiMaterialComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (m_Materials.IsEmpty() || ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;
  auto pWM = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (!pWM) return;
  auto* pRD              = pWM->CreateRenderDataForThisFrame<xiiMaterialRenderData>(this);
  pRD->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRD->m_GlobalBounds    = GetOwner()->GetGlobalBounds();
  pRD->m_hOwnerObject    = GetOwner()->GetHandle();
  pRD->m_hOwnerComponent = GetHandle();
  pRD->m_Materials       = m_Materials;
  pRD->m_uiSortingKey    = GetUniqueIdForRendering();
  ref_msg.AddRenderData(pRD, xiiRenderData::Caching::IfStatic);
}

// ==================== xiiMaterialVariantComponent ====================

xiiMaterialVariantComponent::xiiMaterialVariantComponent()  = default;
xiiMaterialVariantComponent::~xiiMaterialVariantComponent() = default;

void xiiMaterialVariantComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_sActiveVariant.GetString();
  s << static_cast<xiiUInt32>(m_Variants.GetCount());
  for (const auto& v : m_Variants)
  {
    s << v.m_sName.GetString();
    s << v.m_hMaterial;
  }
}

void xiiMaterialVariantComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto&     s = inout_stream.GetStream();
  xiiString sName;
  s >> sName;
  m_sActiveVariant.Assign(sName);
  xiiUInt32 n = 0;
  s >> n;
  m_Variants.SetCount(n);
  for (auto& v : m_Variants)
  {
    s >> sName;
    v.m_sName.Assign(sName);
    s >> v.m_hMaterial;
  }
}

xiiResult xiiMaterialVariantComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bounds);
  XII_IGNORE_UNUSED(ref_bAlwaysVisible);
  XII_IGNORE_UNUSED(ref_msg);
  return XII_FAILURE;
}

void xiiMaterialVariantComponent::SetActiveVariant(xiiStringView sName)
{
  m_sActiveVariant.Assign(sName);
  InvalidateCachedRenderData();
}

xiiStringView xiiMaterialVariantComponent::GetActiveVariant() const { return m_sActiveVariant.GetString(); }

void xiiMaterialVariantComponent::AddVariant(xiiStringView sName, const xiiMaterialResourceHandle& hMat)
{
  auto& v = m_Variants.ExpandAndGetRef();
  v.m_sName.Assign(sName);
  v.m_hMaterial = hMat;
}

xiiMaterialResourceHandle xiiMaterialVariantComponent::GetVariantMaterial(xiiStringView sName) const
{
  xiiTempHashedString key(sName);
  for (const auto& v : m_Variants)
    if (v.m_sName == key)
      return v.m_hMaterial;
  return {};
}

void xiiMaterialVariantComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr) return;
  auto pWM = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (!pWM) return;

  xiiUInt32 uiActive = 0;
  for (xiiUInt32 i = 0; i < m_Variants.GetCount(); ++i)
    if (m_Variants[i].m_sName == m_sActiveVariant)
    {
      uiActive = i;
      break;
    }

  auto* pRD              = pWM->CreateRenderDataForThisFrame<xiiMaterialVariantRenderData>(this);
  pRD->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRD->m_GlobalBounds    = GetOwner()->GetGlobalBounds();
  pRD->m_hOwnerObject    = GetOwner()->GetHandle();
  pRD->m_hOwnerComponent = GetHandle();
  pRD->m_sActiveVariant  = m_sActiveVariant;
  pRD->m_uiActiveIndex   = uiActive;
  pRD->m_uiSortingKey    = GetUniqueIdForRendering();
  ref_msg.AddRenderData(pRD, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Materials_Implementation_MaterialComponent);
