/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <SharedPluginScene/Scene/Implementation/SceneMeshPickingState.h>

xiiSceneMeshPickingState::xiiSceneMeshPickingState()
{
  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiSceneMeshPickingState::OnResourceEvent, this));
}

xiiSceneMeshPickingState::~xiiSceneMeshPickingState()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiSceneMeshPickingState::OnResourceEvent, this));
  xiiGpuVisibilityManager::DestroyContext(m_hVisibility);
}

void xiiSceneMeshPickingState::OnResourceEvent(const xiiResourceEvent& event)
{
  if (event.m_Type != xiiResourceEvent::Type::ResourceContentUpdated && event.m_Type != xiiResourceEvent::Type::ResourceContentUnloading)
    return;
  const auto* type = event.m_pResource->GetDynamicRTTI();
  if (type->IsDerivedFrom<xiiMeshResource>() || type->IsDerivedFrom<xiiMeshBufferResource>() || type->IsDerivedFrom<xiiMaterialResource>() || type->IsDerivedFrom<xiiTexture2DResource>() ||
      type->IsDerivedFrom<xiiShaderResource>() || type->IsDerivedFrom<xiiShaderPermutationResource>())
    m_ResourceEpoch.Increment(); // Loader threads never mutate readback or owner storage.
}

xiiUInt64 xiiSceneMeshPickingState::MaterialSignature(const xiiMeshComponentBase& component)
{
  xiiUInt64 signature = 0;
  for (const auto& material : component.GetMaterialOverrides())
  {
    const auto id = material.GetResourceIDHash();
    signature     = xiiHashingUtils::xxHash64(&id, sizeof(id), signature);
  }
  return signature;
}

void xiiSceneMeshPickingState::Invalidate()
{
  m_Readback.Invalidate();
  m_Owners.Clear();
  for (auto& owners : m_SlotOwners)
    owners.Clear();
}

xiiResult xiiSceneMeshPickingState::Poll()
{
  const xiiInt32 epoch = m_ResourceEpoch;
  if (epoch != m_iObservedEpoch)
  {
    Invalidate();
    m_iObservedEpoch = epoch;
  }
  xiiUInt32 slot;
  XII_SUCCEED_OR_RETURN(m_Readback.Poll(slot));
  if (slot != xiiInvalidIndex)
    m_Owners.Swap(m_SlotOwners[slot]);
  return XII_SUCCESS;
}

xiiResult xiiSceneMeshPickingState::ResizeVisibility(xiiUInt32 instances, xiiUInt64 meshletCount)
{
  // Every instance can select any of its resident LODs. Never truncate work to the native
  // visibility service's default fixed capacity. Check before rounding to avoid UInt64 wrap.
  // Capacity grows geometrically and is retained until this per-view pass is destroyed.
  if (meshletCount > xiiMath::MaxValue<xiiUInt32>())
    return XII_FAILURE;
  const xiiUInt64 meshlets = xiiMath::PowerOfTwo_Ceil(xiiMath::Max<xiiUInt64>(256, meshletCount));
  const xiiUInt64 objects  = xiiMath::PowerOfTwo_Ceil(xiiMath::Max<xiiUInt64>(64, instances));
  if (meshlets > xiiMath::MaxValue<xiiUInt32>() || objects > xiiMath::MaxValue<xiiUInt32>())
    return XII_FAILURE;
  if (xiiGpuVisibilityManager::IsValid(m_hVisibility) && objects <= m_uiInstanceCapacity && meshlets <= m_uiMeshletCapacity)
    return XII_SUCCESS;
  xiiGpuVisibilityDescription description;
  description.m_uiMaxInstances        = static_cast<xiiUInt32>(xiiMath::Max<xiiUInt64>(objects, m_uiInstanceCapacity));
  description.m_uiMaxVisibleMeshlets  = static_cast<xiiUInt32>(xiiMath::Max<xiiUInt64>(meshlets, m_uiMeshletCapacity));
  description.m_uiMaxVisibilitySets   = 1;
  description.m_uiLodFeedbackCapacity = 1;
  const auto replacement              = xiiGpuVisibilityManager::CreateContext(description);
  if (replacement.IsInvalidated())
    return XII_FAILURE;
  Invalidate();
  xiiGpuVisibilityManager::DestroyContext(m_hVisibility);
  m_hVisibility        = replacement;
  m_uiInstanceCapacity = description.m_uiMaxInstances;
  m_uiMeshletCapacity  = description.m_uiMaxVisibleMeshlets;
  return XII_SUCCESS;
}

xiiResult xiiSceneMeshPickingState::Capture(xiiRenderPipelinePassContext& context, xiiRenderGraphTextureHandle surface)
{
  // Poll happened before graph construction. Never assign new contents to a slot until its
  // completed metadata has been consumed. Resource changes during Build invalidate this image.
  if (static_cast<xiiInt32>(m_ResourceEpoch) != m_iObservedEpoch)
  {
    Invalidate();
    return XII_SUCCESS;
  }
  xiiUInt32 slot;
  XII_SUCCEED_OR_RETURN(m_Readback.AddCapture(context, surface, slot));
  if (slot == xiiInvalidIndex)
    return XII_SUCCESS;
  auto& owners = m_SlotOwners[slot];
  owners.Clear();
  owners.Reserve(m_Scene.GetDatabase().GetObjectCount());
  for (const auto& instance : m_Scene.GetDatabase().GetGpuInstances())
  {
    Owner owner;
    XII_SUCCEED_OR_RETURN(xiiSceneDatabaseManager::ResolveExtractedMesh(m_Scene.GetHandle(), instance.m_uiObjectId, owner.m_Source));
    const xiiGameObject*        object    = nullptr;
    const xiiMeshComponentBase* component = nullptr;
    if (!context.m_pView->GetWorld()->TryGetObject(owner.m_Source.m_hOwnerObject, object) ||
        !context.m_pView->GetWorld()->TryGetComponent(owner.m_Source.m_hOwnerComponent, component) || component->GetOwner() != object)
      return XII_FAILURE;
    owner.m_Transform   = object->GetGlobalTransform();
    owner.m_hMesh       = component->GetMesh();
    owner.m_uiMaterials = MaterialSignature(*component);
    owners.Insert(instance.m_uiObjectId, std::move(owner));
  }
  return XII_SUCCESS;
}

bool xiiSceneMeshPickingState::Resolve(const xiiView& view, xiiUInt32 id, const Owner*& out_owner) const
{
  out_owner = nullptr;
  if (!m_Owners.TryGetValue(id, out_owner))
    return false;
  const xiiGameObject*        object    = nullptr;
  const xiiMeshComponentBase* component = nullptr;
  if (!view.GetWorld()->TryGetObject(out_owner->m_Source.m_hOwnerObject, object) ||
      !view.GetWorld()->TryGetComponent(out_owner->m_Source.m_hOwnerComponent, component) || component->GetOwner() != object ||
      !object->IsActive() || !component->IsActive() || object->GetGlobalTransform() != out_owner->m_Transform ||
      component->GetMesh() != out_owner->m_hMesh || MaterialSignature(*component) != out_owner->m_uiMaterials)
    return false;
  const auto& tags = object->GetTags();
  return !tags.IsAnySet(view.m_ExcludeTags) && (view.m_IncludeTags.IsEmpty() || tags.IsAnySet(view.m_IncludeTags));
}

bool xiiSceneMeshPickingState::Pixel(const xiiView& view, xiiUInt32 x, xiiUInt32 y, xiiUInt32& out_index) const
{
  const auto& viewport = view.GetViewport();
  const auto  size     = m_Readback.GetSize();
  const float px = static_cast<float>(x) + 0.5f, py = static_cast<float>(y) + 0.5f;
  if (!viewport.Contains(xiiVec2(px, py)))
    return false;
  const auto ix = static_cast<xiiUInt32>((px - viewport.x) * size.width / viewport.width);
  const auto iy = static_cast<xiiUInt32>((py - viewport.y) * size.height / viewport.height);
  if (ix >= size.width || iy >= size.height)
    return false;
  out_index = iy * size.width + ix;
  return true;
}
