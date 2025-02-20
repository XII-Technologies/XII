#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Lights/Implementation/ShadowPool.h>
#include <GraphicsCore/Lights/PointLightComponent.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPointLightRenderData, 1, xiiRTTIDefaultAllocator<xiiPointLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiPointLightComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(0.0f), new xiiSuffixAttribute(" m"), new xiiMinValueTextAttribute("Auto")),
    //XII_ACCESSOR_PROPERTY("ProjectedTexture", GetProjectedTextureFile, SetProjectedTextureFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSphereManipulatorAttribute("Range"),
    new xiiPointLightVisualizerAttribute("Range", "Intensity", "LightColor"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiPointLightComponent::xiiPointLightComponent()
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);
}

xiiPointLightComponent::~xiiPointLightComponent() = default;

xiiResult xiiPointLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);

  ref_bounds = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), m_fEffectiveRange);
  return XII_SUCCESS;
}

void xiiPointLightComponent::SetRange(float fRange)
{
  m_fRange = fRange;

  TriggerLocalBoundsUpdate();
}

float xiiPointLightComponent::GetRange() const
{
  return m_fRange;
}

float xiiPointLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void xiiPointLightComponent::SetProjectedTexture(const xiiTextureCubeResourceHandle& hProjectedTexture)
{
  m_hProjectedTexture = hProjectedTexture;

  InvalidateCachedRenderData();
}

const xiiTextureCubeResourceHandle& xiiPointLightComponent::GetProjectedTexture() const
{
  return m_hProjectedTexture;
}

void xiiPointLightComponent::SetProjectedTextureFile(xiiStringView sFile)
{
  xiiTextureCubeResourceHandle hProjectedTexture;

  if (!sFile.IsEmpty())
  {
    hProjectedTexture = xiiResourceManager::LoadResource<xiiTextureCubeResource>(sFile);
  }

  SetProjectedTexture(hProjectedTexture);
}

xiiStringView xiiPointLightComponent::GetProjectedTextureFile() const
{
  return m_hProjectedTexture.GetResourceID();
}

void xiiPointLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != xiiInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f || m_fEffectiveRange <= 0.0f)
    return;

  xiiTransform t = GetOwner()->GetGlobalTransform();

  float fScreenSpaceSize = CalculateScreenSpaceSize(xiiBoundingSphere::MakeFromCenterAndRadius(t.m_vPosition, m_fEffectiveRange * 0.5f), *msg.m_pView->GetCullingCamera());

  auto pRenderData = xiiCreateRenderDataForThisFrame<xiiPointLightRenderData>(GetOwner());

  pRenderData->m_GlobalTransform    = t;
  pRenderData->m_LightColor         = m_LightColor;
  pRenderData->m_fIntensity         = m_fIntensity;
  pRenderData->m_fRange             = m_fEffectiveRange;
  pRenderData->m_hProjectedTexture  = m_hProjectedTexture;
  pRenderData->m_uiShadowDataOffset = m_bCastShadows ? xiiShadowPool::AddPointLight(this, fScreenSpaceSize, msg.m_pView) : xiiInvalidIndex;

  pRenderData->FillBatchIdAndSortingKey(fScreenSpaceSize);

  xiiRenderData::Caching::Enum caching = m_bCastShadows ? xiiRenderData::Caching::Never : xiiRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::Light, caching);
}

void xiiPointLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_fRange;
  s << m_hProjectedTexture;
}

void xiiPointLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_fRange;
  s >> m_hProjectedTexture;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPointLightVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiPointLightVisualizerAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiPointLightVisualizerAttribute::xiiPointLightVisualizerAttribute() :
  xiiVisualizerAttribute(nullptr)
{
}

xiiPointLightVisualizerAttribute::xiiPointLightVisualizerAttribute(xiiStringView sRangeProperty, xiiStringView sIntensityProperty, xiiStringView sColorProperty) :
  xiiVisualizerAttribute(sRangeProperty, sIntensityProperty, sColorProperty)
{
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_PointLightComponent);
