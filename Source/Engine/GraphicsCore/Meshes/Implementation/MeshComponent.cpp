/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

namespace
{
  static void WriteMaterialOverrides(xiiStreamWriter& ref_stream, xiiArrayPtr<const xiiMaterialResourceHandle> materials)
  {
    ref_stream << materials.GetCount();

    for (const xiiMaterialResourceHandle& hMaterial : materials)
    {
      ref_stream << hMaterial;
    }
  }

  static void ReadMaterialOverrides(xiiStreamReader& ref_stream, xiiDynamicArray<xiiMaterialResourceHandle>& out_materials)
  {
    xiiUInt32 uiCount = 0U;
    ref_stream >> uiCount;

    out_materials.SetCount(uiCount);
    for (xiiMaterialResourceHandle& hMaterial : out_materials)
    {
      ref_stream >> hMaterial;
    }
  }

  static xiiUInt32 ClampLODIndex(const xiiMeshResource& mesh, xiiUInt32 uiLOD)
  {
    const xiiUInt32 uiLODCount = mesh.GetLODCount();
    if (uiLODCount == 0U)
      return 0U;

    return xiiMath::Min(uiLOD, uiLODCount - 1U);
  }

  static void FillRangeFromLOD(xiiMeshRenderData& ref_renderData, const xiiMeshLOD& lod)
  {
    ref_renderData.m_uiFirstMeshlet = lod.m_uiFirstMeshlet;
    ref_renderData.m_uiMeshletCount = lod.m_uiMeshletCount;

    if (!lod.m_Sections.IsEmpty())
    {
      ref_renderData.m_uiFirstPrimitive = lod.m_Sections[0].m_uiFirstPrimitive;
      ref_renderData.m_uiPrimitiveCount = 0U;

      for (const xiiMeshSection& section : lod.m_Sections)
      {
        ref_renderData.m_uiFirstPrimitive = xiiMath::Min(ref_renderData.m_uiFirstPrimitive, section.m_uiFirstPrimitive);
        ref_renderData.m_uiPrimitiveCount += section.m_uiPrimitiveCount;
      }
    }
  }

  static void FillRangeFromSection(xiiMeshRenderData& ref_renderData, const xiiMeshSection& section)
  {
    ref_renderData.m_uiFirstPrimitive = section.m_uiFirstPrimitive;
    ref_renderData.m_uiPrimitiveCount = section.m_uiPrimitiveCount;
    ref_renderData.m_uiFirstMeshlet   = section.m_uiFirstMeshlet;
    ref_renderData.m_uiMeshletCount   = section.m_uiMeshletCount;
  }
} // namespace

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiMeshRenderDataFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiMeshRenderDataFlags::None),
  XII_BITFLAGS_CONSTANT(xiiMeshRenderDataFlags::StaticObject),
  XII_BITFLAGS_CONSTANT(xiiMeshRenderDataFlags::DynamicObject),
  XII_BITFLAGS_CONSTANT(xiiMeshRenderDataFlags::Skinned),
  XII_BITFLAGS_CONSTANT(xiiMeshRenderDataFlags::MorphTargets),
  XII_BITFLAGS_CONSTANT(xiiMeshRenderDataFlags::Instanced),
  XII_BITFLAGS_CONSTANT(xiiMeshRenderDataFlags::PreferMeshShader),
  XII_BITFLAGS_CONSTANT(xiiMeshRenderDataFlags::ForceLOD),
  XII_BITFLAGS_CONSTANT(xiiMeshRenderDataFlags::CpuCullingFallback),
  XII_BITFLAGS_CONSTANT(xiiMeshRenderDataFlags::RayTracingVisible),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshRenderData, 1, xiiRTTIDefaultAllocator<xiiMeshRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiMeshComponentBase, 1)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("Mesh", GetMesh, SetMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh", xiiDependencyFlags::Package)),
    XII_ARRAY_MEMBER_PROPERTY("MaterialOverrides", m_MaterialOverrides)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material", xiiDependencyFlags::Package)),
    XII_ACCESSOR_PROPERTY("SectionIndex", GetSectionIndex, SetSectionIndex)->AddAttributes(new xiiDefaultValueAttribute(xiiVariant(xiiInvalidIndex))),
    XII_ACCESSOR_PROPERTY("PreferMeshShaders", GetPreferMeshShaders, SetPreferMeshShaders)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("RayTracingVisible", GetRayTracingVisible, SetRayTracingVisible)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Meshes"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

xiiMeshComponentBase::xiiMeshComponentBase()  = default;
xiiMeshComponentBase::~xiiMeshComponentBase() = default;

void xiiMeshComponentBase::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_hMesh;

  WriteMaterialOverrides(s, m_MaterialOverrides);

  s << m_uiSectionIndex;
  s << m_bPreferMeshShaders;
  s << m_bRayTracingVisible;
  s << m_bCpuCullingFallback;
}

void xiiMeshComponentBase::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_hMesh;

  ReadMaterialOverrides(s, m_MaterialOverrides);

  s >> m_uiSectionIndex;
  s >> m_bPreferMeshShaders;
  s >> m_bRayTracingVisible;
  s >> m_bCpuCullingFallback;
}

xiiResult xiiMeshComponentBase::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);

  ref_bAlwaysVisible = false;

  if (!m_hMesh.IsValid())
    return XII_FAILURE;

  xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!pMesh)
    return XII_FAILURE;

  ref_bounds = pMesh->GetBounds();
  if (!ref_bounds.IsValid())
    return XII_FAILURE;

  UpdateLocalBoundsForInstances(ref_bounds);
  return ref_bounds.IsValid() ? XII_SUCCESS : XII_FAILURE;
}

void xiiMeshComponentBase::SetMesh(const xiiMeshResourceHandle& hMesh)
{
  if (m_hMesh == hMesh)
    return;

  m_hMesh = hMesh;

  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

const xiiMeshResourceHandle& xiiMeshComponentBase::GetMesh() const
{
  return m_hMesh;
}

void xiiMeshComponentBase::SetMaterialOverride(xiiUInt32 uiMaterialIndex, const xiiMaterialResourceHandle& hMaterial)
{
  if (uiMaterialIndex >= m_MaterialOverrides.GetCount())
  {
    m_MaterialOverrides.SetCount(uiMaterialIndex + 1U);
  }

  if (m_MaterialOverrides[uiMaterialIndex] == hMaterial)
    return;

  m_MaterialOverrides[uiMaterialIndex] = hMaterial;
  InvalidateCachedRenderData();
}

const xiiMaterialResourceHandle& xiiMeshComponentBase::GetMaterialOverride(xiiUInt32 uiMaterialIndex) const
{
  static xiiMaterialResourceHandle s_InvalidMaterial;

  if (uiMaterialIndex >= m_MaterialOverrides.GetCount())
    return s_InvalidMaterial;

  return m_MaterialOverrides[uiMaterialIndex];
}

void xiiMeshComponentBase::ClearMaterialOverrides()
{
  if (m_MaterialOverrides.IsEmpty())
    return;

  m_MaterialOverrides.Clear();
  InvalidateCachedRenderData();
}

xiiArrayPtr<const xiiMaterialResourceHandle> xiiMeshComponentBase::GetMaterialOverrides() const
{
  return m_MaterialOverrides;
}

void xiiMeshComponentBase::SetSectionIndex(xiiUInt32 uiSectionIndex)
{
  if (m_uiSectionIndex == uiSectionIndex)
    return;

  m_uiSectionIndex = uiSectionIndex;
  InvalidateCachedRenderData();
}

xiiUInt32 xiiMeshComponentBase::GetSectionIndex() const
{
  return m_uiSectionIndex;
}

void xiiMeshComponentBase::SetPreferMeshShaders(bool bPreferMeshShaders)
{
  if (m_bPreferMeshShaders == bPreferMeshShaders)
    return;

  m_bPreferMeshShaders = bPreferMeshShaders;
  InvalidateCachedRenderData();
}

bool xiiMeshComponentBase::GetPreferMeshShaders() const
{
  return m_bPreferMeshShaders;
}

void xiiMeshComponentBase::SetRayTracingVisible(bool bVisible)
{
  if (m_bRayTracingVisible == bVisible)
    return;

  m_bRayTracingVisible = bVisible;
  InvalidateCachedRenderData();
}

bool xiiMeshComponentBase::GetRayTracingVisible() const
{
  return m_bRayTracingVisible;
}

void xiiMeshComponentBase::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  if (!m_hMesh.IsValid())
    return;

  xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!pMesh)
    return;

  const xiiRenderWorldModule* pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiMeshRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiMeshRenderData>(this);
  FillRenderData(*pRenderData, *pMesh.GetPointer());

  if (pRenderData->m_uiInstanceCount == 0U)
    return;

  ref_msg.AddRenderData(pRenderData, GetRenderDataCaching());
}

void xiiMeshComponentBase::FillRenderData(xiiMeshRenderData& ref_renderData, const xiiMeshResource& mesh) const
{
  ref_renderData.m_hMesh          = m_hMesh;
  ref_renderData.m_hMeshBuffer    = mesh.GetMeshBuffer();
  ref_renderData.m_uiUniqueID     = GetUniqueIdForRendering();
  ref_renderData.m_uiLODIndex     = SelectLOD(mesh);
  ref_renderData.m_uiSectionIndex = m_uiSectionIndex;
  ref_renderData.m_Flags          = GetMeshRenderFlags();

  ref_renderData.m_Flags.AddOrRemove(xiiMeshRenderDataFlags::PreferMeshShader, m_bPreferMeshShaders);
  ref_renderData.m_Flags.AddOrRemove(xiiMeshRenderDataFlags::CpuCullingFallback, m_bCpuCullingFallback);
  ref_renderData.m_Flags.AddOrRemove(xiiMeshRenderDataFlags::RayTracingVisible, m_bRayTracingVisible);

  const xiiBitflags<xiiMeshResourceUsageFlags> meshUsage = mesh.GetUsageFlags();
  ref_renderData.m_Flags.AddOrRemove(xiiMeshRenderDataFlags::Skinned, meshUsage.IsSet(xiiMeshResourceUsageFlags::Skinned));
  ref_renderData.m_Flags.AddOrRemove(xiiMeshRenderDataFlags::MorphTargets, meshUsage.IsSet(xiiMeshResourceUsageFlags::MorphTargets));

  const xiiArrayPtr<const xiiMeshLOD> pLODs = mesh.GetLODs();
  if (!pLODs.IsEmpty())
  {
    FillRangeFromLOD(ref_renderData, pLODs[ClampLODIndex(mesh, ref_renderData.m_uiLODIndex)]);
  }

  const xiiArrayPtr<const xiiMeshSection> pSections = mesh.GetSections();
  if (m_uiSectionIndex < pSections.GetCount())
  {
    FillRangeFromSection(ref_renderData, pSections[m_uiSectionIndex]);
  }

  const xiiArrayPtr<const xiiMaterialResourceHandle> pMeshMaterials  = mesh.GetMaterials();
  const xiiUInt32                                    uiMaterialCount = xiiMath::Max(pMeshMaterials.GetCount(), m_MaterialOverrides.GetCount());
  ref_renderData.m_hMaterials.SetCount(uiMaterialCount);
  for (xiiUInt32 i = 0; i < uiMaterialCount; ++i)
  {
    if (i < m_MaterialOverrides.GetCount() && m_MaterialOverrides[i].IsValid())
    {
      ref_renderData.m_hMaterials[i] = m_MaterialOverrides[i];
    }
    else if (i < pMeshMaterials.GetCount())
    {
      ref_renderData.m_hMaterials[i] = pMeshMaterials[i];
    }
    else
    {
      ref_renderData.m_hMaterials[i].Invalidate();
    }
  }

  const xiiUInt64 uiMeshKey     = m_hMesh.IsValid() ? m_hMesh.GetResourceIDHash() : 0ULL;
  const xiiUInt64 uiMaterialKey = (!ref_renderData.m_hMaterials.IsEmpty() && ref_renderData.m_hMaterials[0].IsValid()) ? ref_renderData.m_hMaterials[0].GetResourceIDHash() : 0ULL;
  ref_renderData.m_uiSortingKey = (uiMaterialKey & 0xFFFF000000000000ULL) ^ (uiMeshKey & 0x0000FFFFFFFF0000ULL) ^ ref_renderData.m_uiUniqueID;
}

xiiUInt32 xiiMeshComponentBase::SelectLOD(const xiiMeshResource& mesh) const
{
  return ClampLODIndex(mesh, 0U);
}

xiiRenderData::Caching::Enum xiiMeshComponentBase::GetRenderDataCaching() const
{
  return xiiRenderData::Caching::Never;
}

xiiBitflags<xiiMeshRenderDataFlags> xiiMeshComponentBase::GetMeshRenderFlags() const
{
  return xiiMeshRenderDataFlags::Default;
}

void xiiMeshComponentBase::UpdateLocalBoundsForInstances(xiiBoundingBoxSphere& ref_bounds) const
{
  XII_IGNORE_UNUSED(ref_bounds);
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_COMPONENT_TYPE(xiiStaticMeshComponent, 1, xiiComponentMode::Static)
XII_END_COMPONENT_TYPE;

xiiStaticMeshComponent::xiiStaticMeshComponent()  = default;
xiiStaticMeshComponent::~xiiStaticMeshComponent() = default;

xiiRenderData::Caching::Enum xiiStaticMeshComponent::GetRenderDataCaching() const
{
  return xiiRenderData::Caching::IfStatic;
}

xiiBitflags<xiiMeshRenderDataFlags> xiiStaticMeshComponent::GetMeshRenderFlags() const
{
  xiiBitflags<xiiMeshRenderDataFlags> flags = xiiMeshRenderDataFlags::Default;
  flags.Add(xiiMeshRenderDataFlags::StaticObject);
  return flags;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_COMPONENT_TYPE(xiiMeshComponent, 1, xiiComponentMode::Static)
XII_END_COMPONENT_TYPE;

xiiMeshComponent::xiiMeshComponent()  = default;
xiiMeshComponent::~xiiMeshComponent() = default;

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_COMPONENT_TYPE(xiiDynamicMeshComponent, 1, xiiComponentMode::Dynamic)
XII_END_COMPONENT_TYPE;

xiiDynamicMeshComponent::xiiDynamicMeshComponent()  = default;
xiiDynamicMeshComponent::~xiiDynamicMeshComponent() = default;

xiiBitflags<xiiMeshRenderDataFlags> xiiDynamicMeshComponent::GetMeshRenderFlags() const
{
  xiiBitflags<xiiMeshRenderDataFlags> flags = xiiMeshRenderDataFlags::Default;
  flags.Add(xiiMeshRenderDataFlags::DynamicObject);
  return flags;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSkinnedMeshComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("Skeleton", GetSkeleton, SetSkeleton)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Skeleton", xiiDependencyFlags::Package)),
  }
  XII_END_PROPERTIES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiSkinnedMeshComponent::xiiSkinnedMeshComponent()  = default;
xiiSkinnedMeshComponent::~xiiSkinnedMeshComponent() = default;

void xiiSkinnedMeshComponent::SetSkeleton(const xiiSkeletonResourceHandle& hSkeleton)
{
  if (m_hSkeleton == hSkeleton)
    return;

  m_hSkeleton = hSkeleton;
  InvalidateCachedRenderData();
}

const xiiSkeletonResourceHandle& xiiSkinnedMeshComponent::GetSkeleton() const
{
  return m_hSkeleton;
}

void xiiSkinnedMeshComponent::SetSkinningMatrices(xiiArrayPtr<const xiiMat4> pMatrices)
{
  m_SkinningMatrices.SetCount(pMatrices.GetCount());
  for (xiiUInt32 i = 0; i < pMatrices.GetCount(); ++i)
  {
    m_SkinningMatrices[i] = pMatrices[i];
  }

  InvalidateCachedRenderData();
}

xiiArrayPtr<const xiiMat4> xiiSkinnedMeshComponent::GetSkinningMatrices() const
{
  return m_SkinningMatrices;
}

void xiiSkinnedMeshComponent::SetMorphWeights(xiiArrayPtr<const float> pWeights)
{
  m_MorphWeights.SetCount(pWeights.GetCount());
  for (xiiUInt32 i = 0; i < pWeights.GetCount(); ++i)
  {
    m_MorphWeights[i] = pWeights[i];
  }

  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

xiiArrayPtr<const float> xiiSkinnedMeshComponent::GetMorphWeights() const
{
  return m_MorphWeights;
}

void xiiSkinnedMeshComponent::ClearPose()
{
  m_SkinningMatrices.Clear();
  m_MorphWeights.Clear();

  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiSkinnedMeshComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  inout_stream.GetStream() << m_hSkeleton;
}

void xiiSkinnedMeshComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  inout_stream.GetStream() >> m_hSkeleton;
}

void xiiSkinnedMeshComponent::FillRenderData(xiiMeshRenderData& ref_renderData, const xiiMeshResource& mesh) const
{
  SUPER::FillRenderData(ref_renderData, mesh);

  ref_renderData.m_SkinningMatrices = m_SkinningMatrices;
  ref_renderData.m_MorphWeights     = m_MorphWeights;

  ref_renderData.m_Flags.Add(xiiMeshRenderDataFlags::Skinned);
  ref_renderData.m_Flags.AddOrRemove(xiiMeshRenderDataFlags::MorphTargets, !m_MorphWeights.IsEmpty());
}

xiiBitflags<xiiMeshRenderDataFlags> xiiSkinnedMeshComponent::GetMeshRenderFlags() const
{
  xiiBitflags<xiiMeshRenderDataFlags> flags = SUPER::GetMeshRenderFlags();
  flags.Add(xiiMeshRenderDataFlags::Skinned);
  return flags;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiInstancedMeshComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(ClearInstances),
  }
  XII_END_FUNCTIONS;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiInstancedMeshComponent::xiiInstancedMeshComponent()  = default;
xiiInstancedMeshComponent::~xiiInstancedMeshComponent() = default;

xiiUInt32 xiiInstancedMeshComponent::AddInstance(const xiiMat4& transform)
{
  m_InstanceTransforms.PushBack(transform);

  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();

  return m_InstanceTransforms.GetCount() - 1U;
}

void xiiInstancedMeshComponent::SetInstanceTransform(xiiUInt32 uiIndex, const xiiMat4& transform)
{
  if (uiIndex >= m_InstanceTransforms.GetCount())
    return;

  if (m_InstanceTransforms[uiIndex].IsIdentical(transform))
    return;

  m_InstanceTransforms[uiIndex] = transform;

  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiInstancedMeshComponent::RemoveInstance(xiiUInt32 uiIndex)
{
  if (uiIndex >= m_InstanceTransforms.GetCount())
    return;

  m_InstanceTransforms.RemoveAtAndCopy(uiIndex);

  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiInstancedMeshComponent::SetInstanceCount(xiiUInt32 uiCount)
{
  const xiiUInt32 uiOldCount = m_InstanceTransforms.GetCount();
  if (uiOldCount == uiCount)
    return;

  m_InstanceTransforms.SetCount(uiCount);
  for (xiiUInt32 i = uiOldCount; i < uiCount; ++i)
  {
    m_InstanceTransforms[i].SetIdentity();
  }

  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiInstancedMeshComponent::ClearInstances()
{
  if (m_InstanceTransforms.IsEmpty())
    return;

  m_InstanceTransforms.Clear();

  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

xiiUInt32 xiiInstancedMeshComponent::GetInstanceCount() const
{
  return m_InstanceTransforms.GetCount();
}

xiiArrayPtr<const xiiMat4> xiiInstancedMeshComponent::GetInstanceTransforms() const
{
  return m_InstanceTransforms;
}

void xiiInstancedMeshComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_InstanceTransforms.GetCount();
  for (const xiiMat4& transform : m_InstanceTransforms)
  {
    s << transform;
  }
}

void xiiInstancedMeshComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& s = inout_stream.GetStream();

  xiiUInt32 uiInstanceCount = 0U;
  s >> uiInstanceCount;

  m_InstanceTransforms.SetCount(uiInstanceCount);
  for (xiiMat4& transform : m_InstanceTransforms)
  {
    s >> transform;
  }
}

void xiiInstancedMeshComponent::FillRenderData(xiiMeshRenderData& ref_renderData, const xiiMeshResource& mesh) const
{
  SUPER::FillRenderData(ref_renderData, mesh);

  ref_renderData.m_InstanceTransforms = m_InstanceTransforms;
  ref_renderData.m_uiInstanceCount    = m_InstanceTransforms.GetCount();
  ref_renderData.m_Flags.Add(xiiMeshRenderDataFlags::Instanced);
}

xiiBitflags<xiiMeshRenderDataFlags> xiiInstancedMeshComponent::GetMeshRenderFlags() const
{
  xiiBitflags<xiiMeshRenderDataFlags> flags = xiiMeshRenderDataFlags::Default;
  flags.Add(xiiMeshRenderDataFlags::DynamicObject);
  flags.Add(xiiMeshRenderDataFlags::Instanced);
  return flags;
}

void xiiInstancedMeshComponent::UpdateLocalBoundsForInstances(xiiBoundingBoxSphere& ref_bounds) const
{
  if (m_InstanceTransforms.IsEmpty())
  {
    ref_bounds = xiiBoundingBoxSphere::MakeInvalid();
    return;
  }

  const xiiBoundingBoxSphere meshBounds = ref_bounds;
  ref_bounds                            = xiiBoundingBoxSphere::MakeInvalid();

  for (const xiiMat4& transform : m_InstanceTransforms)
  {
    xiiBoundingBoxSphere transformedBounds = meshBounds;
    transformedBounds.Transform(transform);
    ref_bounds.ExpandToInclude(transformedBounds);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiLODMeshComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("ForcedLOD", GetForcedLOD, SetForcedLOD)->AddAttributes(new xiiDefaultValueAttribute(xiiVariant(xiiInvalidIndex))),
    XII_ACCESSOR_PROPERTY("LodBias", GetLodBias, SetLodBias)->AddAttributes(new xiiDefaultValueAttribute(0.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiLODMeshComponent::xiiLODMeshComponent()  = default;
xiiLODMeshComponent::~xiiLODMeshComponent() = default;

void xiiLODMeshComponent::SetForcedLOD(xiiUInt32 uiLOD)
{
  if (m_uiForcedLOD == uiLOD)
    return;

  m_uiForcedLOD = uiLOD;
  InvalidateCachedRenderData();
}

xiiUInt32 xiiLODMeshComponent::GetForcedLOD() const
{
  return m_uiForcedLOD;
}

void xiiLODMeshComponent::SetLodBias(float fBias)
{
  if (m_fLodBias == fBias)
    return;

  m_fLodBias = fBias;
  InvalidateCachedRenderData();
}

float xiiLODMeshComponent::GetLodBias() const
{
  return m_fLodBias;
}

void xiiLODMeshComponent::FillRenderData(xiiMeshRenderData& ref_renderData, const xiiMeshResource& mesh) const
{
  SUPER::FillRenderData(ref_renderData, mesh);

  ref_renderData.m_fLodBias = m_fLodBias;
  ref_renderData.m_Flags.AddOrRemove(xiiMeshRenderDataFlags::ForceLOD, m_uiForcedLOD != xiiInvalidIndex);
}

xiiUInt32 xiiLODMeshComponent::SelectLOD(const xiiMeshResource& mesh) const
{
  if (m_uiForcedLOD == xiiInvalidIndex)
    return SUPER::SelectLOD(mesh);

  return ClampLODIndex(mesh, m_uiForcedLOD);
}

xiiBitflags<xiiMeshRenderDataFlags> xiiLODMeshComponent::GetMeshRenderFlags() const
{
  xiiBitflags<xiiMeshRenderDataFlags> flags = SUPER::GetMeshRenderFlags();
  flags.AddOrRemove(xiiMeshRenderDataFlags::ForceLOD, m_uiForcedLOD != xiiInvalidIndex);
  return flags;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_MeshComponent);
