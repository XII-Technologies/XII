#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/LightComponent.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLightRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiLightRenderData::FillBatchIdAndSortingKey(float fScreenSpaceSize)
{
  m_uiSortingKey = (m_uiShadowDataOffset != xiiInvalidIndex) ? 0 : 1;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiLightComponent, 4)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("LightColor", GetLightColor, SetLightColor),
    XII_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(10.0f)),
    XII_ACCESSOR_PROPERTY("CastShadows", GetCastShadows, SetCastShadows),
    XII_ACCESSOR_PROPERTY("PenumbraSize", GetPenumbraSize, SetPenumbraSize)->AddAttributes(new xiiClampValueAttribute(0.0f, 0.5f), new xiiDefaultValueAttribute(0.1f), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("SlopeBias", GetSlopeBias, SetSlopeBias)->AddAttributes(new xiiClampValueAttribute(0.0f, 10.0f), new xiiDefaultValueAttribute(0.25f)),
    XII_ACCESSOR_PROPERTY("ConstantBias", GetConstantBias, SetConstantBias)->AddAttributes(new xiiClampValueAttribute(0.0f, 10.0f), new xiiDefaultValueAttribute(0.1f))
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Lighting"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgSetColor, OnMsgSetColor),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

xiiLightComponent::xiiLightComponent()  = default;
xiiLightComponent::~xiiLightComponent() = default;

void xiiLightComponent::SetLightColor(xiiColorGammaUB lightColor)
{
  m_LightColor = lightColor;

  InvalidateCachedRenderData();
}

xiiColorGammaUB xiiLightComponent::GetLightColor() const
{
  return m_LightColor;
}

void xiiLightComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = xiiMath::Max(fIntensity, 0.0f);

  TriggerLocalBoundsUpdate();
}

float xiiLightComponent::GetIntensity() const
{
  return m_fIntensity;
}

void xiiLightComponent::SetCastShadows(bool bCastShadows)
{
  m_bCastShadows = bCastShadows;

  InvalidateCachedRenderData();
}

bool xiiLightComponent::GetCastShadows() const
{
  return m_bCastShadows;
}

void xiiLightComponent::SetPenumbraSize(float fPenumbraSize)
{
  m_fPenumbraSize = fPenumbraSize;

  InvalidateCachedRenderData();
}

float xiiLightComponent::GetPenumbraSize() const
{
  return m_fPenumbraSize;
}

void xiiLightComponent::SetSlopeBias(float fBias)
{
  m_fSlopeBias = fBias;

  InvalidateCachedRenderData();
}

float xiiLightComponent::GetSlopeBias() const
{
  return m_fSlopeBias;
}

void xiiLightComponent::SetConstantBias(float fBias)
{
  m_fConstantBias = fBias;

  InvalidateCachedRenderData();
}

float xiiLightComponent::GetConstantBias() const
{
  return m_fConstantBias;
}

void xiiLightComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  xiiStreamWriter& s = ref_stream.GetStream();

  s << m_LightColor;
  s << m_fIntensity;
  s << m_fPenumbraSize;
  s << m_fSlopeBias;
  s << m_fConstantBias;
  s << m_bCastShadows;
}

void xiiLightComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  const xiiUInt32 uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = ref_stream.GetStream();

  s >> m_LightColor;
  s >> m_fIntensity;

  if (uiVersion >= 3)
  {
    s >> m_fPenumbraSize;
  }

  if (uiVersion >= 4)
  {
    s >> m_fSlopeBias;
    s >> m_fConstantBias;
  }

  s >> m_bCastShadows;
}

void xiiLightComponent::OnMsgSetColor(xiiMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_LightColor);

  InvalidateCachedRenderData();
}

// static
float xiiLightComponent::CalculateEffectiveRange(float fRange, float fIntensity)
{
  const float fThreshold      = 0.10f; // aggressive threshold to prevent large lights
  const float fEffectiveRange = xiiMath::Sqrt(xiiMath::Max(0.0f, fIntensity)) / xiiMath::Sqrt(fThreshold);

  XII_ASSERT_DEBUG(!xiiMath::IsNaN(fEffectiveRange), "Light range is NaN");

  if (fRange <= 0.0f)
  {
    return fEffectiveRange;
  }

  return xiiMath::Min(fRange, fEffectiveRange);
}

// static
float xiiLightComponent::CalculateScreenSpaceSize(const xiiBoundingSphere& sphere, const xiiCamera& camera)
{
  if (camera.IsPerspective())
  {
    float dist        = (sphere.m_vCenter - camera.GetPosition()).GetLength();
    float fHalfHeight = xiiMath::Tan(camera.GetFovY(1.0f) * 0.5f) * dist;
    return xiiMath::Pow(sphere.m_fRadius / fHalfHeight, 0.8f); // tweak factor to make transitions more linear.
  }
  else
  {
    float fHalfHeight = camera.GetDimensionY(1.0f) * 0.5f;
    return sphere.m_fRadius / fHalfHeight;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class xiiLightComponentPatch_1_2 : public xiiGraphPatch
{
public:
  xiiLightComponentPatch_1_2() :
    xiiGraphPatch("xiiLightComponent", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override { pNode->RenameProperty("Light Color", "LightColor"); }
};

xiiLightComponentPatch_1_2 g_xiiLightComponentPatch_1_2;



XII_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_LightComponent);
