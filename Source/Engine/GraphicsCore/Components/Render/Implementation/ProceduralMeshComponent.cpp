#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Render/ProceduralMeshComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProceduralMeshRenderData, 1, xiiRTTIDefaultAllocator<xiiProceduralMeshRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiProceduralMeshComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_ACCESSOR_PROPERTY("CastShadows", GetCastShadows, SetCastShadows)->AddAttributes(new xiiDefaultValueAttribute(false)),
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

xiiProceduralMeshComponent::xiiProceduralMeshComponent()  = default;
xiiProceduralMeshComponent::~xiiProceduralMeshComponent() = default;

void xiiProceduralMeshComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_hMaterial;
  s << m_bCastShadows;
}

void xiiProceduralMeshComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_hMaterial;
  s >> m_bCastShadows;
}

xiiResult xiiProceduralMeshComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
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

void xiiProceduralMeshComponent::SetMaterialFile(xiiStringView sFile)
{
  xiiMaterialResourceHandle hMat;
  if (!sFile.IsEmpty())
    hMat = xiiResourceManager::LoadResource<xiiMaterialResource>(sFile);
  SetMaterial(hMat);
}

xiiStringView xiiProceduralMeshComponent::GetMaterialFile() const
{
  if (m_hMaterial.IsValid())
    return xiiResourceManager::GetResourceIDOrDescription(m_hMaterial);
  return {};
}

void xiiProceduralMeshComponent::SetMaterial(const xiiMaterialResourceHandle& hMaterial)
{
  m_hMaterial = hMaterial;
  InvalidateCachedRenderData();
}

void xiiProceduralMeshComponent::SetCastShadows(bool bCast)
{
  m_bCastShadows = bCast;
  InvalidateCachedRenderData();
}

void xiiProceduralMeshComponent::SetDynamicMeshBuffer(const xiiDynamicMeshBufferResourceHandle& hBuffer, xiiUInt32 uiVertexCount, xiiUInt32 uiIndexCount)
{
  m_hDynamicMesh  = hBuffer;
  m_uiVertexCount = uiVertexCount;
  m_uiIndexCount  = uiIndexCount;
}

void xiiProceduralMeshComponent::SetBounds(const xiiBoundingBoxSphere& bounds)
{
  m_LocalBounds = bounds;
  TriggerLocalBoundsUpdate();
}

void xiiProceduralMeshComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (!m_hDynamicMesh.IsValid() || m_uiVertexCount == 0 || ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiProceduralMeshRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiProceduralMeshRenderData>(this);
  pRenderData->m_GlobalTransform           = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds              = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject              = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent           = GetHandle();
  pRenderData->m_hDynamicMesh              = m_hDynamicMesh;
  pRenderData->m_hMaterial                 = m_hMaterial;
  pRenderData->m_uiVertexCount             = m_uiVertexCount;
  pRenderData->m_uiIndexCount              = m_uiIndexCount;
  pRenderData->m_bCastShadows              = m_bCastShadows;
  pRenderData->m_uiSortingKey              = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Render_Implementation_ProceduralMeshComponent);
