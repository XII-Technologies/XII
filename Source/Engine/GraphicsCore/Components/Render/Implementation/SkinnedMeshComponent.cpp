#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Render/SkinnedMeshComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkinnedMeshRenderData, 1, xiiRTTIDefaultAllocator<xiiSkinnedMeshRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiSkinnedMeshComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned")),
    XII_ACCESSOR_PROPERTY("Material0", GetMaterialFile0Prop, SetMaterialFile0Prop)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_ACCESSOR_PROPERTY("CastShadows", GetCastShadows, SetCastShadows)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Meshes"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiSkinnedMeshComponent::xiiSkinnedMeshComponent()  = default;
xiiSkinnedMeshComponent::~xiiSkinnedMeshComponent() = default;

void xiiSkinnedMeshComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hMesh;
  s << static_cast<xiiUInt32>(m_Materials.GetCount());
  for (const auto& hMat : m_Materials)
    s << hMat;

  s << m_bCastShadows;
}

void xiiSkinnedMeshComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_hMesh;

  xiiUInt32 uiMaterialCount = 0;
  s >> uiMaterialCount;
  m_Materials.SetCount(uiMaterialCount);
  for (auto& hMat : m_Materials)
    s >> hMat;

  s >> m_bCastShadows;
}

xiiResult xiiSkinnedMeshComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bAlwaysVisible);
  XII_IGNORE_UNUSED(ref_msg);

  if (m_hMesh.IsValid())
  {
    xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
    ref_bounds = pMesh->GetBounds();
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiSkinnedMeshComponent::SetMeshFile(xiiStringView sFile)
{
  xiiMeshResourceHandle hMesh;
  if (!sFile.IsEmpty())
    hMesh = xiiResourceManager::LoadResource<xiiMeshResource>(sFile);
  SetMesh(hMesh);
}

xiiStringView xiiSkinnedMeshComponent::GetMeshFile() const
{
  if (m_hMesh.IsValid())
    return xiiResourceManager::GetResourceIDOrDescription(m_hMesh);
  return {};
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
  if (uiIndex < m_Materials.GetCount())
    return m_Materials[uiIndex];
  return {};
}

void xiiSkinnedMeshComponent::SetMaterialFile(xiiUInt32 uiIndex, xiiStringView sFile)
{
  xiiMaterialResourceHandle hMat;
  if (!sFile.IsEmpty())
    hMat = xiiResourceManager::LoadResource<xiiMaterialResource>(sFile);
  SetMaterial(uiIndex, hMat);
}

xiiStringView xiiSkinnedMeshComponent::GetMaterialFile(xiiUInt32 uiIndex) const
{
  if (uiIndex < m_Materials.GetCount() && m_Materials[uiIndex].IsValid())
    return xiiResourceManager::GetResourceIDOrDescription(m_Materials[uiIndex]);
  return {};
}

void          xiiSkinnedMeshComponent::SetMaterialFile0Prop(xiiStringView s) { SetMaterialFile(0, s); }
xiiStringView xiiSkinnedMeshComponent::GetMaterialFile0Prop() const { return GetMaterialFile(0); }

void xiiSkinnedMeshComponent::SetCastShadows(bool bCast)
{
  m_bCastShadows = bCast;
  InvalidateCachedRenderData();
}

void xiiSkinnedMeshComponent::SetBonePalette(xiiArrayPtr<const xiiMat4> palette)
{
  m_BonePalette.SetCount(palette.GetCount());
  xiiMemoryUtils::Copy(m_BonePalette.GetData(), palette.GetPtr(), palette.GetCount());
}

void xiiSkinnedMeshComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (!m_hMesh.IsValid() || ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiSkinnedMeshRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiSkinnedMeshRenderData>(this);
  pRenderData->m_GlobalTransform        = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds           = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject           = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent        = GetHandle();
  pRenderData->m_hMesh                  = m_hMesh;
  pRenderData->m_Materials              = m_Materials;
  pRenderData->m_uiBoneCount            = static_cast<xiiUInt16>(m_BonePalette.GetCount());
  pRenderData->m_bCastShadows           = m_bCastShadows;
  pRenderData->m_uiSortingKey           = GetUniqueIdForRendering();
  // m_uiBonePaletteOffset is filled by the renderer when it uploads the palette

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Render_Implementation_SkinnedMeshComponent);
