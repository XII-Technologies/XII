#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Render/StaticMeshComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/IndirectDrawBatchBuilder.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStaticMeshRenderData, 1, xiiRTTIDefaultAllocator<xiiStaticMeshRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiStaticMeshComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Mesh",                GetMeshFile,          SetMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    XII_ACCESSOR_PROPERTY("Material0",           GetMaterialFile0Prop, SetMaterialFile0Prop)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_ACCESSOR_PROPERTY("LODBias",             GetLODBias,           SetLODBias)->AddAttributes(new xiiDefaultValueAttribute((xiiInt8)0), new xiiClampValueAttribute((xiiInt8)-4, (xiiInt8)4)),
    XII_ACCESSOR_PROPERTY("CastShadows",         GetCastShadows,       SetCastShadows)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("CastDynamicShadows",  GetCastDynamicShadows,SetCastDynamicShadows)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiStaticMeshComponent::xiiStaticMeshComponent()  = default;
xiiStaticMeshComponent::~xiiStaticMeshComponent() = default;

// ---- Serialization ----

void xiiStaticMeshComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  xiiStreamWriter& stream = s.GetStream();
  stream << m_hMesh;
  stream << m_iLODBias;
  stream << m_bCastShadows;
  stream << m_bCastDynShadows;

  const xiiUInt32 uiMats = m_Materials.GetCount();
  stream << uiMats;
  for (const auto& h : m_Materials)
    stream << h;
}

void xiiStaticMeshComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  xiiStreamReader& stream = s.GetStream();
  stream >> m_hMesh;
  stream >> m_iLODBias;
  stream >> m_bCastShadows;
  stream >> m_bCastDynShadows;

  xiiUInt32 uiMats = 0;
  stream >> uiMats;
  m_Materials.SetCount(uiMats);
  for (auto& h : m_Materials)
    stream >> h;
}

// ---- Bounds ----

xiiResult xiiStaticMeshComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);
  ref_bAlwaysVisible = false;

  if (!m_hMesh.IsValid())
    return XII_FAILURE;

  xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::BlockTillLoaded);
  if (pMesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return XII_FAILURE;

  ref_bounds = pMesh->GetBounds();
  return XII_SUCCESS;
}

// ---- Mesh property ----

void xiiStaticMeshComponent::SetMeshFile(xiiStringView sFile)
{
  m_hMesh = sFile.IsEmpty() ? xiiMeshResourceHandle{} : xiiResourceManager::LoadResource<xiiMeshResource>(sFile);
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

xiiStringView xiiStaticMeshComponent::GetMeshFile() const
{
  return m_hMesh.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hMesh) : xiiStringView{};
}

void xiiStaticMeshComponent::SetMesh(const xiiMeshResourceHandle& hMesh)
{
  m_hMesh = hMesh;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

// ---- Materials ----

xiiUInt32 xiiStaticMeshComponent::GetMaterialCount() const { return m_Materials.GetCount(); }

void xiiStaticMeshComponent::SetMaterial(xiiUInt32 uiIndex, const xiiMaterialResourceHandle& hMaterial)
{
  m_Materials.EnsureCount(uiIndex + 1);
  m_Materials[uiIndex] = hMaterial;
  InvalidateCachedRenderData();
}

xiiMaterialResourceHandle xiiStaticMeshComponent::GetMaterial(xiiUInt32 uiIndex) const
{
  return (uiIndex < m_Materials.GetCount()) ? m_Materials[uiIndex] : xiiMaterialResourceHandle{};
}

void xiiStaticMeshComponent::SetMaterialFile(xiiUInt32 uiIndex, xiiStringView sFile)
{
  SetMaterial(uiIndex, sFile.IsEmpty() ? xiiMaterialResourceHandle{} : xiiResourceManager::LoadResource<xiiMaterialResource>(sFile));
}

xiiStringView xiiStaticMeshComponent::GetMaterialFile(xiiUInt32 uiIndex) const
{
  const xiiMaterialResourceHandle& h = GetMaterial(uiIndex);
  return h.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(h) : xiiStringView{};
}

void          xiiStaticMeshComponent::SetMaterialFile0Prop(xiiStringView s) { SetMaterialFile(0, s); }
xiiStringView xiiStaticMeshComponent::GetMaterialFile0Prop() const          { return GetMaterialFile(0); }

// ---- Flags ----

void xiiStaticMeshComponent::SetLODBias(xiiInt8 iBias)
{
  m_iLODBias = xiiMath::Clamp<xiiInt8>(iBias, -4, 4);
  InvalidateCachedRenderData();
}

void xiiStaticMeshComponent::SetCastShadows(bool bCast)
{
  m_bCastShadows = bCast;
  InvalidateCachedRenderData();
}

void xiiStaticMeshComponent::SetCastDynamicShadows(bool bCast)
{
  m_bCastDynShadows = bCast;
  InvalidateCachedRenderData();
}

// ---- OnMsgExtractRenderData ----

void xiiStaticMeshComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (!m_hMesh.IsValid() || !ref_msg.m_pView || !ref_msg.m_pExtractedRenderData)
    return;

  auto* pWM = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (!pWM)
    return;

  // ---- GPU-driven path: write one xiiGPUInstanceData record ----
  // If the view has a xiiIndirectDrawBatchBuilder attached (set by the GPU-driven render pipeline),
  // we write directly to the batch builder instead of creating per-component render data.

  xiiIndirectDrawBatchBuilder* pBatchBuilder = ref_msg.m_pView->GetSharedData<xiiIndirectDrawBatchBuilder>();
  if (pBatchBuilder != nullptr && m_hMesh.IsValid())
  {
    xiiGPUInstanceData inst;
    inst.m_LocalToWorld     = GetOwner()->GetGlobalTransform().GetAsMat4();
    inst.m_LocalToWorldPrev = inst.m_LocalToWorld; // TODO: previous-frame transform tracking
    inst.m_uiEntityID       = GetUniqueIdForRendering();
    inst.m_uiFlags          = (m_bCastShadows ? 1u : 0u) | (m_bCastDynShadows ? 2u : 0u);
    inst.m_uiLODLevel       = 0; // CPU LOD selection would go here
    inst.m_fScreenCoverage  = 1.0f;

    const xiiBoundingBoxSphere& b = GetOwner()->GetGlobalBounds();
    inst.m_vAABBMin = b.GetBox().m_vMin;
    inst.m_vAABBMax = b.GetBox().m_vMax;

    const xiiMaterialResourceHandle hMat = m_Materials.IsEmpty() ? xiiMaterialResourceHandle{} : m_Materials[0];
    pBatchBuilder->AddStaticMeshInstance(inst, m_hMesh, hMat);
    return;
  }

  // ---- Fallback: traditional per-component render data (non-GPU-driven mode) ----
  auto* pRD = pWM->CreateRenderDataForThisFrame<xiiStaticMeshRenderData>(this);

  pRD->m_GlobalTransform  = GetOwner()->GetGlobalTransform();
  pRD->m_GlobalBounds     = GetOwner()->GetGlobalBounds();
  pRD->m_hOwnerObject     = GetOwner()->GetHandle();
  pRD->m_hOwnerComponent  = GetHandle();
  pRD->m_uiSortingKey     = GetUniqueIdForRendering();

  pRD->m_hMesh            = m_hMesh;
  pRD->m_bCastShadows     = m_bCastShadows;
  pRD->m_bCastDynShadow   = m_bCastDynShadows;

  // LOD selection: use per-view camera distance and LOD bias
  pRD->m_uiActiveLOD = 0;
  if (ref_msg.m_pView)
  {
    const xiiVec3 vCamPos = ref_msg.m_pView->GetCamera()->GetPosition();
    const xiiVec3 vObjPos = GetOwner()->GetGlobalPosition();
    const float   fDist   = (vCamPos - vObjPos).GetLength();
    const float   fBiased = fDist * xiiMath::Pow(2.0f, -static_cast<float>(m_iLODBias));
    // Select LOD from mesh resource (LOD thresholds stored in MeshResource)
    xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
    if (pMesh.GetAcquireResult() == xiiResourceAcquireResult::Final)
      pRD->m_uiActiveLOD = pMesh->GetLODForDistance(fBiased);
  }

  // Material overrides
  pRD->m_Materials.SetCount(m_Materials.GetCount());
  for (xiiUInt32 i = 0; i < m_Materials.GetCount(); ++i)
    pRD->m_Materials[i] = m_Materials[i];

  ref_msg.AddRenderData(pRD, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Render_Implementation_StaticMeshComponent);
