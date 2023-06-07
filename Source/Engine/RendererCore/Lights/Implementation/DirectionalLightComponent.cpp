#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDirectionalLightRenderData, 1, xiiRTTIDefaultAllocator<xiiDirectionalLightRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiDirectionalLightComponent, 3, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("NumCascades", GetNumCascades, SetNumCascades)->AddAttributes(new xiiClampValueAttribute(1, 4), new xiiDefaultValueAttribute(2)),
    XII_ACCESSOR_PROPERTY("MinShadowRange", GetMinShadowRange, SetMinShadowRange)->AddAttributes(new xiiClampValueAttribute(0.1f, xiiVariant()), new xiiDefaultValueAttribute(30.0f), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new xiiClampValueAttribute(0.6f, 1.0f), new xiiDefaultValueAttribute(0.8f)),
    XII_ACCESSOR_PROPERTY("SplitModeWeight", GetSplitModeWeight, SetSplitModeWeight)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f), new xiiDefaultValueAttribute(0.7f)),
    XII_ACCESSOR_PROPERTY("NearPlaneOffset", GetNearPlaneOffset, SetNearPlaneOffset)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(100.0f), new xiiSuffixAttribute(" m")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 1.0f, xiiColor::White, "LightColor"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiDirectionalLightComponent::xiiDirectionalLightComponent()  = default;
xiiDirectionalLightComponent::~xiiDirectionalLightComponent() = default;

xiiResult xiiDirectionalLightComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return XII_SUCCESS;
}

void xiiDirectionalLightComponent::SetNumCascades(xiiUInt32 uiNumCascades)
{
  m_uiNumCascades = xiiMath::Clamp(uiNumCascades, 1u, 4u);

  InvalidateCachedRenderData();
}

xiiUInt32 xiiDirectionalLightComponent::GetNumCascades() const
{
  return m_uiNumCascades;
}

void xiiDirectionalLightComponent::SetMinShadowRange(float fMinShadowRange)
{
  m_fMinShadowRange = xiiMath::Max(fMinShadowRange, 0.0f);

  InvalidateCachedRenderData();
}

float xiiDirectionalLightComponent::GetMinShadowRange() const
{
  return m_fMinShadowRange;
}

void xiiDirectionalLightComponent::SetFadeOutStart(float fFadeOutStart)
{
  m_fFadeOutStart = xiiMath::Clamp(fFadeOutStart, 0.0f, 1.0f);

  InvalidateCachedRenderData();
}

float xiiDirectionalLightComponent::GetFadeOutStart() const
{
  return m_fFadeOutStart;
}

void xiiDirectionalLightComponent::SetSplitModeWeight(float fSplitModeWeight)
{
  m_fSplitModeWeight = xiiMath::Clamp(fSplitModeWeight, 0.0f, 1.0f);

  InvalidateCachedRenderData();
}

float xiiDirectionalLightComponent::GetSplitModeWeight() const
{
  return m_fSplitModeWeight;
}

void xiiDirectionalLightComponent::SetNearPlaneOffset(float fNearPlaneOffset)
{
  m_fNearPlaneOffset = xiiMath::Max(fNearPlaneOffset, 0.0f);

  InvalidateCachedRenderData();
}

float xiiDirectionalLightComponent::GetNearPlaneOffset() const
{
  return m_fNearPlaneOffset;
}

void xiiDirectionalLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != xiiInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f)
    return;

  auto pRenderData = xiiCreateRenderDataForThisFrame<xiiDirectionalLightRenderData>(GetOwner());

  pRenderData->m_GlobalTransform    = GetOwner()->GetGlobalTransform();
  pRenderData->m_LightColor         = m_LightColor;
  pRenderData->m_fIntensity         = m_fIntensity;
  pRenderData->m_uiShadowDataOffset = m_bCastShadows ? xiiShadowPool::AddDirectionalLight(this, msg.m_pView) : xiiInvalidIndex;

  pRenderData->FillBatchIdAndSortingKey(1.0f);

  xiiRenderData::Caching::Enum caching = m_bCastShadows ? xiiRenderData::Caching::Never : xiiRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::Light, caching);
}

void xiiDirectionalLightComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  xiiStreamWriter& s = ref_stream.GetStream();

  s << m_uiNumCascades;
  s << m_fMinShadowRange;
  s << m_fFadeOutStart;
  s << m_fSplitModeWeight;
  s << m_fNearPlaneOffset;
}

void xiiDirectionalLightComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  const xiiUInt32  uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = ref_stream.GetStream();

  if (uiVersion >= 3)
  {
    s >> m_uiNumCascades;
    s >> m_fMinShadowRange;
    s >> m_fFadeOutStart;
    s >> m_fSplitModeWeight;
    s >> m_fNearPlaneOffset;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class xiiDirectionalLightComponentPatch_1_2 : public xiiGraphPatch
{
public:
  xiiDirectionalLightComponentPatch_1_2() :
    xiiGraphPatch("xiiDirectionalLightComponent", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    ref_context.PatchBaseClass("xiiLightComponent", 2, true);
  }
};

xiiDirectionalLightComponentPatch_1_2 g_xiiDirectionalLightComponentPatch_1_2;



XII_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_DirectionalLightComponent);
