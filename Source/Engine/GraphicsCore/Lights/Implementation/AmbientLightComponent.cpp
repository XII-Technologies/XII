#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Lights/AmbientLightComponent.h>
#include <GraphicsCore/Lights/Implementation/ReflectionPool.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiAmbientLightComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("TopColor", GetTopColor, SetTopColor)->AddAttributes(new xiiDefaultValueAttribute(xiiColorGammaUB(xiiColor(0.2f, 0.2f, 0.3f)))),
    XII_ACCESSOR_PROPERTY("BottomColor", GetBottomColor, SetBottomColor)->AddAttributes(new xiiDefaultValueAttribute(xiiColorGammaUB(xiiColor(0.1f, 0.1f, 0.15f)))),
    XII_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1.0f))
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Lighting"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiAmbientLightComponent::xiiAmbientLightComponent()  = default;
xiiAmbientLightComponent::~xiiAmbientLightComponent() = default;

void xiiAmbientLightComponent::Deinitialize()
{
  xiiRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  SUPER::Deinitialize();
}

void xiiAmbientLightComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();

  UpdateSkyIrradiance();
}

void xiiAmbientLightComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  xiiReflectionPool::ResetConstantSkyIrradiance(GetWorld());
}

void xiiAmbientLightComponent::SetTopColor(xiiColorGammaUB color)
{
  m_TopColor = color;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

xiiColorGammaUB xiiAmbientLightComponent::GetTopColor() const
{
  return m_TopColor;
}

void xiiAmbientLightComponent::SetBottomColor(xiiColorGammaUB color)
{
  m_BottomColor = color;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

xiiColorGammaUB xiiAmbientLightComponent::GetBottomColor() const
{
  return m_BottomColor;
}

void xiiAmbientLightComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = fIntensity;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

float xiiAmbientLightComponent::GetIntensity() const
{
  return m_fIntensity;
}

void xiiAmbientLightComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? xiiDefaultSpatialDataCategories::RenderDynamic : xiiDefaultSpatialDataCategories::RenderStatic);
}

void xiiAmbientLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_TopColor;
  s << m_BottomColor;
  s << m_fIntensity;
}

void xiiAmbientLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_TopColor;
  s >> m_BottomColor;
  s >> m_fIntensity;
}

void xiiAmbientLightComponent::UpdateSkyIrradiance()
{
  xiiColor topColor    = xiiColor(m_TopColor) * m_fIntensity;
  xiiColor bottomColor = xiiColor(m_BottomColor) * m_fIntensity;
  xiiColor midColor    = xiiMath::Lerp(bottomColor, topColor, 0.5f);

  xiiAmbientCube<xiiColor> ambientLightIrradiance;
  ambientLightIrradiance.m_Values[xiiAmbientCubeBasis::PosX] = midColor;
  ambientLightIrradiance.m_Values[xiiAmbientCubeBasis::NegX] = midColor;
  ambientLightIrradiance.m_Values[xiiAmbientCubeBasis::PosY] = midColor;
  ambientLightIrradiance.m_Values[xiiAmbientCubeBasis::NegY] = midColor;
  ambientLightIrradiance.m_Values[xiiAmbientCubeBasis::PosZ] = topColor;
  ambientLightIrradiance.m_Values[xiiAmbientCubeBasis::NegZ] = bottomColor;

  xiiReflectionPool::SetConstantSkyIrradiance(GetWorld(), ambientLightIrradiance);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class xiiAmbientLightComponentPatch_1_2 : public xiiGraphPatch
{
public:
  xiiAmbientLightComponentPatch_1_2() :
    xiiGraphPatch("xiiAmbientLightComponent", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Top Color", "TopColor");
    pNode->RenameProperty("Bottom Color", "BottomColor");
  }
};

xiiAmbientLightComponentPatch_1_2 g_xiiAmbientLightComponentPatch_1_2;



XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_AmbientLightComponent);
