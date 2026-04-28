/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Render/RenderWorldSettingsComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiRenderWorldSettingsComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("ShadingQualityLevel", xiiShadingQualityLevel, m_ShadingQualityLevel),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld"),
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
}

void xiiRenderWorldSettingsComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_ShadingQualityLevel;
}
