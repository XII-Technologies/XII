#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Lights/Implementation/ShadowPool.h>
#include <GraphicsCore/Lights/SpotLightComponent.h>
#include <GraphicsCore/Pipeline/View.h>

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
xiiCVarBool cvar_RenderingLightingVisScreenSpaceSize("Rendering.Lighting.VisScreenSpaceSize", false, xiiCVarFlags::Default, "Enables debug visualization of light screen space size calculation");
#endif

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSpotLightRenderData, 1, xiiRTTIDefaultAllocator<xiiSpotLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiSpotLightComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(0.0f), new xiiSuffixAttribute(" m"), new xiiMinValueTextAttribute("Auto")),
    XII_ACCESSOR_PROPERTY("InnerSpotAngle", GetInnerSpotAngle, SetInnerSpotAngle)->AddAttributes(new xiiClampValueAttribute(xiiAngle::Degree(0.0f), xiiAngle::Degree(179.0f)), new xiiDefaultValueAttribute(xiiAngle::Degree(15.0f))),
    XII_ACCESSOR_PROPERTY("OuterSpotAngle", GetOuterSpotAngle, SetOuterSpotAngle)->AddAttributes(new xiiClampValueAttribute(xiiAngle::Degree(0.0f), xiiAngle::Degree(179.0f)), new xiiDefaultValueAttribute(xiiAngle::Degree(30.0f))),
    //XII_ACCESSOR_PROPERTY("ProjectedTexture", GetProjectedTextureFile, SetProjectedTextureFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSpotLightVisualizerAttribute("OuterSpotAngle", "Range", "Intensity", "LightColor"),
    new xiiConeLengthManipulatorAttribute("Range"),
    new xiiConeAngleManipulatorAttribute("OuterSpotAngle", 1.5f),
    new xiiConeAngleManipulatorAttribute("InnerSpotAngle", 1.5f),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiSpotLightComponent::xiiSpotLightComponent()
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);
}

xiiSpotLightComponent::~xiiSpotLightComponent() = default;

xiiResult xiiSpotLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);

  ref_bounds = CalculateBoundingSphere(xiiTransform::IdentityTransform(), m_fEffectiveRange);
  return XII_SUCCESS;
}

void xiiSpotLightComponent::SetRange(float fRange)
{
  m_fRange = fRange;

  TriggerLocalBoundsUpdate();
}

float xiiSpotLightComponent::GetRange() const
{
  return m_fRange;
}

float xiiSpotLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void xiiSpotLightComponent::SetInnerSpotAngle(xiiAngle spotAngle)
{
  m_InnerSpotAngle = xiiMath::Clamp(spotAngle, xiiAngle::Degree(0.0f), m_OuterSpotAngle);

  InvalidateCachedRenderData();
}

xiiAngle xiiSpotLightComponent::GetInnerSpotAngle() const
{
  return m_InnerSpotAngle;
}

void xiiSpotLightComponent::SetOuterSpotAngle(xiiAngle spotAngle)
{
  m_OuterSpotAngle = xiiMath::Clamp(spotAngle, m_InnerSpotAngle, xiiAngle::Degree(179.0f));

  TriggerLocalBoundsUpdate();
}

xiiAngle xiiSpotLightComponent::GetOuterSpotAngle() const
{
  return m_OuterSpotAngle;
}

void xiiSpotLightComponent::SetProjectedTexture(const xiiTexture2DResourceHandle& hProjectedTexture)
{
  m_hProjectedTexture = hProjectedTexture;

  InvalidateCachedRenderData();
}

const xiiTexture2DResourceHandle& xiiSpotLightComponent::GetProjectedTexture() const
{
  return m_hProjectedTexture;
}

void xiiSpotLightComponent::SetProjectedTextureFile(const char* szFile)
{
  xiiTexture2DResourceHandle hProjectedTexture;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hProjectedTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(szFile);
  }

  SetProjectedTexture(hProjectedTexture);
}

const char* xiiSpotLightComponent::GetProjectedTextureFile() const
{
  if (!m_hProjectedTexture.IsValid())
    return "";

  return m_hProjectedTexture.GetResourceID();
}

void xiiSpotLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != xiiInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f || m_fEffectiveRange <= 0.0f || m_OuterSpotAngle.GetRadian() <= 0.0f)
    return;

  xiiTransform      t  = GetOwner()->GetGlobalTransform();
  xiiBoundingSphere bs = CalculateBoundingSphere(t, m_fEffectiveRange * 0.5f);

  float fScreenSpaceSize = CalculateScreenSpaceSize(bs, *msg.m_pView->GetCullingCamera());

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingLightingVisScreenSpaceSize)
  {
    xiiStringBuilder sb;
    sb.SetFormat("{0}", fScreenSpaceSize);
    xiiDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, t.m_vPosition, xiiColor::Olive);
    xiiDebugRenderer::DrawLineSphere(msg.m_pView->GetHandle(), bs, xiiColor::Olive);
  }
#endif

  auto pRenderData = xiiCreateRenderDataForThisFrame<xiiSpotLightRenderData>(GetOwner());

  pRenderData->m_GlobalTransform    = t;
  pRenderData->m_LightColor         = m_LightColor;
  pRenderData->m_fIntensity         = m_fIntensity;
  pRenderData->m_fRange             = m_fEffectiveRange;
  pRenderData->m_InnerSpotAngle     = m_InnerSpotAngle;
  pRenderData->m_OuterSpotAngle     = m_OuterSpotAngle;
  pRenderData->m_hProjectedTexture  = m_hProjectedTexture;
  pRenderData->m_uiShadowDataOffset = m_bCastShadows ? xiiShadowPool::AddSpotLight(this, fScreenSpaceSize, msg.m_pView) : xiiInvalidIndex;

  pRenderData->FillBatchIdAndSortingKey(fScreenSpaceSize);

  xiiRenderData::Caching::Enum caching = m_bCastShadows ? xiiRenderData::Caching::Never : xiiRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::Light, caching);
}

void xiiSpotLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_fRange;
  s << m_InnerSpotAngle;
  s << m_OuterSpotAngle;
  s << GetProjectedTextureFile();
}

void xiiSpotLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_fRange;
  s >> m_InnerSpotAngle;
  s >> m_OuterSpotAngle;

  xiiStringBuilder temp;
  s >> temp;
  SetProjectedTextureFile(temp);
}

xiiBoundingSphere xiiSpotLightComponent::CalculateBoundingSphere(const xiiTransform& t, float fRange) const
{
  xiiBoundingSphere res;
  xiiAngle          halfAngle  = m_OuterSpotAngle / 2.0f;
  xiiVec3           position   = t.m_vPosition;
  xiiVec3           forwardDir = t.m_qRotation * xiiVec3(1.0f, 0.0f, 0.0f);

  if (halfAngle > xiiAngle::Degree(45.0f))
  {
    res.m_vCenter = position + xiiMath::Cos(halfAngle) * fRange * forwardDir;
    res.m_fRadius = xiiMath::Sin(halfAngle) * fRange;
  }
  else
  {
    res.m_fRadius = fRange / (2.0f * xiiMath::Cos(halfAngle));
    res.m_vCenter = position + forwardDir * res.m_fRadius;
  }

  return res;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSpotLightVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiSpotLightVisualizerAttribute>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSpotLightVisualizerAttribute::xiiSpotLightVisualizerAttribute() :
  xiiVisualizerAttribute(nullptr)
{
}

xiiSpotLightVisualizerAttribute::xiiSpotLightVisualizerAttribute(const char* szAngleProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty) :
  xiiVisualizerAttribute(szAngleProperty, szRangeProperty, szIntensityProperty, szColorProperty)
{
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_SpotLightComponent);
