#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Render/SkinnedMeshComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GAL/Device/GALDevice.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkinnedMeshRenderData, 1, xiiRTTIDefaultAllocator<xiiSkinnedMeshRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSkinnedMeshComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Mesh",       GetMeshFile,      SetMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned")),
    XII_ACCESSOR_PROPERTY("Material0",  GetMaterial0Prop, SetMaterial0Prop)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_ACCESSOR_PROPERTY("CastShadows",GetCastShadows,   SetCastShadows)->AddAttributes(new xiiDefaultValueAttribute(true)),
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

xiiSkinnedMeshComponent::xiiSkinnedMeshComponent()  = default;
xiiSkinnedMeshComponent::~xiiSkinnedMeshComponent()
{
  xiiGALDevice* pDev = xiiGALDevice::GetDefaultDevice();
  if (pDev && m_hSkinningTransforms.IsValid())
    pDev->DestroyBuffer(m_hSkinningTransforms);
}

// ---- Serialization ----

void xiiSkinnedMeshComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  xiiStreamWriter& stream = s.GetStream();
  stream << m_hMesh;
  stream << m_bCastShadows;

  const xiiUInt32 uiMats = m_Materials.GetCount();
  stream << uiMats;
  for (const auto& h : m_Materials)
    stream << h;
}

void xiiSkinnedMeshComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  xiiStreamReader& stream = s.GetStream();
  stream >> m_hMesh;
  stream >> m_bCastShadows;

  xiiUInt32 uiMats = 0;
  stream >> uiMats;
  m_Materials.SetCount(uiMats);
  for (auto& h : m_Materials)
    stream >> h;
}

// ---- Bounds ----

xiiResult xiiSkinnedMeshComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);
  ref_bAlwaysVisible = false;

  if (m_SkinningBounds.IsValid())
  {
    ref_bounds = m_SkinningBounds;
    return XII_SUCCESS;
  }

  if (!m_hMesh.IsValid())
    return XII_FAILURE;

  xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::BlockTillLoaded);
  if (pMesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return XII_FAILURE;

  // Fallback to static mesh bounds if animation hasn't supplied updated bounds
  ref_bounds = pMesh->GetBounds();
  return XII_SUCCESS;
}

// ---- Properties ----

void xiiSkinnedMeshComponent::SetMeshFile(xiiStringView sFile)
{
  m_hMesh = sFile.IsEmpty() ? xiiMeshResourceHandle{} : xiiResourceManager::LoadResource<xiiMeshResource>(sFile);
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

xiiStringView xiiSkinnedMeshComponent::GetMeshFile() const
{
  return m_hMesh.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hMesh) : xiiStringView{};
}

void xiiSkinnedMeshComponent::SetMesh(const xiiMeshResourceHandle& hMesh)
{
  m_hMesh = hMesh;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

xiiUInt32 xiiSkinnedMeshComponent::GetMaterialCount() const { return m_Materials.GetCount(); }

void xiiSkinnedMeshComponent::SetMaterial(xiiUInt32 uiIndex, const xiiMaterialResourceHandle& hMaterial)
{
  m_Materials.EnsureCount(uiIndex + 1);
  m_Materials[uiIndex] = hMaterial;
  InvalidateCachedRenderData();
}

xiiMaterialResourceHandle xiiSkinnedMeshComponent::GetMaterial(xiiUInt32 uiIndex) const
{
  return (uiIndex < m_Materials.GetCount()) ? m_Materials[uiIndex] : xiiMaterialResourceHandle{};
}

void xiiSkinnedMeshComponent::SetMaterialFile(xiiUInt32 uiIndex, xiiStringView sFile)
{
  SetMaterial(uiIndex, sFile.IsEmpty() ? xiiMaterialResourceHandle{} : xiiResourceManager::LoadResource<xiiMaterialResource>(sFile));
}

xiiStringView xiiSkinnedMeshComponent::GetMaterialFile(xiiUInt32 uiIndex) const
{
  const xiiMaterialResourceHandle& h = GetMaterial(uiIndex);
  return h.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(h) : xiiStringView{};
}

void          xiiSkinnedMeshComponent::SetMaterial0Prop(xiiStringView s) { SetMaterialFile(0, s); }
xiiStringView xiiSkinnedMeshComponent::GetMaterial0Prop() const          { return GetMaterialFile(0); }

void xiiSkinnedMeshComponent::SetCastShadows(bool b)
{
  m_bCastShadows = b;
  InvalidateCachedRenderData();
}

// ---- GPU Skinning ----

void xiiSkinnedMeshComponent::UpdateSkinningTransforms(xiiArrayPtr<const xiiMat4> transforms)
{
  if (transforms.IsEmpty())
    return;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (!pDevice)
    return;

  const xiiUInt32 uiBytes = transforms.GetCount() * sizeof(xiiMat4);

  if (!m_hSkinningTransforms.IsValid())
  {
    xiiGALBufferCreationDescription bd;
    bd.m_sDebugName         = "BoneMatrices";
    bd.m_uiSize             = xiiMath::Max(uiBytes, 64u);
    bd.m_BindFlags          = xiiGALBindFlags::ShaderResource;
    bd.m_ResourceUsage      = xiiGALResourceUsage::Dynamic;
    bd.m_CPUAccessFlags     = xiiGALCPUAccessFlags::Write;
    bd.m_Mode               = xiiGALBufferMode::Structured;
    bd.m_uiElementByteStride= sizeof(xiiMat4);
    m_hSkinningTransforms = pDevice->CreateBuffer(bd);
  }

  void* pMap = pDevice->MapBuffer(m_hSkinningTransforms, xiiGALMapType::Write, xiiGALMapFlags::Discard);
  if (pMap)
  {
    xiiMemoryUtils::Copy(static_cast<xiiMat4*>(pMap), transforms.GetPtr(), transforms.GetCount());
    pDevice->UnmapBuffer(m_hSkinningTransforms, xiiGALMapType::Write);
  }

  // Calculate new bounds from bone matrices (simplified: just taking positions)
  xiiBoundingBoxSphere newBounds = xiiBoundingBoxSphere::MakeInvalid();
  for (const xiiMat4& t : transforms)
  {
    newBounds.ExpandToInclude(t.GetTranslationVector());
  }
  // Add an arbitrary margin for the mesh volume around the bones. In a full system,
  // we would use the per-bone bounds from the skeleton.
  newBounds.m_fSphereRadius += 2.0f;
  newBounds.m_vBoxHalfExtents += xiiVec3(2.0f);

  m_SkinningBounds = newBounds;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

// ---- OnMsgExtractRenderData ----

void xiiSkinnedMeshComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (!m_hMesh.IsValid() || !m_hSkinningTransforms.IsValid() || !ref_msg.m_pView || !ref_msg.m_pExtractedRenderData)
    return;

  auto* pWM = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (!pWM)
    return;

  auto* pRD = pWM->CreateRenderDataForThisFrame<xiiSkinnedMeshRenderData>(this);

  pRD->m_GlobalTransform  = GetOwner()->GetGlobalTransform();
  pRD->m_GlobalBounds     = GetOwner()->GetGlobalBounds();
  pRD->m_hOwnerObject     = GetOwner()->GetHandle();
  pRD->m_hOwnerComponent  = GetHandle();
  pRD->m_uiSortingKey     = GetUniqueIdForRendering();

  pRD->m_hMesh               = m_hMesh;
  pRD->m_hSkinningTransforms = m_hSkinningTransforms;
  pRD->m_bCastShadows        = m_bCastShadows;

  pRD->m_Materials.SetCount(m_Materials.GetCount());
  for (xiiUInt32 i = 0; i < m_Materials.GetCount(); ++i)
    pRD->m_Materials[i] = m_Materials[i];

  ref_msg.AddRenderData(pRD, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Render_Implementation_SkinnedMeshComponent);
