/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Render/DecalComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

namespace
{
  static xiiBoundingBoxSphere MakeDecalVolumeBounds(const xiiVec3& vHalfExtents)
  {
    const xiiVec3 vSafeHalfExtents = vHalfExtents.CompMax(xiiVec3(0.001f));
    return xiiBoundingBoxSphere::MakeFromCenterExtents(xiiVec3::MakeZero(), vSafeHalfExtents, vSafeHalfExtents.GetLength());
  }

  static xiiColor MultiplyColor(const xiiColor& lhs, const xiiColor& rhs)
  {
    return xiiColor(lhs.r * rhs.r, lhs.g * rhs.g, lhs.b * rhs.b, lhs.a * rhs.a);
  }
} // namespace

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalRenderData, 1, xiiRTTIDefaultAllocator<xiiDecalRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiDecalComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_ACCESSOR_PROPERTY("Mode", xiiDecalProjectionMode, GetMode, SetMode),
    XII_RESOURCE_ACCESSOR_PROPERTY("Decal", GetDecal, SetDecal)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Decal", xiiDependencyFlags::Package)),
    XII_RESOURCE_ACCESSOR_PROPERTY("Atlas", GetAtlas, SetAtlas)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_DecalAtlas", xiiDependencyFlags::Package)),
    XII_ACCESSOR_PROPERTY("AtlasId", GetAtlasId, SetAtlasId),
    XII_RESOURCE_ACCESSOR_PROPERTY("Mesh", GetMesh, SetMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh", xiiDependencyFlags::Package)),
    XII_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(1.0f, 1.0f, 0.25f)), new xiiClampValueAttribute(xiiVec3(0.001f), xiiVariant())),
    XII_ACCESSOR_PROPERTY("UVOffset", GetUVOffset, SetUVOffset),
    XII_ACCESSOR_PROPERTY("UVScale", GetUVScale, SetUVScale)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(1.0f))),
    XII_ACCESSOR_PROPERTY("Tint", GetTint, SetTint),
    XII_BITFLAGS_ACCESSOR_PROPERTY("ChannelMask", xiiDecalChannelMask, GetChannelMask, SetChannelMask)->AddAttributes(new xiiDefaultValueAttribute(xiiBitflags<xiiDecalChannelMask>(xiiDecalChannelMask::Default))),
    XII_ACCESSOR_PROPERTY("Opacity", GetOpacity, SetOpacity)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ACCESSOR_PROPERTY("NormalBlend", GetNormalBlend, SetNormalBlend)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ACCESSOR_PROPERTY("Roughness", GetRoughness, SetRoughness)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ACCESSOR_PROPERTY("Metallic", GetMetallic, SetMetallic)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ACCESSOR_PROPERTY("Emissive", GetEmissive, SetEmissive)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Priority", GetPriority, SetPriority)->AddAttributes(new xiiDefaultValueAttribute(128)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Decals"),
    new xiiBoxManipulatorAttribute("Extents", 2.0f, true),
    new xiiBoxVisualizerAttribute("Extents", 2.0f, xiiColorScheme::LightUI(xiiColorScheme::Cyan), "Tint"),
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 0.5f, xiiColorScheme::LightUI(xiiColorScheme::Cyan)),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiDecalComponent::xiiDecalComponent()  = default;
xiiDecalComponent::~xiiDecalComponent() = default;

void xiiDecalComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_Mode;
  s << m_hDecal;
  s << m_hAtlas;
  s << m_sAtlasId;
  s << m_hMesh;
  s << m_vExtents;
  s << m_vUVOffset;
  s << m_vUVScale;
  s << m_Tint;
  s << m_ChannelMask;
  s << m_fOpacity;
  s << m_fNormalBlend;
  s << m_fRoughness;
  s << m_fMetallic;
  s << m_fEmissive;
  s << m_uiPriority;
}

void xiiDecalComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_Mode;
  s >> m_hDecal;
  s >> m_hAtlas;
  s >> m_sAtlasId;
  s >> m_hMesh;
  s >> m_vExtents;
  s >> m_vUVOffset;
  s >> m_vUVScale;
  s >> m_Tint;
  s >> m_ChannelMask;
  s >> m_fOpacity;
  s >> m_fNormalBlend;
  s >> m_fRoughness;
  s >> m_fMetallic;
  s >> m_fEmissive;
  s >> m_uiPriority;
}

xiiResult xiiDecalComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);

  ref_bAlwaysVisible = false;

  if (m_Mode == xiiDecalProjectionMode::Mesh && m_hMesh.IsValid())
  {
    xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pMesh)
    {
      ref_bounds = pMesh->GetBounds();
      return ref_bounds.IsValid() ? XII_SUCCESS : XII_FAILURE;
    }
  }

  ref_bounds = MakeDecalVolumeBounds(m_vExtents);
  return XII_SUCCESS;
}

void xiiDecalComponent::SetMode(xiiEnum<xiiDecalProjectionMode> mode)
{
  if (m_Mode == mode)
    return;

  m_Mode = mode;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

xiiEnum<xiiDecalProjectionMode> xiiDecalComponent::GetMode() const
{
  return m_Mode;
}

void xiiDecalComponent::SetDecal(const xiiDecalResourceHandle& hDecal)
{
  if (m_hDecal == hDecal)
    return;

  m_hDecal = hDecal;
  InvalidateCachedRenderData();
}

const xiiDecalResourceHandle& xiiDecalComponent::GetDecal() const
{
  return m_hDecal;
}

void xiiDecalComponent::SetAtlas(const xiiDecalAtlasResourceHandle& hAtlas)
{
  if (m_hAtlas == hAtlas)
    return;

  m_hAtlas = hAtlas;
  InvalidateCachedRenderData();
}

const xiiDecalAtlasResourceHandle& xiiDecalComponent::GetAtlas() const
{
  return m_hAtlas;
}

void xiiDecalComponent::SetAtlasId(xiiStringView sAtlasId)
{
  if (m_sAtlasId.GetView() == sAtlasId)
    return;

  m_sAtlasId.Assign(sAtlasId);
  InvalidateCachedRenderData();
}

xiiStringView xiiDecalComponent::GetAtlasId() const
{
  return m_sAtlasId.GetView();
}

void xiiDecalComponent::SetMesh(const xiiMeshResourceHandle& hMesh)
{
  if (m_hMesh == hMesh)
    return;

  m_hMesh = hMesh;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

const xiiMeshResourceHandle& xiiDecalComponent::GetMesh() const
{
  return m_hMesh;
}

void xiiDecalComponent::SetExtents(xiiVec3 vExtents)
{
  vExtents = vExtents.CompMax(xiiVec3(0.001f));
  if (m_vExtents == vExtents)
    return;

  m_vExtents = vExtents;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

const xiiVec3& xiiDecalComponent::GetExtents() const
{
  return m_vExtents;
}

void xiiDecalComponent::SetUVOffset(xiiVec2 vOffset)
{
  if (m_vUVOffset == vOffset)
    return;

  m_vUVOffset = vOffset;
  InvalidateCachedRenderData();
}

const xiiVec2& xiiDecalComponent::GetUVOffset() const
{
  return m_vUVOffset;
}

void xiiDecalComponent::SetUVScale(xiiVec2 vScale)
{
  if (m_vUVScale == vScale)
    return;

  m_vUVScale = vScale;
  InvalidateCachedRenderData();
}

const xiiVec2& xiiDecalComponent::GetUVScale() const
{
  return m_vUVScale;
}

void xiiDecalComponent::SetTint(const xiiColor& tint)
{
  if (m_Tint == tint)
    return;

  m_Tint = tint;
  InvalidateCachedRenderData();
}

const xiiColor& xiiDecalComponent::GetTint() const
{
  return m_Tint;
}

void xiiDecalComponent::SetChannelMask(xiiBitflags<xiiDecalChannelMask> mask)
{
  if (m_ChannelMask == mask)
    return;

  m_ChannelMask = mask;
  InvalidateCachedRenderData();
}

xiiBitflags<xiiDecalChannelMask> xiiDecalComponent::GetChannelMask() const
{
  return m_ChannelMask;
}

void xiiDecalComponent::SetOpacity(float fOpacity)
{
  fOpacity = xiiMath::Clamp(fOpacity, 0.0f, 1.0f);
  if (m_fOpacity == fOpacity)
    return;

  m_fOpacity = fOpacity;
  InvalidateCachedRenderData();
}

float xiiDecalComponent::GetOpacity() const
{
  return m_fOpacity;
}

void xiiDecalComponent::SetNormalBlend(float fNormalBlend)
{
  fNormalBlend = xiiMath::Clamp(fNormalBlend, 0.0f, 1.0f);
  if (m_fNormalBlend == fNormalBlend)
    return;

  m_fNormalBlend = fNormalBlend;
  InvalidateCachedRenderData();
}

float xiiDecalComponent::GetNormalBlend() const
{
  return m_fNormalBlend;
}

void xiiDecalComponent::SetRoughness(float fRoughness)
{
  fRoughness = xiiMath::Clamp(fRoughness, 0.0f, 1.0f);
  if (m_fRoughness == fRoughness)
    return;

  m_fRoughness = fRoughness;
  InvalidateCachedRenderData();
}

float xiiDecalComponent::GetRoughness() const
{
  return m_fRoughness;
}

void xiiDecalComponent::SetMetallic(float fMetallic)
{
  fMetallic = xiiMath::Clamp(fMetallic, 0.0f, 1.0f);
  if (m_fMetallic == fMetallic)
    return;

  m_fMetallic = fMetallic;
  InvalidateCachedRenderData();
}

float xiiDecalComponent::GetMetallic() const
{
  return m_fMetallic;
}

void xiiDecalComponent::SetEmissive(float fEmissive)
{
  fEmissive = xiiMath::Max(fEmissive, 0.0f);
  if (m_fEmissive == fEmissive)
    return;

  m_fEmissive = fEmissive;
  InvalidateCachedRenderData();
}

float xiiDecalComponent::GetEmissive() const
{
  return m_fEmissive;
}

void xiiDecalComponent::SetPriority(xiiUInt8 uiPriority)
{
  if (m_uiPriority == uiPriority)
    return;

  m_uiPriority = uiPriority;
  InvalidateCachedRenderData();
}

xiiUInt8 xiiDecalComponent::GetPriority() const
{
  return m_uiPriority;
}

void xiiDecalComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  if (m_fOpacity <= 0.0f || m_ChannelMask.IsNoFlagSet())
    return;

  const xiiRenderWorldModule* pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiDecalResourceDescriptor decalDefaults;
  if (m_hDecal.IsValid())
  {
    xiiResourceLock<xiiDecalResource> pDecal(m_hDecal, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pDecal)
    {
      decalDefaults = pDecal->GetDescriptor();
    }
  }

  xiiDecalRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiDecalRenderData>(this);
  pRenderData->m_Mode             = m_Mode;
  pRenderData->m_hDecal           = m_hDecal;
  pRenderData->m_hAtlas           = m_hAtlas.IsValid() ? m_hAtlas : decalDefaults.m_hAtlas;
  pRenderData->m_sAtlasId         = !m_sAtlasId.IsEmpty() ? m_sAtlasId : decalDefaults.m_sDecalId;

  if (pRenderData->m_sAtlasId.IsEmpty() && m_hDecal.IsValid())
  {
    pRenderData->m_sAtlasId.Assign(m_hDecal.GetResourceID());
  }

  pRenderData->m_hAlbedo   = decalDefaults.m_hAlbedo;
  pRenderData->m_hNormal   = decalDefaults.m_hNormal;
  pRenderData->m_hMaterial = decalDefaults.m_hMaterial;
  pRenderData->m_hEmissive = decalDefaults.m_hEmissive;
  pRenderData->m_hMesh     = m_hMesh;

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds    = MakeDecalVolumeBounds(m_vExtents);
  pRenderData->m_GlobalBounds.Transform(pRenderData->m_GlobalTransform.GetAsMat4());

  pRenderData->m_vExtents  = m_vExtents;
  pRenderData->m_vUVOffset = xiiVec2(decalDefaults.m_vUVOffset.x + m_vUVOffset.x, decalDefaults.m_vUVOffset.y + m_vUVOffset.y);
  pRenderData->m_vUVScale  = xiiVec2(decalDefaults.m_vUVScale.x * m_vUVScale.x, decalDefaults.m_vUVScale.y * m_vUVScale.y);
  pRenderData->m_Tint      = MultiplyColor(decalDefaults.m_Tint, m_Tint);
  pRenderData->m_ChannelMask = m_ChannelMask;

  pRenderData->m_fOpacity     = xiiMath::Clamp(decalDefaults.m_fOpacity * m_fOpacity, 0.0f, 1.0f);
  pRenderData->m_fNormalBlend = xiiMath::Clamp(decalDefaults.m_fNormalBlend * m_fNormalBlend, 0.0f, 1.0f);
  pRenderData->m_fRoughness   = xiiMath::Clamp(m_fRoughness, 0.0f, 1.0f);
  pRenderData->m_fMetallic    = xiiMath::Clamp(m_fMetallic, 0.0f, 1.0f);
  pRenderData->m_fEmissive    = xiiMath::Max(m_fEmissive + decalDefaults.m_fEmissive, 0.0f);
  pRenderData->m_uiPriority   = m_uiPriority;
  pRenderData->m_uiUniqueID   = GetUniqueIdForRendering();

  if (m_Mode == xiiDecalProjectionMode::Mesh && m_hMesh.IsValid())
  {
    xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pMesh)
    {
      FillMeshRange(*pRenderData, *pMesh.GetPointer());
      pRenderData->m_GlobalBounds = pMesh->GetBounds();
      pRenderData->m_GlobalBounds.Transform(pRenderData->m_GlobalTransform.GetAsMat4());
    }
  }

  const xiiUInt64 uiPriorityKey = static_cast<xiiUInt64>(m_uiPriority) << 56U;
  const xiiUInt64 uiModeKey     = static_cast<xiiUInt64>(m_Mode.GetValue()) << 52U;
  pRenderData->m_uiSortingKey   = uiPriorityKey | uiModeKey | pRenderData->m_uiUniqueID;

  ref_msg.AddRenderData(pRenderData, GetOwner()->IsDynamic() ? xiiRenderData::Caching::Never : xiiRenderData::Caching::IfStatic);
}

void xiiDecalComponent::FillMeshRange(xiiDecalRenderData& ref_renderData, const xiiMeshResource& mesh) const
{
  ref_renderData.m_hMeshBuffer = mesh.GetMeshBuffer();

  const xiiArrayPtr<const xiiMeshLOD> pLODs = mesh.GetLODs();
  if (!pLODs.IsEmpty())
  {
    const xiiMeshLOD& lod = pLODs[0];
    ref_renderData.m_uiFirstPrimitive = 0U;
    ref_renderData.m_uiPrimitiveCount = 0U;
    ref_renderData.m_uiFirstMeshlet   = lod.m_uiFirstMeshlet;
    ref_renderData.m_uiMeshletCount   = lod.m_uiMeshletCount;

    if (!lod.m_Sections.IsEmpty())
    {
      ref_renderData.m_uiFirstPrimitive = lod.m_Sections[0].m_uiFirstPrimitive;
      for (const xiiMeshSection& section : lod.m_Sections)
      {
        ref_renderData.m_uiFirstPrimitive = xiiMath::Min(ref_renderData.m_uiFirstPrimitive, section.m_uiFirstPrimitive);
        ref_renderData.m_uiPrimitiveCount += section.m_uiPrimitiveCount;
      }
    }
  }

  const xiiArrayPtr<const xiiMeshSection> pSections = mesh.GetSections();
  if (!pSections.IsEmpty() && ref_renderData.m_uiPrimitiveCount == 0U)
  {
    ref_renderData.m_uiFirstPrimitive = pSections[0].m_uiFirstPrimitive;
    ref_renderData.m_uiPrimitiveCount = pSections[0].m_uiPrimitiveCount;
    ref_renderData.m_uiFirstMeshlet   = pSections[0].m_uiFirstMeshlet;
    ref_renderData.m_uiMeshletCount   = pSections[0].m_uiMeshletCount;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Render_Implementation_DecalComponent);
