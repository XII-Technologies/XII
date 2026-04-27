#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Render/InstancedMeshComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInstancedMeshRenderData, 1, xiiRTTIDefaultAllocator<xiiInstancedMeshRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiInstancedMeshComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    XII_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
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

xiiInstancedMeshComponent::xiiInstancedMeshComponent()  = default;
xiiInstancedMeshComponent::~xiiInstancedMeshComponent() = default;

void xiiInstancedMeshComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_hMesh;
  s << m_hMaterial;
  s << m_bCastShadows;

  s << static_cast<xiiUInt32>(m_Instances.GetCount());
  for (const auto& inst : m_Instances)
  {
    s << inst.m_Transform;
    s << inst.m_Color;
  }
}

void xiiInstancedMeshComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_hMesh;
  s >> m_hMaterial;
  s >> m_bCastShadows;

  xiiUInt32 uiCount = 0;
  s >> uiCount;
  m_Instances.SetCount(uiCount);
  for (auto& inst : m_Instances)
  {
    s >> inst.m_Transform;
    s >> inst.m_Color;
  }

  RecomputeBounds();
}

xiiResult xiiInstancedMeshComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bAlwaysVisible);
  XII_IGNORE_UNUSED(ref_msg);

  if (m_LocalBounds.IsValid())
  {
    ref_bounds = m_LocalBounds;
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiInstancedMeshComponent::SetMeshFile(xiiStringView sFile)
{
  xiiMeshResourceHandle hMesh;
  if (!sFile.IsEmpty())
    hMesh = xiiResourceManager::LoadResource<xiiMeshResource>(sFile);
  SetMesh(hMesh);
}

xiiStringView xiiInstancedMeshComponent::GetMeshFile() const
{
  if (m_hMesh.IsValid())
    return xiiResourceManager::GetResourceIDOrDescription(m_hMesh);
  return {};
}

void xiiInstancedMeshComponent::SetMesh(const xiiMeshResourceHandle& hMesh)
{
  m_hMesh = hMesh;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiInstancedMeshComponent::SetMaterialFile(xiiStringView sFile)
{
  xiiMaterialResourceHandle hMat;
  if (!sFile.IsEmpty())
    hMat = xiiResourceManager::LoadResource<xiiMaterialResource>(sFile);
  SetMaterial(hMat);
}

xiiStringView xiiInstancedMeshComponent::GetMaterialFile() const
{
  if (m_hMaterial.IsValid())
    return xiiResourceManager::GetResourceIDOrDescription(m_hMaterial);
  return {};
}

void xiiInstancedMeshComponent::SetMaterial(const xiiMaterialResourceHandle& hMaterial)
{
  m_hMaterial = hMaterial;
  InvalidateCachedRenderData();
}

void xiiInstancedMeshComponent::SetCastShadows(bool bCast)
{
  m_bCastShadows = bCast;
  InvalidateCachedRenderData();
}

void xiiInstancedMeshComponent::SetInstances(xiiArrayPtr<const xiiMeshInstanceData> instances)
{
  m_Instances.SetCount(instances.GetCount());
  xiiMemoryUtils::Copy(m_Instances.GetData(), instances.GetPtr(), instances.GetCount());
  RecomputeBounds();
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiInstancedMeshComponent::RecomputeBounds()
{
  m_LocalBounds = xiiBoundingBoxSphere::MakeInvalid();
  for (const auto& inst : m_Instances)
  {
    xiiBoundingSphere sphere = xiiBoundingSphere::MakeFromCenterAndRadius(inst.m_Transform.m_vPosition, 0.5f);
    m_LocalBounds.ExpandToInclude(xiiBoundingBoxSphere::MakeFromSphere(sphere));
  }
}

void xiiInstancedMeshComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (!m_hMesh.IsValid() || m_Instances.IsEmpty() || ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiInstancedMeshRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiInstancedMeshRenderData>(this);
  pRenderData->m_GlobalTransform          = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds             = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject             = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent          = GetHandle();
  pRenderData->m_hMesh                    = m_hMesh;
  pRenderData->m_hMaterial                = m_hMaterial;
  pRenderData->m_Instances                = m_Instances.GetArrayPtr();
  pRenderData->m_uiInstanceCount          = m_Instances.GetCount();
  pRenderData->m_bCastShadows             = m_bCastShadows;
  pRenderData->m_uiSortingKey             = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Render_Implementation_InstancedMeshComponent);
