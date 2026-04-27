#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Components/Render/DeformableMeshComponents.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// ============================================================
// xiiSoftBodyMeshComponent
// ============================================================

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSoftBodyMeshComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("TopologyMesh", GetTopologyMeshFile, SetTopologyMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh")),
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

xiiSoftBodyMeshComponent::xiiSoftBodyMeshComponent()  = default;
xiiSoftBodyMeshComponent::~xiiSoftBodyMeshComponent() = default;

void xiiSoftBodyMeshComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_hTopologyMesh;
}

void xiiSoftBodyMeshComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_hTopologyMesh;
}

xiiResult xiiSoftBodyMeshComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);
  ref_bAlwaysVisible = false;
  if (m_SimBounds.IsValid())
  {
    ref_bounds = m_SimBounds;
    return XII_SUCCESS;
  }
  ref_bounds = xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), 1.0f));
  return XII_SUCCESS;
}

void xiiSoftBodyMeshComponent::SetTopologyMeshFile(xiiStringView sFile)
{
  m_hTopologyMesh = sFile.IsEmpty() ? xiiMeshResourceHandle{} : xiiResourceManager::LoadResource<xiiMeshResource>(sFile);
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

xiiStringView xiiSoftBodyMeshComponent::GetTopologyMeshFile() const
{
  return m_hTopologyMesh.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hTopologyMesh) : xiiStringView{};
}

void xiiSoftBodyMeshComponent::SetTopologyMesh(const xiiMeshResourceHandle& hMesh)
{
  m_hTopologyMesh = hMesh;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

xiiUInt32 xiiSoftBodyMeshComponent::GetMaterialCount() const { return m_Materials.GetCount(); }

void xiiSoftBodyMeshComponent::SetMaterial(xiiUInt32 uiIndex, const xiiMaterialResourceHandle& hMaterial)
{
  m_Materials.EnsureCount(uiIndex + 1);
  m_Materials[uiIndex] = hMaterial;
  InvalidateCachedRenderData();
}

xiiMaterialResourceHandle xiiSoftBodyMeshComponent::GetMaterial(xiiUInt32 uiIndex) const
{
  return (uiIndex < m_Materials.GetCount()) ? m_Materials[uiIndex] : xiiMaterialResourceHandle{};
}

void xiiSoftBodyMeshComponent::SetMaterialFile(xiiUInt32 uiIndex, xiiStringView sFile)
{
  SetMaterial(uiIndex, sFile.IsEmpty() ? xiiMaterialResourceHandle{} : xiiResourceManager::LoadResource<xiiMaterialResource>(sFile));
}

xiiStringView xiiSoftBodyMeshComponent::GetMaterialFile(xiiUInt32 uiIndex) const
{
  const xiiMaterialResourceHandle& h = GetMaterial(uiIndex);
  return h.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(h) : xiiStringView{};
}

void xiiSoftBodyMeshComponent::SetSimulationBuffer(xiiGALBufferHandle hVertexData, xiiUInt32 uiVertexCount, const xiiBoundingBoxSphere& bounds)
{
  m_hVertexData = hVertexData;
  m_uiVertexCount = uiVertexCount;
  m_SimBounds = bounds;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiSoftBodyMeshComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  XII_IGNORE_UNUSED(ref_msg);
  // Implementation will extract a specific SoftBodyRenderData structure
}


// ============================================================
// xiiMorphTargetComponent
// ============================================================

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiMorphTargetComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh")),
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

xiiMorphTargetComponent::xiiMorphTargetComponent()  = default;
xiiMorphTargetComponent::~xiiMorphTargetComponent()
{
  xiiGALDevice* pDev = xiiGALDevice::GetDefaultDevice();
  if (pDev && m_hBlendedVertexBuffer.IsValid())
    pDev->DestroyBuffer(m_hBlendedVertexBuffer);
}

void xiiMorphTargetComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_hMesh;
}

void xiiMorphTargetComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_hMesh;
}

xiiResult xiiMorphTargetComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);
  ref_bAlwaysVisible = false;
  ref_bounds = xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), 1.0f));
  return XII_SUCCESS;
}

void xiiMorphTargetComponent::SetMeshFile(xiiStringView sFile)
{
  m_hMesh = sFile.IsEmpty() ? xiiMeshResourceHandle{} : xiiResourceManager::LoadResource<xiiMeshResource>(sFile);
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

xiiStringView xiiMorphTargetComponent::GetMeshFile() const
{
  return m_hMesh.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hMesh) : xiiStringView{};
}

void xiiMorphTargetComponent::SetMesh(const xiiMeshResourceHandle& hMesh)
{
  m_hMesh = hMesh;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiMorphTargetComponent::SetMorphWeight(xiiStringView sShapeName, float fWeight)
{
  XII_IGNORE_UNUSED(sShapeName); XII_IGNORE_UNUSED(fWeight);
  // Lookup index for shape name, set weight, update cbuffers.
}

float xiiMorphTargetComponent::GetMorphWeight(xiiStringView sShapeName) const
{
  XII_IGNORE_UNUSED(sShapeName);
  return 0.0f;
}

void xiiMorphTargetComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  XII_IGNORE_UNUSED(ref_msg);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Render_Implementation_DeformableMeshComponents);
