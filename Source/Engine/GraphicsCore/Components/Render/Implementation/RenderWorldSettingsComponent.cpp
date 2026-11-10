/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Render/RenderWorldSettingsComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiRenderWorldSettingsComponent, 4, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("ShadingQualityLevel", xiiShadingQualityLevel, m_ShadingQualityLevel),
    XII_MEMBER_PROPERTY("RenderConfiguration", m_RenderConfiguration),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Graphics"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiRenderWorldSettingsComponent::xiiRenderWorldSettingsComponent()  = default;
xiiRenderWorldSettingsComponent::~xiiRenderWorldSettingsComponent() = default;

void xiiRenderWorldSettingsComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_ShadingQualityLevel;
  const bool bValid = m_RenderConfiguration.IsValid();
  if (!bValid)
    xiiLog::Error("Invalid scene render configuration. Writing inherited defaults.");
  if ((bValid ? m_RenderConfiguration : xiiRenderConfiguration{}).Serialize(s).Failed())
    xiiLog::Error("Could not write the scene render configuration.");
}

void xiiRenderWorldSettingsComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_ShadingQualityLevel;
  const auto uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  if (uiVersion >= 2 && m_RenderConfiguration.Deserialize(s, uiVersion - 1).Failed())
    xiiLog::Error("Invalid render configuration in the scene. Using inherited defaults.");
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Render_Implementation_RenderWorldSettingsComponent);
