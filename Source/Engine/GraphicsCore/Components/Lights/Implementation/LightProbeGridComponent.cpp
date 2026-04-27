#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/LightProbeGridComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLightProbeGridRenderData, 1, xiiRTTIDefaultAllocator<xiiLightProbeGridRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiLightProbeGridComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("GridDims",  GetGridDims,  SetGridDims),
    XII_ACCESSOR_PROPERTY("Spacing",   GetSpacing,   SetSpacing),
    XII_ACCESSOR_PROPERTY("SHOrder",   GetSHOrder,   SetSHOrder)->AddAttributes(new xiiDefaultValueAttribute(2u), new xiiClampValueAttribute(1u, 3u)),
    XII_ACCESSOR_PROPERTY("SHAtlas",   GetSHAtlasFile, SetSHAtlasFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Lighting/GI"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiLightProbeGridComponent::xiiLightProbeGridComponent()  = default;
xiiLightProbeGridComponent::~xiiLightProbeGridComponent() = default;

void xiiLightProbeGridComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_GridDims << m_vSpacing << m_uiSHOrder << m_hSHAtlas;
}

void xiiLightProbeGridComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_GridDims >> m_vSpacing >> m_uiSHOrder >> m_hSHAtlas;
}

xiiResult xiiLightProbeGridComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_bAlwaysVisible);
  XII_IGNORE_UNUSED(ref_msg);

  const xiiVec3 vHalfExtent = xiiVec3(
                                m_GridDims.x * m_vSpacing.x,
                                m_GridDims.y * m_vSpacing.y,
                                m_GridDims.z * m_vSpacing.z) *
    0.5f;

  ref_bounds = xiiBoundingBoxSphere::MakeFromBox(xiiBoundingBox::MakeFromMinMax(-vHalfExtent, vHalfExtent));
  return XII_SUCCESS;
}

void xiiLightProbeGridComponent::SetGridDims(const xiiVec3I32& dims)
{
  m_GridDims = dims;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiLightProbeGridComponent::SetSpacing(const xiiVec3& vSpacing)
{
  m_vSpacing = vSpacing;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiLightProbeGridComponent::SetSHOrder(xiiUInt8 uiOrder)
{
  m_uiSHOrder = xiiMath::Clamp<xiiUInt8>(uiOrder, 1, 3);
  InvalidateCachedRenderData();
}

void xiiLightProbeGridComponent::SetSHAtlasFile(xiiStringView sFile)
{
  if (!sFile.IsEmpty())
    m_hSHAtlas = xiiResourceManager::LoadResource<xiiTexture3DResource>(sFile);
  else
    m_hSHAtlas.Invalidate();
  InvalidateCachedRenderData();
}

xiiStringView xiiLightProbeGridComponent::GetSHAtlasFile() const
{
  if (m_hSHAtlas.IsValid())
    return xiiResourceManager::GetResourceIDOrDescription(m_hSHAtlas);
  return {};
}

void xiiLightProbeGridComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiLightProbeGridRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiLightProbeGridRenderData>(this);
  pRenderData->m_GlobalTransform           = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds              = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject              = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent           = GetHandle();
  pRenderData->m_hSHAtlas                  = m_hSHAtlas;
  pRenderData->m_GridDims                  = m_GridDims;
  pRenderData->m_vSpacing                  = m_vSpacing;
  pRenderData->m_uiSHOrder                 = m_uiSHOrder;
  pRenderData->m_uiSortingKey              = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Lights_Implementation_LightProbeGridComponent);
