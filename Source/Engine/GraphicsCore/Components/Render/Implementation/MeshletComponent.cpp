#include <GraphicsCore/GraphicsCorePCH.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Render/MeshletComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Resources/MeshletResource.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshletRenderData, 1, xiiRTTIDefaultAllocator<xiiMeshletRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshletRenderFlags, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiMeshletComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("MeshletAsset",   GetMeshletFile,      SetMeshletFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Meshlet")),
    XII_ACCESSOR_PROPERTY("FallbackMesh",   GetFallbackMeshFile, SetFallbackMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    XII_ACCESSOR_PROPERTY("Material0",      GetMat0Prop,         SetMat0Prop)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_ACCESSOR_PROPERTY("LODBias",        GetLODBias,          SetLODBias)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(-2.0f, 2.0f)),
    XII_ACCESSOR_PROPERTY("CastShadows",        GetCastShadows,        SetCastShadows)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("CastDynamicShadows", GetCastDynamicShadows, SetCastDynamicShadows)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("BackfaceCull",       GetBackfaceCull,       SetBackfaceCull)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("OcclusionCulling",   GetOcclusionCulling,   SetOcclusionCulling)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("ReceiveDecals",      GetReceiveDecals,      SetReceiveDecals)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Meshlet"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiMeshletComponent::xiiMeshletComponent()  = default;
xiiMeshletComponent::~xiiMeshletComponent() = default;

// ---- Serialization ----

void xiiMeshletComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  xiiStreamWriter& stream = s.GetStream();
  stream << m_hMeshlets;
  stream << m_hFallbackMesh;
  stream << m_RenderFlags.GetValue();
  stream << m_fLODBias;

  const xiiUInt32 uiMats = m_Materials.GetCount();
  stream << uiMats;
  for (const auto& hMat : m_Materials)
    stream << hMat;
}

void xiiMeshletComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  xiiStreamReader& stream = s.GetStream();
  stream >> m_hMeshlets;
  stream >> m_hFallbackMesh;

  xiiUInt8 flags = 0;
  stream >> flags;
  m_RenderFlags.SetValue(flags);
  stream >> m_fLODBias;

  xiiUInt32 uiMats = 0;
  stream >> uiMats;
  m_Materials.SetCount(uiMats);
  for (auto& hMat : m_Materials)
    stream >> hMat;
}

// ---- GetLocalBounds ----

xiiResult xiiMeshletComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);
  ref_bAlwaysVisible = false;

  if (!m_hMeshlets.IsValid())
    return XII_FAILURE;

  xiiResourceLock<xiiMeshletResource> pRes(m_hMeshlets, xiiResourceAcquireMode::BlockTillLoaded);
  if (pRes.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return XII_FAILURE;

  // Build a conservative AABB by unioning per-meshlet AABBs stored in the resource.
  // In a full implementation the resource stores a pre-computed aggregate AABB.
  // For now report a unit sphere so the object is always considered visible.
  ref_bounds = xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), 1.0f));
  return XII_SUCCESS;
}

// ---- Meshlet file property ----

void xiiMeshletComponent::SetMeshletFile(xiiStringView sFile)
{
  m_hMeshlets = sFile.IsEmpty() ? xiiMeshletResourceHandle{} : xiiResourceManager::LoadResource<xiiMeshletResource>(sFile);
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

xiiStringView xiiMeshletComponent::GetMeshletFile() const
{
  return m_hMeshlets.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hMeshlets) : xiiStringView{};
}

void xiiMeshletComponent::SetMeshlet(const xiiMeshletResourceHandle& h)
{
  m_hMeshlets = h;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

// ---- Fallback mesh ----

void xiiMeshletComponent::SetFallbackMeshFile(xiiStringView sFile)
{
  m_hFallbackMesh = sFile.IsEmpty() ? xiiMeshResourceHandle{} : xiiResourceManager::LoadResource<xiiMeshResource>(sFile);
  InvalidateCachedRenderData();
}

xiiStringView xiiMeshletComponent::GetFallbackMeshFile() const
{
  return m_hFallbackMesh.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hFallbackMesh) : xiiStringView{};
}

// ---- Materials ----

xiiUInt32 xiiMeshletComponent::GetMaterialCount() const { return m_Materials.GetCount(); }

void xiiMeshletComponent::SetMaterial(xiiUInt32 uiIndex, const xiiMaterialResourceHandle& hMaterial)
{
  m_Materials.EnsureCount(uiIndex + 1);
  m_Materials[uiIndex] = hMaterial;
  InvalidateCachedRenderData();
}

xiiMaterialResourceHandle xiiMeshletComponent::GetMaterial(xiiUInt32 uiIndex) const
{
  return (uiIndex < m_Materials.GetCount()) ? m_Materials[uiIndex] : xiiMaterialResourceHandle{};
}

void xiiMeshletComponent::SetMaterialFile(xiiUInt32 uiIndex, xiiStringView sFile)
{
  SetMaterial(uiIndex, sFile.IsEmpty() ? xiiMaterialResourceHandle{} : xiiResourceManager::LoadResource<xiiMaterialResource>(sFile));
}

xiiStringView xiiMeshletComponent::GetMaterialFile(xiiUInt32 uiIndex) const
{
  const xiiMaterialResourceHandle& h = GetMaterial(uiIndex);
  return h.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(h) : xiiStringView{};
}

void xiiMeshletComponent::SetMat0Prop(xiiStringView s) { SetMaterialFile(0, s); }
xiiStringView xiiMeshletComponent::GetMat0Prop()  const { return GetMaterialFile(0); }

// ---- Render flags ----

void xiiMeshletComponent::SetCastShadows(bool b)
{
  m_RenderFlags.AddOrRemove(xiiMeshletRenderFlags::CastShadows, b);
  InvalidateCachedRenderData();
}
bool xiiMeshletComponent::GetCastShadows() const { return m_RenderFlags.IsSet(xiiMeshletRenderFlags::CastShadows); }

void xiiMeshletComponent::SetCastDynamicShadows(bool b)
{
  m_RenderFlags.AddOrRemove(xiiMeshletRenderFlags::CastDynamicShadows, b);
  InvalidateCachedRenderData();
}
bool xiiMeshletComponent::GetCastDynamicShadows() const { return m_RenderFlags.IsSet(xiiMeshletRenderFlags::CastDynamicShadows); }

void xiiMeshletComponent::SetBackfaceCull(bool b)
{
  m_RenderFlags.AddOrRemove(xiiMeshletRenderFlags::UseBackfaceCull, b);
  InvalidateCachedRenderData();
}
bool xiiMeshletComponent::GetBackfaceCull() const { return m_RenderFlags.IsSet(xiiMeshletRenderFlags::UseBackfaceCull); }

void xiiMeshletComponent::SetOcclusionCulling(bool b)
{
  m_RenderFlags.AddOrRemove(xiiMeshletRenderFlags::UseOcclusionCulling, b);
  InvalidateCachedRenderData();
}
bool xiiMeshletComponent::GetOcclusionCulling() const { return m_RenderFlags.IsSet(xiiMeshletRenderFlags::UseOcclusionCulling); }

void xiiMeshletComponent::SetReceiveDecals(bool b)
{
  m_RenderFlags.AddOrRemove(xiiMeshletRenderFlags::ReceiveDecals, b);
  InvalidateCachedRenderData();
}
bool xiiMeshletComponent::GetReceiveDecals() const { return m_RenderFlags.IsSet(xiiMeshletRenderFlags::ReceiveDecals); }

// ---- LOD bias ----

void xiiMeshletComponent::SetLODBias(float f) { m_fLODBias = f; InvalidateCachedRenderData(); }

// ---- OnMsgExtractRenderData ----

void xiiMeshletComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (!m_hMeshlets.IsValid() || !ref_msg.m_pView || !ref_msg.m_pExtractedRenderData)
    return;

  auto* pWM = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (!pWM)
    return;

  // Try to lock the resource; if still loading, skip this frame (extracted data caching will retry).
  xiiResourceLock<xiiMeshletResource> pRes(m_hMeshlets, xiiResourceAcquireMode::AllowLoadingFallback);

  auto* pRD = pWM->CreateRenderDataForThisFrame<xiiMeshletRenderData>(this);

  pRD->m_GlobalTransform  = GetOwner()->GetGlobalTransform();
  pRD->m_GlobalBounds     = GetOwner()->GetGlobalBounds();
  pRD->m_hOwnerObject     = GetOwner()->GetHandle();
  pRD->m_hOwnerComponent  = GetHandle();
  pRD->m_uiSortingKey     = GetUniqueIdForRendering();

  pRD->m_hMeshlets        = m_hMeshlets;
  pRD->m_hFallbackMesh    = m_hFallbackMesh;
  pRD->m_RenderFlags      = m_RenderFlags;
  pRD->m_fLODBias         = m_fLODBias;

  if (pRes.GetAcquireResult() == xiiResourceAcquireResult::Final)
    pRD->m_uiMeshletCount = pRes->GetMeshletCount();

  pRD->m_Materials.SetCount(m_Materials.GetCount());
  for (xiiUInt32 i = 0; i < m_Materials.GetCount(); ++i)
    pRD->m_Materials[i] = m_Materials[i];

  ref_msg.AddRenderData(pRD, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Render_Implementation_MeshletComponent);
