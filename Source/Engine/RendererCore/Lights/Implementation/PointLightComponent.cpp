#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Pipeline/View.h>

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

xiiResult xiiPointLightComponent::GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg)
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);

  bounds = xiiBoundingSphere(xiiVec3::ZeroVector(), m_fEffectiveRange);
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

void xiiPointLightComponent::SetProjectedTextureFile(const char* szFile)
{
  xiiTextureCubeResourceHandle hProjectedTexture;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hProjectedTexture = xiiResourceManager::LoadResource<xiiTextureCubeResource>(szFile);
  }

  SetProjectedTexture(hProjectedTexture);
}

const char* xiiPointLightComponent::GetProjectedTextureFile() const
{
  if (!m_hProjectedTexture.IsValid())
    return "";

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

  float fScreenSpaceSize = CalculateScreenSpaceSize(xiiBoundingSphere(t.m_vPosition, m_fEffectiveRange * 0.5f), *msg.m_pView->GetCullingCamera());

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

void xiiPointLightComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  xiiStreamWriter& s = stream.GetStream();

  s << m_fRange;
  s << m_hProjectedTexture;
}

void xiiPointLightComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = stream.GetStream();

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

xiiPointLightVisualizerAttribute::xiiPointLightVisualizerAttribute(
  const char* szRangeProperty,
  const char* szIntensityProperty,
  const char* szColorProperty) :
  xiiVisualizerAttribute(szRangeProperty, szIntensityProperty, szColorProperty)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class xiiPointLightComponentPatch_1_2 : public xiiGraphPatch
{
public:
  xiiPointLightComponentPatch_1_2() :
    xiiGraphPatch("xiiPointLightComponent", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    context.PatchBaseClass("xiiLightComponent", 2, true);
  }
};

xiiPointLightComponentPatch_1_2 g_xiiPointLightComponentPatch_1_2;

XII_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_PointLightComponent);
